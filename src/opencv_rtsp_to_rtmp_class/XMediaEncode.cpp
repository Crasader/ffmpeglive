#include "XMediaEncode.h"

extern "C"
{
#include <libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")

#include <iostream>
using namespace std;

//��ȡCPU������
//WIN32 32λ�ĺ�
#if defined WIN32 || defined _WIN32//64λ�ĺ�
#include <windows.h>
#endif
static int XGetCpuNum()
{
#if defined WIN32 || defined _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return (int)sysinfo.dwNumberOfProcessors;
#elif defined __linux__
	return (int)sysconf(_SC_NPROCESSORS_ONLN);
#elif defined __APPLE__
	int numCPU = 0;
	int mib[4];
	size_t len = sizeof(numCPU);

	//set the mib for hw.ncpu
	mib[0] = CTL_HW;
	mib0[1] = HW_AVAILCPU;//alternatively, try HW_NCPU

	sysctl(mib, 2, &numCPU, &len, NULL, 0);

	if (numCPU < 1)
	{
		mib[1] = HW_NCPU;
		sysctl(mib, 2, &numCPU, &len, NULL, 0);

		if (numCPU < 1)
			numCPU = 1;
	}
	return (int)numCPU;
#else
	return 1;
#endif
}

class CXMediaEncode : public XMediaEncode
{
public:
	void Close()
	{
		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}

		if (yuv)
		{
			av_frame_free(&yuv);//�˺����������ͷ������ڴ棬�����ڲ���yuv��null
		}

		if (vc)
		{
			avcodec_free_context(&vc);//�ͷű����������ģ��ڲ�ͬʱ��رձ�����
		}

		vpts = 0;
		av_packet_unref(&pack);
	}

	bool InitVideoCodec()
	{
		/// 4.��ʼ������������
		//a �ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);//������Ҫ�ͷ�ָ���ڴ棬��Ϊ����ȡ���ڴ��ַ����
		if (!codec)
		{
			cout << "Can not find h264 encoder!" << endl;
			return false;
		}
		//b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			cout << "avcodec_alloc_context3 failed!" << endl;
		}
		//c ���ñ���������
		vc->flags = AV_CODEC_FLAG_GLOBAL_HEADER;//ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = XGetCpuNum();//������߳�����

		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50kB
		vc->width = outWidth;
		vc->height = outHeight;
		vc->time_base = { 1, fps };//ʱ�����������ʵ����ÿһ֡ռ�õ�ʱ��
		vc->framerate = { fps, 1 };//֡��

								   /*������Ĵ�С������֡һ���ؼ�֡
								   �ؼ�֡Խ����ѹ����Խ�ߣ���Ϊ�����֡����P֡��B֡���Ǳ�����ǰ֡���֡�ı仯���ݡ����Ǵ����ĸ���Ӱ��
								   ���Ǳ���õ���50֡����ô�ý�ǰ��49֡ȫ�����������ʾ��50֡
								   */
		vc->gop_size = 50;
		/*
		����B֡����Ϊ0����ʾ������B֡������pts��dts��һ��
		��Ϊ��B֡�Ļ�����Ҫ�Ƚ�������P֡��B֡�Ǹ���ǰ��֡����ѹ����ͼ��֡��ѹ���ʺܸ�
		*/
		vc->max_b_frames = 0;
		vc->pix_fmt = AV_PIX_FMT_YUV420P;

		//d �򿪱�����������
		/*
		��һ�������Ǳ����������ģ��ڶ��������Ǳ�����������avcodec_alloc_context3����ʱ�����ˣ����������
		��ָ������Ϊ0����������������ҲĬ����Ϊ0
		*/
		int ret = avcodec_open2(vc, 0, 0);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			cout << buf << endl;
			return false;
		}

		cout << "avcodec_open2 success!" << endl;

		return true;
	}

	//��Ƶ����
	AVPacket* EncodeVideo(AVFrame* frame)
	{
		av_packet_unref(&pack);//�������ϴ�packet������

		///h264����
		frame->pts = vpts;
		vpts++;
		/*
		�������������avcodec_send_frame��avcodec_receive_packet
		avcodec_send_frameֻ�ǵ��̼߳򵥵Ľ����ݿ�����ȥ����avcodec_receive_packet�ǻ�
		�ڲ�������߳̽��б���Ȼ�󷵻ر���������
		*/

		/*
		avcodec_send_frame�ڲ��ǻ��л��壬��˲�һ��������avcodec_send_frame���ͻ�avcodec_receive_packet��ȡ�����ݣ�
		�����ڶ���������null�����Ի�ȡ���������ڵ�����
		*/
		int ret = avcodec_send_frame(vc, frame);
		if (ret != 0)
		{
			return NULL;
		}

		/*
		����avcodec_receive_packetʱ���ڶ�������ÿ�ζ����ڲ��ȵ���av_frame_unref�������
		*/
		ret = avcodec_receive_packet(vc, &pack);
		if (ret != 0 || pack.size <= 0)
		{
			return NULL;
		}

		return &pack;
	}

	bool InitScale()
	{
		//2 ��ʼ����ʽת��������  �˺������Ը��ݳߴ�ı仯�����ڲ����´���������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//Դ���ߣ����ظ�ʽ
			outWidth, outHeight, AV_PIX_FMT_YUV420P,//Ŀ����ߣ����ظ�ʽ
			SWS_BICUBIC,//�����Բ�ֵ�� �ߴ�仯ʹ���㷨�����ߴ粻�仯����ʹ��
			0, 0, 0//�������Ȳ�����ʹ��
		);
		if (!vsc)
		{
			cout<<"sws_getCachedContext failed!"<<endl;
			return false;
		}

		//3.��ʼ����������ݽṹ
		yuv = av_frame_alloc();
		yuv->format = AV_PIX_FMT_YUV420P;
		yuv->width = inWidth;
		yuv->height = inHeight;
		yuv->pts = 0;
		//����yuv�ռ䣬������32λ�����
		int ret = av_frame_get_buffer(yuv, 32);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		return true;
	}

	AVFrame* RGBToYUV(char *rgb)
	{
		//rgb to yuv
		//��������ݽṹ
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		//indata[0]  bgrbgrbgr
		//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
		indata[0] = (uint8_t*)rgb;
		int insize[AV_NUM_DATA_POINTERS] = { 0 };
		//һ��(��)���ݵ��ֽ���
		insize[0] = inWidth * inPixSize;

		/*
		��ʽת������
		��һ������Ϊ��ʽת��������
		�ڶ�������Ϊ�������ݣ�����������Ϊ����ͼ��һ�е��ֽ��������ĸ�����Ϊͼ�����ݵ���ʼ
		λ�ã�Ĭ��Ϊ0��ʾ��ͷ��ʼ������������������ݵĸ�
		����ֵΪת����ͼ��ĸ߶�
		*/
		int h = sws_scale(vsc, indata, insize, 0, inHeight, //Դ����
			yuv->data, yuv->linesize);//Ŀ�����ݵ�buffer��Ŀ��ͼ��һ�е��ֽ���
		if (h <= 0)
		{
			return NULL;
		}

		return yuv;
	}

private:
	SwsContext *vsc = NULL;//���ظ�ʽת��������
	AVFrame *yuv = NULL;//�����YUV
	//AVCodecContext *vc = NULL;//������������
	AVPacket pack = {0};
	int vpts = 0;
};

XMediaEncode* XMediaEncode::Get(unsigned char index)
{
	static bool isFirst = true;
	if (isFirst)
	{
		/*
		ע�����еı������,ע������
		av_register_all();��ʼ�����з�װ�ͽ��װ ��flv mp4 mov mp3,�������������ͽ�����
		*/
		avcodec_register_all();
		isFirst = false;
	}

	static CXMediaEncode cxm[255];
	return &cxm[index];
}

XMediaEncode::XMediaEncode()
{
}


XMediaEncode::~XMediaEncode()
{
}
