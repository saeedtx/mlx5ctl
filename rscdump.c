// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"
#include "reg.h"
#include "mlx5lib.h"

static void print_rsc_dump_reg(void *rscdmp)
{
#define print_fld(fld) printf("\t%s %d\n", #fld, MLX5_GET(resource_dump, rscdmp, fld))
#define print_fldx(fld) printf("\t%s %x\n", #fld, MLX5_GET(resource_dump, rscdmp, fld))
#define print_fld64(fld) printf("\t%s 0x%lx\n", #fld, MLX5_GET64(resource_dump, rscdmp, fld))
	print_fldx(segment_type);
	print_fld(seq_num);
	print_fld(more_dump);
	print_fld64(device_opaque);
	print_fldx(index1);
	print_fldx(index2);
	print_fld(num_of_obj1);
	print_fld(num_of_obj2);
	print_fldx(mkey);
	print_fld64(address);
	print_fld(inline_dump);
	print_fld(size);
#undef print_fld
#undef print_fldx
}

static int mlx5_rsc_dump_iter(struct mlx5u_dev *dev, void *in, void *out)
{
	int reg_size = MLX5_ST_SZ_BYTES(resource_dump);
	u32 out_seq_num, in_seq_num;
	int err;

	dbg_msg(1, "Resource dump access reg IN:\n");
	if (verbosity_level)
		print_rsc_dump_reg(in);

	err = mlx5_access_reg(dev, in, reg_size, out, reg_size,
			      MLX5_REG_RESOURCE_DUMP, 0, 0);
	if (err) {
		err_msg("MLX5_REG_RESOURCE_DUMP: Failed to access err %d\n", err);
		return -EIO;
	}

	dbg_msg(1, "Resource dump access reg OUT:\n");
	if (verbosity_level)
		print_rsc_dump_reg(out);

	in_seq_num = MLX5_GET(resource_dump, in, seq_num);
	out_seq_num = MLX5_GET(resource_dump, out, seq_num);
	if (out_seq_num && (in_seq_num + 1 != out_seq_num))
		return -EREMOTEIO;

	/* In case more_dump was set in the device reply, user shall recall the resource_dump command.
	   resource_type, with seq_num, more_dump, index1, index2, num_of_obj1, num_of_obj2,
	   device_opaque and more_dump are set to the same value that was returned by the device upon
	   last return. segment_type shall be kept as was in the original call. */
	if (MLX5_GET(resource_dump, out, more_dump))
	{
#define cpy_out2in(fld) MLX5_SET(resource_dump, in, fld, MLX5_GET(resource_dump, out, fld))
		cpy_out2in(seq_num);
		cpy_out2in(index1);
		cpy_out2in(index2);
		cpy_out2in(num_of_obj1);
		cpy_out2in(num_of_obj2);
		cpy_out2in(more_dump);
		MLX5_SET64(resource_dump, in, device_opaque, MLX5_GET64(resource_dump, out, device_opaque));
		return 1;
	}

	return 0;
}
typedef struct {
	int umem;
	int type;
	int index1;
	int index2;
	int vhca_id;
} Args;

static void *rscdump_collect(struct mlx5u_dev *dev, Args args, int *total)
{
	u8 in[MLX5_ST_SZ_BYTES(resource_dump)] = {};
	u8 out[MLX5_ST_SZ_BYTES(resource_dump)] = {};
	void *data = NULL;
	int size = 4069;
	int err;

	size = *total ?  *total: size;

	data = malloc(size);
	if (!data) {
		err_msg("Failed to allocate memory size %d\n", size);
		return NULL;
	}

	MLX5_SET(resource_dump, in, segment_type, args.type);
	MLX5_SET(resource_dump, in, index1, args.index1);
	MLX5_SET(resource_dump, in, index2, args.index2);
	MLX5_SET(resource_dump, in, vhca_id, args.vhca_id);
	MLX5_SET(resource_dump, in, inline_dump, 1);

	do {
		err = mlx5_rsc_dump_iter(dev, in, out);
		if (err < 0) {
			err_msg("Resource dump: Failed to access err %d\n", err);
			return *total ? data : NULL;
		}

		// copy inline data to data
		void *iter_data = MLX5_ADDR_OF(resource_dump, out, inline_data);
		int iter_size = MLX5_GET(resource_dump, out, size);

		if ( *total + iter_size > size) { /* no room for new data */
			void *new_data = realloc(data, size * 2);
			if (!new_data) {
				err_msg("Failed to re-allocate memory size %d\n", size * 2);
				return data;
			}
			data = new_data;
			size *= 2;
		}

		memcpy(data + *total, iter_data, iter_size);
		*total += iter_size;
	} while (err > 0);

	return data;
}

static void *rscdump_collect_umem(struct mlx5u_dev *dev, Args args,
				  struct mlx5_umem_buff *umem_buff)
{
	u8 in[MLX5_ST_SZ_BYTES(resource_dump)] = {};
	u8 out[MLX5_ST_SZ_BYTES(resource_dump)] = {};
	int remain = umem_buff->size;
	int err;
	int i = 0;

	void *temp_buffer = malloc(1024 * 1024);
	if (!temp_buffer) {
		err_msg("Failed to allocate temp buffer\n");
		return NULL;
	}

	MLX5_SET(resource_dump, in, segment_type, args.type);
	MLX5_SET(resource_dump, in, index1, args.index1);
	MLX5_SET(resource_dump, in, index2, args.index2);
	MLX5_SET(resource_dump, in, vhca_id, args.vhca_id);

	MLX5_SET(resource_dump, in, inline_dump, 0);
	MLX5_SET(resource_dump, in, mkey, umem_buff->umem_mkey);
	MLX5_SET64(resource_dump, in, address, (uint64_t)umem_buff->buff);
	MLX5_SET(resource_dump, in, size, remain);

	int total = 0;

	do {
		err = mlx5_rsc_dump_iter(dev, in, out);
		if (err < 0) {
			err_msg("Resource dump: Failed to access err %d\n", err);
			return total ? temp_buffer : NULL;
		}

		int iter_size = MLX5_GET(resource_dump, out, size);
		if (iter_size < remain)
			remain -= iter_size;

		info_msg("[%d] Resource dump:\n", i++);
		info_msg("\tcollected %d bytes, remain %d bytes, total %zu, has_more %d\n",
			 iter_size, remain, umem_buff->size - remain, err > 0);

		// copy from data into temp buffer
		hexdump(umem_buff->buff, iter_size);
		memcpy(temp_buffer + total, umem_buff->buff, iter_size);
		total = total + iter_size;
		if (remain == 0)
			break;
	} while (err > 0);

	return temp_buffer;
}

enum mlx5_rsc_sgmt_type {
	/* resource segments range 0x0000 to 0xfeff */
	MLX5_RSC_SGMT_TYPE_DEVICE_DUMP = 0x1000,

	/* end resource segment range */
	MLX5_RSC_SGMT_TYPE_NOTICE = 0xfff9,
	MLX5_RSC_SGMT_TYPE_CMD = 0xfffa,
	MLX5_RSC_SGMT_TYPE_TERMINATE = 0xfffb,
	MLX5_RSC_SGMT_TYPE_ERROR = 0xfffc,
	MLX5_RSC_SGMT_TYPE_REFERENCE = 0xfffd,
	MLX5_RSC_SGMT_TYPE_INFO = 0xfffe,
	MLX5_RSC_SGMT_TYPE_MENU = 0xffff,
};

static const char *sgmt_type2str(enum mlx5_rsc_sgmt_type sgmt_type)
{
	switch (sgmt_type) {
	case MLX5_RSC_SGMT_TYPE_DEVICE_DUMP:
		return "Device dump";
	case MLX5_RSC_SGMT_TYPE_NOTICE:
		return "Notice";
	case MLX5_RSC_SGMT_TYPE_CMD:
		return "Command";
	case MLX5_RSC_SGMT_TYPE_TERMINATE:
		return "Terminate";
	case MLX5_RSC_SGMT_TYPE_ERROR:
		return "Error";
	case MLX5_RSC_SGMT_TYPE_REFERENCE:
		return "Reference";
	case MLX5_RSC_SGMT_TYPE_INFO:
		return "Info";
	case MLX5_RSC_SGMT_TYPE_MENU:
		return "Menu";
	default:
		if (sgmt_type >= 0x0000 && sgmt_type <= 0xfeff)
			return "A Resource";
		return "Unknown";
	}
}

static int mlx5_rsc_sgmt_type_by_name(char *name)
{
	return -1; /* Not implemented */
}

void print_menu_record(void *record)
{
	int seg_type = MLX5_GET(resource_dump_menu_record, record, segment_type);

#define print_fld(fld) printf("\t%s: %d\n", #fld, MLX5_GET(resource_dump_menu_record, record, fld))
//#define print_fldx(fld) printf("\t%s: 0x%x\n", #fld, MLX5_GET(resource_dump_menu_record, record, fld))

	printf("\tsegment_type: 0x%x (%s)\n", seg_type, sgmt_type2str(seg_type));
//	print_fldx(segment_type);
	print_fld(support_index1);
	print_fld(must_have_index1);
	print_fld(support_index2);
	print_fld(must_have_index2);
	print_fld(support_num_of_obj1);
	print_fld(must_have_num_of_obj1);
	print_fld(support_num_of_obj2);
	print_fld(must_have_num_of_obj2);
	print_fld(num_of_obj1_supports_all);
	print_fld(num_of_obj1_supports_active);
	print_fld(num_of_obj2_supports_all);
	print_fld(num_of_obj2_supports_active);
	printf("\tsegment_name: %s\n", (char *)MLX5_ADDR_OF(resource_dump_menu_record, record, segment_name[0]));
	printf("\tindex1_name: %s\n", (char *)MLX5_ADDR_OF(resource_dump_menu_record, record, index1_name[0]));
	printf("\tindex2_name: %s\n", (char *)MLX5_ADDR_OF(resource_dump_menu_record, record, index2_name[0]));
}

static void parse_menu_segment(void *menu)
{
	int num_records = 0;
	void *records;
	int size = 0;

	num_records = MLX5_GET(resource_dump_menu_segment, menu, num_of_records);

	info_msg("Resource dump menu size %d num of records %d\n", size, num_records);

	records = MLX5_ADDR_OF(resource_dump_menu_segment, menu, record[0]);
	for (int i = 0; i < num_records; i++) {
		void *record = records + i * MLX5_ST_SZ_BYTES(resource_dump_menu_record);

		printf("Menu Record %d\n", i);
		print_menu_record(record);
	}
}

struct sgmnt_hdr {
	enum  mlx5_rsc_sgmt_type segment_type;
	u32 len; /* bytes */
};

static void get_sgmnt_hdr(void *sgmnt, struct sgmnt_hdr *sgmnt_hdr)
{
	sgmnt_hdr->segment_type = MLX5_GET(resource_dump_segment_header, sgmnt, segment_type);
	sgmnt_hdr->len = MLX5_GET(resource_dump_segment_header, sgmnt, length_dw) * 4;
}

static void parse_err_segment(void *segment)
{
	char str[64] = {};

	memcpy(str, MLX5_ADDR_OF(resource_dump_error_segment, segment, error),
		MLX5_FLD_SZ_BYTES(resource_dump_error_segment, error));
	printf("\terror: syndrome 0x%x, %s\n",
		MLX5_GET(resource_dump_error_segment, segment, syndrome_id),
		str);
}

static void parse_notice_segment(void *segment)
{
	char str[64] = {};

	memcpy(str, MLX5_ADDR_OF(resource_dump_notice_segment, segment, notice),
		MLX5_FLD_SZ_BYTES(resource_dump_notice_segment, notice));
	printf("\tnotice: syndrome 0x%x, %s\n",
		MLX5_GET(resource_dump_notice_segment, segment, syndrome_id),
		str);
}

static void parse_reference_segment(void *segment)
{
	printf("\treference_segment: 0x%x, index1 0x%x, index2 0x%x, num_of_obj1 %d, num_of_obj2 %d\n",
		MLX5_GET(resource_dump_reference_segment, segment, reference_segment),
		MLX5_GET(resource_dump_reference_segment, segment, index1),
		MLX5_GET(resource_dump_reference_segment, segment, index2),
		MLX5_GET(resource_dump_reference_segment, segment, num_of_obj1),
		MLX5_GET(resource_dump_reference_segment, segment, num_of_obj2));
}

static void print_sgmt_header(void *hdr)
{
	struct sgmnt_hdr sgmnt_hdr = {};

	get_sgmnt_hdr(hdr, &sgmnt_hdr);
	info_msg("%s Segment: type 0x%x, length %d\n",
		 sgmt_type2str(sgmnt_hdr.segment_type),
		 sgmnt_hdr.segment_type,
		 sgmnt_hdr.len);
}

static int parse_segment(void *segment) {
	struct sgmnt_hdr sgmnt_hdr = {};

	get_sgmnt_hdr(segment, &sgmnt_hdr);
	print_sgmt_header(segment);
	switch (sgmnt_hdr.segment_type) {
		case MLX5_RSC_SGMT_TYPE_TERMINATE:
			info_msg("end: %s\n", sgmt_type2str(sgmnt_hdr.segment_type));
			return 1;
		case MLX5_RSC_SGMT_TYPE_ERROR:
			parse_err_segment(segment);
			return 1;
		case MLX5_RSC_SGMT_TYPE_NOTICE:
			parse_notice_segment(segment);
			return 0;
		case MLX5_RSC_SGMT_TYPE_REFERENCE:
			parse_reference_segment(segment);
			return 0;
		case MLX5_RSC_SGMT_TYPE_MENU:
			parse_menu_segment(segment);
			return 0;
		default:
			break;
	}

	if (sgmnt_hdr.segment_type > 0xfeff) /* Not a  resource segment */
			return 0;

	info_msg("\tindex1 0x%x, index2 0x%x\n",
		MLX5_GET(resource_dump_resource_segment, segment, index1),
		MLX5_GET(resource_dump_resource_segment, segment, index2));
	hexdump(MLX5_ADDR_OF(resource_dump_resource_segment, segment, payload),
		sgmnt_hdr.len - MLX5_ST_SZ_BYTES(resource_dump_resource_segment));
	return 0;
}

static void parse_resource_dump(void *data, int size)
{
	void *info = MLX5_ADDR_OF(resource_dump_response, data, info);

	info_msg("Resource dump size %d\n", size);
	print_sgmt_header(info);
	printf("\tdump_version %d, hw_version %d, fw_version 0%x\n",
		 MLX5_GET(resource_dump_info_segment, info, dump_version),
		 MLX5_GET(resource_dump_info_segment, info, hw_version),
		 MLX5_GET(resource_dump_info_segment, info, fw_version));

	void *cmd = MLX5_ADDR_OF(resource_dump_response, data, cmd);
	print_sgmt_header(cmd);
	printf("\tsegment_called 0x%x, vhca_id 0x%x, index1 0x%x, index2 0x%x, num_of_obj1 %d, num_of_obj2 %d\n",
		 MLX5_GET(resource_dump_command_segment, cmd, segment_called),
		 MLX5_GET(resource_dump_command_segment, cmd, vhca_id),
		 MLX5_GET(resource_dump_command_segment, cmd, index1),
		 MLX5_GET(resource_dump_command_segment, cmd, index2),
		 MLX5_GET(resource_dump_command_segment, cmd, num_of_obj1),
		 MLX5_GET(resource_dump_command_segment, cmd, num_of_obj2));

	if (size < MLX5_ST_SZ_BYTES(resource_dump_response) + MLX5_ST_SZ_BYTES(resource_dump_terminate_segment))
		return;

	void *segment = MLX5_ADDR_OF(resource_dump_response, data, segment[0]);

	while (!parse_segment(segment)) {
		segment = segment + MLX5_GET(resource_dump_segment_header, segment, length_dw) * 4;
		if (segment - data >= size) {
			err_msg("Segment overflow %ld, data size %d\n", segment - data, size);
			break;
		}
	}
}

static struct mlx5_umem_buff *umem_buff = NULL;

static void mlx5_rsc_dump(struct mlx5u_dev *dev, Args args)
{
	void *data;
	int size = 0;
	if (!umem_buff) {
		data = rscdump_collect(dev, args, &size);
		if (!data)
			return;
	} else {
		data = rscdump_collect_umem(dev, args, umem_buff);
		if (!data)
			return;
		size = umem_buff->size;
	}

	parse_resource_dump(data, size);

	if (!umem_buff)
		free(data);
}

static int str_starts_with(const char *str, const char *prefix)
{
    return strncmp(str, prefix, strlen(prefix)) == 0;
}

static int parse_rsc_dump_type_arg(char *arg)
{
	enum mlx5_rsc_sgmt_type type = 0;
	char byname[64] = {};

	if (sscanf(arg, "--type=0x%x", &type) == 1)
		return type;

	if (sscanf(arg, "--type=%s", byname) > 0)
		return mlx5_rsc_sgmt_type_by_name(byname);

	return 0;
}

static char *usage = "[--umem=<size KB>] [--type=<type>] [--idx1=<index1>] [--idx2=<index2>] [--vhcaid=<vhca_id>] [--help]";
static char *usage_core_dump = "[--umem=<size KB>] [--help]";

Args parse_args(int argc, char *argv[])
{
	Args args = {0};
	char *usage_str;
	args.type = -1;

	if (strcmp(argv[0], "coredump") == 0)
		usage_str = usage_core_dump;
	else
		usage_str = usage;

	for (int i = 1; i < argc; i++) {
		if (str_starts_with(argv[i], "--umem=")) {
			args.umem = atoi(argv[i] + 7);
		} else if (str_starts_with(argv[i], "--type=")) {
			args.type = parse_rsc_dump_type_arg(argv[i]);
		} else if (str_starts_with(argv[i], "--idx1=")) {
			sscanf(argv[i] + 7, "0x%x", &args.index1);
		} else if (str_starts_with(argv[i], "--idx2=")) {
			sscanf(argv[i] + 7, "0x%x", &args.index2);
		} else if (str_starts_with(argv[i], "--vhcaid=")) {
			args.vhca_id = atoi(argv[i] + 9);
		} else if (str_starts_with(argv[i], "--help")) {
			printf("Usage: %s %s\n", argv[0], usage_str);
			exit(0);
		} else {
			printf("Unknown argument %s\n", argv[i]);
			printf("Usage: %s %s\n", argv[0], usage_str);
			exit(1);
		}
	}

	return args;
}

static void mlx5_core_dump(struct mlx5u_dev *dev);

int do_rscdump(struct mlx5u_dev *dev, int argc, char *argv[])
{
	Args args = parse_args(argc, argv);
	uint32_t umem_pdn = 0;

	if (args.type < 0)
		args.type = MLX5_RSC_SGMT_TYPE_MENU;

	printf("umem %d, type 0x%x, index1 0x%x, index2 0x%x, vhca_id=0x%x\n",
	       args.umem, args.type, args.index1, args.index2, args.vhca_id);

	if (args.umem) {
		size_t umem_buff_size = args.umem * 1024;
		int err;

		if (umem_buff_size < 0) {
			err_msg("Invalid data size %zu\n", umem_buff_size);
			return -EINVAL;
		}
		err = mlx5lib_alloc_pd(dev, &umem_pdn, 0);
		if (err)
			return err;

		umem_buff = mlx5lib_alloc_umem_mkey_buff(dev, umem_buff_size, umem_pdn);
		if (!umem_buff) {
			mlx5lib_dealloc_pd(dev, umem_pdn, 0);
			return -ENOMEM;
		}
	}

	if (!strcmp(argv[0], "rscdump"))
		mlx5_rsc_dump(dev, args);
	else if (!strcmp(argv[0], "coredump"))
		mlx5_core_dump(dev);
	else
		err_msg("Unknown command \"%s\"\n", argv[0]);

	if (umem_buff) {
		mlx5lib_free_umem_mkey_buff(dev, umem_buff);
		mlx5lib_dealloc_pd(dev, umem_pdn, 0);
	}
	return 0;
}

/* this is not part of rsc dump, but they share umem implementation */
static void mlx5_core_dump(struct mlx5u_dev *dev)
{
	u8 in[MLX5_ST_SZ_BYTES(core_dump_reg)] = {};
	u8 out[MLX5_ST_SZ_BYTES(core_dump_reg)] = {};
	int err;

	if (!umem_buff) {
		err_msg("please allocate umem buff first\n");
		return;
	}

	MLX5_SET(core_dump_reg, in, core_dump_type,
		 MLX5_CORE_DUMP_REG_CORE_DUMP_TYPE_CR_DUMP_TO_MEM);
	MLX5_SET64(core_dump_reg, in, address, (uint64_t)umem_buff->buff);
	MLX5_SET(core_dump_reg, in, size, umem_buff->size);
	MLX5_SET(core_dump_reg, in, mkey, umem_buff->umem_mkey);

	err = mlx5_access_reg(dev, in, sizeof(in), out, sizeof(out),
			      MLX5_REG_CORE_DUMP, 0, 0);
	if (err) {
		err_msg("Failed to access core dump err %d\n", err);
		return;
	}
	hexdump(umem_buff->buff, MLX5_GET(core_dump_reg, out, size));
	info_msg("Core dump done\n");
	info_msg("Core dump size %d\n", MLX5_GET(core_dump_reg, out, size));
	info_msg("Core dump address 0x%lx\n", MLX5_GET64(core_dump_reg, out, address));
	info_msg("Core dump cookie 0x%lx\n", MLX5_GET64(core_dump_reg, out, cookie));
	info_msg("More Dump %d\n", MLX5_GET(core_dump_reg, out, more_dump));
}
