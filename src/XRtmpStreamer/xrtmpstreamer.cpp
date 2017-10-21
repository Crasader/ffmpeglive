#include "xrtmpstreamer.h"
#include <iostream>
#include "XController.h"

using namespace std;

static bool isStream = false;

XRtmpStreamer::XRtmpStreamer(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
}

//��ʼ��ť����źŵ���Ӧ�ۺ���
void XRtmpStreamer::Stream()
{
	cout << "Stream" << endl;
	if (isStream)
	{
		isStream = false;
		ui.pushButton->setText(QString::fromLocal8Bit("��ʼ"));

		XController::Get()->Stop();
	}
	else
	{
		isStream = true;
		ui.pushButton->setText(QString::fromLocal8Bit("ֹͣ"));
		QString url = ui.inUrl->text();
		bool ok = false;
		int camIndex = url.toInt(&ok);
		if (!ok)
		{
			XController::Get()->inUrl = url.toStdString();//���ô�����ͷ�ķ�ʽ
		}
		else
		{
			XController::Get()->camIndex = camIndex;
		}
		
		XController::Get()->outUrl = ui.outUrl->text().toStdString();//����������ַ
		XController::Get()->Set("d", (ui.face->currentIndex() + 1) * 3);//�������ռ���

		XController::Get()->Start();//��ʼ����
	}
}