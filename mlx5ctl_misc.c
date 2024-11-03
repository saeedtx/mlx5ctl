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
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <libgen.h> // For dirname() function
#include <limits.h> // For PATH_MAX
#include <ctype.h> // For isdigit()
#include <sys/sysmacros.h>

#include <stdio.h>
#include <glob.h>

#include "mlx5ctlu.h"
#include "ifcutil.h"

#include "uapi/fwctl/fwctl.h"
#include "uapi/fwctl/mlx5.h"

#define DEV_PATH_MAX 256
#define DEV_NAME_MAX 64

struct mlx5u_dev {
	char devname[DEV_NAME_MAX];
	int fd;
};

struct mlx5ctl_dev {
	char ctldev[DEV_NAME_MAX];
	char mdev[DEV_NAME_MAX];
};

static char *realpathat(int dirfd, const char *path, char *resolved_path)
{
	int curfd;
	char *ret = NULL;

	curfd = open(".", O_DIRECTORY);
	if (curfd == -1)
		return NULL;
	if (fchdir(dirfd))
		goto out;
	ret = realpath(path, resolved_path);
out:
	fchdir(curfd);
	close(curfd);
	return ret;
}

static int check_fwctl(const char *sysfs_path, struct mlx5ctl_dev *dev)
{
	char tmp[PATH_MAX];
	int sysfs_fd;
	ssize_t ret;

	sysfs_fd = open(sysfs_path, O_DIRECTORY);
	if (sysfs_fd == -1)
		return -1;

	/*
	 * Bit hacky but check if the fwctl is for mlx5 directly from sysfs
	 * FIXME: consider if we should put the driver_type in a sysfs file
	 * instead */
	ret = readlinkat(sysfs_fd, "device/driver", tmp, sizeof(tmp));
	if (ret == -1 || ret == sizeof(tmp)) {
		ret = -1;
		goto out_close;
	}
	if (!strstr(tmp, "drivers/mlx5_core")) {
		ret = -1;
		goto out_close;
	}

	if (!realpathat(sysfs_fd, "device", tmp)) {
		ret = -1;
		goto out_close;
	}
	if (strstr(tmp, "/sys/devices/") != tmp) {
		ret = -1;
		goto out_close;
	}

	/*
	 * FIXME we should load the dev file to know the char dev major/minor,
	 * this is just a guess
	 */
	snprintf(dev->ctldev, sizeof(dev->ctldev), "/dev/fwctl/%s",
		 sysfs_path + strlen("/sys/class/fwctl/"));
	snprintf(dev->mdev, sizeof(dev->mdev), "%s",
		 tmp + strlen("/sys/devices/"));

	ret = 0;
out_close:
	close(sysfs_fd);
	return ret;
}

static struct mlx5ctl_dev *_scan_ctl_devs(int *count)
{
	struct mlx5ctl_dev *devs;
	glob_t glob_result;
	int ret;

	ret = glob("/sys/class/fwctl/*", GLOB_TILDE, NULL, &glob_result);
	if (ret != 0) {
		err_msg("Error while searching for files: %d\n", ret);
		return NULL;
	}

	*count = 0;
	devs = calloc(glob_result.gl_pathc, sizeof(struct mlx5ctl_dev));
	for (unsigned int i = 0; i < glob_result.gl_pathc; ++i) {
		if (check_fwctl(glob_result.gl_pathv[i], &devs[*count]))
			continue;
		(*count)++;
	}

	globfree(&glob_result);

	if (*count == 0) {
		free(devs);
		return NULL;
	}
	return devs;
}

static int is_a_number(const char *str)
{
	for (int i = 0; i < strlen(str); i++)
		if (!isdigit(str[i]))
			return 0;

	return 1;
}

/*
 * Open a fwctl by name. Name can be
 *  - A number meaning /dev/fwctl/fwctl[XX]
 *  - A fwctl name /dev/fwctl/[fwctlXX]
 *  - A sysfs device name pci0000:00/0000:00:0a.0
 */
static int find_dev(const char *str, struct mlx5u_dev *dev)
{
	struct mlx5ctl_dev *devs;
	int count, i;

	if (is_a_number(str)) {
		snprintf(dev->devname, sizeof(dev->devname),
			 "/dev/fwctl/fwctl%s", str);
		dev->fd = open(dev->devname, O_RDWR);
		if (dev->fd != -1)
			return 0;
	}

	snprintf(dev->devname, sizeof(dev->devname), "/dev/fwctl/%s", str);
	dev->fd = open(dev->devname, O_RDWR);
	if (dev->fd != -1)
		return 0;

	devs = _scan_ctl_devs(&count);
	if (devs == NULL)
		return -1;

	for (i = 0; i < count; i++) {
		/* FIXME match a short form PCI name too */
		if (strcmp(devs[i].mdev, str) != 0)
			continue;

		snprintf(dev->devname, sizeof(dev->devname), "%s",
			 devs[i].ctldev);
		dev->fd = open(dev->devname, O_RDWR);
		if (dev->fd != -1) {
			free(devs);
			return 0;
		}
	}

	free(devs);
	return -1;
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
        struct mlx5u_dev *dev;

	dev = malloc(sizeof(*dev));
	if (!dev)
		return NULL;

	dbg_msg(1, "looking for dev %s\n", name);
	if (find_dev(name, dev)) {
		err_msg("device %s not found\n", name);
		err_msg("please insmod fwctl_mlx5.ko and make sure the device file exists\n");
		free(dev);
		return NULL;
	}
	dbg_msg(1, "opened %s descriptor fd(%d)\n", dev->devname, dev->fd);
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
	struct fwctl_info_mlx5 info_mlx5 = {};
	struct fwctl_info info = {
		.size = sizeof(info),
		.device_data_len = sizeof(info_mlx5),
		.out_device_data = (uintptr_t)&info_mlx5,
	};
	struct mlx5ctl_dev dev_desc = {};
	char sysfs_dev[PATH_MAX];
	int fd = dev->fd;
	struct stat st;
	int ret;

	ret = ioctl(fd, FWCTL_INFO, &info);
	if (ret) {
		err_msg("ioctl failed: %d errno(%d): %s\n", ret, errno,
			strerror(errno));
		return ret;
	}

	if (info.out_device_type != FWCTL_DEVICE_TYPE_MLX5) {
		err_msg("Not a MLX5 device");
		return -1;
	}

	if (fstat(fd, &st)) {
		return -1;
	}

	snprintf(sysfs_dev, sizeof(sysfs_dev), "/sys/dev/char/%u:%u",
		 major(st.st_rdev), minor(st.st_rdev));
	if (check_fwctl(sysfs_dev, &dev_desc)) {
		err_msg("Problem parsing sysfs");
		return -1;
	}

	printf("ctldev: %s\n", dev->devname);
	printf("Parent dev: %s\n", dev_desc.mdev);
	printf("UCTX UID: %d\n", info_mlx5.uid);
	printf("UCTX CAP: 0x%x\n", info_mlx5.uctx_caps);
/*	printf("DEV UCTX CAP: 0x%x\n", info.dev_uctx_cap);
	printf("USER CAP: 0x%x\n", info.ucap);
*/
	printf("Current PID: %d FD %d\n", getpid(), fd);
	return 0;
}

int mlx5u_cmd(struct mlx5u_dev *dev, void *in, size_t inlen, void *out, size_t outlen)
{
	struct fwctl_rpc rpc = {
		.size = sizeof(rpc),
		.in = (uintptr_t)in,
		.in_len = inlen,
		.out = (uintptr_t)out,
		.out_len = outlen,
	};
	int ret;


	ret = ioctl(dev->fd, FWCTL_RPC, &rpc);
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

#if 0
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
#else
int mlx5u_umem_reg(struct mlx5u_dev *dev, void *addr, size_t len)
{
	return -1;
}
int mlx5u_umem_unreg(struct mlx5u_dev *dev, __uint32_t umem_id)
{
	return -1;
}
#endif
