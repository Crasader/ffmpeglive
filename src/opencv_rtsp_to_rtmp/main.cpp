#include <opencv2/highgui.hpp>
#include<iostream>
extern "C"
{
	#include <libswscale/swscale.h>
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

using namespace std;
using namespace cv;

#pragma comment(lib, "opencv_world320d.lib")
#pragma comment(lib, "swscale.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")

//�����������rtsp������ת��Ϊrtmp��������
int main(int argc, char* argv[])
{
	//���������rtsp url
	char *inUrl = "rtsp://test:test123456@192.168.1.109";
	//nginx-rtmp ֱ��������rtmp����URL
	char *outUrl = "rtmp://192.168.124.139/live";
	
	/*
	ע�����еı������,ע������
	av_register_all();��ʼ�����з�װ�ͽ��װ ��flv mp4 mov mp3,�������������ͽ�����
	*/
	avcodec_register_all();

	//ע�����еķ�װ��
	av_register_all();

	//ע�����е�����Э��
	avformat_network_init();

	VideoCapture cam;
	Mat frame;
	namedWindow("video"); 

	//���ظ�ʽת��������
	SwsContext *vsc = NULL;
	AVFrame *yuv = NULL;

	//������������
	AVCodecContext *vc = NULL;

	//rtmp flv��װ��
	AVFormatContext *ic = NULL;

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

		//2 ��ʼ����ʽת��������  �˺������Ը��ݳߴ�ı仯�����ڲ����´���������
		vsc = sws_getCachedContext(vsc,
			inWidth, inHeight, AV_PIX_FMT_BGR24,//Դ���ߣ����ظ�ʽ
			inWidth, inHeight, AV_PIX_FMT_YUV420P,//Ŀ����ߣ����ظ�ʽ
			SWS_BICUBIC,//�����Բ�ֵ�� �ߴ�仯ʹ���㷨�����ߴ粻�仯����ʹ��
			0, 0, 0//�������Ȳ�����ʹ��
		);
		if (!vsc)
		{
			throw exception("sws_getCachedContext failed!");
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

		/// 4.��ʼ������������
		//a �ҵ�������
		AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);//������Ҫ�ͷ�ָ���ڴ棬��Ϊ����ȡ���ڴ��ַ����
		if (!codec)
		{
			throw exception("Can not find h264 encoder!");
		}
		//b ����������������
		vc = avcodec_alloc_context3(codec);
		if (!vc)
		{
			throw exception("avcodec_alloc_context3 failed!");
		}
		//c ���ñ���������
		vc->flags = AV_CODEC_FLAG_GLOBAL_HEADER;//ȫ�ֲ���
		vc->codec_id = codec->id;
		vc->thread_count = 8;//������߳�����
		vc->bit_rate = 50*1024*8;//ѹ����ÿ����Ƶ��bitλ��С 50kB
		vc->width = inWidth;
		vc->height = inHeight;
		vc->time_base = {1, fps};//ʱ�����������ʵ����ÿһ֡ռ�õ�ʱ��
		vc->framerate = {fps, 1};//֡��

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
		ret = avcodec_open2(vc, 0, 0);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		cout << "avcodec_open2 success!" << endl;

		///5 �����װ������Ƶ������ 
		//a ���������װ��������
		ret = avformat_alloc_output_context2(&ic, 0, "flv", outUrl);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		//b �����Ƶ��, ����
		AVStream *vs = avformat_new_stream(ic, NULL);
		if (!vs)
		{
			throw exception("avformat_new_stream failed!");
		}
		vs->codecpar->codec_tag = 0;//��������Ϊ0���������
		//�ӱ��������Ʋ���
		avcodec_parameters_from_context(vs->codecpar, vc);
		
		av_dump_format(ic, 0, outUrl, 1);

		///��rtmp���������IO
		ret = avio_open(&ic->pb, outUrl, AVIO_FLAG_WRITE);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		/*д���װͷ ��һ�����ܻὫ������AVStream�е�timebase���ĵ���
		��˺���ʹ��ʱ,timebase��Ҫ���´�AVStream�л�ȡ,ʹ�øĺ��timebase���������ȷ�ģ�
		����ʹ��֮ǰ���õ�timebase
		*/
		ret = avformat_write_header(ic, NULL);
		if (ret != 0)
		{
			char buf[1024] = { 0 };
			av_strerror(ret, buf, sizeof(buf) - 1);
			throw exception(buf);
		}

		AVPacket pack;
		memset(&pack, 0, sizeof(pack));
		int vpts = 0;
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
			//��������ݽṹ
			uint8_t *indata[AV_NUM_DATA_POINTERS] = { 0 };
			//indata[0]  bgrbgrbgr
			//plane indata[0] bbbbb indata[1]ggggg indata[2]rrrrr
			indata[0] = frame.data;
			int insize[AV_NUM_DATA_POINTERS] = { 0 };
			//һ��(��)���ݵ��ֽ���
			insize[0] = frame.cols * frame.elemSize();
			
			/*
			��ʽת������
			��һ������Ϊ��ʽת��������
			�ڶ�������Ϊ�������ݣ�����������Ϊ����ͼ��һ�е��ֽ��������ĸ�����Ϊͼ�����ݵ���ʼ
			λ�ã�Ĭ��Ϊ0��ʾ��ͷ��ʼ������������������ݵĸ�
			����ֵΪת����ͼ��ĸ߶�
			*/
			int h = sws_scale(vsc, indata, insize, 0, frame.rows, //Դ����
			yuv->data, yuv->linesize);//Ŀ�����ݵ�buffer��Ŀ��ͼ��һ�е��ֽ���
			if (h <= 0)
			{
				continue;
			}
			//cout << h << " " << flush;

			///h264����
			yuv->pts = vpts;
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
			ret = avcodec_send_frame(vc, yuv);
			if (ret != 0)
			{
				continue;
			}
			
			/*
			����avcodec_receive_packetʱ���ڶ�������ÿ�ζ����ڲ��ȵ���av_frame_unref�������
			*/
			ret = avcodec_receive_packet(vc, &pack);
			if (ret != 0 || pack.size > 0)
			{
				cout << "*" << pack.size << flush;
			}
			else
			{
				continue;
			}


			//����
			/*
			�����������timebase���б仯��������ʹ��֡�ʽ��м���ġ�����ʱҪ��Ϊʹ������ʱ��timebase���½��м���
			*/
			pack.pts = av_rescale_q(pack.pts, vc->time_base, vs->time_base);
			pack.dts = av_rescale_q(pack.dts, vc->time_base, vs->time_base);
			/*��������ڲ��л������򣬻����P֡B֡����ͬʱ�����Զ��ͷ�packet�Ŀռ䣬���ܳɹ����
			�����av_write_frame
			*/
			ret = av_interleaved_write_frame(ic, &pack);
			if (ret == 0)
			{
				cout << "#" << flush;
			}
		}
	}
	catch (exception &ex)
	{
		if (cam.isOpened()) 
				cam.release();
		if (vsc)
		{
			sws_freeContext(vsc);
			vsc = NULL;
		}
		if (vc)
		{
			/*
			����ʹ��avio_close�ر�IO����Ϊ��avcodec_free_context�ֻ�ر�һ�Σ���ɶ�ιر�
			���ʹ��avio_closep�����ڲ���null��������ֱ��ic->pb=NULL;
			*/
			avio_closep(&ic->pb);
			avcodec_free_context(&vc);//�ͷű����������ģ��ڲ�ͬʱ��رձ�����
		}
		cerr << ex.what() << endl;
	}
	
	getchar();
	return 0;
}