TARGET = aroundview
QT = quickcontrols2
QT += widgets
QT += qml quick gui


SOURCES = main.cpp \
			camera.cpp \
			capturing.cpp \


HEADERS += camera.h \
			capturing.h \

RESOURCES += AroundView.qrc\
				images/images.qrc

#INCLUDEPATH += /usr/include/opencv2
#DEPENDPATH += /usr/include/opencv2

LIBS += `pkg-config opencv --libs`

LIBS += -lopencv_core \
	-lopencv_highgui \
	-lopencv_videoio \
	-lopencv_imgproc \
