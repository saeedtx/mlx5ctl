/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef __MLX5_IFCUTIL_H__
#define __MLX5_IFCUTIL_H__

/* helpers and typedefs for mlx5_ifc.h users in user spaces */

#include <sys/types.h>
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef u8    __u8;
typedef u16   __u16;
typedef u32   __u32;
typedef u64   __u64;

typedef __u16       __be16;
typedef __u32       __be32;
typedef __u64       __be64;

typedef __u64 __aligned_u64 __attribute__((aligned(8)));
#define BIT(nr)     (1UL << (nr))
#define BIT_ULL(nr)     (1ULL << (nr))
#define __packed
#define gcm

/* helper macros */
#define __mlx5_nullp(typ) ((struct mlx5_ifc_##typ##_bits *)0)
#define __mlx5_bit_sz(typ, fld) sizeof(__mlx5_nullp(typ)->fld)
#define __mlx5_bit_off(typ, fld) (offsetof(struct mlx5_ifc_##typ##_bits, fld))
#define __mlx5_16_off(typ, fld) (__mlx5_bit_off(typ, fld) / 16)
#define __mlx5_dw_off(typ, fld) (__mlx5_bit_off(typ, fld) / 32)
#define __mlx5_64_off(typ, fld) (__mlx5_bit_off(typ, fld) / 64)
#define __mlx5_16_bit_off(typ, fld) (16 - __mlx5_bit_sz(typ, fld) - (__mlx5_bit_off(typ, fld) & 0xf))
#define __mlx5_dw_bit_off(typ, fld) (32 - __mlx5_bit_sz(typ, fld) - (__mlx5_bit_off(typ, fld) & 0x1f))
#define __mlx5_mask(typ, fld) ((u32)((1ull << __mlx5_bit_sz(typ, fld)) - 1))
#define __mlx5_dw_mask(typ, fld) (__mlx5_mask(typ, fld) << __mlx5_dw_bit_off(typ, fld))
#define __mlx5_mask16(typ, fld) ((u16)((1ull << __mlx5_bit_sz(typ, fld)) - 1))
#define __mlx5_16_mask(typ, fld) (__mlx5_mask16(typ, fld) << __mlx5_16_bit_off(typ, fld))
#define __mlx5_st_sz_bits(typ) sizeof(struct mlx5_ifc_##typ##_bits)

#define MLX5_FLD_SZ_BYTES(typ, fld) (__mlx5_bit_sz(typ, fld) / 8)
#define MLX5_ST_SZ_BYTES(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 8)
#define MLX5_ST_SZ_DW(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 32)
#define MLX5_ST_SZ_QW(typ) (sizeof(struct mlx5_ifc_##typ##_bits) / 64)
#define MLX5_UN_SZ_BYTES(typ) (sizeof(union mlx5_ifc_##typ##_bits) / 8)
#define MLX5_UN_SZ_DW(typ) (sizeof(union mlx5_ifc_##typ##_bits) / 32)
#define MLX5_BYTE_OFF(typ, fld) (__mlx5_bit_off(typ, fld) / 8)
#define MLX5_ADDR_OF(typ, p, fld) ((void *)((uint8_t *)(p) + MLX5_BYTE_OFF(typ, fld)))

#define be32_to_cpu(x) be32toh(x)
#define cpu_to_be32(x) htobe32(x)
#define be16_to_cpu(x) be16toh(x)
#define cpu_to_be16(x) htobe16(x)
#define be64_to_cpu(x) be64toh(x)
#define cpu_to_be64(x) htobe64(x)

#define MLX5_GET(typ, p, fld) ((be32_to_cpu(*((__be32 *)(p) +\
	__mlx5_dw_off(typ, fld))) >> __mlx5_dw_bit_off(typ, fld)) & \
	__mlx5_mask(typ, fld))

#define MLX5_GET64(typ, p, fld) be64_to_cpu(*((__be64 *)(p) + __mlx5_64_off(typ, fld)))

/* insert a value to a struct */
#define MLX5_SET(typ, p, fld, v) do { \
	u32 _v = v; \
	*((__be32 *)(p) + __mlx5_dw_off(typ, fld)) = \
	cpu_to_be32((be32_to_cpu(*((__be32 *)(p) + __mlx5_dw_off(typ, fld))) & \
		     (~__mlx5_dw_mask(typ, fld))) | (((_v) & __mlx5_mask(typ, fld)) \
		     << __mlx5_dw_bit_off(typ, fld))); \
} while (0)

#define __MLX5_SET64(typ, p, fld, v) do { \
	*((__be64 *)(p) + __mlx5_64_off(typ, fld)) = cpu_to_be64(v); \
} while (0)

#define MLX5_SET64(typ, p, fld, v) do { \
	__MLX5_SET64(typ, p, fld, v); \
} while (0)


struct mlx5_ifc_mbox_out_bits {
	u8         status[0x8];
	u8         reserved_at_8[0x18];

	u8         syndrome[0x20];

	u8         reserved_at_40[0x40];
};

struct mlx5_ifc_mbox_in_bits {
	u8         opcode[0x10];
	u8         uid[0x10];

	u8         reserved_at_20[0x10];
	u8         op_mod[0x10];

	u8         reserved_at_40[0x40];
};

#define min(a, b) ((a) < (b) ? (a) : (b))

static inline void hexdump(void *data, int size) {
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

#endif /* __MLX5_IFCUTIL_H__ */
