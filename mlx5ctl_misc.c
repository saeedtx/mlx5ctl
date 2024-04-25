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
#include <libgen.h> // For dirname() function
#include <limits.h> // For PATH_MAX
#include <ctype.h> // For isdigit()

#include <stdio.h>
#include <glob.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"
#include "mlx5ctl.h"

#define DEV_PATH_MAX 256
#define DEV_NAME_MAX 64

struct mlx5u_dev {
	char devname[DEV_NAME_MAX];
	int fd;
};

struct mlx5ctl_dev {
	int ctl_id;
	char ctldev[DEV_NAME_MAX];
	char mdev[DEV_NAME_MAX];
};

/* find parent device under symlink: /sys/bus/auxiliary/devices/<device_name> */
static char *find_parent_device(char *device_name, char parent_dev[DEV_NAME_MAX])
{
	char resolved_path[PATH_MAX];
	char dev_path[PATH_MAX];
        char *parent, basename;

	snprintf(dev_path, sizeof(dev_path),
		 "/sys/bus/auxiliary/devices/%s", device_name);

        if (realpath(dev_path, resolved_path) == NULL)
                return NULL;

        parent = dirname(resolved_path);
        if (parent == NULL)
                return NULL;

	strncpy(parent_dev, strrchr(parent, '/') + 1, strlen(parent));
	return parent_dev;
}

static struct mlx5ctl_dev *_scan_ctl_devs(int *count) {
	glob_t glob_result;
	struct mlx5ctl_dev *devs;
        const char *pattern = "/dev/mlx5ctl*";
        char parent_device_name[DEV_NAME_MAX];
	int ret = glob(pattern, GLOB_TILDE, NULL, &glob_result);

	if(ret != 0) {
		err_msg("Error while searching for files: %d\n", ret);
		return NULL;
	}

	if (glob_result.gl_pathc == 0)
		return NULL;

	devs = malloc(sizeof(struct mlx5ctl_dev) * glob_result.gl_pathc);
	memset(devs, 0, sizeof(struct mlx5ctl_dev) * glob_result.gl_pathc);
	if(devs == NULL) {
		err_msg("Error while allocating memory for devs: %d\n", ret);
		return NULL;

	}

	*count = glob_result.gl_pathc;

	for(unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
		char *pdev, *ctldev;

		/* /dev/mlx5ctl-mlx5_core.ctl.X */
		ctldev = strrchr(glob_result.gl_pathv[i], '-') + 1;
		/* mlx5_core.ctl.X */
		pdev = find_parent_device(ctldev, parent_device_name);

		devs[i].ctl_id = atoi(strrchr(ctldev, '.') + 1);
		strncpy(devs[i].ctldev, ctldev, DEV_NAME_MAX);
		if (pdev)
			strncpy(devs[i].mdev, pdev, DEV_NAME_MAX);
	}

	globfree(&glob_result);
	return devs;
}

static int is_a_number(const char *str)
{
	for (int i = 0; i < strlen(str); i++)
		if (!isdigit(str[i]))
			return 0;

	return 1;
}

/* checks if /dev/mlx5ctl-<str> file exists
*  if exists, return it
*  else if str is a number:
*         checks if /dev/mlx5ctl-mlx5_core.ctl.<str> file exists
*         return it
*  else:
*  assume str is a mlx5_core device name, look for corresponding ctldev
*  devs = _scan_ctl_devs()
*  for dev in devs:
*	 find dev with dev->mdev == str
*/
static char *find_dev(const char *str, char dev_path[DEV_PATH_MAX])
{
	struct mlx5ctl_dev *devs;
	int count, i;

	snprintf(dev_path, DEV_PATH_MAX, "/dev/mlx5ctl-%s", str);
	if (access(dev_path, F_OK) == 0)
		return dev_path;

	if (is_a_number(str)) {
		int ctl_id = atoi(str);

		snprintf(dev_path, DEV_PATH_MAX, "/dev/mlx5ctl-mlx5_core.ctl.%d", ctl_id);
		if (access(dev_path, F_OK) == 0)
			return dev_path;
	}

	devs = _scan_ctl_devs(&count);
	if (devs == NULL)
		return NULL;

	for (i = 0; i < count; i++) {
		if (strcmp(devs[i].mdev, str))
			continue;

		snprintf(dev_path, DEV_PATH_MAX, "/dev/mlx5ctl-%s", devs[i].ctldev);
		free(devs);
		return dev_path;
	}

	free(devs);
	return NULL;
}

/******************************************************************/
/* mlx5u API implementation */

int mlx5u_lsdevs(void) {
	int count;
	struct mlx5ctl_dev *devs = _scan_ctl_devs(&count);

	if (devs == NULL)
		return 0;

	printf("Found %d mlx5ctl devices:\n", count);
	for (int i = 0; i < count; i++)
		printf("%s %s\n", devs[i].ctldev, devs[i].mdev);

	free(devs);
	return 0;
}

struct mlx5u_dev *mlx5u_open(const char *name)
{
	char dev_path[DEV_PATH_MAX];
        struct mlx5u_dev *dev;
        int fd;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;

	dbg_msg(1, "looking for dev %s\n", name);
	if (find_dev(name, dev_path) == NULL) {
		err_msg("device %s not found\n", name);
		free(dev);
		return NULL;
	}

	strncpy(dev->devname, dev_path, sizeof(dev->devname));
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
	char parent_dev[DEV_PATH_MAX];
	int fd = dev->fd;
	int ret;

	ret = ioctl(fd, MLX5CTL_IOCTL_INFO, &info);
	if (ret) {
		err_msg("ioctl failed: %d errno(%d): %s\n", ret, errno, strerror(errno));
		return ret;
	}

	printf("ctldev: %s\n", dev->devname);
	printf("Parent dev: %s\n", find_parent_device(strrchr(dev->devname, '-') + 1, parent_dev));
	printf("UCTX UID: %d\n", info.uctx_uid);
	printf("UCTX CAP: 0x%x\n", info.uctx_cap);
	printf("DEV UCTX CAP: 0x%x\n", info.dev_uctx_cap);
	printf("USER CAP: 0x%x\n", info.ucap);

	printf("Current PID: %d FD %d\n", getpid(), fd);
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
		err_msg("command failed opcode 0x%x opmod 0x%x\n",
		        MLX5_GET(mbox_in, in, opcode), MLX5_GET(mbox_in, in, op_mod));
		err_msg("status: 0x%x\n", MLX5_GET(mbox_out, out, status));
		err_msg("syndrome: 0x%x\n", MLX5_GET(mbox_out, out, syndrome));
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