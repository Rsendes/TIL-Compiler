#pragma once

#include <cdk/ast/sequence_node.h>
#include <cdk/ast/expression_node.h>
#include <string>
#include "ast/block_node.h"

namespace til {

  /**
   * Class for describing function definition nodes.
   */
  class function_definition_node : public cdk::expression_node {
    int _qualifier;
    cdk::sequence_node *_arguments;
    til::block_node *_block;

  public:
    function_definition_node(int lineno, int qualifier, std::shared_ptr<cdk::basic_type> function_type,
      cdk::sequence_node *arguments, til::block_node *block) :
        cdk::expression_node(lineno), _qualifier(qualifier), _arguments(arguments), _block(block) {
          type(function_type);
    }

	int qualifier() {
		return _qualifier;
	}

	cdk::sequence_node *arguments() {
		return _arguments;
	}

  til::block_node* block() {
      return _block;
  }

  void accept(basic_ast_visitor *sp, int level) {
      sp->do_function_definition_node(this, level); 
  }

  };

} // til
