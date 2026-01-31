#pragma once

#include "common.h"
#include <libpq-fe.h>


class PostgreSqlDb: public Database
{
  PGconn* pgconn;

  void clean_up() noexcept;

  void exec_sql(const char* sql);

public:

  PostgreSqlDb(const char* dbname, const char* user, const char* password, const char* host, const char* port);

  virtual ~PostgreSqlDb();

  virtual void
  save_file_record_with_text_units(FileRecord& record) override;

  virtual std::vector<TextUnitResult>
  search(const std::vector<float>& embedding) override;

  virtual void set_database_up() override;

};
