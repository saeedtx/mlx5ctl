// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <getopt.h>
#include <ctype.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"

static char *obj_name = NULL;
static unsigned int obj_id = 0;
static unsigned int op_mod = 0;
static unsigned	int bin_format = 0;

static void print_query_funcs(void);

static void help(void)
{
	fprintf(stdout, "Usage: mlx5ctl <device> obj <obj_name> --id=<obj_id> [--op_mod=op_mod] [--bin]\n");
	fprintf(stdout, "executes PRM command query_<obj_name>_in\n");
	fprintf(stdout, "hex dumps query_<obj_name>_out, unless [--bin|-B], then binary dump\n");
	fprintf(stdout, "Supported obj_names:\n");
	print_query_funcs();
}

static void parse_args(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"id", optional_argument, 0, 'i'},
		{"op_mod", optional_argument, 0, 'o'},
		{"bin", no_argument, 0, 'B'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	int c;

	obj_name = argv[1];
	if (!obj_name) {
		fprintf(stderr, "Missing obj name\n");
		help();
		exit(1);
	}

	while ((c = getopt_long(argc, argv, "io:Bh", long_options, &option_index)) != -1)
	{
		switch (c) {
		case 'i':
			obj_id = strtoul(optarg, NULL, 0);
			break;
		case 'o':
			op_mod = strtoul(optarg, NULL, 0);
			break;
		case 'B':
			bin_format = 1;
			break;
		case 'h':
			help();
			exit(0);
			break;
		default:
			fprintf(stderr, "Invalid option %c\n", c);
			exit(1);
			break;
		}
	}
}

//typedef void *(*query_obj_func)(struct mlx5u_dev *, unsigned int, unsigned int *);
typedef void (*query_obj_func)(void *, unsigned int *, unsigned int *);
#define QUERY_FUNC(name) \
	static void query_##name(void *in, unsigned int *in_sz, unsigned int *out_sz)

QUERY_FUNC(eq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_eq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_eq_in);
	MLX5_SET(query_eq_in, in, opcode, MLX5_CMD_OP_QUERY_EQ);
	MLX5_SET(query_eq_in, in, eq_number, obj_id);
	MLX5_SET(query_eq_in, in, op_mod, op_mod);
}

QUERY_FUNC(cq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_cq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_cq_in);
	MLX5_SET(query_cq_in, in, opcode, MLX5_CMD_OP_QUERY_CQ);
	MLX5_SET(query_cq_in, in, cqn, obj_id);
	MLX5_SET(query_cq_in, in, op_mod, op_mod);
}

QUERY_FUNC(qp)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_qp_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_qp_in);
	MLX5_SET(query_qp_in, in, opcode, MLX5_CMD_OP_QUERY_QP);
	MLX5_SET(query_qp_in, in, qpn, obj_id);
	MLX5_SET(query_qp_in, in, op_mod, op_mod);
}

QUERY_FUNC(sq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_sq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_sq_in);
	MLX5_SET(query_sq_in, in, opcode, MLX5_CMD_OP_QUERY_SQ);
	MLX5_SET(query_sq_in, in, sqn, obj_id);
	MLX5_SET(query_sq_in, in, op_mod, op_mod);
}

QUERY_FUNC(rq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_rq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_rq_in);
	MLX5_SET(query_rq_in, in, opcode, MLX5_CMD_OP_QUERY_RQ);
	MLX5_SET(query_rq_in, in, rqn, obj_id);
	MLX5_SET(query_rq_in, in, op_mod, op_mod);
}

QUERY_FUNC(tis)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_tis_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_tis_in);
	MLX5_SET(query_tis_in, in, opcode, MLX5_CMD_OP_QUERY_TIS);
	MLX5_SET(query_tis_in, in, tisn, obj_id);
	MLX5_SET(query_tis_in, in, op_mod, op_mod);
}

QUERY_FUNC(tir)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_tir_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_tir_in);
	MLX5_SET(query_tir_in, in, opcode, MLX5_CMD_OP_QUERY_TIR);
	MLX5_SET(query_tir_in, in, tirn, obj_id);
	MLX5_SET(query_tir_in, in, op_mod, op_mod);
}

QUERY_FUNC(rqt)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_rqt_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_rqt_in);
	MLX5_SET(query_rqt_in, in, opcode, MLX5_CMD_OP_QUERY_RQT);
	MLX5_SET(query_rqt_in, in, rqtn, obj_id);
	MLX5_SET(query_rqt_in, in, op_mod, op_mod);
}

QUERY_FUNC(rmp)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_rmp_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_rmp_in);
	MLX5_SET(query_rmp_in, in, opcode, MLX5_CMD_OP_QUERY_RMP);
	MLX5_SET(query_rmp_in, in, rmpn, obj_id);
	MLX5_SET(query_rmp_in, in, op_mod, op_mod);
}

QUERY_FUNC(dct)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_dct_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_dct_in);
	MLX5_SET(query_dct_in, in, opcode, MLX5_CMD_OP_QUERY_DCT);
	MLX5_SET(query_dct_in, in, dctn, obj_id);
	MLX5_SET(query_dct_in, in, op_mod, op_mod);
}

QUERY_FUNC(srq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_srq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_srq_in);
	MLX5_SET(query_srq_in, in, opcode, MLX5_CMD_OP_QUERY_SRQ);
	MLX5_SET(query_srq_in, in, srqn, obj_id);
	MLX5_SET(query_srq_in, in, op_mod, op_mod);
}

QUERY_FUNC(xrq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_xrq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_xrq_in);
	MLX5_SET(query_xrq_in, in, opcode, MLX5_CMD_OP_QUERY_XRC_SRQ);
	MLX5_SET(query_xrq_in, in, xrqn, obj_id);
	MLX5_SET(query_xrq_in, in, op_mod, op_mod);
}

QUERY_FUNC(xrc_srq)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_xrc_srq_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_xrc_srq_in);
	MLX5_SET(query_xrc_srq_in, in, opcode, MLX5_CMD_OP_QUERY_XRC_SRQ);
	MLX5_SET(query_xrc_srq_in, in, xrc_srqn, obj_id);
	MLX5_SET(query_xrc_srq_in, in, op_mod, op_mod);
}

QUERY_FUNC(q_counter)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_q_counter_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_q_counter_in);
	MLX5_SET(query_q_counter_in, in, opcode, MLX5_CMD_OP_QUERY_Q_COUNTER);
	MLX5_SET(query_q_counter_in, in, counter_set_id, obj_id);
	MLX5_SET(query_q_counter_in, in, op_mod, op_mod);
}

QUERY_FUNC(mkey)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_mkey_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_mkey_in);
	MLX5_SET(query_mkey_in, in, opcode, MLX5_CMD_OP_QUERY_MKEY);
	MLX5_SET(query_mkey_in, in, mkey_index, obj_id);
	MLX5_SET(query_mkey_in, in, op_mod, op_mod);
}

QUERY_FUNC(pages)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_pages_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_pages_in);
	MLX5_SET(query_pages_in, in, opcode, MLX5_CMD_OP_QUERY_PAGES);
	MLX5_SET(query_pages_in, in, function_id, obj_id);
	MLX5_SET(query_pages_in, in, op_mod, op_mod);
}

QUERY_FUNC(l2_table_entry)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_l2_table_entry_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_l2_table_entry_in);
	MLX5_SET(query_l2_table_entry_in, in, opcode, MLX5_CMD_OP_QUERY_L2_TABLE_ENTRY);
	MLX5_SET(query_l2_table_entry_in, in, table_index, obj_id);
	MLX5_SET(query_l2_table_entry_in, in, op_mod, op_mod);
}

QUERY_FUNC(issi)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_issi_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_issi_in);
	MLX5_SET(query_issi_in, in, opcode, MLX5_CMD_OP_QUERY_ISSI);
	MLX5_SET(query_issi_in, in, op_mod, op_mod);
}

QUERY_FUNC(vport_state) {
	*out_sz = MLX5_ST_SZ_BYTES(query_vport_state_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_vport_state_in);
	MLX5_SET(query_vport_state_in, in, opcode, MLX5_CMD_OP_QUERY_VPORT_STATE);
	MLX5_SET(query_vport_state_in, in, vport_number, obj_id);
	MLX5_SET(query_vport_state_in, in, other_vport, 1);
	MLX5_SET(query_vport_state_in, in, op_mod, op_mod);
}

QUERY_FUNC(esw_vport_context)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_esw_vport_context_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_esw_vport_context_in);
	MLX5_SET(query_esw_vport_context_in, in, opcode, MLX5_CMD_OP_QUERY_ESW_VPORT_CONTEXT);
	MLX5_SET(query_esw_vport_context_in, in, vport_number, obj_id);
	MLX5_SET(query_esw_vport_context_in, in, other_vport, 1);
	MLX5_SET(query_esw_vport_context_in, in, op_mod, op_mod);
}

QUERY_FUNC(vport_counter)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_vport_counter_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_vport_counter_in);
	MLX5_SET(query_vport_counter_in, in, opcode, MLX5_CMD_OP_QUERY_VPORT_COUNTER);
	MLX5_SET(query_vport_counter_in, in, vport_number, obj_id);
	MLX5_SET(query_vport_counter_in, in, other_vport, 1);
	MLX5_SET(query_vport_counter_in, in, op_mod, op_mod);
}

QUERY_FUNC(vnic_env)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_vnic_env_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_vnic_env_in);
	MLX5_SET(query_vnic_env_in, in, opcode, MLX5_CMD_OP_QUERY_VNIC_ENV);
	MLX5_SET(query_vnic_env_in, in, op_mod, op_mod);
}

QUERY_FUNC(packet_reformat_context)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_packet_reformat_context_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_packet_reformat_context_in);
	MLX5_SET(query_packet_reformat_context_in, in, opcode, MLX5_CMD_OP_QUERY_PACKET_REFORMAT_CONTEXT);
	MLX5_SET(query_packet_reformat_context_in, in, packet_reformat_id, obj_id);
	MLX5_SET(query_packet_reformat_context_in, in, op_mod, op_mod);
}

QUERY_FUNC(special_contexts)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_special_contexts_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_special_contexts_in);
	MLX5_SET(query_special_contexts_in, in, opcode, MLX5_CMD_OP_QUERY_SPECIAL_CONTEXTS);
	MLX5_SET(query_special_contexts_in, in, op_mod, op_mod);
}

QUERY_FUNC(mad_demux)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_mad_demux_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_mad_demux_in);
	MLX5_SET(query_mad_demux_in, in, opcode, MLX5_CMD_OP_QUERY_MAD_DEMUX);
	MLX5_SET(query_mad_demux_in, in, op_mod, op_mod);
}

#if 0 /*ifc is missing out for these */
QUERY_FUNC(modify_header_context)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_modify_header_context_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_modify_header_context_in);
	MLX5_SET(query_modify_header_context_in, in, opcode, MLX5_CMD_OP_QUERY_MODIFY_HEADER_CONTEXT);
	MLX5_SET(query_modify_header_context_in, in, modify_header_id, obj_id);
	MLX5_SET(query_modify_header_context_in, in, op_mod, op_mod);
}
#endif

QUERY_FUNC(cong_statistics)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_cong_statistics_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_cong_statistics_in);
	MLX5_SET(query_cong_statistics_in, in, opcode, MLX5_CMD_OP_QUERY_CONG_STATISTICS);
	MLX5_SET(query_cong_statistics_in, in, op_mod, op_mod);
}

QUERY_FUNC(cong_params)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_cong_params_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_cong_params_in);
	MLX5_SET(query_cong_params_in, in, opcode, MLX5_CMD_OP_QUERY_CONG_PARAMS);
	MLX5_SET(query_cong_params_in, in, cong_protocol, obj_id);
	MLX5_SET(query_cong_params_in, in, op_mod, op_mod);
}

QUERY_FUNC(cong_status)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_cong_status_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_cong_status_in);
	MLX5_SET(query_cong_status_in, in, opcode, MLX5_CMD_OP_QUERY_CONG_STATUS);
	MLX5_SET(query_cong_status_in, in, priority, (obj_id >> 4) & 0xf);
	MLX5_SET(query_cong_status_in, in, cong_protocol, obj_id & 0xf);
	MLX5_SET(query_cong_status_in, in, op_mod, op_mod);
}

QUERY_FUNC(adapter)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_adapter_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_adapter_in);
	MLX5_SET(query_adapter_in, in, opcode, MLX5_CMD_OP_QUERY_ADAPTER);
	MLX5_SET(query_adapter_in, in, op_mod, op_mod);
}

QUERY_FUNC(wol_rol)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_wol_rol_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_wol_rol_in);
	MLX5_SET(query_wol_rol_in, in, opcode, MLX5_CMD_OP_QUERY_WOL_ROL);
	MLX5_SET(query_wol_rol_in, in, op_mod, op_mod);
}

QUERY_FUNC(lag)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_lag_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_lag_in);
	MLX5_SET(query_lag_in, in, opcode, MLX5_CMD_OP_QUERY_LAG);
	MLX5_SET(query_lag_in, in, op_mod, op_mod);
}

QUERY_FUNC(esw_functions)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_esw_functions_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_esw_functions_in);
	MLX5_SET(query_esw_functions_in, in, opcode, MLX5_CMD_OP_QUERY_ESW_FUNCTIONS);
	MLX5_SET(query_esw_functions_in, in, op_mod, op_mod);
}

QUERY_FUNC(vhca_migration_state)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_vhca_migration_state_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_vhca_migration_state_in);
	MLX5_SET(query_vhca_migration_state_in, in, opcode, MLX5_CMD_OP_QUERY_VHCA_MIGRATION_STATE);
	MLX5_SET(query_vhca_migration_state_in, in, vhca_id, obj_id);
	MLX5_SET(query_vhca_migration_state_in, in, op_mod, op_mod);
}

#if 0
QUERY_FUNC(sf_partitions)
{
	*out_sz = MLX5_ST_SZ_BYTES(query_sf_partitions_out);
	*in_sz = MLX5_ST_SZ_BYTES(query_sf_partitions_in);
	MLX5_SET(query_sf_partitions_in, in, opcode, MLX5_CMD_OP_QUERY_SF_PARTITIONS);
	MLX5_SET(query_sf_partitions_in, in, op_mod, op_mod);
}
#endif


struct obj_name_func {
	const char *obj_name;
	query_obj_func obj_func;
	const char *help;
};

#define QUERY_PAIR(name, help) {#name, query_##name, help}
struct obj_name_func query_funcs[] = {
	QUERY_PAIR(eq, "--id=eqn"),
	QUERY_PAIR(cq, "--id=cqn"),
	QUERY_PAIR(qp, "--id=qpn"),
	QUERY_PAIR(sq, "--id=sqn"),
	QUERY_PAIR(rq, "--id=rqn"),
	QUERY_PAIR(tis, "--id=tisn"),
	QUERY_PAIR(tir, "--id=tirn"),
	QUERY_PAIR(rqt, "--id=rqtn"),
	QUERY_PAIR(rmp, "--id=rmpn"),
	QUERY_PAIR(dct, "--id=dctn"),
	QUERY_PAIR(srq, "--id=srqn"),
	QUERY_PAIR(xrq, "--id=xrqn"),
	QUERY_PAIR(xrc_srq, "--id=xrc_srqn"),
	QUERY_PAIR(q_counter, "--id=counter_set_id"),
	QUERY_PAIR(mkey, "--id=mkey_index"),
	QUERY_PAIR(pages, "--id=function_id"),
	QUERY_PAIR(l2_table_entry, "--id=table_index"),
	QUERY_PAIR(issi, "--op_mod=op_mod"),
	QUERY_PAIR(vport_state, "--id=vport_number (other_vport=1)"),
	QUERY_PAIR(esw_vport_context, "--id=vport_number (other_vport=1)"),
	QUERY_PAIR(vport_counter, "--id=vport_number (other_vport=1)"),
	QUERY_PAIR(vnic_env, "--op_mod=op_mod"),
	QUERY_PAIR(packet_reformat_context, "--id=packet_reformat_id"),
	QUERY_PAIR(special_contexts, "--op_mod=op_mod"),
	QUERY_PAIR(mad_demux, "--op_mod=op_mod"),
	QUERY_PAIR(cong_statistics, "--op_mod=op_mod"),
	QUERY_PAIR(cong_params, "--id=cong_protocol"),
	QUERY_PAIR(cong_status, "--id=priority:cong_protocol"),
	QUERY_PAIR(adapter, "--op_mod=op_mod"),
	QUERY_PAIR(wol_rol, "--op_mod=op_mod"),
	QUERY_PAIR(lag, "--op_mod=op_mod"),
	QUERY_PAIR(esw_functions, "--op_mod=op_mod"),
	QUERY_PAIR(vhca_migration_state, "--id=vhca_id"),
};

int num_query_funcs = sizeof(query_funcs) / sizeof(query_funcs[0]);

static void print_query_funcs(void)
{
	for (int i = 0; i < num_query_funcs; i++)
		printf("\t%s %s, dump PRM name: query_%s_out\n", query_funcs[i].obj_name,
		       query_funcs[i].help,  query_funcs[i].obj_name);
}

static query_obj_func get_query_func(char *obj_name)
{
	if (!obj_name)
		return NULL;

	for (int i = 0; i < num_query_funcs; i++) {
		if (!strcmp(obj_name, query_funcs[i].obj_name))
			return query_funcs[i].obj_func;
	}
	return NULL;
}

static char in[4096] =	{}; /* big enough buffer to fit any query_xxx_in */
int query_obj(struct mlx5u_dev *dev, int argc, char *argv[])
{
	query_obj_func fun = get_query_func(argv[1]);
	unsigned int out_sz, in_sz;
	void *out;
	int err;

	parse_args(argc, argv);
	if (!fun) {
		fprintf(stderr, "Invalid obj name %s\n", obj_name);
		return 1;
	}

	fun(in, &in_sz, &out_sz);
	out = malloc(out_sz);
	if (!out) {
		fprintf(stderr, "Failed to allocate %d bytes\n", out_sz);
		return 1;
	}
	memset(out, 0, out_sz);

	err = mlx5u_cmd(dev, in, in_sz, out, out_sz);
	if (err < 0) {
		fprintf(stderr, "Failed to query %s id=%d op_mod=0x%x err(%d)\n",
			obj_name, obj_id, op_mod, err);
		free(out);
		return 1;
	}

	if (err > 0)
		fprintf(stderr, "Warning: %s id=%d op_mod=0x%x returned %d\n",
			obj_name, obj_id, op_mod, err);

	if (bin_format)
		fwrite(out, out_sz, 1, stdout);
	else
		hexdump(out, out_sz);
	free(out);
	return 0;
}
