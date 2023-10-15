// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"
#include "reg.h"

enum {
	MLX5_PTYS_IB = 1 << 0,
	MLX5_PTYS_EN = 1 << 2,
};

const char* reg2str(u32 reg_id)
{
#define MLX5_REG_STR_CASE(__reg) case MLX5_REG_ ## __reg: return #__reg
	switch (reg_id) {
		MLX5_REG_STR_CASE(SBPR);
		MLX5_REG_STR_CASE(SBCM);
		MLX5_REG_STR_CASE(QPTS);
		MLX5_REG_STR_CASE(QETCR);
		MLX5_REG_STR_CASE(QTCT);
		MLX5_REG_STR_CASE(QPDPM);
		MLX5_REG_STR_CASE(QCAM);
		MLX5_REG_STR_CASE(DCBX_PARAM);
		MLX5_REG_STR_CASE(DCBX_APP);
		MLX5_REG_STR_CASE(FPGA_CAP);
		MLX5_REG_STR_CASE(FPGA_CTRL);
		MLX5_REG_STR_CASE(FPGA_ACCESS_REG);
		MLX5_REG_STR_CASE(CORE_DUMP);
		MLX5_REG_STR_CASE(PCAP);
		MLX5_REG_STR_CASE(PMTU);
		MLX5_REG_STR_CASE(PTYS);
		MLX5_REG_STR_CASE(PAOS);
		MLX5_REG_STR_CASE(PFCC);
		MLX5_REG_STR_CASE(PPCNT);
		MLX5_REG_STR_CASE(PPTB);
		MLX5_REG_STR_CASE(PBMC);
		MLX5_REG_STR_CASE(PMAOS);
		MLX5_REG_STR_CASE(PUDE);
		MLX5_REG_STR_CASE(PMPE);
		MLX5_REG_STR_CASE(PELC);
		MLX5_REG_STR_CASE(PVLC);
		MLX5_REG_STR_CASE(PCMR);
		MLX5_REG_STR_CASE(PDDR);
		MLX5_REG_STR_CASE(PMLP);
		MLX5_REG_STR_CASE(PPLM);
		MLX5_REG_STR_CASE(PCAM);
		MLX5_REG_STR_CASE(NODE_DESC);
		MLX5_REG_STR_CASE(HOST_ENDIANNESS);
		MLX5_REG_STR_CASE(MTCAP);
		MLX5_REG_STR_CASE(MTMP);
		MLX5_REG_STR_CASE(MCIA);
		MLX5_REG_STR_CASE(MFRL);
		MLX5_REG_STR_CASE(MLCR);
		MLX5_REG_STR_CASE(MRTC);
		MLX5_REG_STR_CASE(MTRC_CAP);
		MLX5_REG_STR_CASE(MTRC_CONF);
		MLX5_REG_STR_CASE(MTRC_STDB);
		MLX5_REG_STR_CASE(MTRC_CTRL);
		MLX5_REG_STR_CASE(MPEIN);
		MLX5_REG_STR_CASE(MPCNT);
		MLX5_REG_STR_CASE(MTPPS);
		MLX5_REG_STR_CASE(MTPPSE);
		MLX5_REG_STR_CASE(MTUTC);
		MLX5_REG_STR_CASE(MPEGC);
		MLX5_REG_STR_CASE(MCQS);
		MLX5_REG_STR_CASE(MCQI);
		MLX5_REG_STR_CASE(MCC);
		MLX5_REG_STR_CASE(MCDA);
		MLX5_REG_STR_CASE(MCAM);
		MLX5_REG_STR_CASE(MIRC);
		MLX5_REG_STR_CASE(SBCAM);
		MLX5_REG_STR_CASE(RESOURCE_DUMP);
		MLX5_REG_STR_CASE(DTOR);
		MLX5_REG_STR_CASE(RCR);
		default: return "UNKNOWN_REG_STR";
	}
}

static int reg_has_pretty_print(u32 reg_id);

static void print_reg_names_ids(void)
{
	int i;
	for (i = MLX5_REG_IDS_FIRST; i <= MLX5_REG_LAST_ENUM; i++) {
		const char* str = reg2str(i);
		if (strcmp(str, "UNKNOWN_REG_STR"))
			printf("%s 0x%x %s\n", reg2str(i), i, reg_has_pretty_print(i) ? "[PP]" : "");
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

typedef void (*reg_pretty_print)(void* data);

typedef struct {
	u32 reg_id;
	reg_pretty_print print_func;
} reg_print_map;

static void print_reg_ptys(void *data);
static void print_reg_dtor(void *out);
static void print_reg_node_desc(void *out);
static void print_reg_rcr(void *out);
static void print_reg_mcam(void *out);

reg_print_map reg_print_map_array[] = {
	{MLX5_REG_PTYS, print_reg_ptys},
	{MLX5_REG_DTOR, print_reg_dtor},
	{MLX5_REG_NODE_DESC, print_reg_node_desc},
	{MLX5_REG_RCR, print_reg_rcr},
	{MLX5_REG_MCAM, print_reg_mcam},
};

static int reg_has_pretty_print(u32 reg_id)
{
	int i;
	for (i = 0; i < sizeof(reg_print_map_array) / sizeof(reg_print_map); i++) {
		if (reg_print_map_array[i].reg_id == reg_id)
			return 1;
	}
	return 0;
}

// Function to get the pretty print function for a register id
reg_pretty_print get_print_func_for_reg(u32 reg_id)
{
	int i;
	for (i = 0; i < sizeof(reg_print_map_array) / sizeof(reg_print_map); i++) {
		if (reg_print_map_array[i].reg_id == reg_id)
			return reg_print_map_array[i].print_func;
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

struct mlx5_ifc_local_port_reg_bits {
	u8         reserved_at_0[0x8];
	u8         local_port[0x8];
};

static int mlx5_reg_dump(struct mlx5u_dev *dev, u32 reg_id, u32 port, u32 argument)
{
	u32 out[MLX5_UN_SZ_DW(ports_control_registers_document)]  = {};
	u32 data[MLX5_UN_SZ_DW(ports_control_registers_document)] = {};
	reg_pretty_print print_fn = NULL;
	int err;

	info_msg("dumping register %s 0x%x local_port %d argumet 0x%x\n",
		 reg2str(reg_id), reg_id, port, argument);
	MLX5_SET(local_port_reg, data, local_port, port);
	err = mlx5_access_reg(dev, data, sizeof(data), out, sizeof(out),
			      reg_id, argument, 0);
	if (err) {
		err_msg("Failed to access register, err %d errno(%d)\n", err, errno);
		return err;
	}

	info_msg("%s 0x%x register fields:\n", reg2str(reg_id), reg_id);
	hexdump(out, sizeof(out));
	print_fn = get_print_func_for_reg(reg_id);
	if (print_fn) {
		info_msg("\n%s 0x%x fields:\n", reg2str(reg_id), reg_id);
		print_fn(out);
	}

	return 0;
}

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

static void help() {
	printf("mlx5ctl <device> reg <reg_id> <port> <argument>\n");
	print_reg_names_ids();
}

int do_reg(struct mlx5u_dev *dev, int argc, char *argv[])
{
	u32 argument = 0;
	u32 port = 1;
	u32 reg_id;

	if (argc < 2) {
		help();
		return -1;
	}

	if (!strcmp(argv[1], "help")) {
		help();
		return 0;
	}

	// check if argv[1] is a register name
	reg_id = get_reg_id_from_str(argv[1]);
	if (reg_id < 0)
		reg_id = strtol(argv[1], NULL, 0);

	if (argc > 2)
		port = strtol(argv[2], NULL, 0);

	if (argc > 3)
		argument = strtol(argv[3], NULL, 0);


	return mlx5_reg_dump(dev, reg_id, port, argument);
}
