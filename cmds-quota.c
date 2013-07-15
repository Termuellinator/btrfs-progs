/*
 * Copyright (C) 2012 STRATO.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License v2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 021110-1307, USA.
 */

#include <sys/ioctl.h>
#include <unistd.h>

#include "ctree.h"
#include "ioctl.h"

#include "commands.h"
#include "utils.h"

static const char * const quota_cmd_group_usage[] = {
	"btrfs quota <command> [options] <path>",
	NULL
};

int quota_ctl(int cmd, int argc, char **argv)
{
	int ret = 0;
	int fd;
	int e;
	char *path = argv[1];
	struct btrfs_ioctl_quota_ctl_args args;
	DIR *dirstream = NULL;

	if (check_argc_exact(argc, 2))
		return -1;

	memset(&args, 0, sizeof(args));
	args.cmd = cmd;

	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		fprintf(stderr, "ERROR: can't access '%s'\n", path);
		return 12;
	}

	ret = ioctl(fd, BTRFS_IOC_QUOTA_CTL, &args);
	e = errno;
	close_file_or_dir(fd, dirstream);
	if (ret < 0) {
		fprintf(stderr, "ERROR: quota command failed: %s\n",
			strerror(e));
		return 30;
	}
	return 0;
}

static const char * const cmd_quota_enable_usage[] = {
	"btrfs quota enable <path>",
	"Enable subvolume quota support for a filesystem.",
	"Any data already present on the filesystem will not count towards",
	"the space usage numbers. It is recommended to enable quota for a",
	"filesystem before writing any data to it.",
	NULL
};

static int cmd_quota_enable(int argc, char **argv)
{
	int ret = quota_ctl(BTRFS_QUOTA_CTL_ENABLE, argc, argv);
	if (ret < 0)
		usage(cmd_quota_enable_usage);
	return ret;
}

static const char * const cmd_quota_disable_usage[] = {
	"btrfs quota disable <path>",
	"Disable subvolume quota support for a filesystem.",
	NULL
};

static int cmd_quota_disable(int argc, char **argv)
{
	int ret = quota_ctl(BTRFS_QUOTA_CTL_DISABLE, argc, argv);
	if (ret < 0)
		usage(cmd_quota_disable_usage);
	return ret;
}

static const char * const cmd_quota_rescan_usage[] = {
	"btrfs quota rescan [-s] <path>",
	"Trash all qgroup numbers and scan the metadata again with the current config.",
	"",
	"-s   show status of a running rescan operation",
	NULL
};

static int cmd_quota_rescan(int argc, char **argv)
{
	int ret = 0;
	int fd;
	int e;
	char *path = NULL;
	struct btrfs_ioctl_quota_rescan_args args;
	int ioctlnum = BTRFS_IOC_QUOTA_RESCAN;
	DIR *dirstream = NULL;

	optind = 1;
	while (1) {
		int c = getopt(argc, argv, "s");
		if (c < 0)
			break;
		switch (c) {
		case 's':
			ioctlnum = BTRFS_IOC_QUOTA_RESCAN_STATUS;
			break;
		default:
			usage(cmd_quota_rescan_usage);
		}
	}

	if (check_argc_exact(argc - optind, 1))
		usage(cmd_quota_rescan_usage);

	memset(&args, 0, sizeof(args));

	path = argv[optind];
	fd = open_file_or_dir(path, &dirstream);
	if (fd < 0) {
		fprintf(stderr, "ERROR: can't access '%s'\n", path);
		return 12;
	}

	ret = ioctl(fd, ioctlnum, &args);
	e = errno;
	close_file_or_dir(fd, dirstream);

	if (ioctlnum == BTRFS_IOC_QUOTA_RESCAN) {
		if (ret < 0) {
			fprintf(stderr, "ERROR: quota rescan failed: "
				"%s\n", strerror(e));
			return 30;
		}  else {
			printf("quota rescan started\n");
		}
	} else {
		if (!args.flags) {
			printf("no rescan operation in progress\n");
		} else {
			printf("rescan operation running (current key %lld)\n",
				args.progress);
		}
	}

	return 0;
}

const struct cmd_group quota_cmd_group = {
	quota_cmd_group_usage, NULL, {
		{ "enable", cmd_quota_enable, cmd_quota_enable_usage, NULL, 0 },
		{ "disable", cmd_quota_disable, cmd_quota_disable_usage, 0, 0 },
		{ "rescan", cmd_quota_rescan, cmd_quota_rescan_usage, NULL, 0 },
		{ 0, 0, 0, 0, 0 }
	}
};

int cmd_quota(int argc, char **argv)
{
	return handle_command_group(&quota_cmd_group, argc, argv);
}
