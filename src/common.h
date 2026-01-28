#pragma once

#include <filesystem>
#include <vector>

#include <json/json.h>


std::filesystem::path get_default_config_json_file();

Json::Value get_json_member_with_type(const Json::Value& object,
  const char* key, Json::ValueType required_type, bool required=true);

Json::Value get_settings_from_default_json_file();

Json::Value
get_settings_from_json_file(const std::filesystem::path& file_path);

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


class TextUnit
{
  std::string _text;
  std::string _anchor;
  std::vector<float> _embedding;

public:

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
};


class FileRecord
{
  std::string _file_path;
  ModelInfo _model_info;
  std::vector<TextUnit> _text_units;

public:

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


class FileProcessor
{
public:

  virtual ~FileProcessor() = default;

  virtual void process_file(const char* file_path) = 0;
};