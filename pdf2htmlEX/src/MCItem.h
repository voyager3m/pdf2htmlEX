#pragma once

#include <string>
#include <ostream>
#include <vector>
#include <jsoncpp/json/json.h>


namespace pdf2htmlEX {

  // MCItem MarkedContext item
  class MCItem 
  {
    public:
      int id;
      std::string type;
      std::string alt_text;
      std::string actual_text;
      std::string summary;
      std::string title;
      std::vector<MCItem> children;

      operator bool() const { return !type.empty(); };

      Json::Value getJson() const;
      void dump(std::ostream & out) const;
  };

}

