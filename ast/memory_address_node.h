#pragma once

#include "cdk/ast/lvalue_node.h"
#include "cdk/ast/expression_node.h"

namespace til {

  /**
   * Class for describing memory address snodes.
   */
  class memory_address_node : public cdk::expression_node {
    cdk::lvalue_node *_lvalue;

  public:
    memory_address_node(int lineno, cdk::lvalue_node *lvalue) :
        cdk::expression_node(lineno), _lvalue(lvalue) {
    }

    cdk::lvalue_node *lvalue() {
        return _lvalue; 
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_memory_address_node(this, level); 
    }

  };

} // til
