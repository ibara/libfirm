/*
 * Project:     libFIRM
 * File name:   testprograms/call_str_example.c
 * Purpose:     Shows representation of constant string.
 * Author:      Goetz Lindenmaier
 * Modified by:
 * Created:
 * CVS-ID:      $Id$
 * Copyright:   (c) 1999-2003 Universitšt Karlsruhe
 * Licence:     This file protected by GPL -  GNU GENERAL PUBLIC LICENSE.
 */

# include <string.h>
# include <stdio.h>

# include "irvrfy.h"
# include "irdump.h"
# include "firm.h"

/**
 *  This file constructs the ir for the following pseudo-program:
 *
 *  void f(char *);
 *
 *  void CALL_STR_EXAMPLE_main () {
 *      f("Hello World\n");
 *  }
 *
 *  This program demonstrates how to represent string constants.
 */

int main(int argc, char **argv)
{
  ir_graph *irg;         /* this variable contains the irgraph */
  type     *owner;       /* the class in which this method is defined */
  type     *proc_main;   /* type information for the method main */
  type     *proc_called; /* type information for called method f */
  type     *U8, *U8array, *string_ptr;  /* type for pointers to strings. */
  entity   *ent;         /* represents this method as entity of owner */
  entity   *const_str;   /* represents a constant string. */
  char     *str = "Hello World\n"; /* The constant string. */
  ir_node  *x, *str_addr, *proc_ptr, *call;
  int i;

  printf("\nCreating an IR graph: CALL_STR_EXAMPLE...\n");

  /* init library */
  init_firm (NULL);

  /* An unsinged 8 bit type */
  U8 = new_type_primitive (new_id_from_chars("char", 4), mode_Bu);
  /* An array containing unsigned 8 bit elements. */
  U8array = new_type_array (new_id_from_chars("char_arr", 8), 1, U8);
  set_array_lower_bound_int(U8array, 0, 0);

  string_ptr = new_type_pointer (new_id_from_chars ("ptr_to_string", 13), U8array, mode_P);

  /* Make a global entity that represents the constant String. */
  const_str = new_entity(get_glob_type(), new_id_from_str("constStr"), U8array);
  set_entity_variability(const_str, variability_constant);
  for (i = 0; i < strlen(str); i++) {
    tarval *val = new_tarval_from_long(str[i], mode_Bu);
    ir_node *con =  new_Const(mode_Bu, val);
    add_compound_ent_value(const_str, con, get_array_element_entity(U8array));
  }

  /* FIRM was designed for oo languages where all methods belong to a class.
   * For imperative languages like C we view a program as a large class containing
   * all functions of the program as methods in this class.  This class is
   * automatically generated.
   * We use the same name for the method type as for the method entity.
   */
#define METHODNAME "CALL_STR_EXAMPLE_main"
#define NRARGS 0
#define NRES 0
  owner = get_glob_type();
  proc_main = new_type_method(new_id_from_chars(METHODNAME, strlen(METHODNAME)),
                              NRARGS, NRES);

  /* Make type information for called method which also belongs to the
     global type. */
#define F_METHODNAME "f"
#define F_NRARGS 1
#define F_NRES 0
  owner = get_glob_type();
  proc_called = new_type_method(new_id_from_chars(F_METHODNAME, strlen(F_METHODNAME)),
                              F_NRARGS, F_NRES);
  set_method_param_type(proc_called, 0, string_ptr);


  /* Make the entity for main needed for a correct  ir_graph.  */
#define ENTITYNAME "CALL_STR_EXAMPLE_main"
  ent = new_entity (owner, new_id_from_chars (ENTITYNAME, strlen(ENTITYNAME)),
                    proc_main);

  /* Generates the basic graph for the method represented by entity ent, that
   * is, generates start and end blocks and nodes and a first, initial block.
   * The constructor needs to know how many local variables the method has.
   */
#define NUM_OF_LOCAL_VARS 0
  irg = new_ir_graph (ent, NUM_OF_LOCAL_VARS);

  /* get the pointer to the string constant */
  symconst_symbol sym;
  sym.entity_p = const_str;
  str_addr = new_SymConst(sym, symconst_addr_ent);

  /* get the pointer to the procedure from the class type */
  /* this is how a pointer to be fixed by the linker is represented. */
  sym.ident_p = new_id_from_str (F_METHODNAME);
  proc_ptr = new_SymConst (sym, symconst_addr_name);

  /* call procedure set_a, first built array with parameters */
  {
    ir_node *in[1];
    in[0] = str_addr;
    call = new_Call(get_store(), proc_ptr, 1, in, proc_called);
  }
  /* make the possible changes by the called method to memory visible */
  set_store(new_Proj(call, mode_M, 0));

  /* Make the return node returning the memory. */
  x = new_Return (get_store(), 0, NULL);
  /* Now we generated all instructions for this block and all its predecessor blocks
   * so we can mature it. */
  mature_immBlock (get_irg_current_block(irg));

  /* This adds the in edge of the end block which originates at the return statement.
   * The return node passes controlflow to the end block.  */
  add_immBlock_pred (get_irg_end_block(irg), x);
  /* Now we can mature the end block as all it's predecessors are known. */
  mature_immBlock (get_irg_end_block(irg));

  irg_finalize_cons (irg);

  printf("Optimizing ...\n");
  dead_node_elimination(irg);

  /* verify the graph */
  irg_vrfy(irg);

  printf("Done building the graph.  Dumping it.\n");
  char *dump_file_suffix = "";
  dump_ir_block_graph (irg, dump_file_suffix);
  dump_all_types(dump_file_suffix);
  printf("Use xvcg to view this graph:\n");
  printf("/ben/goetz/bin/xvcg GRAPHNAME\n\n");

  return (0);
}
