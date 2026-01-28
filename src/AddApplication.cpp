#include "AddApplication.h"

#include <iostream>
#include <stdexcept>

#include "HTMLFileProcessor.h"
#include "OpenDocProcessor.h"


namespace filesystem = std::filesystem;

const char* const html_mime_types[] = {"text/html", nullptr};

const char* const open_doc_mime_types[] = {
  "application/vnd.oasis.opendocument.text",
  "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
  nullptr
};


AddApplication::AddApplication():
  magic_hdl(magic_open(MAGIC_MIME_TYPE))
{
  using std::placeholders::_1;

  if (!magic_hdl)
  {
    throw std::runtime_error("magic_open() failed");
  }

  if (magic_load(magic_hdl, nullptr) != 0)
  {
    clean_up();
    const char* err = magic_error(magic_hdl);
    std::string msg = "magic_load() failed: ";
    msg += (err ? err : "unknown error");
    throw std::runtime_error(msg);
  }

  file_processors.emplace_back(html_mime_types, [&] {
    auto func = std::bind(std::mem_fn(&AddApplication::on_text_unit), this,
      _1);
    return std::make_unique<HTMLFileProcessor>(func);
  });

  file_processors.emplace_back(open_doc_mime_types, [&] {
    auto func = std::bind(std::mem_fn(&AddApplication::on_text_unit), this,
      _1);
    return std::make_unique<OpenDocProcessor>(func);
  });
}


AddApplication::~AddApplication()
{
  clean_up();
}


int AddApplication::run(int argc, char** argv)
{
  if (argc < 2)
  {
    std::cerr << "No files or directories given.\n";
    return 0;
  }

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

  for (int pos = 1; pos < argc; pos++)
  {
    filesystem::path path_obj(argv[pos]);
    process_given_file_or_directory(path_obj);
  }

  return 0;
}


void AddApplication::clean_up() noexcept
{
  if (magic_hdl)
  {
    magic_close(magic_hdl);
    magic_hdl = nullptr;
  }
}


void AddApplication::on_text_unit(const TextUnit& text_unit)
{
  text_units_staged.push_back(text_unit);
}



void AddApplication::
process_given_file_or_directory(const std::filesystem::path& path_obj)
{
  if (!filesystem::exists(path_obj))
  {
    std::string msg("The file \"");
    msg += path_obj.string();
    msg += "\" does not exist.";
    throw std::runtime_error(msg);
  }

  if (filesystem::is_regular_file(path_obj))
  {
    process_one_file(path_obj);
    return;
  }

  for (const auto& entry : filesystem::recursive_directory_iterator(path_obj))
  {
    if (entry.is_regular_file())
    {
        process_one_file(entry.path());
    }
  }
}


void AddApplication::process_one_file(const char* file_path)
{
  std::cerr << "Processing file " << file_path << "\n";
  const char* mime_type = magic_file(magic_hdl, file_path);

  if (!mime_type)
  {
    std::string msg("magic_file failed().");

    if (const char* error = magic_error(magic_hdl))
    {
      msg += " "; msg += error;
    }

    throw std::runtime_error(msg);
  }

  std::cerr << "MIME type: " << mime_type << "\n";
  for (processor_for_mime_type& p : file_processors)
  {
    bool have_match = false;

    for (const char* const * item = p.mime_types; *item; item++)
    {
      if (strcmp(*item, mime_type) == 0)
      {
        have_match = true;
        break;
      }
    }

    if (have_match)
    {
      // Create the processor object if it's the case.
      if (!p.processor)
      {
        if (!p.proc_factory)
          throw std::logic_error("proc_factory is empty");

        p.processor = p.proc_factory();
        if (!p.processor)
          throw std::logic_error("processor returned by proc_factory is empty");

      }

      p.processor->process_file(file_path);
    }
  }

  model_service.get_embeddings_and_set(text_units_staged);
  text_units_staged.clear();
}


void AddApplication::process_one_file(const std::filesystem::path& file_path)
{
  process_one_file(file_path.c_str());
}