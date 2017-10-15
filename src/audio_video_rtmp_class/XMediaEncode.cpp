#include "XMediaEncode.h"

extern "C"
{
#include <libswscale/swscale.h>
#include<libavcodec/avcodec.h>
#include<libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swresample.lib")

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

		if (asc)
		{
			swr_free(&asc);
		}

		if (yuv)
		{
			av_frame_free(&yuv);//�˺����������ͷ������ڴ棬�����ڲ���yuv��null
		}

		if (vc)
		{
			avcodec_free_context(&vc);//�ͷű����������ģ��ڲ�ͬʱ��رձ�����
		}

		if (pcm)
		{
			av_frame_free(&pcm);//�˺����������ͷ������ڴ棬�����ڲ���yuv��null
		}


		vpts = 0;
		av_packet_unref(&vpack);
		apts = 0;
		av_packet_unref(&apack);
	}

	bool InitAudioCode()
	{
		///4 ��ʼ����Ƶ������
		if (!(ac = CreateCodec(AV_CODEC_ID_AAC)))
		{
			return false;
		}

		ac->bit_rate = 40000;
		ac->sample_rate = sampleRate;
		ac->sample_fmt = AV_SAMPLE_FMT_FLTP;
		ac->channels = channels;
		ac->channel_layout = av_get_default_channel_layout(channels);

		//����Ƶ������
		return OpenCodec(&ac);
	}

	bool InitVideoCodec()
	{
		/// 4.��ʼ������������
		//a �ҵ�������
		if (!(vc = CreateCodec(AV_CODEC_ID_H264)))
		{
			return false;
		}

		vc->bit_rate = 50 * 1024 * 8;//ѹ����ÿ����Ƶ��bitλ��С 50kB
		vc->width = outWidth;
		vc->height = outHeight;
		//vc->time_base = { 1, fps };//ʱ�����������ʵ����ÿһ֡ռ�õ�ʱ��
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
		return OpenCodec(&vc);;
	}

	//��Ƶ����
	XData EncodeVideo(XData frame)
	{
		av_packet_unref(&vpack);//�������ϴ�packet������

		XData r;
		if (frame.size <= 0 || !frame.data) return r;
		AVFrame* p = (AVFrame*)frame.data;

		///h264����
		//frame->pts = vpts;
		//vpts++;
		/*
		�������������avcodec_send_frame��avcodec_receive_packet
		avcodec_send_frameֻ�ǵ��̼߳򵥵Ľ����ݿ�����ȥ����avcodec_receive_packet�ǻ�
		�ڲ�������߳̽��б���Ȼ�󷵻ر���������
		*/

		/*
		avcodec_send_frame�ڲ��ǻ��л��壬��˲�һ��������avcodec_send_frame���ͻ�avcodec_receive_packet��ȡ�����ݣ�
		�����ڶ���������null�����Ի�ȡ���������ڵ�����
		*/
		int ret = avcodec_send_frame(vc, p);
		if (ret != 0)
		{
			return r;
		}

		/*
		����avcodec_receive_packetʱ���ڶ�������ÿ�ζ����ڲ��ȵ���av_frame_unref�������
		*/
		ret = avcodec_receive_packet(vc, &vpack);
		if (ret != 0 || vpack.size <= 0)
		{
			return r;
		}

		r.data = (char*)&vpack;
		r.size = vpack.size;
		r.pts = frame.pts;

		return r;
	}

	long long lasta = -1;
	XData EncodeAudio(XData frame)
	{
		XData r;

		//pts����
		//nb_sample/sample_rate = һ֡��Ƶ������sec
		//timebase pts = sec * timebase.den
		if (frame.size <= 0 || !frame.data) return r;

		AVFrame* p = (AVFrame* )frame.data;
		if (lasta == p->pts)//ǰ��ʱ���������ȣ�����ȣ����Խ���ǰʱ�������һ��
		{
			p->pts += 1200;
		}
		lasta = p->pts;

		int ret = avcodec_send_frame(ac, p);
		if (ret != 0) return r;

		av_packet_unref(&apack);
		ret = avcodec_receive_packet(ac, &apack);
		if (ret != 0) return r;

		cout << apack.size << " " << flush;
		r.data = (char*)&apack;
		r.size = apack.size;
		r.pts = frame.pts;

		return r;
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

	XData RGBToYUV(XData d)
	{
		XData r;
		r.pts = d.pts;

		//rgb to yuv
		//��������ݽṹ
		uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		//indata[0]  bgrbgrbgr
		//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
		indata[0] = (uint8_t*)d.data;
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
			return r;
		}

		yuv->pts = d.pts;

		r.data = (char*)yuv;
		int *p = yuv->linesize;
		while (*p)//�ۼӵõ�yuvͼ����ܴ�С
		{
			r.size += (*p)*outHeight;
			p++;
		}

		return r;
	}

	bool InitResample()
	{
		/*
		qt¼�Ƴ�������Ƶ�������ʽ��UnSignedInt������aac������Ҫ�ĸ�ʽ��float���ͣ������Ҫ�ز���
		*/
		///2 ��Ƶ�ز��� �����ĳ�ʼ�� 
		asc = swr_alloc_set_opts(asc,
			av_get_default_channel_layout(channels), (AVSampleFormat)outSampleFmt, sampleRate,//�����ʽ
			av_get_default_channel_layout(channels), (AVSampleFormat)inSampleFmt, sampleRate, 0, 0); //�����ʽ																	//�����ʽ
		if (!asc)
		{
			cout << "swr_alloc_set_opts failed!" << endl;
			return false;
		}

		int ret = swr_init(asc);
		if (ret != 0)
		{
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;

			return false;
		}

		cout << "��Ƶ�ز��������ĳ�ʼ���ɹ�" << endl;

		///3 ��Ƶ�ز�������ռ����
		pcm = av_frame_alloc();
		pcm->format = outSampleFmt;
		pcm->channels = channels;
		pcm->channel_layout = av_get_default_channel_layout(channels);
		pcm->nb_samples = nbSample; //һ֡��Ƶһͨ���Ĳ����������ڲ�Ȼ�����ͨ�����ֵ��ͨ��������������Ĵ�С����һ�����Ƶ�ֽ�����һ֡��Ƶ���ֽ���
		ret = av_frame_get_buffer(pcm, 0);//��pcm����洢�ռ䣬�ڶ���������0��ʾ����Ҫ����
		if (ret != 0)
		{
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;

			return false;
		}

		return true;
	}

	XData Resample(XData d)
	{
		XData r;

		//�Ѿ���һ֡Դ����
		//�ز���Դ����
		const uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
		indata[0] = (uint8_t *)d.data;
		//�����ز�����ÿһ�������Ĳ�������
		int len = swr_convert(asc, pcm->data, pcm->nb_samples,//�������������洢��ַ����������
			indata, pcm->nb_samples);

		if (len <= 0)
		{
			return r;
		}

		pcm->pts = d.pts;

		r.data = (char*)pcm;
		r.size = pcm->nb_samples*pcm->channels * 2;
		r.pts = d.pts;

		return r;
	}
private:
	AVCodecContext* CreateCodec(AVCodecID cid) 
	{
		///4 ��ʼ����Ƶ������
		AVCodec *codec = avcodec_find_encoder(cid);
		if (!codec)
		{
			cout << "avcodec_find__encoder failed!" << endl;
			return NULL;
		}

		//��Ƶ������������
		AVCodecContext* c = avcodec_alloc_context3(codec);
		if (!c)
		{
			cout << "avcodec_alloc_context3 failed!" << endl;
			return NULL;
		}

		cout << "avcodec_alloc_context3 success!" << endl;

		c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
		c->thread_count = XGetCpuNum();//�����˱��������ڲ������߳�����
		c->time_base = { 1, 1000000 };//����ʱ�����׼

		return c;
	}

	bool OpenCodec(AVCodecContext **c)
	{
		//����Ƶ������
		int ret = avcodec_open2(*c, 0, 0);
		if (ret != 0)
		{
			char err[1024] = { 0 };
			av_strerror(ret, err, sizeof(err) - 1);
			cout << err << endl;
			avcodec_free_context(c);
			return false;
		}
		cout << "avcodec_open2 success!" << endl;
	}

	SwsContext *vsc = NULL;//���ظ�ʽת��������
	AVFrame *yuv = NULL;//�����YUV
	//AVCodecContext *vc = NULL;//������������
	AVPacket vpack = {0};//��Ƶ֡
	AVPacket apack = {0};//��Ƶ֡
	int vpts = 0;
	int apts = 0;
	SwrContext* asc = NULL;//��Ƶ�ز���������
	AVFrame *pcm = NULL;//�ز��������pcm
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
