// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"

void hexdump(void *data, int size) {
        unsigned char *byte = data;
        for (int i = 0; i < size; i++) {
                printf("%02x", byte[i]);
                printf(" ");
                if ((i + 1) % 16 == 0) {
                     printf("\n");
                }
        }
        printf("\n");
}

enum {
	MLX5_PTYS_IB = 1 << 0,
	MLX5_PTYS_EN = 1 << 2,
};

enum mlx5_reg_ids {
	MLX5_REG_QPTS            = 0x4002,
	MLX5_REG_QETCR		 = 0x4005,
	MLX5_REG_QTCT		 = 0x400a,
	MLX5_REG_QPDPM           = 0x4013,
	MLX5_REG_QCAM            = 0x4019,
	MLX5_REG_DCBX_PARAM      = 0x4020,
	MLX5_REG_DCBX_APP        = 0x4021,
	MLX5_REG_FPGA_CAP	 = 0x4022,
	MLX5_REG_FPGA_CTRL	 = 0x4023,
	MLX5_REG_FPGA_ACCESS_REG = 0x4024,
	MLX5_REG_CORE_DUMP	 = 0x402e,
	MLX5_REG_PCAP		 = 0x5001,
	MLX5_REG_PMTU		 = 0x5003,
	MLX5_REG_PTYS		 = 0x5004,
	MLX5_REG_PAOS		 = 0x5006,
	MLX5_REG_PFCC            = 0x5007,
	MLX5_REG_PPCNT		 = 0x5008,
	MLX5_REG_PPTB            = 0x500b,
	MLX5_REG_PBMC            = 0x500c,
	MLX5_REG_PMAOS		 = 0x5012,
	MLX5_REG_PUDE		 = 0x5009,
	MLX5_REG_PMPE		 = 0x5010,
	MLX5_REG_PELC		 = 0x500e,
	MLX5_REG_PVLC		 = 0x500f,
	MLX5_REG_PCMR		 = 0x5041,
	MLX5_REG_PDDR		 = 0x5031,
	MLX5_REG_PMLP		 = 0x5002,
	MLX5_REG_PPLM		 = 0x5023,
	MLX5_REG_PCAM		 = 0x507f,
	MLX5_REG_NODE_DESC	 = 0x6001,
	MLX5_REG_HOST_ENDIANNESS = 0x7004,
	MLX5_REG_MTCAP		 = 0x9009,
	MLX5_REG_MTMP		 = 0x900A,
	MLX5_REG_MCIA		 = 0x9014,
	MLX5_REG_MFRL		 = 0x9028,
	MLX5_REG_MLCR		 = 0x902b,
	MLX5_REG_MRTC		 = 0x902d,
	MLX5_REG_MTRC_CAP	 = 0x9040,
	MLX5_REG_MTRC_CONF	 = 0x9041,
	MLX5_REG_MTRC_STDB	 = 0x9042,
	MLX5_REG_MTRC_CTRL	 = 0x9043,
	MLX5_REG_MPEIN		 = 0x9050,
	MLX5_REG_MPCNT		 = 0x9051,
	MLX5_REG_MTPPS		 = 0x9053,
	MLX5_REG_MTPPSE		 = 0x9054,
	MLX5_REG_MTUTC		 = 0x9055,
	MLX5_REG_MPEGC		 = 0x9056,
	MLX5_REG_MCQS		 = 0x9060,
	MLX5_REG_MCQI		 = 0x9061,
	MLX5_REG_MCC		 = 0x9062,
	MLX5_REG_MCDA		 = 0x9063,
	MLX5_REG_MCAM		 = 0x907f,
	MLX5_REG_MIRC		 = 0x9162,
        MLX5_REG_SBPR            = 0xb001,
        MLX5_REG_SBCM            = 0xb002,
	MLX5_REG_SBCAM		 = 0xB01F,
	MLX5_REG_RESOURCE_DUMP   = 0xC000,
	MLX5_REG_DTOR            = 0xC00E,
        MLX5_REG_RCR             = 0xc00f,
        MLX5_REG_LAST_ENUM,
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

static void print_reg_names_ids(void)
{
        int i;
        for (i = MLX5_REG_QPTS; i <= MLX5_REG_LAST_ENUM; i++) {
                const char* str = reg2str(i);
                if (strcmp(str, "UNKNOWN_REG_STR"))
                        printf("%s 0x%x\n", reg2str(i), i);
        }
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

reg_print_map reg_print_map_array[] = {
        {MLX5_REG_PTYS, print_reg_ptys},
        {MLX5_REG_DTOR, print_reg_dtor},
        {MLX5_REG_NODE_DESC, print_reg_node_desc},
        {MLX5_REG_RCR, print_reg_rcr},
};

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

static int
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

        reg_id = strtol(argv[1], NULL, 0);
        if (argc > 2)
                port = strtol(argv[2], NULL, 0);

        if (argc > 3)
                argument = strtol(argv[3], NULL, 0);


        return mlx5_reg_dump(dev, reg_id, port, argument);
}
