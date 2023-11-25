# SPDX-FileCopyrightText: Copyright (c) 2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause

CC=gcc
CFLAGS=-Wall -Wno-gnu-variable-sized-type-not-at-end
EXE=mlx5ctl
PREFIX=/usr/local
BINDIR=$(PREFIX)/bin

#cp linux-source/include/uapi/misc/mlx5ctl.h .

all: misc

misc: mlx5ctlu.c
	$(CC) $(CFLAGS) mlx5ctlu.c mlx5lib.c \
	devcaps.c reg.c diag_cnt.c rscdump.c query_obj.c \
	mlx5ctl_misc.c \
	-o $(EXE)

clean:
	rm -f $(EXE)

install: misc
	install -Dm755 $(EXE) $(DESTDIR)$(BINDIR)/$(EXE)
