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

static void tokenise_path(const char *path, char *tokens[10])
{
    char *token = strtok((char *)path, "/");
    int i = 0;

    while (token != NULL && i < 10) {
        tokens[i++] = token;
        token = strtok(NULL, "/");
    }
}

//find /sys/bus/*/devices/*/mlx5_core.ctl.0 -maxdepth 1
const char *find_parent_device(char *device_name, char parent_device_name[64])
{
	char pattern[64];
	char *tokens[10] = {0};
	glob_t glob_result;
	int ret;

	device_name = strchr(device_name, '-');
	if (device_name == NULL)
		return NULL;

	device_name++;
	sprintf(pattern, "/sys/bus/*/devices/*/%s", device_name);

	ret = glob(pattern, GLOB_TILDE, NULL, &glob_result);

	if(ret != 0) {
		dbg_msg(1, "Error while searching for files: %d\n", ret);
		return NULL;
	}

	if(glob_result.gl_pathc == 0) {
		dbg_msg(1, "No mlx5ctl devices found\n");
		return NULL;
	}

	if(glob_result.gl_pathc < 1) {
		dbg_msg(1, "Found more than one mlx5ctl device\n");
		return NULL;
	}

	tokenise_path(glob_result.gl_pathv[0], tokens);
	sprintf(parent_device_name, "%s:%s", tokens[2], tokens[4]);

	globfree(&glob_result);
	return parent_device_name;

}

int mlx5u_lsdevs(void) {
	glob_t glob_result;
	char* pattern = "/dev/mlx5ctl*";
	char parent_device_name[64];
	int ret = glob(pattern, GLOB_TILDE, NULL, &glob_result);

	if(ret != 0) {
		err_msg("Error while searching for files: %d\n", ret);
		return ret;
	}

	printf("Found %lu mlx5ctl devices: \n", glob_result.gl_pathc);
	for(unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
		const char *pdev = find_parent_device(glob_result.gl_pathv[i], parent_device_name);

		printf("%s %s\n", glob_result.gl_pathv[i], pdev ? pdev : "");
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

int mlx5u_umem_reg(struct mlx5u_dev *dev, void *addr, size_t len)
{
	struct mlx5ctl_umem_reg umem = {};
	int fd = dev->fd;
	int ret;

	umem.addr = (u64)addr;
	umem.len = len;
	dbg_msg(1, "umem.addr %p umem.len %llu UMEM ID=0x%x\n", (void *)umem.addr, umem.len, umem.umem_id);
	ret = ioctl(fd, MLX5CTL_IOCTL_UMEM_REG, &umem);
	if (ret) {
		err_msg("MLX5CTL_IOCTL_UMEM_REG failed: %d errno(%d): %s\n", ret, errno, strerror(errno));
		return ret > 0 ? -ret : ret;
	}
	dbg_msg(1, "umem.addr reg success %p umem.len %llu UMEM ID=0x%x\n", (void *)umem.addr, umem.len, umem.umem_id);
	return umem.umem_id;
}

int mlx5u_umem_unreg(struct mlx5u_dev *dev, __uint32_t umem_id)
{
	struct mlx5ctl_umem_unreg umem = {};
	int fd = dev->fd;
	int ret;

	umem.umem_id = umem_id;
	ret = ioctl(fd, MLX5CTL_IOCTL_UMEM_UNREG, &umem);
	if (ret) {
		err_msg("MLX5CTL_IOCTL_UMEM_UNREG failed: %d errno(%d): %s\n", ret, errno, strerror(errno));
		return ret;
	}
	dbg_msg(1, "umem.umem_id unreg success 0x%x\n", umem_id);
	return 0;
}