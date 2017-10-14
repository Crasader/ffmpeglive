#pragma once

#include "XData.h"

struct AVFrame;//������
struct AVPacket;
class AVCodecContext;
enum XSampleFMT
{
	X_S16=1,
	X_FLATP=8
};

//����Ƶ����ӿ���
class XMediaEncode
{
public:
	//�������
	int inWidth = 640;
	int inHeight = 480;
	int inPixSize = 3;

	int channels = 2;
	int sampleRate = 44100;
	XSampleFMT inSampleFmt = X_S16;

	//�������
	int outWidth = 640;
	int outHeight = 480;
	int bitrate = 4000000;//ѹ����ÿ����Ƶ��bitλ��С 50kB
	int fps = 25;

	XSampleFMT outSampleFmt = X_FLATP;
	int nbSample = 1024;

	//������������
	static XMediaEncode* Get(unsigned char index = 0);

	//��ʼ�����ظ�ʽת����������
	virtual bool InitScale() = 0;

	//����ֵ�������������
	virtual XData RGBToYUV(XData rgb) = 0;

	//��Ƶ�ز��������ĳ�ʼ��
	virtual bool InitResample() = 0;

	//����ֵ�������������
	virtual XData Resample(XData d) = 0;

	//��Ƶ��������ʼ��
	virtual bool InitVideoCodec() = 0;

	//��Ƶ��������ʼ��
	virtual bool InitAudioCode() = 0;

	//��Ƶ����  ����ֵ�������������
	virtual XData EncodeVideo(XData frame) = 0;

	//��Ƶ���� ����ֵ�������������
	virtual XData EncodeAudio(XData frame) = 0;

	virtual ~XMediaEncode();

	AVCodecContext *vc = 0;//��Ƶ������������
	AVCodecContext* ac = 0;//��Ƶ������������
protected:
	//��Ƴɹ���ģʽ�������캯�����ڱ��������У������û���������,���ڲ���������ʵ�ʾ��ǵ���ģʽ
	XMediaEncode();
};

