#pragma once

#include <QThread>
#include <string>
#include "XDataThread.h"

//�̳��߳���
class XController : public XDataThread
{
public:
	std::string outUrl;//�����������ַ
	int camIndex = -1;//��¼����ͷ�Ĵ�id
	std::string inUrl = "";//rtsp������ַ
	std::string err = "";

	//ʹ�õ���ģʽ��ÿ�λ�ȡֻ�ܷ���ͬһ������
	static XController *Get()//�����Ǿ�̬�����ͳ�Ա
	{
		static XController xc;
		return &xc;
	}

	//�趨���ղ���
	virtual bool Set(std::string key, double val);
	//
	virtual bool Start();
	virtual void Stop();
	void run();//��Ҫ��д�߳�run����
	virtual ~XController();

protected:
	int vindex = 0;//��Ƶ������
	int aindex = 1;//��Ƶ������
	XController();
};

