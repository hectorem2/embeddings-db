#pragma once

#include <memory>

#include <json/json.h>

#include "common.h"
#include "HTTPModelService.h"
#include "PostgreSqlDb.h"


class SearchApplication
{
  Json::Value config_root;
  std::unique_ptr<Database> database;
  HTTPModelService model_service;

  void clean_up() noexcept;

public:

  SearchApplication();

  SearchApplication(const SearchApplication&) = delete;

  SearchApplication& operator=(const SearchApplication&) = delete;

  virtual ~SearchApplication();

  int run(int argc, char** argv);
};