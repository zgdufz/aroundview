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


#ifndef CAPTURING_H
#define CAPTURING_H


class CapturingItem
{

public:
	CapturingItem();
	~CapturingItem();
	int open_device(int dev);
	int init_device(void);
	void start_capturing(void);
	void close_device(void);
	unsigned char* snap_frame(void);
	int width(void) const
	{
		return _width;
	}

	int height(void) const
	{
		return _height;
	}


private:
	struct buffer {
		void *start;
		size_t length;
	};

	std::string _dev_name;
	int _fd;
	struct buffer *p_buffers;
	unsigned int n_buffers;
	int _fps_count;
	int _framerate;
	int _timeout; /* secs */
	int _left;
	int _top;
	int _width;
	int _height;

	void errno_exit(const char *s);
	int xioctl(int fh, int request, void *arg);
	void uninit_device(void);
	void init_read(unsigned int buffer_size);
	void init_mmap(void);
	void init_userp(unsigned int buffer_size);
	int read_frame(void);
};
#endif	/* CAPTURING_H */
