// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <glob.h>
#include "mlx5ctlu.h"
#include "mlx5ctl.h"

// Define the global verbosity level
int verbosity_level = 0;
struct cmd {
	const char *name;
	int (*func)(struct mlx5u_dev *dev, int argc, char *argv[]);
	const char* desc;
};

int do_help(struct mlx5u_dev *dev, int argc, char *argv[]);
int do_devinfo(struct mlx5u_dev *dev, int argc, char *argv[]);

static const struct cmd commands[] = {
	{ "info", do_devinfo, "Print device information" }, // Default
	{ "help", do_help,    "Show this help" },
	{ 0 }
};

int cmd_select(struct mlx5u_dev *dev, int argc, char **argv)
{
	const struct cmd *cmds = commands;

	if (argc < 3)
		return cmds[0].func(dev, argc, argv); // Default

	for (; cmds->name; cmds++) {
		if (!strcmp(cmds->name, argv[2]))
			return cmds->func(dev, argc - 2, argv + 2);
	}

	err_msg("Unknown command \"%s\"\n", argv[2]);
	do_help(dev, argc, argv);
	return -1;
}

int do_help(struct mlx5u_dev *dev, int argc, char *argv[])
{
	fprintf(stdout, "Usage: %s <mlx5 pci device> <command> [options]\n", argv[0]);
	fprintf(stdout, "Verbosity: %s -v <mlx5 pci device> <command> [options]\n", argv[0]);
	fprintf(stdout, "Commands:\n");
	for (const struct cmd *cmd = commands; cmd->name; cmd++)
		fprintf(stdout, "\t%s: %s\n", cmd->name, cmd->desc);
	mlx5u_lsdevs();
	return 0;;
}

int do_devinfo(struct mlx5u_dev *dev, int argc, char *argv[])
{
	return mlx5u_devinfo(dev);
}

int main(int argc, char *argv[])
{
	struct mlx5u_dev *dev;

	if (argc < 2)
		return do_help(NULL, argc, argv);

	if (!strcmp(argv[1], "-v")) {
		verbosity_level = 1;
		argc -= 1;
		argv += 1;
	}

	dev = mlx5u_open(argv[1]);
	if (!dev) {
		err_msg("Failed to open device %s\n", argv[1]);
		return 1;
	}
	cmd_select(dev, argc, argv);
	mlx5u_close(dev);
	return 0;
}
