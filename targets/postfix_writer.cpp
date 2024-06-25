#include "targets/type_checker.h"
#include "targets/postfix_writer.h"
#include "targets/frame_size_calculator.h"
#include ".auto/all_nodes.h"  // all_nodes.h is automatically generated

//---------------------------------------------------------------------------

void til::postfix_writer::do_nil_node(cdk::nil_node * const node, int lvl) {
  // EMPTY
}
void til::postfix_writer::do_data_node(cdk::data_node * const node, int lvl) {
  // EMPTY
}
void til::postfix_writer::do_double_node(cdk::double_node * const node, int lvl) {
  if (_inFunctionBody) {
    _pf.DOUBLE(node->value()); // load number to the stack
  } else {
    _pf.SDOUBLE(node->value());    // double is on the DATA segment
  }
}
void til::postfix_writer::do_not_node(cdk::not_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->argument()->accept(this, lvl + 2);
  _pf.INT(0);
  _pf.EQ();
}

void til::postfix_writer::do_and_node(cdk::and_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));
  node->right()->accept(this, lvl);
  _pf.AND();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));

}

void til::postfix_writer::do_or_node(cdk::or_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  int lbl = ++_lbl;
  node->left()->accept(this, lvl + 2);
  _pf.DUP32();
  _pf.JZ(mklbl(lbl));
  node->right()->accept(this, lvl);
  _pf.OR();
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl));

}

void til::postfix_writer::do_block_node(til::block_node * const node, int lvl) {
  _symtab.push(); // for block-local variables

  if (node->declarations()) {
    node->declarations()->accept(this, lvl + 2);
  }
  if (node->instructions()) {
    node->instructions()->accept(this, lvl + 2);
  }

  _symtab.pop();
}

void til::postfix_writer::do_stop_node(til::stop_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if ((size_t)node->cicle() > _whileEnd.size()) {
    throw std::string("Stop goes outside of all loops");
  }

  int number_stops = node->cicle();
  std::stack<int> _whileTempEnd = _whileEnd;

  while(number_stops > 1) {
    _whileTempEnd.pop();
    number_stops -= 1;
  }

  _pf.JMP(mklbl(_whileTempEnd.top()));
}

void til::postfix_writer::do_next_node(til::next_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  if ((size_t)node->cicle() > _whileEnd.size()) {
    throw std::string("Next goes outside of all loops");
  }

  int number_stops = node->cicle();
  std::stack<int> _whileTempInit = _whileInit;

  while(number_stops > 1) {
    _whileTempInit.pop();
    number_stops -= 1;
  }

  _pf.JMP(mklbl(_whileTempInit.top()));
} 
void til::postfix_writer::do_return_node(til::return_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto symbol = _symtab.find("@", 1);

  auto returnType = std::shared_ptr<cdk::basic_type>();

  if (_inMainFunction)
    return;

  // if the function is main, it has no type
  if (symbol == nullptr) 
    return;
  
  returnType = cdk::functional_type::cast(symbol->type())->output(0);

  // check if the function isn't void
  if (returnType->name() != cdk::TYPE_VOID) {
    node->returnVal()->accept(this, lvl);
    if (returnType->name() == cdk::TYPE_INT) {
      _pf.STFVAL32();
    } else if (returnType->name() == cdk::TYPE_DOUBLE) {
      _pf.STFVAL64();
    }
  }

  _pf.JMP(_currentFunctionLabel);
}

void til::postfix_writer::do_null_node(til::null_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  if (_inFunctionBody) {
    _pf.INT(0);
  } else {
    _pf.SINT(0);
  }
}

void til::postfix_writer::do_sizeof_node(til::sizeof_node * const node, int lvl) {
    ASSERT_SAFE_EXPRESSIONS;
    _pf.INT(node->expression()->type()->size());
}

void til::postfix_writer::do_indexing_node(til::indexing_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->pointer()->accept(this, lvl + 2);
  node->index()->accept(this, lvl + 2);
  _pf.INT(node->type()->size());
  _pf.MUL();
  _pf.ADD();
}

void til::postfix_writer::do_memory_address_node(til::memory_address_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl + 2);
}

void til::postfix_writer::do_memory_alloc_node(til::memory_alloc_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto ref = cdk::reference_type::cast(node->type())->referenced();
  node->argument()->accept(this, lvl);
  // void has size 0, but we want to alloc 1 byte for it
  _pf.INT(std::max(static_cast<size_t>(1), ref->size()));
  _pf.MUL();
  _pf.ALLOC();
  _pf.SP();
}

void til::postfix_writer::do_function_definition_node(til::function_definition_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  std::string functionLabel = mklbl(++_lbl);

  _functionLabels.push(functionLabel);
  _pf.TEXT(functionLabel);
  _pf.ALIGN();
  _pf.LABEL(functionLabel);

  int oldOffset = _offset;
  _offset = 8;
  _symtab.push();

  node->arguments()->accept(this, lvl);

  frame_size_calculator lsc(_compiler, _symtab);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize());

  auto oldFunctionRetLabel = _currentFunctionRetLabel;
  _currentFunctionRetLabel = mklbl(++_lbl);

  _offset = 0;

  node->block()->accept(this, lvl);

  _pf.ALIGN();
  _pf.LABEL(_currentFunctionRetLabel);
  _pf.LEAVE();
  _pf.RET();

  _currentFunctionRetLabel = oldFunctionRetLabel;
  _offset = oldOffset;
  _symtab.pop();
  _functionLabels.pop();

  //Function can be an expression 
  if (_functionLabels.size() > 0) {
    _pf.TEXT(_functionLabels.top());
    _pf.ADDR(functionLabel);
  }
}
void til::postfix_writer::do_function_call_node(til::function_call_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _inMainFunction = 0;
  std::shared_ptr<cdk::functional_type> func_type;
  if (node->function() == nullptr) { // recursive call; "@"
    auto symbol = _symtab.find("@", 1);
    func_type = cdk::functional_type::cast(symbol->type());
  } else {
    func_type = cdk::functional_type::cast(node->function()->type());
  }

  int args_size = 0;
  // arguments must be visited in reverse order since the first argument has to be
  // on top of the stack
  for (size_t i = node->arguments()->size(); i > 0; i--) {
    auto arg = dynamic_cast<cdk::expression_node*>(node->arguments()->node(i - 1));

    args_size += arg->type()->size();
    arg->accept(this, lvl);
  }

  if (node->function() == nullptr) { // recursive call; "@"
    _pf.ADDR(_functionLabels.top());
  } else {
    node->function()->accept(this, lvl);
  }

  if (args_size > 0) {
    _pf.TRASH(args_size);
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDFVAL64();
  } else if (!node->is_typed(cdk::TYPE_VOID)) {
    _pf.LDFVAL32();
  }

}

void til::postfix_writer::do_variable_declaration_node(til::variable_declaration_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto id = node->identifier();
  int typesize = node->type()->size();
  int offset = 0;

  auto symbol = new_symbol();

  std::cout << "ARG: " << id << ", " << typesize << std::endl;
  if (_inFunctionBody) {
    std::cout << "IN BODY" << std::endl;
    _offset -= typesize;
    offset = _offset;
    std::cout << _offset << std::endl;
  } else {
    offset = 0;
  }

  std::cout << "OFFSET: " << id << ", " << offset << std::endl;
  if (symbol) {
    symbol->set_offset(offset);
    reset_new_symbol();
  }

  if (_inFunctionBody) {
    if (node->initializer()) {
      node->initializer()->accept(this, lvl);
      if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_STRING) || node->is_typed(cdk::TYPE_POINTER)) {
        _pf.LOCAL(symbol->offset());
        _pf.STINT();
      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (node->initializer()->is_typed(cdk::TYPE_INT))
          _pf.I2D();
        _pf.LOCAL(symbol->offset());
        _pf.STDOUBLE();
      } else {
        std::cerr << "cannot initialize" << std::endl;
      }
    }
  } else {
    if (node->initializer() == nullptr) {
      _pf.BSS();
      _pf.ALIGN();
      _pf.LABEL(id);
      _pf.SALLOC(typesize);
    } else {
      _pf.DATA();
      _pf.ALIGN();
      _pf.LABEL(id);

      if (node->is_typed(cdk::TYPE_INT) || node->is_typed(cdk::TYPE_POINTER) ||  node->is_typed(cdk::TYPE_STRING)) {
        node->initializer()->accept(this, lvl);
      } else if (node->is_typed(cdk::TYPE_DOUBLE)) {
        if (node->initializer()->is_typed(cdk::TYPE_DOUBLE)) {
          node->initializer()->accept(this, lvl);
        } else if (node->initializer()->is_typed(cdk::TYPE_INT)) {
          cdk::integer_node *dclini = dynamic_cast<cdk::integer_node*>(node->initializer());
          cdk::double_node ddi(dclini->lineno(), dclini->value());
          ddi.accept(this, lvl);
        } else {
          std::cerr << node->lineno() << ": '" << id << "' has bad initializer for real value\n";
        }
      } 
    }
  }
}
  
//---------------------------------------------------------------------------

void til::postfix_writer::do_sequence_node(cdk::sequence_node * const node, int lvl) {
  for (size_t i = 0; i < node->size(); i++) {
    node->node(i)->accept(this, lvl);
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_integer_node(cdk::integer_node * const node, int lvl) {
  
  if (_inFunctionBody) {
    _pf.INT(node->value()); // integer literal is on the stack: push an integer
  } else {
    _pf.SINT(node->value()); // integer literal is on the DATA segment
  }
}

void til::postfix_writer::do_string_node(cdk::string_node * const node, int lvl) {
  int lbl1;

  /* generate the string */
  _pf.RODATA(); // strings are DATA readonly
  _pf.ALIGN(); // make sure we are aligned
  _pf.LABEL(mklbl(lbl1 = ++_lbl)); // give the string a name
  _pf.SSTRING(node->value()); // output string characters

  /* leave the address on the stack */
   if (_inFunctionBody) {
    // local variable initializer
    _pf.TEXT();
    _pf.ADDR(mklbl(lbl1));
  } else {
    // global variable initializer
    _pf.DATA();
    _pf.SADDR(mklbl(lbl1));
  }

}

//---------------------------------------------------------------------------

void til::postfix_writer::do_unary_minus_node(cdk::unary_minus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
  _pf.NEG(); // 2-complement
}

void til::postfix_writer::do_unary_plus_node(cdk::unary_plus_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_add_node(cdk::add_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  
  node->left()->accept(this, lvl + 2);
  
  if (node->is_typed(cdk::TYPE_DOUBLE) && !node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.I2D();
  } else if (node->right()->is_typed(cdk::TYPE_POINTER)) {
    _pf.INT(cdk::reference_type::cast(node->type())->referenced()->size());
    _pf.MUL();
  }
   
  node->right()->accept(this, lvl + 2);
  
  if (node->is_typed(cdk::TYPE_DOUBLE) && !node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.I2D();
  } else if (node->left()->is_typed(cdk::TYPE_POINTER)) {
    _pf.INT(cdk::reference_type::cast(node->type())->referenced()->size());
    _pf.MUL();
    }
  
  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DADD();
  } else {
    _pf.ADD();
  }
}
void til::postfix_writer::do_sub_node(cdk::sub_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) && !node->left()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.I2D();
  } else if (!node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER)) {
    _pf.INT(cdk::reference_type::cast(node->type())->referenced()->size());
    _pf.MUL();
  }
  node->right()->accept(this, lvl + 2);
  if (node->is_typed(cdk::TYPE_DOUBLE) && !node->right()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.I2D();
  } else if (node->left()->is_typed(cdk::TYPE_POINTER) && !node->right()->is_typed(cdk::TYPE_POINTER)) {
    _pf.INT(cdk::reference_type::cast(node->type())->referenced()->size());
    _pf.MUL();
  }

  if (node->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.DSUB();
  } else {
    _pf.SUB();
    if (node->left()->is_typed(cdk::TYPE_POINTER) && node->right()->is_typed(cdk::TYPE_POINTER) && 
        cdk::reference_type::cast(node->left()->type())->referenced()->name() != cdk::TYPE_VOID) { 
      _pf.INT(cdk::reference_type::cast(node->left()->type())->referenced()->size());
      _pf.DIV();
    }
  }
}
void til::postfix_writer::do_mul_node(cdk::mul_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MUL();
}
void til::postfix_writer::do_div_node(cdk::div_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.DIV();
}
void til::postfix_writer::do_mod_node(cdk::mod_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.MOD();
}
void til::postfix_writer::do_lt_node(cdk::lt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.LT();
}
void til::postfix_writer::do_le_node(cdk::le_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.LE();
}
void til::postfix_writer::do_ge_node(cdk::ge_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.GE();
}
void til::postfix_writer::do_gt_node(cdk::gt_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.GT();
}
void til::postfix_writer::do_ne_node(cdk::ne_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.NE();
}
void til::postfix_writer::do_eq_node(cdk::eq_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->left()->accept(this, lvl);
  node->right()->accept(this, lvl);
  _pf.EQ();
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_variable_node(cdk::variable_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  auto symbol = _symtab.find(node->name());
  if (symbol->global()) {
    _pf.ADDR(symbol->name());
  } else {
    _pf.LOCAL(symbol->offset());
  }
}

void til::postfix_writer::do_rvalue_node(cdk::rvalue_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->lvalue()->accept(this, lvl);

  if (node->lvalue()->is_typed(cdk::TYPE_DOUBLE)) {
    _pf.LDDOUBLE();
  } else {
    _pf.LDINT();
  }
}

void til::postfix_writer::do_assignment_node(cdk::assignment_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  node->rvalue()->accept(this, lvl + 2);

  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    if (node->rvalue()->type()->name() == cdk::TYPE_INT) _pf.I2D();
    _pf.DUP64();
  } else {
    _pf.DUP32();
  }

  if (new_symbol() == nullptr) {
    node->lvalue()->accept(this, lvl);
  } else {
    _pf.DATA();
    _pf.ALIGN();
    _pf.LABEL(new_symbol()->name());
    reset_new_symbol();
    _pf.SINT(0);
    _pf.TEXT();
    node->lvalue()->accept(this, lvl);
  }
  
  if (node->type()->name() == cdk::TYPE_DOUBLE) {
    _pf.STDOUBLE();
  } else {
    _pf.STINT();
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_program_node(til::program_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _offset = 8; // prepare for arguments (4: remember to account for return address)
  _symtab.push(); // scope of args

  _pf.TEXT();
  _pf.ALIGN();
  _pf.GLOBAL("_main", _pf.FUNC());
  _pf.LABEL("_main");

  frame_size_calculator lsc(_compiler, _symtab);
  node->accept(&lsc, lvl);
  _pf.ENTER(lsc.localsize());

  _offset = 0;
  _inFunctionBody = 1;
  _inMainFunction = 1;

  node->statements()->accept(this, lvl);

  _inMainFunction = 1;

  _pf.INT(0);
  _pf.STFVAL32();
  _pf.LEAVE();
  _pf.RET();

  _symtab.pop();

  // these are just a few library function imports
  _pf.EXTERN("readi");
  _pf.EXTERN("printi");
  _pf.EXTERN("printd");
  _pf.EXTERN("prints");
  _pf.EXTERN("println");
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_evaluation_node(til::evaluation_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  node->argument()->accept(this, lvl); // determine the value

  if (node->argument()->is_typed(cdk::TYPE_INT) || node->argument()->is_typed(cdk::TYPE_POINTER) || node->argument()->is_typed(cdk::TYPE_DOUBLE) ) {
    _pf.TRASH(4); // delete the evaluated value
  } else if (node->argument()->is_typed(cdk::TYPE_STRING)) {
    _pf.TRASH(4); // delete the evaluated value's address
  } else {
    std::cerr << "ERROR: CANNOT HAPPEN!" << std::endl;
    exit(1);
  }
}

void til::postfix_writer::do_print_node(til::print_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  for (size_t arg_num = 0; arg_num < node->arguments()->size(); arg_num++) {
    auto child = dynamic_cast<cdk::expression_node*>(node->arguments()->node(arg_num));

    std::shared_ptr<cdk::basic_type> arg_type = child->type();
    child->accept(this, lvl);

    if (arg_type->name() == cdk::TYPE_INT) {
      _pf.CALL("printi");
      _pf.TRASH(4); // delete the printed value's address
    } else if (arg_type->name() == cdk::TYPE_DOUBLE) {
      _pf.CALL("printd");
      _pf.TRASH(8);
    } else if (arg_type->name() == cdk::TYPE_STRING) {
      _pf.CALL("prints");
      _pf.TRASH(4);
    } else {
      std::cerr << "cannot print expression of unkown type" << std::endl;
      return;
    }
  }
  
  if (node->newLine()) {
    _pf.CALL("println");
  }
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_read_node(til::read_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  _pf.CALL("readi");
  _pf.LDFVAL32();
  node->accept(this, lvl);
  _pf.STINT();
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_while_node(til::while_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;

  _whileInit.push(++_lbl);
  _whileEnd.push(++_lbl);
  _pf.ALIGN();
  _pf.LABEL(mklbl(_whileInit.top()));
  node->condition()->accept(this, lvl + 2);
  _pf.JZ(mklbl(_whileEnd.top()));
  node->block()->accept(this, lvl + 2);
  _pf.JMP(mklbl(_whileInit.top()));
  _pf.ALIGN();
  _pf.LABEL(mklbl(_whileEnd.top()));

  _whileInit.pop();
  _whileEnd.pop();
}

//---------------------------------------------------------------------------

void til::postfix_writer::do_if_node(til::if_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->block()->accept(this, lvl + 2);
  _pf.LABEL(mklbl(lbl1));
}

void til::postfix_writer::do_if_else_node(til::if_else_node * const node, int lvl) {
  ASSERT_SAFE_EXPRESSIONS;
  int lbl1, lbl2;
  node->condition()->accept(this, lvl);
  _pf.JZ(mklbl(lbl1 = ++_lbl));
  node->thenblock()->accept(this, lvl + 2);
  _pf.JMP(mklbl(lbl2 = ++_lbl));
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1));
  node->elseblock()->accept(this, lvl + 2);
  _pf.ALIGN();
  _pf.LABEL(mklbl(lbl1 = lbl2));
}

