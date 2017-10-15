#include "XVideoCapture.h"
#include <iostream>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;

#pragma comment(lib, "opencv_world320d.lib")

/*
��Ƶ�ɼ��࣬���ù���ģʽ
*/
class CXVideoCapture : public XVideoCapture
{
public:
	VideoCapture cam;

	//��Ϊ�̳����߳��࣬��˴˴���дrun���������߳��н������ݲɼ�
	void run()
	{
		Mat frame;

		while (!isExit)
		{
			/*
			VideoCapture��read�����ǽ����grab��retrieve����
			grab:������һ֡,����rtsp����ͷ���䲶�����һ֡h264����,��Ϊrtsp��������ͷ�ڲ��Ѿ����˱��������Ȼ��
			���紫������ģ������grab����Ҫ���н������,�����yuvͼ��Ȼ�����ڱ��ص���ͨ����ͷ������ֱ�Ӳ����ͼ�����ݣ�����ͷ
			Ӳ���ڲ�����������yuvͼ�����ݡ�
			retrieve:����������ڲ�����ͼ��ת������yuvͼ��ת��Ϊrgb��ʽ�����frame�о���rgbͼ���ʽ���ݡ�

			read�ڲ�����ͬʱ����grab��retrieve�Ĺ���
			*/
			if (!cam.read(frame))
			{
				msleep(1);
				continue;
			}

			if (frame.empty())
			{
				msleep(1);
				continue;
			}

			//ȷ��������������
			XData d((char*)frame.data, frame.cols*frame.rows*frame.elemSize(), GetCurTime());//ÿ�ɼ�һ֡����һ�ε�ǰʱ��
			Push(d);
		}
	}

	bool Init(int camIndex)
	{
		////////////////////////////////
		///1 ʹ��opencv��rtsp���
		//cam.open(inUrl);
		cam.open(camIndex);//����0��ʾ����������ͷ���ڲ�ƥ��
		if (!cam.isOpened())
		{
			cout << "cam open failed!" << endl;

			return false;
		}
		cout << camIndex << "cam open success" << endl;

		width = cam.get(CAP_PROP_FRAME_WIDTH);//��ȡ������Ŀ�
		height = cam.get(CAP_PROP_FRAME_HEIGHT);////��ȡ������ĸ�
		fps = cam.get(CAP_PROP_FPS);//ֱ�Ӵ�����ͷ����ȡ��֡��Ϊ0��������Ϊ�˲���ǿ��д����25
		if (fps == 0) fps = 25;

		return true;
	}

	bool Init(const char* url)
	{
		////////////////////////////////
		///1 ʹ��opencv��rtsp���
		//cam.open(inUrl);
		cam.open(url);//����0��ʾ����������ͷ���ڲ�ƥ��
		if (!cam.isOpened())
		{
			cout << "cam open failed!" << endl;

			return false;
		}
		cout << url << "cam open success" << endl;

		width = cam.get(CAP_PROP_FRAME_WIDTH);//��ȡ������Ŀ�
		height = cam.get(CAP_PROP_FRAME_HEIGHT);////��ȡ������ĸ�
		fps = cam.get(CAP_PROP_FPS);//ֱ�Ӵ�����ͷ����ȡ��֡��Ϊ0��������Ϊ�˲���ǿ��д����25
		if (fps == 0) fps = 25;

		return true;
	}

	void Stop()
	{
		XDataThread::Stop();

		if (cam.isOpened())
		{
			cam.release();
		}
	}
};

XVideoCapture *XVideoCapture::Get(unsigned char index)
{
	static CXVideoCapture xc[255];
	return &xc[index];
}

XVideoCapture::XVideoCapture()
{
}


XVideoCapture::~XVideoCapture()
{
}
