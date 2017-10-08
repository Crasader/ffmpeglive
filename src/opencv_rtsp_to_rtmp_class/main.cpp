/*
ʹ��ffmpeg�Ĳ�������
ffplay "rtmp://192.168.124.142/live live=1"
*/


#include <opencv2/highgui.hpp>
#include<iostream>

#include "XMediaEncode.h"
#include "XRtmp.h"

using namespace std;
using namespace cv;

#pragma comment(lib, "opencv_world320d.lib")


//�����������rtsp������ת��Ϊrtmp��������
int main(int argc, char* argv[])
{
	//���������rtsp url
	char *inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp ֱ��������rtmp����URL
	char *outUrl = "rtmp://192.168.124.142/live";
	
	//�����������ظ�ʽת��
	XMediaEncode* me = XMediaEncode::Get(0);
	
	//��װ����������
	XRtmp *xr = XRtmp::Get(0);

	VideoCapture cam;
	Mat frame;
	namedWindow("video"); 

	int ret = 0;
	try
	{
		////////////////////////////////
		///1 ʹ��opencv��rtsp���
		//cam.open(inUrl);
		cam.open(0);//����0��ʾ����������ͷ���ڲ�ƥ��
		if (!cam.isOpened())
		{
			throw exception("cam open failed!");
		}
		cout << inUrl << "cam open success" << endl;
		
		int inWidth = cam.get(CAP_PROP_FRAME_WIDTH);//��ȡ������Ŀ�
		int inHeight = cam.get(CAP_PROP_FRAME_HEIGHT);////��ȡ������ĸ�
		int fps = 25;//cam.get(CAP_PROP_FPS);//ֱ�Ӵ�����ͷ����ȡ��֡��Ϊ0��������Ϊ�˲���ǿ��д����25

		//2 ��ʼ����ʽת��������
		//3 ��ʼ����������ݽṹ
		me->inWidth = inWidth;
		me->inHeight = inHeight;
		me->outWidth = inWidth;
		me->outHeight = inHeight;
	
		me->InitScale();

		/// 4.��ʼ������������
		//a �ҵ�������
		if (!me->InitVideoCodec())
		{
			throw exception("InitVideoCodec failed");
		}
		
		///5 �����װ������Ƶ������
		xr->Init(outUrl);

		//�����Ƶ��
		xr->AddStream(me->vc);
		xr->SendHead();

		for (;;)
		{
			///��ȡrtsp��Ƶ֡��������Ƶ֡
			if (!cam.grab())//ֻ�������룬����ͼ��ת��
			{
				continue;
			}
			///ת��yuv��rgb
			if (!cam.retrieve(frame))
			{
				continue;
			}

			imshow("video", frame);
			waitKey(1);

			//rgb to yuv
			me->inPixSize = frame.elemSize();
			AVFrame* yuv = me->RGBToYUV((char*)frame.data);
			if (!yuv)
			{
				continue;
			}
			//cout << h << " " << flush;

			///h264����
			AVPacket *pack = me->EncodeVideo(yuv);
			if (!pack) continue;

			xr->SendFrame(pack);
		}
	}
	catch (exception &ex)
	{
		if (cam.isOpened()) 
				cam.release();
		
		/*if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}*/
		//if (vc)
		//{
		//	/*
		//	����ʹ��avio_close�ر�IO����Ϊ��avcodec_free_context�ֻ�ر�һ�Σ���ɶ�ιر�
		//	���ʹ��avio_closep�����ڲ���null��������ֱ��ic->pb=NULL;
		//	*/
		//	avio_closep(&ic->pb);
		//	avcodec_free_context(&vc);//�ͷű����������ģ��ڲ�ͬʱ��رձ�����
		//}
		cerr << ex.what() << endl;
	}
	
	getchar();
	return 0;
}