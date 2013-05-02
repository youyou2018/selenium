// Copyright 2013 Software Freedom Conservancy
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef WEBDRIVER_IE_CLEARELEMENTCOMMANDHANDLER_H_
#define WEBDRIVER_IE_CLEARELEMENTCOMMANDHANDLER_H_

#include "../Browser.h"
#include "../IECommandHandler.h"
#include "../IECommandExecutor.h"
#include "../Generated/atoms.h"

namespace webdriver {

class ClearElementCommandHandler : public IECommandHandler {
 public:
  ClearElementCommandHandler(void) {
  }

  virtual ~ClearElementCommandHandler(void) {
  }

 protected:
  void ExecuteInternal(const IECommandExecutor& executor,
                       const LocatorMap& locator_parameters,
                       const ParametersMap& command_parameters,
                       Response* response) {
    LocatorMap::const_iterator id_parameter_iterator = locator_parameters.find("id");
    if (id_parameter_iterator == locator_parameters.end()) {
      response->SetErrorResponse(400, "Missing parameter in URL: id");
      return;
    } else {
      std::wstring text = L"";
      int status_code = WD_SUCCESS;
      std::string element_id = id_parameter_iterator->second;

      BrowserHandle browser_wrapper;
      status_code = executor.GetCurrentBrowser(&browser_wrapper);
      if (status_code != WD_SUCCESS) {
        response->SetErrorResponse(status_code, "Unable to get browser");
        return;
      }

      ElementHandle element_wrapper;
      status_code = this->GetElement(executor, element_id, &element_wrapper);
      if (status_code == WD_SUCCESS) {
        // Yes, the clear atom should check this for us, but executing it asynchronously
        // does not return the proper error code when this error condition is encountered.
        // Thus, we'll check the interactable and editable states of the element before 
        // attempting to clear it.
        if (!element_wrapper->IsInteractable() || !element_wrapper->IsEditable()) {
          response->SetErrorResponse(EELEMENTNOTENABLED,
                                     "Element must not be hidden, disabled or read-only");
          return;
        }
        // The atom is just the definition of an anonymous
        // function: "function() {...}"; Wrap it in another function so we can
        // invoke it with our arguments without polluting the current namespace.
        std::wstring script_source = L"(function() { return (";
        script_source += atoms::asString(atoms::CLEAR);
        script_source += L")})();";

        CComPtr<IHTMLDocument2> doc;
        browser_wrapper->GetDocument(&doc);
        Script script_wrapper(doc, script_source, 1);
        script_wrapper.AddArgument(element_wrapper);
        if (executor.allow_asynchronous_javascript()) {
          status_code = script_wrapper.ExecuteAsync(ASYNC_SCRIPT_EXECUTION_TIMEOUT_IN_MILLISECONDS);
        } else {
          status_code = script_wrapper.Execute();
        }
        if (status_code != WD_SUCCESS) {
          // Assume that a JavaScript error returned by the atom is that
          // the element is either invisible, disabled, or read-only.
          // This may be a bad assumption, but we currently have no way
          // to get information about exceptions thrown from JS.
          response->SetErrorResponse(EELEMENTNOTENABLED,
                                     "A JavaScript error was encountered clearing the element. The driver assumes this is because the element is hidden, disabled or read-only, and it must not be to clear the element.");
          return;
        }
      } else {
        response->SetErrorResponse(status_code, "Element is no longer valid");
        return;
      }

      response->SetSuccessResponse(Json::Value::null);
    }
  }
};

} // namespace webdriver

#endif // WEBDRIVER_IE_CLEARELEMENTCOMMANDHANDLER_H_
