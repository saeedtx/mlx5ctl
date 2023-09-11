// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved. */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> // for open
#include <unistd.h> // for close
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <glob.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5ctl.h"

struct mlx5u_dev {
	char devname[64];
        int fd;
};

struct mlx5u_dev *mlx5u_open(const char *name)
{
        struct mlx5u_dev *dev;
        int fd;

        dev = malloc(sizeof(*dev));
        if (!dev)
                return NULL;

        snprintf(dev->devname, sizeof(dev->devname), "/dev/mlx5ctl-%s", name);
	dbg_msg(1, "opening %s\n", dev->devname);

	fd = open(dev->devname, O_RDWR);
	if (fd < 0) {
		err_msg("open %s failed %d errno(%d): %s\n", dev->devname, fd, errno, strerror(errno));
		err_msg("please insmod mlx5ctl.ko and make sure the device file exists\n");
                free(dev);
		return NULL;
	}
        dev->fd = fd;
	dbg_msg(1, "opened %s descriptor fd(%d)\n", dev->devname, fd);
        return dev;
}

void mlx5u_close(struct mlx5u_dev *dev)
{
        dbg_msg(1, "closing %s descriptor fd(%d)\n", dev->devname, dev->fd);
        close(dev->fd);
        free(dev);
}

int mlx5u_devinfo(struct mlx5u_dev *dev)
{
	struct mlx5ctl_info info = {};
        int fd = dev->fd;
	int ret;

	ret = ioctl(fd, MLX5CTL_IOCTL_INFO, &info);
	if (ret) {
		err_msg("ioctl failed: %d errno(%d): %s\n", ret, errno, strerror(errno));
		return ret;
	}

	printf("mlx5dev: %s\n", info.devname);
	printf("UCTX UID: %d\n", info.uctx_uid);
	printf("UCTX CAP: 0x%x\n", info.uctx_cap);
	printf("DEV UCTX CAP: 0x%x\n", info.dev_uctx_cap);
	printf("USER CAP: 0x%x\n", info.ucap);

	printf("Current PID: %d FD %d\n", getpid(), fd);
	return 0;
}

int mlx5u_lsdevs(void) {
	glob_t glob_result;
	char* pattern = "/dev/mlx5ctl*";
	int ret = glob(pattern, GLOB_TILDE, NULL, &glob_result);

	if(ret != 0) {
		err_msg("Error while searching for files: %d\n", ret);
		return ret;
	}

	printf("Found %lu mlx5ctl devices: \n", glob_result.gl_pathc);
	for(unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
		printf("%s\n", glob_result.gl_pathv[i]);
	}

	globfree(&glob_result);
	return 0;
}

int mlx5u_cmd(struct mlx5u_dev *dev, void *in, size_t inlen, void *out, size_t outlen)
{
	struct mlx5ctl_cmdrpc rpc = {};
	int fd = dev->fd;
	int ret;

	rpc.in = (__aligned_u64)in;
	rpc.inlen = inlen;
	rpc.out = (__aligned_u64)out;
	rpc.outlen = outlen;

	ret = ioctl(fd, MLX5CTL_IOCTL_CMDRPC, &rpc);
	if (ret) {
		err_msg("MLX5CTL_IOCTL_CMDRPC failed: %d errno(%d): %s\n", ret, errno, strerror(errno));
		return ret;
	}

	if (MLX5_GET(mbox_out, out, status)) {
		printf("command failed opcode 0x%x opmod 0x%x\n",
		       MLX5_GET(mbox_in, in, opcode), MLX5_GET(mbox_in, in, op_mod));
		printf("status: 0x%x\n", MLX5_GET(mbox_out, out, status));
		printf("syndrome: 0x%x\n", MLX5_GET(mbox_out, out, syndrome));
		return MLX5_GET(mbox_out, out, status);
	}

	return ret;
}
