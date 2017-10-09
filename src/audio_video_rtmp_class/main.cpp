#include <QtCore/QCoreApplication>
#include <iostream>
#include <QThread>
#include "XMediaEncode.h"
#include "XRtmp.h"
#include "XAudioRecord.h"

using namespace std;

/*
ʹ��QT��Ƶ¼�ƽӿڽ���¼��
*/
int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

	//nginx-rtmp ֱ��������rtmp����URL
	char *outUrl = "rtmp://192.168.124.146/live";

	int ret = 0;

	int sampleRate = 44100;
	int channels = 2;
	int sampleByte = 2;
	int nbSample = 1024;

	//1 qt��Ƶ��ʼ¼��
	XAudioRecord *ar= XAudioRecord::Get();
	ar->sampleRate = sampleRate;
	ar->channels = channels;
	ar->sampleByte = sampleByte;
	ar->nbSamples = nbSample;
	if (!ar->Init())
	{
		cout << "XAudioRecord Init failed!" << endl;
		getchar();
		return -1;
	}

	/*
	qt¼�Ƴ�������Ƶ�������ʽ��UnSignedInt������aac������Ҫ�ĸ�ʽ��float���ͣ������Ҫ�ز���
	*/
	///2 ��Ƶ�ز��� �����ĳ�ʼ�� 
	XMediaEncode* xe = XMediaEncode::Get();
	xe->channels = channels;
	xe->nbSample = nbSample;
	xe->sampleRate = sampleRate;
	xe->inSampleFmt = XSampleFMT::X_S16;
	xe->outSampleFmt = XSampleFMT::X_FLATP;
	if (!xe->InitResample())
	{
		getchar();
		return -1;
	}
 
	///4 ��ʼ����Ƶ������
	if (!xe->InitAudioCode())
	{
		getchar();
		return -1;
	}

	///5. �����װ������Ƶ������
	//a ���������װ��������
	XRtmp *xr = XRtmp::Get(0);
	if (!xr->Init(outUrl))
	{
		getchar();
		return -1;
	}

	//b�����Ƶ��
	if (!xr->AddStream(xe->ac))
	{
		getchar();
		return -1;
	}

	//��rtmp����������IO
	//д���װͷ
	if (!xr->SendHead())
	{
		getchar();
		return -1;
	}

	//һ�ζ�ȡһ֡��Ƶ���ֽ���
	int readSize = xe->nbSample * channels * sampleByte;
	for (;;)
	{
		//һ�ζ�ȡһ֡��Ƶ
		XData d = ar->Pop();
		if (d.size <= 0)
		{
			QThread::msleep(1);
			continue;
		}

		//�Ѿ���һ֡Դ����
		//�ز���Դ����
		AVFrame* pcm = xe->Resample(d.data);
		d.Drop();

		//pts����
		//nb_sample/sample_rate = һ֡��Ƶ������sec
		//timebase pts = sec * timebase.den
		AVPacket *pkt = xe->EncodeAudio(pcm);
		if (!pkt) continue;

		//����
		xr->SendFrame(pkt);
		if (ret == 0)
		{
			cout << "#" << flush;
		}
	}

	getchar();
	return a.exec();
}
