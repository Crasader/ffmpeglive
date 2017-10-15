#include "XDataThread.h"

//�����������������
void XDataThread::Clear()
{
	mutex.lock();
	while (!datas.empty())
	{
		datas.front().Drop();
		datas.pop_front();
	}

	mutex.unlock();
}

//���б��β����
void XDataThread::Push(XData d)
{
	mutex.lock();
	if (datas.size() > maxList)//���������еĻ���������ֵ
	{
		datas.front().Drop();//�����ͷ�ڵ������
		datas.pop_front();//�����������ݰ��ڵ��������ɾ��
	}
	datas.push_back(d);//�������ݷ��������β�ڵ�
	mutex.unlock();
}

//��ȡ�б������������
XData XDataThread::Pop()
{
	mutex.lock();
	if (datas.empty())//�����������ݽڵ㣬�򷵻����ݰ���������ʵ������
	{
		mutex.unlock();//��ס������Ҫ����   
		return XData();
	}
	XData d = datas.front();//����ͷ�ڵ�����
	datas.pop_front();
	mutex.unlock();

	return d;
}

//�����߳�
bool XDataThread::Start()
{
	isExit = false;
	QThread::start();

	return true;
}

//�˳��̣߳����ȴ��߳��˳�(����)
void XDataThread::Stop()
{
	isExit = true;
	wait();
}

XDataThread::XDataThread()
{
}


XDataThread::~XDataThread()
{
}
