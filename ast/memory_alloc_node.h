#pragma once

#include <cdk/ast/unary_operation_node.h>

namespace til {

  /**
   * Class for describing memory alloc nodes.
   */
  class memory_alloc_node : public cdk::unary_operation_node {
  public:
    memory_alloc_node(int lineno, cdk::expression_node *argument) :
        cdk::unary_operation_node(lineno, argument) {
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_memory_alloc_node(this, level); 
    }

  };

} // til
