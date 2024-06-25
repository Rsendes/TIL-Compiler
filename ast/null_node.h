#pragma once

#include <cdk/ast/expression_node.h>

namespace til {

  /**
   * Class for describing null nodes.
   */
  class null_node : public cdk::expression_node {
  public:
    null_node(int lineno):
        cdk::expression_node(lineno) {
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_null_node(this, level); 
    }

  };

} // til
