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
#include "reg.h"

enum {
	MLX5_PTYS_IB = 1 << 0,
	MLX5_PTYS_EN = 1 << 2,
};

typedef void (*reg_pretty_print)(void* data);

struct reg_info {
	u32 reg_id;
	char *str;
	u32 size;
	reg_pretty_print ppfun;
};

#define DEFINER_REG_ATTR(name, size, ppfun) \
	{ MLX5_REG_ ## name, #name, size, ppfun }

#define DEFINE_REG_SZ(name, lower_name) \
	DEFINER_REG_ATTR(name, MLX5_ST_SZ_BYTES(lower_name ## _reg), NULL)

#define DEFINE_REG_SZ_PP(name, lower_name) \
	DEFINER_REG_ATTR(name, MLX5_ST_SZ_BYTES(lower_name ## _reg), print_reg_ ## lower_name)

#define DEFINE_REG_PP(name, lower_name) \
	DEFINER_REG_ATTR(name, MLX5_UN_SZ_BYTES(ports_control_registers_document), print_reg_ ## lower_name)

#define DEFINE_REG_DEF(name) \
	DEFINER_REG_ATTR(name, MLX5_UN_SZ_BYTES(ports_control_registers_document), NULL)

/* pretty print functions implemented at the bottom of the file */
static void print_reg_ptys(void *data);
static void print_reg_dtor(void *out);
static void print_reg_node_desc(void *out);
static void print_reg_rcr(void *out);
static void print_reg_mcam(void *out);

static struct reg_info regs[] = {
	DEFINE_REG_SZ_PP(PTYS, ptys),
	DEFINE_REG_SZ_PP(DTOR, dtor),
	DEFINE_REG_SZ_PP(RCR, rcr),
	DEFINE_REG_SZ_PP(MCAM, mcam),
	DEFINE_REG_PP(NODE_DESC, node_desc),

	DEFINE_REG_SZ(QPTS, qpts),
	DEFINE_REG_SZ(QTCT, qtct),
	DEFINE_REG_SZ(QPDPM, qpdpm),
	DEFINE_REG_SZ(QCAM, qcam),
	DEFINE_REG_SZ(CORE_DUMP, core_dump),
	DEFINE_REG_SZ(PCAP, pcap),
	DEFINE_REG_SZ(PMTU, pmtu),
	DEFINE_REG_SZ(PAOS, paos),
	DEFINE_REG_SZ(PFCC, pfcc),
	DEFINE_REG_SZ(PPCNT, ppcnt),
	DEFINE_REG_SZ(PPTB, pptb),
	DEFINE_REG_SZ(PBMC, pbmc),
	DEFINE_REG_SZ(PMAOS, pmaos),
	DEFINE_REG_SZ(PUDE, pude),
	DEFINE_REG_SZ(PMPE, pmpe),
	DEFINE_REG_SZ(PELC, pelc),
	DEFINE_REG_SZ(PVLC, pvlc),
	DEFINE_REG_SZ(PCMR, pcmr),
	DEFINE_REG_SZ(PDDR, pddr),
	DEFINE_REG_SZ(PMLP, pmlp),
	DEFINE_REG_SZ(PPLM, pplm),
	DEFINE_REG_SZ(PCAM, pcam),
	DEFINE_REG_SZ(MCIA, mcia),
	DEFINE_REG_SZ(MFRL, mfrl),
	DEFINE_REG_SZ(MLCR, mlcr),
	DEFINE_REG_SZ(MRTC, mrtc),
	DEFINE_REG_SZ(MPEIN, mpein),
	DEFINE_REG_SZ(MPCNT, mpcnt),
	DEFINE_REG_SZ(MTUTC, mtutc),
	DEFINE_REG_SZ(MPEGC, mpegc),
	DEFINE_REG_SZ(MCQS, mcqs),
	DEFINE_REG_SZ(MCQI, mcqi),
	DEFINE_REG_SZ(MCC, mcc),
	DEFINE_REG_SZ(MCDA, mcda),
	DEFINE_REG_SZ(MIRC, mirc),
	DEFINE_REG_SZ(SBCAM, sbcam),
	DEFINE_REG_SZ(DCBX_PARAM, dcbx_param),

	/* missing in mlx5_ifc, add there and then use DEFINE_REG_SZ for better hex dump */
	DEFINE_REG_DEF(SBPR),
	DEFINE_REG_DEF(SBCM),
	DEFINE_REG_DEF(QETCR),
	DEFINE_REG_DEF(DCBX_APP),
	DEFINE_REG_DEF(FPGA_CAP),
	DEFINE_REG_DEF(FPGA_CTRL),
	DEFINE_REG_DEF(HOST_ENDIANNESS),
	DEFINE_REG_DEF(MTCAP),
	DEFINE_REG_DEF(MTMP),
	DEFINE_REG_DEF(MTRC_CAP),
	DEFINE_REG_DEF(MTRC_CONF),
	DEFINE_REG_DEF(MTRC_STDB),
	DEFINE_REG_DEF(MTRC_CTRL),
	DEFINE_REG_DEF(MTPPS),
	DEFINE_REG_DEF(MTPPSE),
	DEFINE_REG_DEF(RESOURCE_DUMP),
	DEFINE_REG_DEF(FPGA_ACCESS_REG),
};

static const char* reg2str(u32 reg_id)
{
	for (int i = 0; i < sizeof(regs) / sizeof(struct reg_info); i++) {
		if (regs[i].reg_id == reg_id)
			return regs[i].str;
	}
	return "UNKNOWN_REG_STR";
}

static const unsigned int get_reg_size(u32 reg_id)
{
	for (int i = 0; i < sizeof(regs) / sizeof(struct reg_info); i++) {
		if (regs[i].reg_id == reg_id)
			return regs[i].size;
	}
	return MLX5_UN_SZ_BYTES(ports_control_registers_document);
}

static void to_lower(char *str) {
	while (*str != '\0') {
		*str = tolower((unsigned char)*str);
		str++;
	}
}

static void print_reg_names_ids(void)
{
	char lower_reg_name[128] = {};

	for (int i=0; i < sizeof(regs) / sizeof(struct reg_info); i++) {
		strncpy(lower_reg_name, regs[i].str, sizeof(lower_reg_name));
		to_lower(lower_reg_name);
		printf("\t%s 0x%x%s, dump PRM name: %s_reg\n", regs[i].str, regs[i].reg_id,
		       regs[i].ppfun ? " (pretty print) " : " ", lower_reg_name);
	}
}

static int get_reg_id_from_str(char *reg_name)
{
	int i;
	for (i = MLX5_REG_IDS_FIRST; i <= MLX5_REG_LAST_ENUM; i++) {
		const char* str = reg2str(i);
		if (!strcmp(str, reg_name))
			return i;
	}
	return -1;
}

// Function to get the pretty print function for a register id
static reg_pretty_print get_print_func_for_reg(u32 reg_id)
{
	for (int i = 0; i < sizeof(regs) / sizeof(struct reg_info); i++) {
		if (regs[i].reg_id == reg_id)
			return regs[i].ppfun;
	}
	return NULL;
}

int
mlx5_access_reg(struct mlx5u_dev *dev, void *data_in, int size_in, void *data_out, int size_out,
		u16 reg_id, int arg, int write)
{
	int outlen = MLX5_ST_SZ_BYTES(access_register_out) + size_out;
	int inlen = MLX5_ST_SZ_BYTES(access_register_in) + size_in;
	int err = -ENOMEM;
	u32 *out = NULL;
	u32 *in = NULL;
	void *data;

	in = malloc(inlen);
	out = malloc(outlen);
	if (!in || !out)
		goto out;

	memset(in, 0, inlen);
	memset(out, 0, outlen);

	if (size_in) {
		data = MLX5_ADDR_OF(access_register_in, in, register_data);
		memcpy(data, data_in, size_in);
	}

	MLX5_SET(access_register_in, in, opcode, MLX5_CMD_OP_ACCESS_REG);
	MLX5_SET(access_register_in, in, op_mod, !write);
	MLX5_SET(access_register_in, in, argument, arg);
	MLX5_SET(access_register_in, in, register_id, reg_id);

        dbg_msg(1, "accessing register %s 0x%x argumet 0x%x\n", reg2str(reg_id), reg_id, arg);
	err = mlx5u_cmd(dev, in, inlen, out, outlen);
	if (err)
		goto out;

	data = MLX5_ADDR_OF(access_register_out, out, register_data);
	memcpy(data_out, data, size_out);
out:
	free(out);
	free(in);
	return err;
}

enum pr_format {
	PR_HEX = 'H',
	PR_BIN = 'B',
	PR_PRETTY = 'P',
};

static int reg_id = -1;
static int port;
static int argument;
static int pr_format = PR_HEX;

static void help() {
	fprintf(stdout, "mlx5ctl <device> reg --id=<reg_id> [--port=port] [--argument=argument]\n");
	fprintf(stdout, "\t--id=<reg_id> - register id in hex or decimal\n");
	fprintf(stdout, "\t--port=<port> - port number, default 1\n");
	fprintf(stdout, "\t--argument=<argument> - register argument, default 0\n");
	fprintf(stdout, "\t--bin - print register in binary format\n");
	fprintf(stdout, "\t--hex - print register in hex format\n");
	fprintf(stdout, "\t--pretty - print register in pretty format\n");
	fprintf(stdout, "\t--help - print this help\n");
	fprintf(stdout, "Known Registers:\n");
	print_reg_names_ids();
}

static void parse_args(int argc, char *argv[])
{
	static struct option long_options[] = {
		{"id", required_argument, 0, 'i'},
		{"port", optional_argument, 0, 'p'},
		{"argument", required_argument, 0, 'a'},
		{"bin", no_argument, 0, 'B'},
		{"hex", no_argument, 0, 'H'},
		{"pretty", no_argument, 0, 'P'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, "i:p:a:BHPh", long_options, &option_index)) != -1)
	{
		switch (c) {
		case 'i':
			reg_id = get_reg_id_from_str(optarg);
			if (reg_id < 0)
				reg_id = arg2uint(optarg);
			if (reg_id < 0) {
				fprintf(stderr, "Invalid register id %s\n", optarg);
				exit(1);
			}
			break;
		case 'p':
			port = arg2uint(optarg);
			if (port < 0) {
				fprintf(stderr, "Invalid port %s\n", optarg);
				exit(1);
			}
			break;
		case 'a':
			argument = arg2uint(optarg);
			if (argument < 0) {
				fprintf(stderr, "Invalid argument %s\n", optarg);
				exit(1);
			}
			break;
		case 'B':
		case 'H':
		case 'P':
			pr_format = c;
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
	if (reg_id < 0) {
		fprintf(stderr, "Missing register id\n");
		help();
		exit(1);
	}
}

struct mlx5_ifc_local_port_reg_bits {
	u8         reserved_at_0[0x8];
	u8         local_port[0x8];
};

static int mlx5_reg_dump(struct mlx5u_dev *dev, u32 reg_id, u32 port, u32 argument)
{
	u32 out[MLX5_UN_SZ_DW(ports_control_registers_document)]  = {};
	u32 data[MLX5_UN_SZ_DW(ports_control_registers_document)] = {};
	unsigned int reg_size = get_reg_size(reg_id);
	reg_pretty_print print_fn = NULL;
	int err;

	MLX5_SET(local_port_reg, data, local_port, port);
	err = mlx5_access_reg(dev, data, sizeof(data), out, sizeof(out),
			      reg_id, argument, 0);
	if (err) {
		err_msg("Failed to access register, err %d errno(%d)\n", err, errno);
		return err;
	}

	switch (pr_format) {
	case PR_PRETTY:
		info_msg("%s 0x%x register fields:\n", reg2str(reg_id), reg_id);
		print_fn = get_print_func_for_reg(reg_id);
		if (print_fn) {
			info_msg("\n%s 0x%x fields:\n", reg2str(reg_id), reg_id);
			print_fn(out);
			break;
		}
		err_msg("No pretty print function for register %s 0x%x\n", reg2str(reg_id), reg_id);
	case PR_HEX:
		hexdump(out, reg_size);
		break;
	case PR_BIN:
		fwrite(out, reg_size, 1, stdout);
		break;
	default:
		break;
	}

	return 0;
}

int do_reg(struct mlx5u_dev *dev, int argc, char *argv[])
{

	parse_args(argc, argv);

	return mlx5_reg_dump(dev, reg_id, port, argument);
}

/* ==================================================================== */
/* Selected registers PRETTY PRINT FUNCTIONs */

#define print_reg_field(__out, __reg, __field) \
	printf("\t%s: %x\n", #__field, MLX5_GET(__reg, __out, __field))

static void print_reg_ptys(void *out)
{
#define prprint(__field) print_reg_field(out, ptys_reg, __field)
	prprint(proto_mask);
	prprint(eth_proto_capability);
	prprint(eth_proto_admin);
	prprint(eth_proto_oper);
	prprint(eth_proto_lp_advertise);
	prprint(connector_type);
	prprint(data_rate_oper);
	prprint(an_status);
#undef prprint
}

static void print_reg_dtor(void *out)
{
#define dtor_reg_field(__out, __reg, __field) \
        printf("\t%s: val %d mult %d\n", #__field, \
               MLX5_GET(__reg, __out, __field.to_value),\
               MLX5_GET(__reg, __out, __field.to_multiplier))
#undef prprint
#define prprint(__field) dtor_reg_field(out, dtor_reg, __field)

	prprint(pcie_toggle_to);
	prprint(full_crdump_to);
	prprint(fw_reset_to);
	prprint(flush_on_err_to);
	prprint(pci_sync_update_to);
	prprint(tear_down_to);
	prprint(fsm_reactivate_to);
	prprint(reclaim_pages_to);
	prprint(reclaim_vfs_pages_to);
}

static void print_reg_mcam(void *out)
{
#define print_fld(fld) printf("\t%s %d\n", #fld, MLX5_GET(mcam_reg, out, fld))
	print_fld(feature_group);
	print_fld(access_reg_group);
	print_fld(mng_access_reg_cap_mask.access_regs.mcqs);
	print_fld(mng_access_reg_cap_mask.access_regs.mcqi);
	print_fld(mng_access_reg_cap_mask.access_regs.mcc);
	print_fld(mng_access_reg_cap_mask.access_regs.mcda);
	print_fld(mng_access_reg_cap_mask.access_regs.mpegc);
	print_fld(mng_access_reg_cap_mask.access_regs.mtutc);
	print_fld(mng_access_reg_cap_mask.access_regs.mrtc);
	print_fld(mng_access_reg_cap_mask.access_regs.tracer_registers);
	print_fld(mng_feature_cap_mask.enhanced_features.mcia_32dwords);
	print_fld(mng_feature_cap_mask.enhanced_features.out_pulse_duration_ns);
	print_fld(mng_feature_cap_mask.enhanced_features.npps_period);
	print_fld(mng_feature_cap_mask.enhanced_features.reset_state);
	print_fld(mng_feature_cap_mask.enhanced_features.ptpcyc2realtime_modify);
	print_fld(mng_feature_cap_mask.enhanced_features.pci_status_and_power);
	print_fld(mng_feature_cap_mask.enhanced_features.mark_tx_action_cnp);
	print_fld(mng_feature_cap_mask.enhanced_features.mark_tx_action_cqe);
	print_fld(mng_feature_cap_mask.enhanced_features.dynamic_tx_overflow);
	print_fld(mng_feature_cap_mask.enhanced_features.pcie_outbound_stalled);
	print_fld(mng_feature_cap_mask.enhanced_features.tx_overflow_buffer_pkt);
	print_fld(mng_feature_cap_mask.enhanced_features.mtpps_enh_out_per_adj);
	print_fld(mng_feature_cap_mask.enhanced_features.mtpps_fs);
	print_fld(mng_feature_cap_mask.enhanced_features.pcie_performance_group);
#undef print_fld
}

static void print_reg_node_desc(void *out)
{
	printf("Node description: %s\n", (char *)out);
}

/* sevirity bitmask:
A bit mask to enable health traces. Bit 0 represents severity
0, and so on. Valid only when health_severity_bitmas-
k_valid==1.
Severity codes are listed below:
0: Emergency: system is unusable
1: Alert: action must be taken immediately
2: Critical: critical conditions
3: Error: error conditions
4: Warning: warning conditions
5: Notice: normal but significant condition
6: Informational: informational messages
7: Debug: debug-level messages
*/

static void print_reg_rcr(void *out)
{
	u8 servirity_bit_mask = 0;

	servirity_bit_mask = MLX5_GET(rcr_reg, out, health_severity_bitmask);
	printf("Health severity bitmask: 0x%x valid %d\n", servirity_bit_mask,
	       MLX5_GET(rcr_reg, out, health_severity_bitmask_valid));
	if (servirity_bit_mask & 0x1)
		printf("\t\tEmergency: system is unusable\n");
	if (servirity_bit_mask & 0x2)
		printf("\t\tAlert: action must be taken immediately\n");
	if (servirity_bit_mask & 0x4)
		printf("\t\tCritical: critical conditions\n");
	if (servirity_bit_mask & 0x8)
		printf("\t\tError: error conditions\n");
	if (servirity_bit_mask & 0x10)
		printf("\t\tWarning: warning conditions\n");
	if (servirity_bit_mask & 0x20)
		printf("\t\tNotice: normal but significant condition\n");
	if (servirity_bit_mask & 0x40)
		printf("\t\tInformational: informational messages\n");
	if (servirity_bit_mask & 0x80)
		printf("\t\tDebug: debug-level messages\n");
}
