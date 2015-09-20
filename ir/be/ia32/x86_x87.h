/*
 * This file is part of libFirm.
 * Copyright (C) 2012 University of Karlsruhe.
 */

/**
 * @file
 * @brief       This file implements the x87 support and virtual to stack
 *              register translation for the ia32 backend.
 * @author      Michael Beck
 */
#ifndef FIRM_BE_IA32_X86_X87_H
#define FIRM_BE_IA32_X86_X87_H

#include <stdbool.h>
#include "be_types.h"
#include "firm_types.h"

/** Attributes for x87 nodes. */
typedef struct x87_attr_t {
	arch_register_t const *reg;        /**< The explicit register operand. */
	/** True if result is in the explicit register operand, %st0 otherwise. */
	bool                   res_in_reg;
	bool                   pop;        /**< Emit a pop suffix. */
} x87_attr_t;

typedef struct x87_simulator_config_t {
	const arch_register_class_t *regclass;
	ir_node *(*new_bd_fdup)(dbg_info *dbgi, ir_node *block, ir_node *value,
	                        const arch_register_t *reg);
	ir_node *(*new_bd_fxch)(dbg_info *dbgi, ir_node *block,
	                        const arch_register_t *reg);
	ir_node *(*new_bd_fpop)(dbg_info *dbgi, ir_node *block,
	                        const arch_register_t *reg);
	ir_node *(*new_bd_ffreep)(dbg_info *dbgi, ir_node *block,
	                          const arch_register_t *reg);
	x87_attr_t *(*get_x87_attr)(ir_node *node);
} x87_simulator_config_t;

typedef struct x87_state x87_state;

/**
 * The type of an instruction simulator function.
 *
 * @param state  the x87 state
 * @param n      the node to be simulated
 */
typedef void (*sim_func)(x87_state *state, ir_node *n);

void x86_prepare_x87_callbacks(void);

void x86_prepare_x87_callbacks_ia32(void);

void x86_sim_x87_load(x87_state *state, ir_node *n, ir_node *value);

void x86_sim_x87_store(x87_state *state, ir_node *n, int val_pos,
                       unsigned store_bits);

void x86_sim_x87_store_pop(x87_state *state, ir_node *n, int val_pos);

void x86_sim_x87_ret(x87_state *state, ir_node *node);

/** Push a value on the x87 stack. Intended to be used in sim functions. */
void x86_x87_push(x87_state *state, ir_node *value);

/**
 * Register a simulator function.
 *
 * @param op    the opcode to simulate
 * @param func  the simulator function for the opcode
 */
void x86_register_x87_sim(ir_op *op, sim_func func);

/**
 * Run a simulation and fix all virtual instructions for a graph.
 * Replaces all virtual floating point instructions and registers
 * by real ones.
 * Registers must be allocated.
 */
void x86_x87_simulate_graph(ir_graph *irg, const x87_simulator_config_t *cfg);

/**
 * Initializes the x87 simulator.
 */
void x86_init_x87(void);

/** Initialize x87 mode+type/ */
void x86_init_x87_type(void);

extern ir_mode *x86_mode_E;
extern ir_type *x86_type_E;

#endif