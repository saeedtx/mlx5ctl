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

static void *rscdump_collect(struct mlx5u_dev *dev, u32 seg_type, int *total)
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

	MLX5_SET(resource_dump, in, segment_type, seg_type);
	MLX5_SET(resource_dump, in, inline_dump, 1);

	do {
		err = mlx5_rsc_dump_iter(dev, in, out);
		if (err < 0) {
			err_msg("Resource dump: Failed to access err %d\n", err);
			return data;
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

static void *rscdump_collect_umem(struct mlx5u_dev *dev, u32 seg_type,struct mlx5_umem_buff *umem_buff)
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

	MLX5_SET(resource_dump, in, segment_type, seg_type);
	MLX5_SET(resource_dump, in, inline_dump, 0);
	MLX5_SET(resource_dump, in, mkey, umem_buff->umem_mkey);
	MLX5_SET64(resource_dump, in, address, (uint64_t)umem_buff->buff);
	MLX5_SET(resource_dump, in, size, remain);

	int total = 0;

	do {
		err = mlx5_rsc_dump_iter(dev, in, out);
		if (err < 0) {
			err_msg("Resource dump: Failed to access err %d\n", err);
			return NULL;
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

#define MLX5_RSC_DUMP_MENU_SEGMENT 0xffff
void print_menu_record(void *record)
{
#define print_fld(fld) printf("\t%s: %d\n", #fld, MLX5_GET(resource_dump_menu_record, record, fld))
#define print_fldx(fld) printf("\t%s: 0x%x\n", #fld, MLX5_GET(resource_dump_menu_record, record, fld))

	print_fldx(segment_type);
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

static struct mlx5_umem_buff *umem_buff = NULL;

static int rscdump_menu_print(struct mlx5u_dev *dev)
{
	void *data , *menu, *records;
	int size = 0;
	int num_records = 0;
	if (!umem_buff) {
		data = rscdump_collect(dev, MLX5_RSC_DUMP_MENU_SEGMENT, &size);
		if (!data)
			return -ENOMEM;
	} else {
		data = rscdump_collect_umem(dev, MLX5_RSC_DUMP_MENU_SEGMENT, umem_buff);
		if (!data)
			return -ENOMEM;
	}

	menu = MLX5_ADDR_OF(menu_resource_dump_response, data, menu);
	num_records = MLX5_GET(resource_dump_menu_segment, menu, num_of_records);

	info_msg("Resource dump menu size %d num of records %d\n", size, num_records);

	records = MLX5_ADDR_OF(resource_dump_menu_segment, menu, record[0]);
	for (int i = 0; i < num_records; i++) {
		void *record = records + i * MLX5_ST_SZ_BYTES(resource_dump_menu_record);

		printf("Menu Record %d\n", i);
		print_menu_record(record);
	}
	if (!umem_buff)
		free(data);
	return 0;
}


int do_rscdump(struct mlx5u_dev *dev, int argc, char *argv[])
{
	uint32_t umem_pdn = 0;
	if (argc > 1) {
		int err;
		size_t umem_buff_size = atoi(argv[1]);

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

	rscdump_menu_print(dev);

	if (umem_buff) {
		mlx5lib_free_umem_mkey_buff(dev, umem_buff);
		mlx5lib_dealloc_pd(dev, umem_pdn, 0);
	}
	return 0;
}
