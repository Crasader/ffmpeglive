#pragma once

#include <QThread>

enum XAUDIOTYPE
{
	X_AUDIO_QT
};

struct XData
{
	char *data = NULL;
	int size = 0;
	void Drop()
	{
		if (data)
			delete data;
		data = NULL;
		size = 0;
	}
};
class XAudioRecord : public QThread
{
public:
	int channels = 2;//声道数
	int sampleRate = 44100;//样本采样率
	int sampleByte = 2;//样本字节大小
	int nbSamples = 1024;//一帧音频每个通道的样本数量

	//调用者清理空间
	virtual XData Pop() = 0;

	//开始录制
	virtual bool Init() = 0;

	//停止录制
	virtual void Stop() = 0;

	static XAudioRecord *Get(XAUDIOTYPE type = X_AUDIO_QT, unsigned char index = 0);
	virtual ~XAudioRecord();

protected:
	XAudioRecord();
};

