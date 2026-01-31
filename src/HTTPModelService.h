#pragma once

#include <string>
#include <vector>

#include <curl/curl.h>
#include <json/json.h>

#include "common.h"


class HTTPModelService
{
  std::string api_auth_key;
  CURL* curl;
  std::string embeddings_api_url;
  std::string model_name;

  // Callback function to send POST data
  static size_t read_func(void* contents, size_t size, size_t nmemb,
    void* userp);

  // Callback function to handle the response body
  static size_t write_func(void* contents, size_t size, size_t nmemb,
    void* userp);

  void clean_up() noexcept;

  Json::Value post_json(const Json::Value& json);

public:

  HTTPModelService();

  HTTPModelService(const HTTPModelService& other) = delete;

  HTTPModelService& operator=(const HTTPModelService&) = delete;

  virtual ~HTTPModelService();

  std::vector<float> get_embedding(const char* str);

  void get_embeddings_and_set(std::vector<TextUnit>& text_units);

  void set_embeddings_api_url(const std::string& url);

  void set_model_name(const std::string& name);
};
