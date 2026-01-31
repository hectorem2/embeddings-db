#pragma once

#include <filesystem>
#include <memory>
#include <vector>

#include <json/json.h>


Json::Value ask_for_postgresql_settings();

std::filesystem::path get_default_config_json_file();

Json::Value get_json_member_with_type(const Json::Value& object,
  const char* key, Json::ValueType required_type, bool required=true);

Json::Value get_settings_from_default_json_file();

Json::Value
get_settings_from_json_file(const std::filesystem::path& file_path);

bool get_yes_or_no_response(const char* msg);

uint32_t htonf(float f);

bool is_all_spaces(const char* str);


class ModelInfo
{
  std::string _model_id;

public:

  const std::string& model_id() const
  {
    return _model_id;
  }

  void model_id(const std::string& model_id)
  {
    _model_id = model_id;
  }
};


class FileRecord;


class TextUnit
{
  unsigned long long _id;
  std::string _text;
  std::string _anchor;
  std::vector<float> _embedding;
  std::shared_ptr<FileRecord> _file_record;

public:

  unsigned long long id() const
  {
    return _id;
  }

  void id(unsigned long long id)
  {
    _id = id;
  }

  const std::string& text() const
  {
    return _text;
  }

  void text(const std::string& text)
  {
    _text = text;
  }

  const std::string& anchor() const
  {
    return _anchor;
  }

  void anchor(const std::string& anchor)
  {
    _anchor = anchor;
  }

  const std::vector<float>& embedding() const
  {
    return _embedding;
  }

  void embedding(const std::vector<float>& embedding)
  {
    _embedding = embedding;
  }

  void file_record(const std::shared_ptr<FileRecord>& record)
  {
    _file_record = record;
  }

  const std::shared_ptr<FileRecord> file_record() const
  {
    return _file_record;
  }
};


class FileRecord
{
  unsigned long _id;
  std::string _file_path;
  ModelInfo _model_info;
  std::vector<TextUnit> _text_units;

public:

  unsigned long id() const
  {
    return _id;
  }

  void id(unsigned long id)
  {
    _id = id;
  }

  const std::string& file_path() const
  {
    return _file_path;
  }

  void file_path(const std::string& file_path)
  {
    _file_path = file_path;
  }

  const ModelInfo& model_info() const
  {
    return _model_info;
  }

  void model_info(const ModelInfo& model_info)
  {
    _model_info = model_info;
  }

  const std::vector<TextUnit>& text_units() const
  {
    return _text_units;
  }

  void text_units(const std::vector<TextUnit>& text_units)
  {
    _text_units = text_units;
  }
};


struct TextUnitResult
{
  TextUnit unit;
  float distance;
};


class Database
{
public:

  virtual ~Database() = default;

  virtual void save_file_record_with_text_units(FileRecord& record) = 0;

  virtual std::vector<TextUnitResult>
  search(const std::vector<float>& embedding) = 0;

  virtual void set_database_up() = 0;
};


class FileProcessor
{
public:

  virtual ~FileProcessor() = default;

  virtual void process_file(const char* file_path) = 0;
};