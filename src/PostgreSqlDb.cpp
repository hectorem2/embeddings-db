#include "PostgreSqlDb.h"
#include "common.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <arpa/inet.h>


typedef std::unique_ptr<PGresult, decltype(&PQclear)> PGresult_unique_ptr;


static std::vector<uint8_t> to_pgvector_binary(const float* embedding, uint16_t n)
{
  std::vector<uint8_t> buffer;
  buffer.resize(4 + n * 4);

  uint16_t n_be = htons(n);
  std::memcpy(buffer.data(), &n_be, 2);

  for (size_t i = 0; i < n; ++i)
  {
    uint32_t f_be = htonf(embedding[i]);
    std::memcpy(buffer.data() + 4 + i * 4, &f_be, 4);
  }

  return buffer;
}


static std::vector<uint8_t> to_pgvector_binary(const std::vector<float>& embedding)
{
  return to_pgvector_binary(embedding.data(), embedding.size());
}


PostgreSqlDb::PostgreSqlDb(const char* dbname, const char* user, const char* password, const char* host, const char* port): pgconn(nullptr)
{
  std::vector<const char*> params_keys = {
    "dbname", "user", "password", "host", "port", nullptr
  };

  std::vector<const char*> params = {
    dbname, user, password, host, port
  };

  pgconn = PQconnectdbParams(params_keys.data(), params.data(), 0);

  if (PQstatus(pgconn) != CONNECTION_OK)
  {
    std::string msg(PQerrorMessage(pgconn));
    clean_up();
    throw std::runtime_error(msg);
  }
}


PostgreSqlDb::~PostgreSqlDb()
{
  clean_up();
}


void PostgreSqlDb::clean_up() noexcept
{
  if (pgconn)
  {
    PQfinish(pgconn);
    pgconn = nullptr;
  }
}


void PostgreSqlDb::exec_sql(const char* sql)
{
  PGresult_unique_ptr res(PQexec(pgconn, sql), PQclear);

  ExecStatusType res_code = PQresultStatus(res.get());

  if (res_code == PGRES_NONFATAL_ERROR)
  {
    std::cerr << PQerrorMessage(pgconn) << "\n";
  }
  else if (!(res_code == PGRES_COMMAND_OK || res_code == PGRES_TUPLES_OK))
  {
    throw std::runtime_error(PQerrorMessage(pgconn));
  }
}


void PostgreSqlDb::save_file_record_with_text_units(FileRecord& record)
{
  const char* param_values[3];
  int param_lengths[3];
  int param_formats[3];

  param_values[0] = record.file_path().c_str();

  exec_sql("BEGIN");

  PGresult_unique_ptr res(PQexecParams(
    pgconn,
    "INSERT INTO FileRecords(file_path) VALUES ($1) RETURNING id",
    1,
    nullptr,            // infer types
    param_values,
    nullptr,
    nullptr,
    0                   // text result
  ), PQclear);

  ExecStatusType res_code = PQresultStatus(res.get());

  if (res_code == PGRES_NONFATAL_ERROR)
  {
    std::cerr << PQerrorMessage(pgconn) << "\n";
  }
  else if (res_code != PGRES_TUPLES_OK)
  {
    throw std::runtime_error(PQerrorMessage(pgconn));
  }

  // Have to copy the ID string there, since it goes away when res for the
  // FileRecord INSERT is cleared.
  std::string fr_id_str(PQgetvalue(res.get(), 0, 0));
  record.id(strtoul(fr_id_str.c_str(), nullptr, 10));

  // This needs to be set only once, since it's the file record ID for all the
  // text units being saved.
  param_values[2] = fr_id_str.c_str();
  param_lengths[2] = fr_id_str.size();
  param_formats[2] = 0;

  for (const TextUnit& text_unit : record.text_units())
  {
    std::vector<uint8_t> binary_vec = to_pgvector_binary(text_unit.embedding());

    // text column
    param_values[0]  = text_unit.text().c_str();
    param_lengths[0] = text_unit.text().size();
    param_formats[0] = 0; // text

    // vector column (binary)
    param_values[1]  = reinterpret_cast<const char*>(binary_vec.data());
    param_lengths[1] = binary_vec.size();
    param_formats[1] = 1; // binary

    res.reset(PQexecParams(pgconn,
      "INSERT INTO TextUnits768(text, embd, file_record_id) "
      "VALUES ($1, $2::vector, $3)",
      3, nullptr, param_values, param_lengths, param_formats, 0));

    ExecStatusType res_code = PQresultStatus(res.get());

    if (res_code == PGRES_NONFATAL_ERROR)
    {
      std::cerr << PQerrorMessage(pgconn) << "\n";
    }
    else if (res_code != PGRES_COMMAND_OK)
    {
      throw std::runtime_error(PQerrorMessage(pgconn));
    }
    else
    {
      std::cerr << "Inserted binary embedding successfully\n";
    }
  }

  exec_sql("COMMIT");
}


std::vector<TextUnitResult>
PostgreSqlDb::search(const std::vector<float>& embedding)
{
  std::vector<uint8_t> binary_vec = to_pgvector_binary(embedding);

  const char* param_value  = reinterpret_cast<const char*>(binary_vec.data());
  int param_length = binary_vec.size();
  int param_format = 1; // binary

  PGresult_unique_ptr res(PQexecParams(
    pgconn,
    "SELECT TextUnits768.id, TextUnits768.text, FileRecords.id, FileRecords.file_path, "
    "TextUnits768.embd <-> $1::vector AS distance FROM FileRecords "
    "INNER JOIN TextUnits768 ON TextUnits768.file_record_id=FileRecords.id ORDER BY distance LIMIT 20;",
    1, nullptr, &param_value, &param_length, &param_format, 0), PQclear);

  ExecStatusType res_code = PQresultStatus(res.get());

  if (res_code == PGRES_NONFATAL_ERROR)
  {
    std::cerr << PQerrorMessage(pgconn) << "\n";
  }
  else if (res_code != PGRES_TUPLES_OK)
  {
    throw std::runtime_error(PQerrorMessage(pgconn));
  }

  PGresult* r = res.get();
  int n_results = PQntuples(r);
  std::vector<std::shared_ptr<FileRecord>> frecords;
  std::vector<TextUnitResult> results;
  results.reserve(n_results);

  for (int idx = 0; idx < n_results; idx++)
  {
    TextUnitResult unit_res;

    char* v = PQgetvalue(r, idx, 0);
    unit_res.unit.id(strtoul(v, nullptr, 10));

    v = PQgetvalue(r, idx, 1);
    unit_res.unit.text(v);

    v = PQgetvalue(r, idx, 2);
    unsigned long long frecord_res_id = strtoull(v, nullptr, 10);

    std::shared_ptr<FileRecord> fr_for_unit;
    for (std::shared_ptr<FileRecord>& record : frecords)
    {
      if (record->id() == frecord_res_id)
      {
        fr_for_unit = record;
        break;
      }
    }

    if (!fr_for_unit)
    {
      fr_for_unit.reset(new FileRecord);
      fr_for_unit->id(frecord_res_id);
      v = PQgetvalue(r, idx, 3);
      fr_for_unit->file_path(v);
      frecords.push_back(fr_for_unit);
    }

    unit_res.unit.file_record(fr_for_unit);

    v = PQgetvalue(r, idx, 4);
    unit_res.distance = strtof(v, nullptr);

    results.push_back(unit_res);
  }

  return results;
}


void PostgreSqlDb::set_database_up()
{
  exec_sql("CREATE TABLE IF NOT EXISTS FileRecords("
    "id serial PRIMARY KEY,"
    "file_path TEXT"
  ")");

  exec_sql("CREATE TABLE IF NOT EXISTS TextUnits768 ("
    "id bigserial PRIMARY KEY, "
    "text TEXT, "
    "embd VECTOR(768), "
    "file_record_id INTEGER REFERENCES FileRecords(id)"
  ")");
}