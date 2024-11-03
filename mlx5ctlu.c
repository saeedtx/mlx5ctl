// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <stdio.h>
#include <glob.h>
#include "mlx5ctlu.h"

// Define the global verbosity level
int verbosity_level = 0;

static char *help_cmd;

static int do_help(struct mlx5u_dev *dev, int argc, char *argv[]);
static int do_devinfo(struct mlx5u_dev *dev, int argc, char *argv[]);
static int do_sleep(struct mlx5u_dev *dev, int argc, char *argv[]);

static const cmd commands[] = {
	{ "info", do_devinfo,  "Print device information" }, // Default
	{ "cap", do_devcap, "Query device caps" },
	{ "reg", do_reg, "Dump access registers" },
	{ "obj", query_obj, "Query objects" },
	{ "diagcnt", do_diag_cnt, "Dump diagnostic counters" },
	{ "rscdump", do_rscdump, "Dump resources" },
	{ "coredump", do_rscdump, "CR core dump" },
	{ "help", do_help, "Show this help" },
	{ "sleep", do_sleep, "sleep" },
	{ 0 }
};

int cmd_select(struct mlx5u_dev *dev, const cmd *cmds, int argc, char **argv)
{
	if (argc == 0)
		return cmds[0].func(dev, argc, argv); // Default

	for (; cmds->name; cmds++) {
		if (!strcmp(cmds->name, argv[0]))
			return cmds->func(dev, argc, argv);
	}

	err_msg("Unknown command \"%s\"\n", argv[0]);
	do_help(dev, argc, argv);
	return -1;
}

static int do_help(struct mlx5u_dev *dev, int argc, char *argv[])
{
	fprintf(stdout, "Usage: %s <mlx5 pci device> <command> [options]\n", help_cmd);
	fprintf(stdout, "Verbosity: %s -v <mlx5 pci device> <command> [options]\n", help_cmd);
	fprintf(stdout, "Commands:\n");
	for (const cmd *cmd = commands; cmd->name; cmd++)
		fprintf(stdout, "\t%s: %s\n", cmd->name, cmd->desc);
	mlx5u_lsdevs();
	return 0;;
}

static int do_devinfo(struct mlx5u_dev *dev, int argc, char *argv[])
{
	return mlx5u_devinfo(dev);
}

static int do_sleep(struct mlx5u_dev *dev, int argc, char *argv[])
{
	int sleep_time;

	if (argc < 2) {
		err_msg("sleep: missing operand\n");
		return -1;
	}

	sleep_time = atoi(argv[1]);
	cmd_select(dev, commands, argc - 2, argv + 2);
	sleep(sleep_time);
	return 0;
}

int main(int argc, char *argv[])
{
	struct mlx5u_dev *dev;
	int ret;

	help_cmd = argv[0];
	if (argc < 2 || !strcmp(argv[1], "-h") ||
	    !strcmp(argv[1], "--help") || !strcmp(argv[1], "help"))
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
	ret = cmd_select(dev, commands, argc - 2, argv + 2);
	mlx5u_close(dev);
	return (ret > 0 ? ret : -ret);
}
