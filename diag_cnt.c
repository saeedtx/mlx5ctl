// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"

struct diag_cnt_desc {
	u16 id;
	const char *short_desc;
	const char *long_desc;
	const char *unit;
};

static const struct diag_cnt_desc diag_cnt_desc[] = {
	{ 0x0402, "Outbound Stalled Writes", "No PCIe posted credits to write data", "Device clocks" },
	{ 0x0403, "Outbound Stalled Reads", "No PCIe non-posted credits to read data", "Device clocks" },
	{ 0x0409, "PCI Read Stalled due to No Read Engines", "All PCIe read engines are occupied", "Device clocks" },
	{ 0x040a, "PCI Read Stalled due to No Completion Buffer", "PCIe buffer is full", "Device clocks" },
	{ 0x040b, "PCI Read Stalled due to Ordering", "PCIe writes are stalling PCIe reads", "Device clocks" },
	{ 0x0401, "Back Pressure from Datalink to Transport Unit", "Back pressure from PCIe block to NIC blocks, valid only if PCIe stalled counters are increasing", "Device clocks" },
	{ 0x0406, "RX 128B Data", "Inbound PCIe Data", "128 Bytes" },
	{ 0x1001, "RX Steering Packets", "All packets which went thru receive pipe before going to QPs", "Packets" },
	{ 0x2006, "TX Packets", "All packets in TX pipe", "Packets" },
	{ 0x2c02, "Line Transmitted Port 1", "Data sent from port 1", "TBD" },
	{ 0x2c03, "Line Transmitted Port 2", "Data sent from port 2", "TBD" },
	{ 0x2c04, "Line Transmitted Loop Back", "Data sent back receive pipe", "TBD" },
	{ 0x2001, "Send Queue Stopped due to Limited VL", "Transmit buffer is full", "Device clocks" },
};

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static char *help_cmd;
static int get_dev_freq(struct mlx5u_dev *dev);

static char *diag_counter_desc(u16 id)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(diag_cnt_desc); i++) {
		if (diag_cnt_desc[i].id == id)
			return (char *)diag_cnt_desc[i].short_desc;
	}
	return "N/A";
}

enum mlx5_cap_type {
	MLX5_CAP_GENERAL = 0,
	MLX5_CAP_DEBUG = 13,
};

enum mlx5_cap_mode {
	HCA_CAP_OPMOD_GET_MAX	= 0,
	HCA_CAP_OPMOD_GET_CUR	= 1,
};

static int query_devcap(struct mlx5u_dev *dev, enum mlx5_cap_type type,
			void *out, unsigned int out_sz)
{
	u16 opmod = (type << 1) | (HCA_CAP_OPMOD_GET_CUR & 0x01);
	u8 in[MLX5_ST_SZ_BYTES(query_hca_cap_in)] = {};
	int err;

	MLX5_SET(query_hca_cap_in, in, opcode, MLX5_CMD_OP_QUERY_HCA_CAP);
	MLX5_SET(query_hca_cap_in, in, op_mod, opmod);
	err = mlx5u_cmd(dev, in, sizeof(in), out, out_sz);
	if (err) {
		err_msg("query_hca_cap(%d) failed: %d\n", type, err);
		return err;
	}

	return 0;
}

/* this is private prm, mlx5_ifc has a different version of this */
struct mlx5_ifc_debug_capX_bits {
	u8         core_dump_general[0x1];
	u8         core_dump_qp[0x1];
	u8         reserved_at_2[0x7];
	u8         resource_dump[0x1];
	u8         reserved_at_a[0xe];
	u8         log_max_samples[0x8];
	u8         single[0x1];
	u8         repetitive[0x1];
	/*The below field OFED name was health_mon_rx_activity and Upstream
	 *          * stall_detect, for now i take the upstream one*/
	u8         stall_detect[0x1];
	u8         reserved_at_23[0x15];
	u8         log_min_sample_period[0x8];
	u8         reserved_at_40[0x1c0];
	struct mlx5_ifc_diagnostic_cntr_layout_bits diagnostic_counter[0];
};

static int mlx5_diag_cnt_query_param(struct mlx5u_dev *dev, int num_cnt);
static int mlx5_diag_cnt_query_cap(struct mlx5u_dev *dev)
{
	u8 out[MLX5_ST_SZ_BYTES(query_hca_cap_out)] = {};
	int out_sz = sizeof(out);
	void *capptr;
	int err;
	int dev_freq;

	err = query_devcap(dev, MLX5_CAP_GENERAL, out, out_sz);
	if (err)
		return err;
	capptr = MLX5_ADDR_OF(query_hca_cap_out, out, capability);
#define MLX5_CAP_GEN(cap) MLX5_GET(cmd_hca_cap, capptr, cap)
//#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_GEN(cap))
//	printcap(debug);
//	printcap(num_of_diagnostic_counters);
	if (!MLX5_CAP_GEN(debug)) {
		errno = ENOTSUP;
		err_msg("debug capability is not supported\n");
		return ENOTSUP;
	}

	u16 num_counters = MLX5_CAP_GEN(num_of_diagnostic_counters);

	out_sz = MLX5_ST_SZ_BYTES(query_hca_cap_out) +
		 num_counters *
		 MLX5_ST_SZ_BYTES(diagnostic_cntr_layout);
	void *out2 = malloc(out_sz);
	if (!out2)
		return ENOMEM;

	err = query_devcap(dev, MLX5_CAP_DEBUG, out2, out_sz);
	if (err)
		return err;
	capptr = MLX5_ADDR_OF(query_hca_cap_out, out2, capability);
	printf("diag counters:\n\tnum_of_diagnostic_counters: %d\n", num_counters);
	//printf("diag counters CAP:\n");
#define MLX5_CAP_DEBUG(cap) MLX5_GET(debug_capX, capptr, cap)
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_CAP_DEBUG(cap))
	printcap(single);
	printcap(repetitive);
	printcap(log_max_samples);
	printcap(log_min_sample_period);

	dev_freq = get_dev_freq(dev);
	if (dev_freq < 0) {
		err_msg("Can't get device frequency.\n");
		return dev_freq;
	}
	printf("\tdev_freq: %d kHz\n", dev_freq);
#if 1
	for (int i = 0; i < num_counters; i++) {
		void *cntr = MLX5_ADDR_OF(debug_capX, capptr, diagnostic_counter[i]);
		u16 counter_id = MLX5_GET(diagnostic_cntr_layout, cntr, counter_id);
		printf("\tcounter[%d]: 0x%x sync(%d) %s\n", i, counter_id,
		       MLX5_GET(diagnostic_cntr_layout, cntr, sync), diag_counter_desc(counter_id));

	}
#endif

	return 0;
}

struct mlx5_ifc_counter_id_bits {
	u8         reserved_at_0[0x10];
	u8         counter_id[0x10];
};

struct mlx5_ifc_diagnostic_params_context_bits {
	u8         num_of_counters[0x10];
	u8         reserved_at_10[0x8];
	u8         log_num_of_samples[0x8];

	u8         single[0x1];
	u8         repetitive[0x1];
	u8         sync[0x1];
	u8         clear[0x1];
	u8         on_demand[0x1];
	u8         enable[0x1];
	u8         reserved_at_26[0x12];
	u8         log_sample_period[0x8];

	u8         reserved_at_40[0x80];

	struct mlx5_ifc_counter_id_bits counter_id[0];
};

struct mlx5_ifc_query_diagnostic_params_in_bits {
	u8         opcode[0x10];
	u8         reserved_at_10[0x10];

	u8         reserved_at_20[0x10];
	u8         op_mod[0x10];

	u8         reserved_at_40[0x40];
};

struct mlx5_ifc_query_diagnostic_params_out_bits {
	u8         status[0x8];
	u8         reserved_at_8[0x18];

	u8         syndrome[0x20];

	struct mlx5_ifc_diagnostic_params_context_bits diagnostic_params_context;
};

static int mlx5_diag_cnt_query_param(struct mlx5u_dev *dev, int num_cnt)
{
	u8 in[MLX5_ST_SZ_BYTES(query_diagnostic_params_in)] = {};
	void *cnt_id;
	u16 out_sz;
	void *ctx;
	int err;
	u8 *out;
	int i;

	out_sz = MLX5_ST_SZ_BYTES(query_diagnostic_params_out) +
		 num_cnt * MLX5_ST_SZ_BYTES(counter_id);

	out = malloc(out_sz);
	if (!out)
		return ENOMEM;

	MLX5_SET(query_diagnostic_params_in, in, opcode, MLX5_CMD_OP_QUERY_DIAGNOSTIC_PARAMS);

	err = mlx5u_cmd(dev, in, sizeof(in), out, out_sz);
	if (err) {
		err_msg("query diagnostic params failed, %d\n", err);
		free(out);
		return err;
	}

	ctx = MLX5_ADDR_OF(query_diagnostic_params_out, out, diagnostic_params_context);
	printf("diag params:\n");
#undef  printcap
#define printcap(cap) printf("\t" #cap ": %d\n", MLX5_GET(diagnostic_params_context, ctx, cap))
	//printcap(num_of_counters);
	printcap(sync);
	printcap(clear);
	printcap(enable);
	printcap(single);
	printcap(repetitive);
	printcap(on_demand);
	printcap(log_num_of_samples);
	printcap(log_sample_period);

	for (i = 0; i < num_cnt; i++) {
		cnt_id = MLX5_ADDR_OF(diagnostic_params_context, ctx, counter_id[i]);
		printf("\tcounter[%d]: 0x%x\n", i, MLX5_GET(counter_id, cnt_id, counter_id));
	}
	return 0;
}

struct mlx5_ifc_set_diagnostic_params_in_bits {
	u8         opcode[0x10];
	u8         reserved_at_10[0x10];

	u8         reserved_at_20[0x10];
	u8         op_mod[0x10];

	struct mlx5_ifc_diagnostic_params_context_bits diagnostic_params_context;
};

struct mlx5_ifc_set_diagnostic_params_out_bits {
	u8         status[0x8];
	u8         reserved_at_8[0x18];

	u8         syndrome[0x20];

	u8         reserved_at_40[0x40];
};

struct set_diag_params {
	int log_num_of_samples;
	u16 single:1;
	u16 repetitive:1;
	u16 sync:1;
	u16 clear:1;
	u16 on_demand:1;
	u16 enable:1;
	int log_sample_period;
	int num_of_counters;
	u32 *counter_id;
};

static int mlx5_diag_cnt_set_param(struct mlx5u_dev *dev, struct set_diag_params *params)
{
	u8 out[MLX5_ST_SZ_BYTES(set_diagnostic_params_out)] = {};
	void *cnt_id;
	void *ctx;
	void *in;
	int sz_in;
	int err;
	int i;

	sz_in = MLX5_ST_SZ_BYTES(set_diagnostic_params_in) +
		 params->num_of_counters * MLX5_ST_SZ_BYTES(counter_id);
	in = malloc(sz_in);
	if (!in)
		return ENOMEM;
	ctx = MLX5_ADDR_OF(set_diagnostic_params_in, in, diagnostic_params_context);
	MLX5_SET(set_diagnostic_params_in, in, opcode, MLX5_CMD_OP_SET_DIAGNOSTIC_PARAMS);

	MLX5_SET(diagnostic_params_context, ctx, log_num_of_samples, params->log_num_of_samples);
	MLX5_SET(diagnostic_params_context, ctx, single, params->single);
	MLX5_SET(diagnostic_params_context, ctx, repetitive, params->repetitive);
	MLX5_SET(diagnostic_params_context, ctx, sync, params->sync);
	MLX5_SET(diagnostic_params_context, ctx, clear, params->clear);
	MLX5_SET(diagnostic_params_context, ctx, on_demand, params->on_demand);
	MLX5_SET(diagnostic_params_context, ctx, enable, params->enable);
	MLX5_SET(diagnostic_params_context, ctx, log_sample_period, params->log_sample_period);
	MLX5_SET(diagnostic_params_context, ctx, num_of_counters, params->num_of_counters);

	for (i = 0; i < params->num_of_counters; i++) {
		cnt_id = MLX5_ADDR_OF(diagnostic_params_context, ctx, counter_id[i]);
		MLX5_SET(counter_id, cnt_id, counter_id, params->counter_id[i]);
	}

	err = mlx5u_cmd(dev, in, sz_in, out, sizeof(out));
	if (err)
		err_msg("set diagnostic params failed, %d\n", err);
	free(in);
	return err;
}

static int do_param(struct mlx5u_dev *dev, int argc, char *argv[])
{
	int num_cnt = 20;

	if (argc > 1 && !strcmp(argv[1], "help")) {
		printf("Usage: %s [num counters currently enabled in HW or more..]\n", argv[0]);
		return 0;
	}

	if (argc > 1)
		num_cnt = atoi(argv[1]);
	printf("query diag counter params with %d counters\n", num_cnt);
	return mlx5_diag_cnt_query_param(dev, num_cnt);
}

static int parse_flags(char *flags, struct set_diag_params *params) {
	int i = 0;
	int ret = 0;
	while (flags[i] != '\0') {
		switch (flags[i++]) {
		case '-':
			break;
		case 'c':
			params->clear = 1;
			break;
		case 'S':
			params->sync = 1;
			break;
		case 's':
			params->single = 1;
			break;
		case 'r':
			params->repetitive = 1;
			break;
		case 'o':
			params->on_demand = 1;
			ret |= 0x8;
			break;
		default:
			printf("invalid flag %c\n", flags[i]);
			return -1;
		}
	}
	return ret;
}

static int get_dev_freq(struct mlx5u_dev *dev)
{
	u8 out[MLX5_ST_SZ_BYTES(query_hca_cap_out)] = {};
	void *capptr;
	int err;

	err = query_devcap(dev, MLX5_CAP_GENERAL, out, sizeof(out));
	if (err)
		return err;
	capptr = MLX5_ADDR_OF(query_hca_cap_out, out, capability);
	return  MLX5_GET(cmd_hca_cap, capptr, device_frequency_khz);
}

static double sample_period_to_us(struct mlx5u_dev *dev, int log_sample_period, int dev_freq)
{
	return (1 << log_sample_period) * 1000.0 / dev_freq;
}

static int do_set(struct mlx5u_dev *dev, int argc, char *argv[])
{
	struct set_diag_params params = {};
	int dev_freq;
	int err;
	char *counter_string;

	if (argc > 1 && !strcmp(argv[1], "help")) {
		printf("Usage: %s [flags] <log num of samples> <sample period> <counter id1>,<counter id2>...\n", argv[0]);
		printf("\tflags: -Scsro = (S)ync (c)lear (s)ingle (r)epetitive (o)n_demand\n");
		return 0;
	}

	if (argc < 5) {
		err_msg("usage: set [flags] <log num of samples> <sample period> <num of counters> <counter id1>,<counter id2>...\n");
		return EINVAL;
	}

	if (parse_flags(argv[1], &params))
		return EINVAL;

	params.log_num_of_samples = atoi(argv[2]);
	params.log_sample_period = atoi(argv[3]);

	counter_string = argv[4];
	for (; *counter_string; counter_string++)
		params.num_of_counters += (*counter_string == ',');
	params.num_of_counters++;
	params.counter_id = malloc(params.num_of_counters * sizeof(u32));
	if (!params.counter_id)
		return ENOMEM;

	char *tok = strtok(argv[4], ",");
	int i = 0;
	while (tok) {
		params.counter_id[i++] = strtoul(tok, NULL, 0);
		tok = strtok(NULL, ",");
	}

	dev_freq = get_dev_freq(dev);
	if (dev_freq < 0) {
		err_msg("Can't get device frequency.\n");
		return dev_freq;
	}

	params.enable = 1;
	printf("setting params:\n");
	printf("\tsingle: %d (0x%x)\n", params.single, params.single);
	printf("\trepetitive: %d (0x%x)\n", params.repetitive, params.repetitive);
	printf("\tsync: %d \n", params.sync);
	printf("\tclear: %d\n", params.clear);
	printf("\ton_demand: %d\n", params.on_demand);
	printf("\tenable: %d\n", params.enable);
	printf("\tnum_of_counters: %d (0x%x)\n", params.num_of_counters, params.num_of_counters);
	printf("\tlog_num_of_samples: %d (0x%x)\n", params.log_num_of_samples, params.log_num_of_samples);
	printf("\tlog_sample_period: %d (0x%x)\n", params.log_sample_period, params.log_sample_period);
	printf("\tsample_period: %.1f μs\n", sample_period_to_us(dev, params.log_sample_period, dev_freq));
	printf("\tdev_freq: %d kHz\n", dev_freq);
	printf("\tcounter_id:\n");
	for (i = 0; i < params.num_of_counters; i++)
		printf("\t\t[%d] = %d (0x%x)\n", i, params.counter_id[i], params.counter_id[i]);
	err = mlx5_diag_cnt_set_param(dev, &params);
	free(params.counter_id);
	if (err) {
		err_msg("set diagnostic params failed, %d\n", err);
		return err;
	} else {
		printf("set diagnostic params succeeded\n");
		return 0;
	}
}

static int query_diag_counters(struct mlx5u_dev *dev, int num_samples, int sample_index);
static int do_dump(struct mlx5u_dev *dev, int argc, char *argv[])
{
	int num_samples = 1;
	int sample_index = 0;

	if (argc > 1 && !strcmp(argv[1], "help")) {
		printf("Usage: %s [num samples] [sample index]\n", argv[0]);
		return 0;
	}

	if (argc > 1)
		num_samples = atoi(argv[1]);
	if (argc > 2)
		sample_index = atoi(argv[2]);

	printf("query diag counters: %d samples %d sample_index\n", num_samples, sample_index);
	return query_diag_counters(dev, num_samples, sample_index);
}

static int do_disable(struct mlx5u_dev *dev, int argc, char *argv[])
{
	struct set_diag_params params = {};

	params.clear = 1;
	params.enable = 0;
	printf("disabling diag counters and clearing HW buffer\n");
	return mlx5_diag_cnt_set_param(dev, &params);
}

static int do_cap(struct mlx5u_dev *dev, int argc, char *argv[])
{
	return mlx5_diag_cnt_query_cap(dev);
}

static int do_help(struct mlx5u_dev *dev, int argc, char *argv[]);

static const cmd commands[] = {
	{ "cap", do_cap, "show diag counters cap" },
	{ "help", do_help, "show this help" },
	{ "set",   do_set, "set param" },
	{ "disable", do_disable, "disable diag counters" },
	{ "param", do_param, "query param"},
	{ "dump", do_dump, "dump samples"},
	{ 0 }
};

static int do_help(struct mlx5u_dev *dev, int argc, char *argv[])
{
	fprintf(stdout, "Usage: %s <command> [options]\n", help_cmd);
	fprintf(stdout, "Commands:\n");
	for (const cmd *cmd = commands; cmd->name; cmd++)
		fprintf(stdout, "\t%s: %s\n", cmd->name, cmd->desc);
	return 0;;
}

int do_diag_cnt(struct mlx5u_dev *dev, int argc, char *argv[])
{
	int err;

	help_cmd = argv[0];
	err = cmd_select(dev, commands, argc - 1, argv + 1);
	if (err)
		err_msg("diag_cnt %s failed, %d\n", argv[1], err);
	return err;
}

struct mlx5_ifc_diagnostic_cntr_struct_bits {
	u8         counter_id[0x10];
	u8         sample_id[0x10];

	u8         time_stamp_31_0[0x20];

	u8         counter_value_h[0x20];

	u8         counter_value_l[0x20];
};

struct mlx5_ifc_query_diagnostic_cntrs_in_bits {
	u8         opcode[0x10];
	u8         reserved_at_10[0x10];

	u8         reserved_at_20[0x10];
	u8         op_mod[0x10];

	u8         num_of_samples[0x10];
	u8         sample_index[0x10];

	u8         reserved_at_60[0x20];
};

struct mlx5_ifc_query_diagnostic_cntrs_out_bits {
	u8         status[0x8];
	u8         reserved_at_8[0x18];

	u8         syndrome[0x20];

	u8         reserved_at_40[0x40];

	struct mlx5_ifc_diagnostic_cntr_struct_bits diag_counter[0];
};

static int query_diag_counters(struct mlx5u_dev *dev, int num_samples, int sample_index)
{
	u8 in[MLX5_ST_SZ_BYTES(query_diagnostic_cntrs_in)] = {};
	u16 out_sz;
	u8 *out;
	int err;

	out_sz = MLX5_ST_SZ_BYTES(query_diagnostic_cntrs_out) +
		 num_samples * MLX5_ST_SZ_BYTES(diagnostic_cntr_struct);

	out = malloc(out_sz);
	if (!out)
		return -ENOMEM;

	MLX5_SET(query_diagnostic_cntrs_in, in, opcode, MLX5_CMD_OP_QUERY_DIAGNOSTIC_COUNTERS);
	MLX5_SET(query_diagnostic_cntrs_in, in, num_of_samples, num_samples);
	MLX5_SET(query_diagnostic_cntrs_in, in, sample_index, sample_index);

	err = mlx5u_cmd(dev, in, sizeof(in), out, out_sz);
	if (err)
		goto out;

	//dump samples:
	for (int i = 0; i < num_samples; i++) {
		void *diag_cnt = MLX5_ADDR_OF(query_diagnostic_cntrs_out, out, diag_counter[i]);
		u32 counter_id = MLX5_GET(diagnostic_cntr_struct, diag_cnt, counter_id);
		u32 sample_id = MLX5_GET(diagnostic_cntr_struct, diag_cnt, sample_id);
		u32 time_stamp = MLX5_GET(diagnostic_cntr_struct, diag_cnt, time_stamp_31_0);
		unsigned long counter_value = MLX5_GET(diagnostic_cntr_struct, diag_cnt, counter_value_h) << 31 |
					      MLX5_GET(diagnostic_cntr_struct, diag_cnt, counter_value_l);
		fprintf(stdout, "counter_id: 0x%04x, sample_id: %010d, time_stamp: %010u counter_value: %lu\n",
			counter_id, sample_id, time_stamp, counter_value);
	}
out:
	free(out);
	return err;
}
