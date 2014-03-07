/*
 * This file is part of libFirm.
 * Copyright (C) 2012 University of Karlsruhe.
 */

/**
 * @file
 * @brief   Lower some High-level constructs, moved from the firmlower.
 * @author  Boris Boesler, Goetz Lindenmaier, Michael Beck
 */
#include "error.h"
#include "lowering.h"
#include "irmode_t.h"
#include "irnode_t.h"
#include "entity_t.h"
#include "typerep.h"
#include "irprog_t.h"
#include "ircons.h"
#include "irhooks.h"
#include "irgmod.h"
#include "irgwalk.h"

/**
 * Lower a Sel node. Do not touch Sels accessing entities on the frame type.
 */
static void lower_sel(ir_node *sel)
{
	ir_graph  *irg   = get_irn_irg(sel);
	ir_entity *ent   = get_Sel_entity(sel);
	ir_type   *owner = get_entity_owner(ent);
	dbg_info  *dbg   = get_irn_dbg_info(sel);
	ir_mode   *mode  = get_irn_mode(sel);
	ir_node   *bl    = get_nodes_block(sel);
	ir_node   *newn;

	/* we can only replace Sels when the layout of the owner type is decided. */
	if (get_type_state(owner) != layout_fixed)
		return;

	if (0 < get_Sel_n_indexs(sel)) {
		/* an Array access */
		ir_type *basetyp = get_entity_type(ent);
		ir_mode *basemode;
		if (is_Primitive_type(basetyp))
			basemode = get_type_mode(basetyp);
		else
			basemode = mode_P_data;

		assert(basemode && "no mode for lowering Sel");
		assert((get_mode_size_bits(basemode) % 8 == 0) && "can not deal with unorthodox modes");
		ir_node *index = get_Sel_index(sel, 0);

		if (is_Array_type(owner)) {
			ir_mode *mode_int = get_reference_mode_unsigned_eq(mode);
			assert(get_Sel_n_indexs(sel) == 1
				&& "array dimension must match number of indices of Sel node");

			/* Size of the array element */
			unsigned   size    = get_type_size_bytes(basetyp);
			ir_tarval *tv      = new_tarval_from_long(size, mode_int);
			ir_node   *el_size = new_rd_Const(dbg, irg, tv);
			ir_node   *ind     = new_rd_Conv(dbg, bl, index, mode_int);
			ir_node   *mul     = new_rd_Mul(dbg, bl, ind, el_size, mode_int);

			ir_node   *ptr     = get_Sel_ptr(sel);
			newn = new_rd_Add(dbg, bl, ptr, mul, mode);
		} else {
			/* no array type */
			ir_mode   *idx_mode = get_irn_mode(index);
			ir_tarval *tv       = new_tarval_from_long(get_mode_size_bytes(basemode), idx_mode);

			newn = new_rd_Add(dbg, bl, get_Sel_ptr(sel),
				new_rd_Mul(dbg, bl, index,
				new_r_Const(irg, tv),
				idx_mode),
				mode);
		}
	} else {
		int offset = get_entity_offset(ent);

		/* replace Sel by add(obj, const(ent.offset)) */
		newn = get_Sel_ptr(sel);
		if (offset != 0) {
			ir_mode   *mode_UInt = get_reference_mode_unsigned_eq(mode);
			ir_tarval *tv        = new_tarval_from_long(offset, mode_UInt);
			ir_node   *cnst      = new_r_Const(irg, tv);
			newn = new_rd_Add(dbg, bl, newn, cnst, mode);
		}
	}

	/* run the hooks */
	hook_lower(sel);

	exchange(sel, newn);
}

static void replace_by_Const(ir_node *const node, long const value)
{
	ir_graph *const irg  = get_irn_irg(node);
	ir_mode  *const mode = get_irn_mode(node);
	ir_node  *const newn = new_r_Const_long(irg, mode, value);
	/* run the hooks */
	hook_lower(node);
	exchange(node, newn);
}

/**
 * Lower an Offset node.
 */
static void lower_offset(ir_node *const offset)
{
	/* rewrite the Offset node by a Const node */
	ir_entity *const ent = get_Offset_entity(offset);
	assert(get_type_state(get_entity_type(ent)) == layout_fixed);
	replace_by_Const(offset, get_entity_offset(ent));
}

/**
 * Lower an Align node.
 */
static void lower_align(ir_node *const align)
{
	ir_type *const tp = get_Align_type(align);
	assert(get_type_state(tp) == layout_fixed);
	/* rewrite the Align node by a Const node */
	replace_by_Const(align, get_type_alignment_bytes(tp));
}

/**
 * Lower a Size node.
 */
static void lower_size(ir_node *const size)
{
	ir_type *const tp = get_Size_type(size);
	assert(get_type_state(tp) == layout_fixed);
	/* rewrite the Size node by a Const node */
	replace_by_Const(size, get_type_size_bytes(tp));
}

/**
 * lowers IR-nodes, called from walker
 */
static void lower_irnode(ir_node *irn, void *env)
{
	(void) env;
	switch (get_irn_opcode(irn)) {
	case iro_Align:  lower_align(irn);  break;
	case iro_Offset: lower_offset(irn); break;
	case iro_Sel:    lower_sel(irn);    break;
	case iro_Size:   lower_size(irn);   break;
	default:         break;
	}
}

void lower_highlevel_graph(ir_graph *irg)
{
	/* Finally: lower Offset/TypeConst-size and Sel nodes, unaligned Load/Stores. */
	irg_walk_graph(irg, NULL, lower_irnode, NULL);

	confirm_irg_properties(irg, IR_GRAPH_PROPERTIES_CONTROL_FLOW);
}

/*
 * does the same as lower_highlevel() for all nodes on the const code irg
 */
void lower_const_code(void)
{
	walk_const_code(NULL, lower_irnode, NULL);
}

void lower_highlevel()
{
	size_t i, n;

	n = get_irp_n_irgs();
	for (i = 0; i < n; ++i) {
		ir_graph *irg = get_irp_irg(i);
		lower_highlevel_graph(irg);
	}
	lower_const_code();
}
