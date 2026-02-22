#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>

#include <json/json.h>
#include <magic.h>

#include "common.h"
#include "HTMLFileProcessor.h"
#include "OpenDocProcessor.h"


using std::placeholders::_1;


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


static void process_text_unit(const TextUnit& text_unit,
  std::vector<TextUnit>* dest_vec)
{
  dest_vec->push_back(text_unit);
}


int main(int argc, char** argv)
{
  const char* const html_mime_types[] = {"text/html", nullptr};

  const char* const open_doc_mime_types[] = {
    "application/vnd.oasis.opendocument.text",
    "application/vnd.openxmlformats-officedocument.wordprocessingml.document",
    nullptr
  };

  std::unique_ptr<std::remove_pointer<magic_t>::type, decltype(&magic_close)>
    magic_hdl(magic_open(MAGIC_MIME_TYPE), magic_close);

  if (!magic_hdl)
  {
    throw std::runtime_error("magic_open() failed");
  }

  if (magic_load(magic_hdl.get(), nullptr) != 0)
  {
    const char* err = magic_error(magic_hdl.get());
    std::string msg = "magic_load() failed: ";
    msg += (err ? err : "unknown error");
    throw std::runtime_error(msg);
  }

  std::vector<processor_for_mime_type> file_processors;
  std::vector<TextUnit> units;
  auto on_text_unit_func = std::bind(process_text_unit, _1, &units);

  file_processors.emplace_back(html_mime_types, [&] {
    return std::make_unique<HTMLFileProcessor>(on_text_unit_func);
  });

  file_processors.emplace_back(open_doc_mime_types, [&] {
    return std::make_unique<OpenDocProcessor>(on_text_unit_func);
  });

  if (argc < 2)
  {
    std::cerr << "No files given.\n";
    return 1;
  }

  Json::StreamWriterBuilder builder;
  builder["indentation"] = "  ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());

  for (int idx = 1; idx < argc; idx++)
  {
    const char* file_path = argv[idx];
    std::cerr << "Processing file " << file_path << "\n";
    const char* mime_type = magic_file(magic_hdl.get(), file_path);

    if (!mime_type)
    {
      std::string msg("magic_file failed().");

      if (const char* error = magic_error(magic_hdl.get()))
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

    Json::Value root_json(Json::ValueType::arrayValue);

    for (const TextUnit& unit : units)
    {
      std::cerr << "Size of text for text unit: " << unit.text().size() <<
        " bytes.\n";
      Json::Value unit_json(Json::ValueType::objectValue);
      unit_json["text"] = unit.text();
      root_json.append(unit_json);
    }

    units.clear();

    std::string dest_file_path(file_path);
    dest_file_path += ".json";
    std::fstream dest_file(dest_file_path, std::ios::out | std::ios::trunc |
      std::ios::binary);
    if (!dest_file.is_open())
    {
      std::string msg("Cannot open file \"");
      msg += dest_file_path;
      msg += "\" for writing.";
      throw std::ios_base::failure(msg);
    }

    writer->write(root_json, &dest_file);
  }
}
