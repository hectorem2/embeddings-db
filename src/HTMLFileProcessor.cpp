#include "HTMLFileProcessor.h"

#include <cstring>
#include <iostream>


/*
Helper to compare a xmlChar* and a const char* ignoring their case.
*/
inline bool strs_case_equal(const xmlChar* s1, const char* s2)
{
  return (strcasecmp(reinterpret_cast<const char*>(s1), s2) == 0);
}


/*
The last const char* in strs is nullptr.
*/
static const char* str_case_in(const char* str, const char* const * strs)
{
  for (const char* const * item = strs; *item; item++)
  {
    if (strcasecmp(str, *item) == 0) return *item;
  }

  return nullptr;
}


static const char* str_case_in(const xmlChar* str, const char* const * strs)
{
  for (const char* const * item = strs; *item; item++)
  {
    if (strs_case_equal(str, *item)) return *item;
  }

  return nullptr;
}


void HTMLFileProcessor::on_read_text_func(void* ctx, const xmlChar* text,
  int len)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);
  const char* _text = reinterpret_cast<const char*>(text);

  const char* const tags_to_skip[] = {
    "style", "script", "svg", nullptr
  };

  if (str_case_in(self->curr_tag.c_str(), tags_to_skip))
  {
    return;
  }

  self->curr_text.append(_text, len);
}


void HTMLFileProcessor::on_start_element_func(void* ctx, const xmlChar* name,
  const xmlChar** atts)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);
  self->curr_tag = reinterpret_cast<const char*>(name);

  const char* const tags_to_nl[] = {
    "p", "br", "div", "li", "tr", "h2", "h3", "h4", nullptr
  };

  if (str_case_in(name, tags_to_nl))
  {
    trim_string(self->curr_text);
    if (!(self->curr_text.empty()))
      self->curr_text += "\n";
  }
  else if (strs_case_equal(name, "td") || strs_case_equal(name, "th"))
  {
    self->curr_text += "  ";
  }
  else if (strs_case_equal(name, "h1"))
  {
    self->call_on_text_unit_and_clear();
  }
}


void HTMLFileProcessor::on_end_document_func(void* ctx)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);

  self->call_on_text_unit_and_clear();
}


void HTMLFileProcessor::call_on_text_unit_and_clear()
{
    trim_string(curr_text);

    if (curr_text.empty())
      return;

    if (curr_text.size() > max_bytes_per_text_unit)
    {
      std::vector parts = split_text(curr_text, max_bytes_per_text_unit);
      for (std::string& p : parts)
      {
        TextUnit unit;
        unit.text(p);
        _on_text_unit_func(unit);
      }
    }
    else
    {
      TextUnit unit;
      unit.text(curr_text);
      _on_text_unit_func(unit);
    }

    curr_text.clear();
}


HTMLFileProcessor::
HTMLFileProcessor(std::function<void(const TextUnit&)> on_text_unit_func) :
  max_bytes_per_text_unit(4096),
  _on_text_unit_func(on_text_unit_func),
  sax_handler({})
{
  sax_handler.startElement = on_start_element_func;
  sax_handler.endDocument = on_end_document_func;
  sax_handler.characters = on_read_text_func;
}


void HTMLFileProcessor::process_file(const char* file_path)
{
  htmlDocPtr doc = htmlSAXParseFile(file_path, nullptr, &sax_handler, this);
  if (doc)
    xmlFreeDoc(doc);
}
