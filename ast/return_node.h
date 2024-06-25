#pragma once

#include <cdk/ast/basic_node.h>


namespace til {

  /**
   * Class for describing return nodes.
   */
  class return_node : public cdk::basic_node {
    cdk::expression_node *_returnVal;

  public:
    return_node(int lineno, cdk::expression_node *returnVal) :
        cdk::basic_node(lineno), _returnVal(returnVal){
    }

    cdk::expression_node *returnVal() {
        return _returnVal;
    }

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_return_node(this, level); 
    }

  };

} // til
