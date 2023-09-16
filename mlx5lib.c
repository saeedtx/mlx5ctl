// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5_ifc.h"
#include "mlx5lib.h"

int mlx5lib_alloc_pd(struct mlx5u_dev *dev, u32 *pdn, u16 uid)
{
	u32 out[MLX5_ST_SZ_DW(alloc_pd_out)] = {};
	u32 in[MLX5_ST_SZ_DW(alloc_pd_in)] = {};
	int err;

	MLX5_SET(alloc_pd_in, in, opcode, MLX5_CMD_OP_ALLOC_PD);
	MLX5_SET(alloc_pd_in, in, uid, uid);

	err = mlx5u_cmd(dev, in, sizeof(in), out, sizeof(out));
	if (!err)
		*pdn = MLX5_GET(alloc_pd_out, out, pd);

        return err;
}

int mlx5lib_dealloc_pd(struct mlx5u_dev *dev, u32 pdn, u16 uid)
{
	u32 in[MLX5_ST_SZ_DW(dealloc_pd_in)] = {};
        u32 out[MLX5_ST_SZ_DW(dealloc_pd_out)] = {};
	MLX5_SET(dealloc_pd_in, in, opcode, MLX5_CMD_OP_DEALLOC_PD);
	MLX5_SET(dealloc_pd_in, in, pd, pdn);
	MLX5_SET(dealloc_pd_in, in, uid, uid);

	return mlx5u_cmd(dev, in, sizeof(in), out, sizeof(out));
}

int mlx5lib_create_umem_mkey(struct mlx5u_dev *dev,
		             u32 pdn, u32 umem_id,
                             uint64_t addr, u32 len, u32 *mkey)
{
	u32 out[MLX5_ST_SZ_DW(create_mkey_out)] = {};
	u32 in[MLX5_ST_SZ_DW(create_mkey_in)] = {};
        u32 mkey_idx;
	void *mkc;
        int err;

	MLX5_SET(create_mkey_in, in, opcode, MLX5_CMD_OP_CREATE_MKEY);
	mkc = MLX5_ADDR_OF(create_mkey_in, in, memory_key_mkey_entry);
	MLX5_SET(mkc, mkc, access_mode_1_0, 0x1);
	MLX5_SET(mkc, mkc, umr_en, 0);
	MLX5_SET(mkc, mkc, pd, pdn);
	MLX5_SET(mkc, mkc, lr, 1);
	MLX5_SET(mkc, mkc, lw, 1);
	MLX5_SET(mkc, mkc, qpn, 0xffffff);
	MLX5_SET64(mkc, mkc, start_addr, addr);
	MLX5_SET64(mkc, mkc, len, len);

	MLX5_SET(create_mkey_in, in, mkey_umem_valid, 1);
	MLX5_SET(create_mkey_in, in, mkey_umem_id, umem_id);

        err = mlx5u_cmd(dev, in, sizeof(in), out, sizeof(out));
        if (err)
                return err;

        mkey_idx = MLX5_GET(create_mkey_out, out, mkey_index);
	*mkey = MLX5_GET(create_mkey_in, in, memory_key_mkey_entry.mkey_7_0) |
		mlx5_idx_to_mkey(mkey_idx);
	return 0;
}

void mlx5lib_destroy_mkey(struct mlx5u_dev *dev, u32 mkey)
{
	u32 in[MLX5_ST_SZ_DW(destroy_mkey_in)] = {};
        u32 out[MLX5_ST_SZ_DW(destroy_mkey_out)] = {};

	MLX5_SET(destroy_mkey_in, in, opcode, MLX5_CMD_OP_DESTROY_MKEY);
	MLX5_SET(destroy_mkey_in, in, mkey_index, mlx5_mkey_to_idx(mkey));

	mlx5u_cmd(dev, in, sizeof(in), out, sizeof(out));
}

#define UMEM_PAGE_SIZE 4096
#define __ALIGN_MASK(x,mask)    (((x)+(mask))&~(mask))
#define ALIGN(x,a) __ALIGN_MASK(x,(typeof(x))(a)-1)

struct mlx5_umem_buff *
mlx5lib_alloc_umem_mkey_buff(struct mlx5u_dev *dev, size_t size, uint32_t pdn)
{
	struct mlx5_umem_buff *umem_buff;
	int ret;

	umem_buff = malloc(sizeof(*umem_buff));
	if (!umem_buff) {
		err_msg("Failed to allocate umem_buff\n");
		return NULL;
	}
	memset(umem_buff, 0, sizeof(*umem_buff));

	umem_buff->size = ALIGN(size, UMEM_PAGE_SIZE);
	umem_buff->buff = memalign(UMEM_PAGE_SIZE, umem_buff->size);
	if (!umem_buff->buff) {
		err_msg("memalign Failed with size %lu\n", umem_buff->size);
		free(umem_buff);
		return NULL;
	}
	memset(umem_buff->buff, 0, umem_buff->size);

	dbg_msg(1, "Allocated umem buff %p Aligned to bytes %zu\n", umem_buff->buff, umem_buff->size);

	ret = mlx5u_umem_reg(dev, umem_buff->buff, umem_buff->size);
	if (ret < 0) {
		err_msg("Failed to register umem buff %p, size %zu, err %d\n",
			umem_buff->buff, umem_buff->size, ret);
		free(umem_buff->buff);
		free(umem_buff);
		return NULL;
	}
	umem_buff->umem_id = ret;
	dbg_msg(1, "\tAllocated umem_id %d for buff %p\n", umem_buff->umem_id, umem_buff->buff);

	ret = mlx5lib_create_umem_mkey(dev, pdn, umem_buff->umem_id, (uint64_t)umem_buff->buff,
				       umem_buff->size, &umem_buff->umem_mkey);
	if (ret) {
		err_msg("Failed to create umem mkey, err = %d\n", ret);
		mlx5u_umem_unreg(dev, umem_buff->umem_id);
		free(umem_buff->buff);
		free(umem_buff);
		return NULL;
	}

	dbg_msg(1, "\tAllocated umem_mkey 0x%x for umem_id %d\n",
		umem_buff->umem_mkey, umem_buff->umem_id);

	return umem_buff;
}

void mlx5lib_free_umem_mkey_buff(struct mlx5u_dev *dev, struct mlx5_umem_buff *umem_buff)
{
	mlx5lib_destroy_mkey(dev, umem_buff->umem_mkey);
	mlx5u_umem_unreg(dev, umem_buff->umem_id);
	free(umem_buff->buff);
	free(umem_buff);
}
