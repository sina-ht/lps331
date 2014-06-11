/* -*- c-basic-offset: 8 -*-
 * lps331-get.c -- LPS331 Pressure Sensor Tester
 * Copyright (C) 2014 Hiroshi Takekawa
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * ATTENTION: GPL version 2 only. You cannot apply any later version.
 * This situation may change in the future.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define LPS331_SLAVE_ADDR    0x5c

#define LPS331_REG_WHO_AM_I          0x0f
#define LPS331_REG_CTRL_REG1         0x20
#define LPS331_REG_PRESS_POUT_XL_REH 0x28
#define LPS331_REG_PRESS_OUT_L       0x29
#define LPS331_REG_PRESS_OUT_H       0x2a

#define LPS331_RETRY_COUNT 10
#define LPS331_RETRY_INTERVAL 10

static int open_i2c_dev(int adapter_n) {
	char dev_fn[20];

	snprintf(dev_fn, 19, "/dev/i2c-%d", adapter_n);

	return open(dev_fn, O_RDWR);
}

static int set_slave_addr(int fd, int addr) {
	return ioctl(fd, I2C_SLAVE, addr);
}

static int lps331_read_reg(int fd, int addr) {
	int i, byte;

	for (i = 0; i < LPS331_RETRY_COUNT; i++) {
		byte = i2c_smbus_read_byte_data(fd, addr);
		if (byte >= 0 || i == LPS331_RETRY_COUNT - 1)
			break;
		usleep(LPS331_RETRY_INTERVAL * 1000);
	}

	return byte;
}

static int lps331_write_reg(int fd, int addr, unsigned char byte) {
	int i, res;

	for (i = 0; i < LPS331_RETRY_COUNT; i++) {
		res = i2c_smbus_write_byte_data(fd, addr, byte);
		if (res >= 0 || i == LPS331_RETRY_COUNT - 1)
			break;
		usleep(LPS331_RETRY_INTERVAL * 1000);
	}

	return res;
}

static int who_am_i(int fd) {
	int byte = lps331_read_reg(fd, LPS331_REG_WHO_AM_I);

	if (byte < 0)
		return byte;
	if (byte != 0xbb)
		return 0;

	return 1;
}

static int enable_lps331(int fd) {
	int res, byte;

	byte = lps331_read_reg(fd, LPS331_REG_CTRL_REG1);
	if (byte < 0)
		return byte;
	if (byte == 0x90)
		return 1;

	res = lps331_write_reg(fd, LPS331_REG_CTRL_REG1, 0x90);
	if (res < 0)
		return res;

	return 1;
}

static int read_pressure(int fd, double *pressure_r) {
	int xl, l, h;
	double v;

	xl = lps331_read_reg(fd, LPS331_REG_PRESS_POUT_XL_REH);
	if (xl < 0)
		return xl;
	l = lps331_read_reg(fd, LPS331_REG_PRESS_OUT_L);
	if (l < 0)
		return l;
	h = lps331_read_reg(fd, LPS331_REG_PRESS_OUT_H);
	if (h < 0)
		return h;

	v = (h << 16) + (l << 8) + xl;
	*pressure_r = v / 4096;

	return 1;
}

int main(int argc, char **argv)
{
	int fd, res;
	double pressure;

	if ((fd = open_i2c_dev(1)) < 0) {
		fprintf(stderr, "open_i2c_dev() failed: %s\n", strerror(errno));
		return fd;
	}

	if ((res = set_slave_addr(fd, LPS331_SLAVE_ADDR)) < 0) {
		fprintf(stderr, "set_slave_addr() failed: %s\n", strerror(errno));
		close(fd);
		return res;
	}

	if ((res = who_am_i(fd)) < 0) {
		fprintf(stderr, "who_am_i() failed: %s\n", strerror(errno));
		close(fd);
		return res;
	}
	if (res == 0) {
		fprintf(stderr, "who_am_i() didn't return 0xbb\n");
		return -2;
	}

	if ((res = enable_lps331(fd)) < 0) {
		fprintf(stderr, "enable_lps331() failed: %s\n", strerror(errno));
		close(fd);
		return res;
	}

	if ((res = read_pressure(fd, &pressure)) < 0) {
		printf("Cannot get pressure\n");
	} else {
		printf("pressure = %g\n", pressure);
	}

	close(fd);

	return 0;
}
