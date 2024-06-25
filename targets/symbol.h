#pragma once

#include <string>
#include <memory>
#include <cdk/types/basic_type.h>

namespace til {

  class symbol {
    int _qualifier;
    std::shared_ptr<cdk::basic_type> _type;
    std::string _name;
    bool _initialized;
    int _offset;

  public:
    symbol(int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name, bool initialized) :
        _qualifier(qualifier), _type(type), _name(name), _initialized(initialized), _offset(0) {
    }

    virtual ~symbol() {
      // EMPTY
    }

    std::shared_ptr<cdk::basic_type> type() const {
      return _type;
    }
    bool is_typed(cdk::typename_type name) const {
      return _type->name() == name;
    }
    const std::string &name() const {
      return _name;
    }

    void set_offset(int offset) {
      _offset = offset;
    }

    int offset() {
      return _offset;
    }

    bool global() const {
      return _offset == 0;
    }

    int qualifier() {
      return _qualifier;
    }
  };

  inline auto make_symbol(int qualifier, std::shared_ptr<cdk::basic_type> type, const std::string &name,
                          bool initialized) {
    return std::make_shared<symbol>(qualifier, type, name, initialized);
  }

} // til
