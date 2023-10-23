# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

CC=gcc
CFLAGS=-Wall
EXE=mlx5ctl
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

#cp linux-source/include/uapi/misc/mlx5ctl.h .

all: mlx5tlu

mlx5tlu: mlx5ctlu.c
	$(CC) $(CFLAGS) mlx5ctlu.c mlx5ctl_misc.c mlx5lib.c \
	devcaps.c reg.c diag_cnt.c rscdump.c \
	-o $(EXE)

clean:
	rm -f $(EXE)

install: mlx5tlu
	install -Dm755 $(EXE) $(DESTDIR)$(BINDIR)/$(EXE)
