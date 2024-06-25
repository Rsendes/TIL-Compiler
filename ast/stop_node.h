#pragma once

#include <cdk/ast/basic_node.h>

namespace til {

  /**
   * Class for describing stop nodes.
   */
  class stop_node : public cdk::basic_node {
    int _cicle;

  public:
    stop_node(int lineno, int cicle) :
        cdk::basic_node(lineno), _cicle(cicle) {
    }

    stop_node(int lineno) :
        cdk::basic_node(lineno), _cicle(1) {
    }

    int cicle() {
        return _cicle;
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_stop_node(this, level); 
    }

  };

} // til
