#pragma once

#include "common.h"

#include <functional>

#include <libxml/HTMLparser.h>


class HTMLFileProcessor : public FileProcessor
{
  std::string curr_tag;
  std::string curr_text;
  std::function<void(const TextUnit&)> _on_text_unit_func;
  htmlSAXHandler sax_handler;
  bool text_retrieved;

  static void on_read_text_func(void* ctx, const xmlChar* text, int len);

  static void on_start_element_func(void* ctx, const xmlChar* name,
    const xmlChar** atts);

  static void on_end_document_func(void* ctx);

  static void on_end_element_func(void* ctx, const xmlChar* name);

public:

  HTMLFileProcessor(std::function<void(const TextUnit&)> on_text_unit_func);

  virtual void process_file(const char* file_path) override;

};