#include "OpenDocProcessor.h"

#include <iostream>
#include <memory>

#include <cppuhelper/bootstrap.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/container/XEnumerationAccess.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/text/XTextDocument.hpp>
#include <com/sun/star/text/XText.hpp>
#include <com/sun/star/text/XTextRange.hpp>
#include <com/sun/star/text/XTextContent.hpp>
#include <com/sun/star/text/XTextCursor.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/awt/ActionEvent.hpp>
#include <com/sun/star/awt/MouseEvent.hpp>
#include <com/sun/star/container/XIndexAccess.hpp>
#include <osl/file.hxx>
#include <rtl/process.h>


using com::sun::star::beans::XPropertySet;
using com::sun::star::container::XEnumeration;
using com::sun::star::container::XEnumerationAccess;
using com::sun::star::frame::XComponentLoader;
using com::sun::star::frame::XDesktop;
using com::sun::star::lang::XComponent;
using com::sun::star::lang::XMultiComponentFactory;
using com::sun::star::lang::XServiceInfo;
using com::sun::star::text::XText;
using com::sun::star::text::XTextCursor;
using com::sun::star::text::XTextDocument;
using com::sun::star::text::XTextRange;


void OpenDocProcessor::call_on_text_unit_and_clear()
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


OpenDocProcessor::
OpenDocProcessor(std::function<void(const TextUnit&)> on_text_unit_func):
  max_bytes_per_text_unit(4096),
  _on_text_unit_func(on_text_unit_func)
{
  // Initialize LibreOffice environment
  xComponentContext = ::cppu::bootstrap();
  xMultiComponentFactory = xComponentContext->getServiceManager();
  if (!xMultiComponentFactory.is())
  {
    throw std::runtime_error("Could not create the XMultiServiceFactory.");
  }

  xDesktop = css::uno::Reference<XDesktop>(xMultiComponentFactory->
    createInstanceWithContext("com.sun.star.frame.Desktop", xComponentContext),
    css::uno::UNO_QUERY);
  if (!xDesktop.is())
  {
    throw std::runtime_error("Could not create the XDesktop.");
  }

  xComponentLoader = css::uno::Reference<XComponentLoader>(xDesktop, css::uno::UNO_QUERY);
  if (!xComponentLoader.is())
  {
    throw std::runtime_error("Could not create the XComponentLoader.");
  }
}


void OpenDocProcessor::process_file(const char* file_path)
{
  // Opening the document
  rtl::OUString wdir;
  osl_getProcessWorkingDir(&wdir.pData);
  rtl::OUString url;
  osl::FileBase::getFileURLFromSystemPath(rtl::OUString::fromUtf8(rtl::OString(file_path)), url);
  rtl::OUString absUrl;
  osl::FileBase::getAbsoluteFileURL(wdir, url, absUrl);
  css::uno::Sequence<css::beans::PropertyValue> loadProps(1);
  loadProps[0].Name = "Hidden";
  loadProps[0].Value <<= true;
  css::uno::Reference<XComponent> xLoadedDoc = xComponentLoader->loadComponentFromURL(absUrl, "_blank", 0, loadProps);
  css::uno::Reference<XTextDocument> xTextDoc(xLoadedDoc, css::uno::UNO_QUERY_THROW);

  // Get the text object
  css::uno::Reference<XText> xDocText = xTextDoc->getText();

  css::uno::Reference<XEnumerationAccess> xParAccess(xDocText, css::uno::UNO_QUERY_THROW);
  css::uno::Reference<XEnumeration> xParEnum = xParAccess->createEnumeration();
  while (xParEnum->hasMoreElements())
  {
    css::uno::Any paragraph = xParEnum->nextElement();
    css::uno::Reference<XServiceInfo> xParInfo(paragraph, css::uno::UNO_QUERY_THROW);
    css::uno::Reference<XEnumerationAccess> xTextPortionAccess(xParInfo, css::uno::UNO_QUERY);
    if (!xTextPortionAccess.is())
      continue;
    css::uno::Reference<XEnumeration> xTexts = xTextPortionAccess->createEnumeration();
    css::uno::Reference<XPropertySet> xParProps(paragraph, css::uno::UNO_QUERY_THROW);
    sal_Int16 outlineLevel = 0;
    xParProps->getPropertyValue("OutlineLevel") >>= outlineLevel;

    std::string par_text;

    while (xTexts->hasMoreElements())
    {
      css::uno::Reference<XServiceInfo> xTextInfo(xTexts->nextElement(), css::uno::UNO_QUERY_THROW);
      css::uno::Reference<XTextRange> xTextRange(xTextInfo, css::uno::UNO_QUERY_THROW);
      rtl::OUString u16text = xTextRange->getString();
      rtl::OString u8text = u16text.toUtf8();
      par_text += u8text.getStr();
    }

    trim_string(par_text);

    // We wouldn't like to begin a text unit with an empty heading.
    if (outlineLevel == 1 && !par_text.empty())
    {
      call_on_text_unit_and_clear();
    }
    else
    {
      trim_string(curr_text);
      if (!curr_text.empty())
        curr_text += "\n";
    }

    curr_text += par_text;
  }

  call_on_text_unit_and_clear();
}