#pragma once

#include <cdk/ast/basic_node.h>

namespace til {

  /**
   * Class for describing next nodes.
   */
  class next_node : public cdk::basic_node {
    int _cicle;

  public:
    next_node(int lineno, int cicle) :
        cdk::basic_node(lineno), _cicle(cicle) {
    }

    next_node(int lineno) :
        cdk::basic_node(lineno), _cicle(1) {
    }

    int cicle() {
        return _cicle;
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_next_node(this, level); 
    }

  };

} // til
