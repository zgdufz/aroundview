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

import QtQuick 2.6
import QtQuick.Controls 2.0
import OpenCV 1.0
import Qt5Compat.GraphicalEffects

ApplicationWindow {
	visible: true
	width: 1280
	height: 720
	title: qsTr("around view!")


	//Car image
	Image {
		id: car
		anchors.centerIn: parent
		source: './images/HMI_Radiocontrol_Car.png'
		z: 3
	}

	//border
	Rectangle {
		width: 5
		height: 1550
		anchors.bottom: back_rec.bottom
		anchors.bottomMargin:371
		visible: true
		color: "blue"

		transformOrigin: Rectangle.BottomLeft
		rotation: 45

		z: 2
	}
	Rectangle {
		width: 5
		height: 1550
		anchors.right: parent.right
		anchors.bottom: back_rec.bottom
		anchors.bottomMargin:371
		visible: true
		color: "blue"

		transformOrigin: Rectangle.BottomLeft
		rotation: -45
		z: 2
	}

	//front
	Rectangle {
		id: front_rec
		width: 1080
		height: 910
		anchors.bottom: car.verticalCenter
		anchors.horizontalCenter: parent.horizontalCenter
		visible: false

		Camera1 {
			id: camera1
			property int ret: 0
			anchors.fill: parent
		}
	}

	//mask
	Image {
		id:front_mask
		source: "images/triangle_front_full.png"
		sourceSize: Qt.size(1080, 540)
		smooth: true
		visible: false
	}

	OpacityMask {
		anchors.fill: front_rec
		source: front_rec
		maskSource: front_mask
		z: 1
	}


	//back
	Rectangle {
		id: back_rec
		width: 1080
		height: 910
		anchors.top: car.verticalCenter
		// anchors.topMargin : -100
		anchors.horizontalCenter: parent.horizontalCenter
		visible: false

		Camera0 {
			id: camera0
			property int ret: 0
			anchors.fill: parent
		}
	}

	//mask
	Image {
		id:back_mask
		source: "images/triangle_back_full.png"
		sourceSize: Qt.size(back_rec.width, back_rec.height)
		smooth: true
		visible: false
	}

	OpacityMask {
		anchors.fill: back_rec
		source: back_rec
		maskSource: back_mask
		z: 1
	}


	//left
	Rectangle {
		id: left_rec
		width: 1080
		height: 540
		anchors.top: parent.top
		anchors.topMargin: (parent.height/2)+(width/2)
		anchors.left: parent.left
		transformOrigin: Rectangle.TopLeft
		rotation: 270

		visible: true

		Camera2 {
			id: camera2
			property int ret: 0
			anchors.fill: parent
		}
	}


	//right
	Rectangle {
		id: right_rec
		width: 1080
		height: 540
		anchors.top: parent.top
		anchors.topMargin: (parent.height/2)+(width/2)
		anchors.right: parent.right
		transformOrigin: Rectangle.TopRight
		rotation: 90

		visible: true

		Camera3 {
			id: camera3
			property int ret: 0
			anchors.fill: parent
		}
	}

		Timer {
			id: timer
			property int frameRate: 30
			interval: 1000 / timer.frameRate
			running: true
			repeat: true

			onTriggered:  {
				camera0.update();
				camera1.update();
				camera2.update();
				camera3.update();
			}
	}

	Component.onCompleted: {
		camera0.ret=camera0.init(0,100)
		camera1.ret=camera1.init(1,-100)
		camera2.ret=camera2.init(2,0)
		camera3.ret=camera3.init(3,0)
	}
}
