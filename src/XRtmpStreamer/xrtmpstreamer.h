#pragma once

#include <QtWidgets/QWidget>
#include "ui_xrtmpstreamer.h"

class XRtmpStreamer : public QWidget
{
    Q_OBJECT

public:
    XRtmpStreamer(QWidget *parent = Q_NULLPTR);

	//QT�źŵĲۺ�������ʱ����ʹ��public slots:   ���б�����Q_OBJECT
public slots:
	void Stream();
private:
    Ui::XRtmpStreamerClass ui;
};
