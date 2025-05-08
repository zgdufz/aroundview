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

#include "camera.h"
#include "capturing.h"
#include <QDebug>




CameraItem::CameraItem(QQuickItem *parent)
	:	QQuickPaintedItem(parent), _device(-1), _offset(0)
{
}

CameraItem::~CameraItem()
{
	if(_device != -1) {
		p_cap_item->close_device();
	}
	if(p_cap_item)
		delete p_cap_item;
}


int CameraItem::init(int device, int offset)
{
	p_cap_item = new CapturingItem();

	int ret = p_cap_item->open_device(device);

	if(ret != 0) {
		qDebug() << "device " << device << " failed to open";
		return -1;
	}

	ret = p_cap_item->init_device();
	if(ret != 0) {
		qDebug() << "device " << device << " failed to initialize";
		return -1;
	}
	p_cap_item->start_capturing();
	_device = device;
	_offset = offset;

	return ret;
}

void CameraItem::paint(QPainter *painter)
{
	if(_device != -1){
		unsigned char* data = NULL;
		cv::Mat src(p_cap_item->height(), p_cap_item->width(), CV_8UC4);
		cv::Mat dst;
		data = p_cap_item->snap_frame();
		if(data) {
			memcpy(src.data, data, src.step * src.rows);

			cv::cvtColor(src, dst, cv::COLOR_BGRA2RGB);

			QImage output_img(dst.data, dst.cols, dst.rows, QImage::Format_RGB888);

			output_img = output_img.scaled(1080, 911);

			painter->drawImage(0,0,
								output_img,
								0,((output_img.size().height() / 2) - (height() / 2) + _offset));
		}
		else {
			qDebug() << "null image";
		}
	}

}

