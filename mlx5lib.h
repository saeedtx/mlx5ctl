/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef __MLX5CTL_LIB___
#define __MLX5CTL_LIB___

#include "ifcutil.h"
#include "mlx5ctlu.h"

int mlx5lib_alloc_pd(struct mlx5u_dev *dev, u32 *pdn, u16 uid);
int mlx5lib_dealloc_pd(struct mlx5u_dev *dev, u32 pdn, u16 uid);

int mlx5lib_create_umem_mkey(struct mlx5u_dev *dev,
			     u32 pdn, u32 umem_id,
			     uint64_t addr, u32 len, u32 *mkey);
void mlx5lib_destroy_mkey(struct mlx5u_dev *dev, u32 mkey);

static inline u32 mlx5_mkey_to_idx(u32 mkey)
{
	return mkey >> 8;
}

static inline u32 mlx5_idx_to_mkey(u32 mkey_idx)
{
	return mkey_idx << 8;
}

static inline u8 mlx5_mkey_variant(u32 mkey)
{
	return mkey & 0xff;
}

struct mlx5_umem_buff {
	void *buff;
	size_t size;
	uint32_t umem_id;
	uint32_t umem_mkey;
};

struct mlx5_umem_buff *
mlx5lib_alloc_umem_mkey_buff(struct mlx5u_dev *dev, size_t size, uint32_t pdn);
void mlx5lib_free_umem_mkey_buff(struct mlx5u_dev *dev, struct mlx5_umem_buff *umem_buff);

#endif /* __MLX5CTL_LIB___ */