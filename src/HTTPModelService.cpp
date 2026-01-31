#include <iostream>
#include <string>

#include "HTTPModelService.h"


size_t HTTPModelService::read_func(void* contents, size_t size, size_t nmemb,
  void* userp)
{
  size_t total_size = size*nmemb;
  auto* stream = reinterpret_cast<std::stringstream*>(userp);
  auto* dest = reinterpret_cast<char*>(contents);
  stream->read(dest, total_size);
  return stream->gcount();
}


size_t HTTPModelService::write_func(void* contents, size_t size, size_t nmemb,
  void* userp)
{
  size_t total_size = size*nmemb;
  std::string* response = reinterpret_cast<std::string*>(userp);
  response->append(reinterpret_cast<char*>(contents), total_size);
  return total_size;
}


HTTPModelService::HTTPModelService():
  curl(nullptr)
{
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();

  if (!curl)
  {
    clean_up();
    throw std::runtime_error("curl_easy_init() failed");
  }

  curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_func);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_func);
}


HTTPModelService::~HTTPModelService()
{
  clean_up();
}


void HTTPModelService::clean_up() noexcept
{
  if (curl)
  {
      curl_easy_cleanup(curl);
  }

  curl_global_cleanup();
}


Json::Value HTTPModelService::post_json(const Json::Value& json)
{
  // Convert the payload to string
  Json::StreamWriterBuilder builder;
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  std::stringstream post_sstream;
  writer->write(json, &post_sstream);

  // Set up the curl options
  curl_slist* headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: application/json");

  if (!api_auth_key.empty())
  {
    headers = curl_slist_append(headers, ("Authorization: Bearer " +
      api_auth_key).c_str());
  }

  std::string response_body;

  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_READDATA, &post_sstream);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, post_sstream.str().size());

  // Perform the request
  CURLcode res = curl_easy_perform(curl);
   // Clean up
  curl_slist_free_all(headers);

  // Check for errors
  if (res != CURLE_OK) {
    std::string msg("curl_easy_perform() failed: ");
    msg += curl_easy_strerror(res);
    throw std::runtime_error(msg);
  }

  // Check the HTTP status.
  long http_status;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status);

  // TODO: Re-design this error handling.
  if (http_status < 200 || http_status >= 400)
  {
    std::string msg("Received HTTP status ");
    msg += std::to_string(http_status);
    std::cerr << msg << "\n";
    std::cerr << "Response ***\n" << response_body << "\n";
    throw std::runtime_error(msg);
  }

  // Parse the response JSON
  Json::CharReaderBuilder reader_builder;
  Json::Value response_json;
  std::string errs;

  std::istringstream response_stream(response_body);

  if (!Json::parseFromStream(reader_builder, response_stream, &response_json,
    &errs))
  {
    std::string msg("Failed to parse response from API:\n");
    throw std::runtime_error(msg + errs);
  }

  return response_json;
}


std::vector<float> HTTPModelService::get_embedding(const char* str)
{
  // Prepare the JSON payload
  Json::Value request_data;

  if (!model_name.empty())
  {
    request_data["model"] = model_name;
  }

  request_data["input"] = str;

  Json::Value response_json = post_json(request_data);

  Json::Value embeddings = get_json_member_with_type(response_json, "data",
    Json::ValueType::arrayValue);
  Json::Value::ArrayIndex embd_c = embeddings.size();

  if (embd_c != 1)
  {
    std::string msg("The number of embeddings returned was ");
    msg += std::to_string(embd_c);
    msg += " for a single string";
    throw std::runtime_error(msg);
  }

  Json::Value embd_obj = embeddings[0];
  Json::Value embd_arr = get_json_member_with_type(embd_obj, "embedding",
    Json::ValueType::arrayValue);

  std::vector<float> embd_floats;
  Json::Value::ArrayIndex embd_size = embd_arr.size();
  embd_floats.reserve(embd_size);
  for (Json::Value::ArrayIndex float_idx = 0; float_idx < embd_size;
    float_idx++)
  {
    Json::Value float_obj = embd_arr[float_idx];
    if (!float_obj.isNumeric())
    {
      throw std::runtime_error("element of \"embedding\" array is not a "
        "number");
    }

    embd_floats.push_back(float_obj.asFloat());
  }

  return embd_floats;
}


void HTTPModelService::get_embeddings_and_set(std::vector<TextUnit>& text_units)
{
  // Prepare the JSON payload
  Json::Value request_data;

  if (!model_name.empty())
  {
    request_data["model"] = model_name;
  }

  Json::Value input_arr(Json::arrayValue);

  for (const TextUnit& unit : text_units)
  {
    input_arr.append(Json::Value(unit.text()));
  }

  request_data["input"] = input_arr;

  Json::Value response_json = post_json(request_data);

  Json::Value embeddings = get_json_member_with_type(response_json, "data",
    Json::ValueType::arrayValue);

  Json::Value::ArrayIndex num_embds = embeddings.size();
  for (Json::Value::ArrayIndex idx = 0; idx < num_embds; idx++)
  {
    Json::Value obj = embeddings[idx];
    Json::Value embd_arr = get_json_member_with_type(obj, "embedding",
      Json::ValueType::arrayValue);
    Json::Value idx_obj = get_json_member_with_type(obj, "index",
      Json::ValueType::intValue);

    std::vector<float> embd_floats;
    Json::Value::ArrayIndex embd_size = embd_arr.size();
    embd_floats.reserve(embd_size);
    for (Json::Value::ArrayIndex float_idx = 0; float_idx < embd_size;
      float_idx++)
    {
      Json::Value float_obj = embd_arr[float_idx];
      if (!float_obj.isNumeric())
      {
        throw std::runtime_error("element of \"embedding\" array is not a "
          "number");
      }

      embd_floats.push_back(float_obj.asFloat());
    }

    int src_idx = idx_obj.asInt();
    text_units.at(src_idx).embedding(embd_floats);
  }
}


void HTTPModelService::set_embeddings_api_url(const std::string& url)
{
  embeddings_api_url = url;
  curl_easy_setopt(curl, CURLOPT_URL, embeddings_api_url.c_str());
}


void HTTPModelService::set_model_name(const std::string& name)
{
  model_name = name;
}