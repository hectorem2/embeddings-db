#include "SearchApplication.h"

#include <iostream>
#include <stdexcept>
#include <vector>


SearchApplication::SearchApplication():
  database()
{
}


SearchApplication::~SearchApplication()
{
  clean_up();
}


int SearchApplication::run(int argc, char** argv)
{
  config_root = get_settings_from_default_json_file();

  std::string embd_api_url;
  std::string model_name;

  Json::Value embd_serv = get_json_member_with_type(config_root,
    "embeddingsHttp", Json::ValueType::objectValue, false);

  if (embd_serv)
  {
    Json::Value url_obj = get_json_member_with_type(embd_serv, "embeddingsUrl",
      Json::ValueType::stringValue, false);
    if (url_obj)
      embd_api_url = url_obj.asString();

    Json::Value model_id_obj = get_json_member_with_type(embd_serv, "idModelToSave",
      Json::ValueType::stringValue, false);
    model_name = model_id_obj.asString();
  }

  if (embd_api_url.empty())
    embd_api_url = "http://localhost:8080/v1/embeddings";

  model_service.set_embeddings_api_url(embd_api_url);
  model_service.set_model_name(model_name);

  Json::Value pgsql_settings = get_json_member_with_type(config_root,
    "postgresql", Json::ValueType::objectValue, false);

  if (pgsql_settings)
  {
    std::string dbname;
    std::string user;
    std::string password;
    std::string host;
    std::string port;

    Json::Value value = get_json_member_with_type(pgsql_settings, "dbname",
      Json::ValueType::stringValue, false);
    if (value) dbname = value.asString();

    value = get_json_member_with_type(pgsql_settings, "user",
      Json::ValueType::stringValue, false);
    if (value) user = value.asString();

    value = get_json_member_with_type(pgsql_settings, "password",
      Json::ValueType::stringValue, false);
    if (value) password = value.asString();

    value = get_json_member_with_type(pgsql_settings, "host",
      Json::ValueType::stringValue, false);
    if (value) host = value.asString();

    value = get_json_member_with_type(pgsql_settings, "port",
      Json::ValueType::stringValue, false);
    if (value) port = value.asString();

    database.reset(new PostgreSqlDb(dbname.empty() ? nullptr : dbname.c_str(),
      user.empty() ? nullptr : user.c_str(),
      password.empty() ? nullptr : password.c_str(),
      host.empty() ? nullptr : host.c_str(),
      port.empty() ? nullptr : port.c_str()));
  }
  else
  {
    std::cerr << "No database settings found in the settings file. "
      "Connecting to a local PostgreSQL database with the default values...\n";
    database.reset(new PostgreSqlDb(nullptr, nullptr, nullptr, nullptr,
      nullptr));
  }

  if (database)
  {
    database->set_database_up();
  }
  else
  {
    throw std::runtime_error("database is empty");
  }

  std::string query_str_stdin;
  const char* query_str = nullptr;
  if (argc >= 2)
  {
    query_str = argv[1];
  }
  else
  {
    std::cerr << "Search query: ";
    std::getline(std::cin, query_str_stdin);
    query_str = query_str_stdin.c_str();

    if (query_str_stdin.empty())
    {
      std::cerr << "Empty query string, exiting...\n";
      return 0;
    }
  }

  std::vector<float> embd = model_service.get_embedding(query_str);
  std::vector<TextUnitResult> results = database->search(embd);

  size_t n_results = results.size();
  for (size_t idx = 0; idx < n_results; idx++)
  {
    const TextUnitResult& res = results[idx];
    std::cerr << "*** Result #" << idx + 1 << " ***\n\n";

    std::cerr << "Text Unit ID: " << res.unit.id() << "\n";
    std::cerr << "Distance: " << res.distance << "\n\n";
    std::cerr << "File path: " << res.unit.file_record()->file_path() << "\n\n";
    std::cerr << "Text\n==============================\n" <<
      res.unit.text() << "\n\n";
  }

  return 0;
}


void SearchApplication::clean_up() noexcept
{
}