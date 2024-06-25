#pragma once

#include "targets/basic_ast_visitor.h"

#include <stack>
#include <set>
#include <sstream>
#include <cdk/emitters/basic_postfix_emitter.h>

namespace til {

  //!
  //! Traverse syntax tree and generate the corresponding assembly code.
  //!
  class postfix_writer: public basic_ast_visitor {

    std::set<std::string> _functions_to_declare;

    cdk::symbol_table<til::symbol> &_symtab;
    cdk::basic_postfix_emitter &_pf;
    int _lbl;
    bool _inFunctionBody;
    int _offset;
    std::stack<int> _whileInit, _whileEnd; 
    std::shared_ptr<til::symbol> _function;
    std::stack<std::string> _functionLabels;
    std::string _currentFunctionLabel;
    std::string _currentFunctionRetLabel;
    
    bool _inMainFunction;

  public:
    postfix_writer(std::shared_ptr<cdk::compiler> compiler, cdk::symbol_table<til::symbol> &symtab,
                   cdk::basic_postfix_emitter &pf) :
        basic_ast_visitor(compiler), _symtab(symtab), _pf(pf), _lbl(0), _inFunctionBody(0), _offset(0),
        _inMainFunction(0) {
    }

  public:
    ~postfix_writer() {
      os().flush();
    }

  private:
    /** Method used to generate sequential labels. */
    inline std::string mklbl(int lbl) {
      std::ostringstream oss;
      if (lbl < 0)
        oss << ".L" << -lbl;
      else
        oss << "_L" << lbl;
      return std::move(oss).str();
    }

  public:
  // do not edit these lines
#define __IN_VISITOR_HEADER__
#include ".auto/visitor_decls.h"       // automatically generated
#undef __IN_VISITOR_HEADER__
  // do not edit these lines: end

  };

} // til
