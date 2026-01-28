#pragma once

#include "common.h"

#include <functional>

#include <com/sun/star/frame/XComponentLoader.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/lang/XMultiComponentFactory.hpp>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/XComponentContext.hpp>


class OpenDocProcessor : public FileProcessor
{
  std::string curr_text;
  std::function<void(const TextUnit&)> _on_text_unit_func;

  css::uno::Reference<css::uno::XComponentContext> xComponentContext;
  css::uno::Reference<css::frame::XComponentLoader> xComponentLoader;
  css::uno::Reference<css::frame::XDesktop> xDesktop;
  css::uno::Reference<css::lang::XMultiComponentFactory> xMultiComponentFactory;

public:

  OpenDocProcessor(std::function<void(const TextUnit&)> on_text_unit_func);

  virtual void process_file(const char* file_path) override;
};