#pragma once

#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <string>

namespace til {

  /**
   * Class for describing function call nodes.
   */
  class function_call_node : public cdk::expression_node {
    cdk::expression_node *_function;
    cdk::sequence_node *_arguments;

  public:
    function_call_node(int lineno,  cdk::expression_node *function, cdk::sequence_node *arguments) :
        cdk::expression_node(lineno), _function(function), _arguments(arguments) {
    }

	cdk::expression_node *function() {
    return _function;
  }
  
	cdk::sequence_node *arguments() {
		return _arguments;
	}

    void accept(basic_ast_visitor *sp, int level) {
        sp->do_function_call_node(this, level); 
    }

  };

} // til
