#include "common.h"

#include <cstdlib>
#include <cctype>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>


namespace filesystem = std::filesystem;


Json::Value ask_for_postgresql_settings()
{
  Json::Value settings;
  std::string input;

  std::cerr << "Name of the database: ";
  std::getline(std::cin, input);
  if (!input.empty())
    settings["dbname"] = input;

  std::cerr << "User name: ";
  std::getline(std::cin, input);
  if (!input.empty())
    settings["user"] = input;

  std::cerr << "Password: ";
  std::getline(std::cin, input);
  if (!input.empty())
    settings["password"] = input;

  std::cerr << "Host name [Auto-determined UNIX socket by default]: ";
  std::getline(std::cin, input);
  if (!input.empty())
    settings["host"] = input;

  std::cerr << "Port [5432]: ";
  std::getline(std::cin, input);
  if (!input.empty())
    settings["port"] = input;

  return settings;
}


std::filesystem::path get_default_settings_json_file()
{
  const char* dir = getenv("XDG_CONFIG_HOME");
  filesystem::path p;
  if (dir && dir[0] != 0)
  {
    p.assign(dir);
    p.append("embeddings-db");
  }
  else if ((dir = getenv("HOME")) && dir[0] != 0)
  {
    p.assign(dir);
    p.append(".config");
    p.append("embeddings-db");
  }

  p.append("settings.json");
  return p;
}


Json::Value get_json_member_with_type(const Json::Value& object,
  const char* key, Json::ValueType required_type, bool required)
{
  if (!object.isMember(key))
  {
    if (required)
    {
      std::string msg("Member \"");
      msg += key;
      msg += "\" was not found in object";
      throw std::runtime_error(msg);
    }

    return Json::Value();
  }

  const Json::Value& v = object[key];
  Json::ValueType type = v.type();

  if (type != required_type)
  {
    const char* type_str = "of the required type";
    switch (required_type)
    {
    case Json::ValueType::nullValue:
      if (!required)
      {
        throw std::logic_error("required cannot be false when required_type is"
          "nullValue");
      }
      type_str = "a null value"; break;
    case Json::ValueType::intValue:
      // if type() returns uintValue, it's OK here
      if (type == Json::ValueType::uintValue)
        return v;
      type_str = "an integer"; break;
    case Json::ValueType::uintValue:
      type_str = "an integer greater than 0"; break;
    case Json::ValueType::realValue:
      type_str = "a number"; break;
    case Json::ValueType::stringValue:
      type_str = "a string"; break;
    case Json::ValueType::booleanValue:
      type_str = "a boolean value"; break;
    case Json::ValueType::arrayValue:
      type_str = "an array"; break;
    case Json::ValueType::objectValue:
      type_str = "an object";
    }

    std::string msg("Member with the key \"");
    msg += key;
    msg += "\" is not ";
    msg += type_str;
    throw std::runtime_error(msg);
  }

  return v;
}


Json::Value get_settings_from_default_json_file()
{
  filesystem::path file_path = get_default_settings_json_file();
  if (!filesystem::exists(file_path))
  {
    // Ask for the settings values and create them.
    Json::Value root(Json::objectValue);
    std::string input;

    std::cerr << "Embeddings API URL [http://localhost:8080/v1/embeddings]: ";
    std::getline(std::cin, input);
    if (!input.empty())
    {
      root["embeddingsHttp"]["embeddingsUrl"] = input;
    }

    std::cerr << "Name/identifier used to generate embeddings to save: ";
    std::getline(std::cin, input);
    if (!input.empty())
    {
      root["embeddingsHttp"]["idModelToSave"] = input;
    }

    const char* const pgsql_msg = "Configure a PostgreSQL database? [Y or N] ";
    if (get_yes_or_no_response(pgsql_msg))
    {
      Json::Value pgsql_settings = ask_for_postgresql_settings();
      if (pgsql_msg)
        root["postgresql"] = pgsql_settings;
    }

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "  ";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

    if (!file_path.parent_path().empty())
      filesystem::create_directories(file_path.parent_path());
    std::fstream dest_stream(file_path.string(),
      std::ios::out | std::ios::binary);
    if (!dest_stream.is_open())
    {
      std::string msg("Cannot open file \"");
      msg += file_path.string();
      msg += "\".";
      throw std::ios_base::failure(msg);
    }
    writer->write(root, &dest_stream);

    return root;
  }

  return get_settings_from_json_file(file_path);
}


Json::Value get_settings_from_json_file(const std::filesystem::path& file_path)
{
  std::ifstream json_file(file_path.string(), std::ifstream::binary);
  if (!json_file.is_open())
  {
    std::string msg("Cannot open file \"");
    msg += file_path.string();
    msg += "\".";
    throw std::ios_base::failure(msg);
  }

  Json::Value root;
  json_file >> root;
  json_file.close();
  return root;
}


bool get_yes_or_no_response(const char* msg)
{
    std::string input;
    std::cerr << msg;
    std::getline(std::cin, input);
    const char* _input = input.c_str();
    for (;;)
    {
      if (strcasecmp(_input, "Y") == 0 || strcasecmp(_input, "YES") == 0)
        return true;
      else if (strcasecmp(_input, "N") == 0 || strcasecmp(_input, "NO") == 0)
        return false;
    }
}


uint32_t htonf(float f)
{
    uint32_t i;
    static_assert(sizeof(float) == sizeof(uint32_t), "float size mismatch");
    std::memcpy(&i, &f, sizeof(float));
    return htonl(i);
}


bool is_all_spaces(const char* str)
{
  for (size_t idx = 0; str[idx]; idx++)
  {
    if (!isspace(str[idx]))
      return false;
  }

  return true;
}