#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

#include <json/json.h>
#include <magic.h>

#include "common.h"
#include "HTTPModelService.h"
#include "PostgreSqlDb.h"


class AddApplication
{
  struct processor_for_mime_type
  {
    typedef std::function<std::unique_ptr<FileProcessor>()> ProcessorFactory;

    // Last const char* must be nullptr
    const char* const * mime_types;
    ProcessorFactory proc_factory;
    std::unique_ptr<FileProcessor> processor;

    processor_for_mime_type(const char* const * mime_types,
      ProcessorFactory proc_factory) :
      mime_types(mime_types), proc_factory(proc_factory), processor()
    {
    }
  };

  Json::Value config_root;
  std::unique_ptr<Database> database;
  std::vector<processor_for_mime_type> file_processors;
  magic_t magic_hdl;
  HTTPModelService model_service;
  std::vector<TextUnit> text_units_staged;

  void clean_up() noexcept;

public:

  AddApplication();

  AddApplication(const AddApplication&) = delete;

  AddApplication& operator=(const AddApplication&) = delete;

  virtual ~AddApplication();

  int run(int argc, char** argv);

protected:

  virtual void on_text_unit(const TextUnit& text_unit);

  virtual void process_one_file(const char* file_path);

  virtual void process_one_file(const std::filesystem::path& file_path);

  virtual void
  process_given_file_or_directory(const std::filesystem::path& path_obj);
};