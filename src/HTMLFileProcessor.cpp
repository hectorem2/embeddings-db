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
static const char* str_case_in(const xmlChar* str, const char* const * strs)
{
  for (const char* const * item = strs; *item; item++)
  {
    if (strs_case_equal(str, *item)) return *item;
  }

  return nullptr;
}


static bool is_all_spaces(const xmlChar* str, int len)
{
  for (int idx = 0; idx < len; idx++)
  {
    if (!isspace(str[idx]))
      return false;
  }

  return true;
}


void HTMLFileProcessor::on_read_text_func(void* ctx, const xmlChar* text,
  int len)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);
  const char* _text = reinterpret_cast<const char*>(text);

  if (strcasecmp(self->curr_tag.c_str(), "style") == 0 ||
    strcasecmp(self->curr_tag.c_str(), "script") == 0)
  {
    return;
  }

  if (strcasecmp(self->curr_tag.c_str(), "pre") != 0)
  {
    if (is_all_spaces(text, len)) return;
  }

  self->curr_text.append(_text, len);
  self->text_retrieved = true;
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
    if (!(self->curr_text.empty()) && self->text_retrieved)
      self->curr_text += "\n";
    self->text_retrieved = false;
  }

  if (strs_case_equal(name, "td"))
  {
    self->curr_text += "  ";
  }

  if (strs_case_equal(name, "h1"))
  {
    if (!(self->curr_text.empty()))
    {
      TextUnit unit;
      unit.text(self->curr_text);
      self->_on_text_unit_func(unit);
      self->curr_text.clear();
    }
    self->text_retrieved = false;
  }
}


void HTMLFileProcessor::on_end_document_func(void* ctx)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);

  if (!(self->curr_text.empty()))
  {
    TextUnit unit;
    unit.text(self->curr_text);
    self->_on_text_unit_func(unit);
    self->curr_text.clear();
  }
}


void HTMLFileProcessor::on_end_element_func(void* ctx, const xmlChar* name)
{
  HTMLFileProcessor* self = static_cast<HTMLFileProcessor*>(ctx);

  if (strs_case_equal(name, "title") && !(self->curr_text.empty()))
  {
    TextUnit unit;
    unit.text(self->curr_text);
    self->_on_text_unit_func(unit);
    self->curr_text.clear();
  }

  if (strs_case_equal(name, "style") == 0 ||
    strs_case_equal(name, "script") == 0)
  {
    self->curr_tag.clear();
  }
}


HTMLFileProcessor::
HTMLFileProcessor(std::function<void(const TextUnit&)> on_text_unit_func) :
  _on_text_unit_func(on_text_unit_func),
  sax_handler({}),
  text_retrieved(false)
{
  sax_handler.startElement = on_start_element_func;
  sax_handler.endDocument = on_end_document_func;
  sax_handler.endElement = on_end_element_func;
  sax_handler.characters = on_read_text_func;
}


void HTMLFileProcessor::process_file(const char* file_path)
{
  htmlDocPtr doc = htmlSAXParseFile(file_path, nullptr, &sax_handler, this);
  if (doc)
    xmlFreeDoc(doc);
}
