/* SPDX-License-Identifier: BSD-3-Clause */
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#ifndef __MLX5CTL_H__
#define __MLX5CTL_H__

#include "ifcutil.h"

#define err_msg(fmt, ...) \
	fprintf(stderr, "Error : " fmt, ##__VA_ARGS__)

#define info_msg(fmt, ...) \
	fprintf(stdout, "INFO : " fmt,  ##__VA_ARGS__)

extern int verbosity_level;

#define dbg_msg(verbosity, fmt, ...) \
	do { \
		if (verbosity <= verbosity_level) { \
			fprintf(stdout, "[DEBUG] (%s:%d %s) "fmt , __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
		} \
	} while (0)

struct mlx5u_dev;
typedef struct cmd {
	const char *name;
	int (*func)(struct mlx5u_dev *dev, int argc, char *argv[]);
	const char* desc;
} cmd;

struct mlx5u_dev *mlx5u_open(const char *devname);
void mlx5u_close(struct mlx5u_dev *dev);
int mlx5u_devinfo(struct mlx5u_dev *dev);
int mlx5u_lsdevs(void);

int mlx5u_cmd(struct mlx5u_dev *dev, void *in, size_t inlen, void *out, size_t outlen);
int mlx5u_umem_reg(struct mlx5u_dev *dev, void *addr, size_t len);
int mlx5u_umem_unreg(struct mlx5u_dev *dev, __uint32_t umem_id);
int cmd_select(struct mlx5u_dev *dev, const cmd *cmds, int argc, char **argv);

#endif /* __MLX5CTL_H__ */
