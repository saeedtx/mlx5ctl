# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

CC=gcc
CFLAGS=-Wall -Wno-gnu-variable-sized-type-not-at-end

#cp linux-source/include/uapi/misc/mlx5ctl.h .

all: misc

misc: mlx5ctlu.c
	$(CC) $(CFLAGS) mlx5ctlu.c mlx5lib.c \
	devcaps.c reg.c diag_cnt.c rscdump.c \
	mlx5ctl_misc.c \
	-o mlx5ctl

clean:
	rm -f mlx5ctl
