/*
 * Copyright (C) 2019 DENSO TEN Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <string>
#include <sstream>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>
#include <linux/fb.h>

#include <pthread.h>
#include "capturing.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#define FIELD V4L2_FIELD_NONE

using std::istringstream;

CapturingItem::CapturingItem()
	:	_dev_name(""),
		_fd(-1), _fps_count(0), _framerate(0), _timeout(60),
		_left(0), _top(0), _width(1280), _height(1080)
{
}

CapturingItem::~CapturingItem()
{
}

int CapturingItem::open_device(int dev)
{
	struct stat st;
	std::stringstream ss;
	ss << "/dev/video";
	ss << dev;
	ss >> _dev_name;

	if (-1 == stat(_dev_name.c_str(), &st)) {
		fprintf(stderr, "Cannot identify '%s': %d, %s\n",
					_dev_name.c_str(), errno, strerror(errno));
		return -1;
	}

	if (!S_ISCHR(st.st_mode)) {
		fprintf(stderr, "%s is no device\n", _dev_name.c_str());
		return -1;
	}

	_fd = open(_dev_name.c_str(), O_RDWR /* Required */ | O_NONBLOCK, 0);

	if (-1 == _fd) {
		fprintf(stderr, "Cannot open '%s': %d, %s\n",
					_dev_name.c_str(), errno, strerror(errno));
		return -1;
	}
	return 0;
}

int CapturingItem::init_device(void)
{
	struct v4l2_capability cap;
	struct v4l2_cropcap cropcap;
	struct v4l2_crop crop;
	struct v4l2_format fmt;

	if (-1 == xioctl(_fd, VIDIOC_QUERYCAP, &cap)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s is no V4L2 device\n", _dev_name.c_str());
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
		fprintf(stderr, "%s is no video capture device\n", _dev_name.c_str());
		exit(EXIT_FAILURE);
	}


	/* Select video input, video standard and adjust. */
	CLEAR(cropcap);
	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (0 == xioctl(_fd, VIDIOC_CROPCAP, &cropcap)) {
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		crop.c.left = _left;
		crop.c.top = _top;
		crop.c.width = _width;
		crop.c.height = _height;

		if (-1 == xioctl(_fd, VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Trimming is not supported. */
				break;
			default:
				/* The error is ignored. */
				break;
			}
		}
	} else {
		/* The error is ignored. */
	}

	if (_framerate) {
		struct v4l2_streamparm parm;

		parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(_fd, VIDIOC_G_PARM, &parm))
			errno_exit("VIDIOC_G_PARM");

		parm.parm.capture.timeperframe.numerator = 1;
		parm.parm.capture.timeperframe.denominator = _framerate;
		if (-1 == xioctl(_fd, VIDIOC_S_PARM, &parm))
			errno_exit("VIDIOC_S_PARM");
	}

	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = _width;
	fmt.fmt.pix.height = _height;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_XBGR32;

	if (-1 == xioctl(_fd, VIDIOC_S_FMT, &fmt))
		errno_exit("VIDIOC_S_FMT");

	init_mmap();

	return 0;
}

void CapturingItem::start_capturing(void)
{
	unsigned int i;
	enum v4l2_buf_type type;

	for (i = 0; i < n_buffers; ++i) {
			struct v4l2_buffer buf;

			CLEAR(buf);
			buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			buf.memory = V4L2_MEMORY_MMAP;
			buf.index = i;

			if (-1 == xioctl(_fd, VIDIOC_QBUF, &buf))
					errno_exit("VIDIOC_QBUF");
	}
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(_fd, VIDIOC_STREAMON, &type))
			errno_exit("VIDIOC_STREAMON");
}

unsigned char* CapturingItem::snap_frame(void)
{
	int index = 0;
	for (;;)
	{
		fd_set fds;
		struct timeval tv;
		int r;

		FD_ZERO(&fds);
		FD_SET(_fd, &fds);

		/* Timeout. */
		tv.tv_sec = 2;
		tv.tv_usec = 0;

		r = select(_fd + 1, &fds, NULL, NULL, &tv);

		if (-1 == r)
		{
			if (EINTR == errno)
			continue;
			errno_exit("select");
		}

		if (0 == r)
		{
			fprintf(stderr, "select timeout!\n");
			exit(EXIT_FAILURE);
		}

		index = read_frame();
		if (-1 != index)
			break;
		/* EAGAIN (continue select loop). */
	}

	return (unsigned char*)(p_buffers)[index].start;
}

int CapturingItem::read_frame(void)
{
	struct v4l2_buffer buf;

	CLEAR(buf);

	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(_fd, VIDIOC_DQBUF, &buf))
	{
		switch (errno)
		{
			case EAGAIN:
				return -1;

			case EIO:
				/* Could ignore EIO. */
			default:
				errno_exit("VIDIOC_DQBUF");
		}
	}

	assert(buf.index < n_buffers);



	if (-1 == xioctl(_fd, VIDIOC_QBUF, &buf))
		errno_exit("VIDIOC_QBUF");

	return buf.index;
}

void CapturingItem::errno_exit(const char *s)
{
	fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
	exit(EXIT_FAILURE);
}

int CapturingItem::xioctl(int fh, int request, void *arg)
{
	int r;

	do {
			r = ioctl(fh, request, arg);
	} while (-1 == r && EINTR == errno);

	return r;
}

void CapturingItem::uninit_device(void)
{
	unsigned int i;

	for (i = 0; i < n_buffers; ++i)
		if (-1 == munmap((p_buffers)[i].start, (p_buffers)[i].length))
			errno_exit("munmap");

	free(p_buffers);
}

void CapturingItem::init_read(unsigned int buffer_size)
{
	p_buffers = (struct buffer*)(calloc(1, sizeof(*p_buffers)));

	if (!p_buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	(p_buffers)[0].length = buffer_size;
	(p_buffers)[0].start = malloc(buffer_size);

	if (!(p_buffers)[0].start) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}
}

void CapturingItem::init_mmap(void)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			fprintf(stderr, "%s does not support "
						"memory mapping\n", _dev_name.c_str());
			exit(EXIT_FAILURE);
		} else {
			errno_exit("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		fprintf(stderr, "Insufficient buffer memory on %s\n",
					_dev_name.c_str());
		exit(EXIT_FAILURE);
	}

	p_buffers = (struct buffer*)(calloc(req.count, sizeof(*p_buffers)));

	if (!p_buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;

		if (-1 == xioctl(_fd, VIDIOC_QUERYBUF, &buf))
			errno_exit("VIDIOC_QUERYBUF");

		(p_buffers)[n_buffers].length = buf.length;
		(p_buffers)[n_buffers].start =
				mmap(NULL /* Start from anywhere */,
					buf.length,
					PROT_READ | PROT_WRITE /* Required */,
					MAP_SHARED /* Recommended */,
					_fd, buf.m.offset);

		if (MAP_FAILED == (p_buffers)[n_buffers].start)
			errno_exit("mmap");
	}
}

void CapturingItem::init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(_fd, VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
				fprintf(stderr, "%s does not support "
							"user pointer i/o\n", _dev_name.c_str());
				exit(EXIT_FAILURE);
		} else {
				errno_exit("VIDIOC_REQBUFS");
		}
	}

	p_buffers = (struct buffer*)(calloc(4, sizeof(*p_buffers)));

	if (!p_buffers) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
		(p_buffers)[n_buffers].length = buffer_size;
		(p_buffers)[n_buffers].start = malloc(buffer_size);

		if (!(p_buffers)[n_buffers].start) {
			fprintf(stderr, "Out of memory\n");
			exit(EXIT_FAILURE);
		}
	}
}

void CapturingItem::close_device(void)
{
	if (-1 == close(_fd))
		errno_exit("close");

	_fd = -1;
}
