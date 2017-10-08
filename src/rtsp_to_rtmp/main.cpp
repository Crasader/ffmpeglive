extern "C"
{
#include "libavformat\avformat.h"//����ͷ�ļ� ǿ�Ʊ�����ʹ��C��ʽ����
#include "libavutil\time.h"
}

#include <iostream>
using namespace std;

/*Ԥ����ָ� ���ÿ�
  �ڶ��ַ�ʽ��������Ŀ-����-������-����-����������������༭��ȥ
*/
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avcodec.lib")

int XError(int errNum)
{
	char buf[1024] = { 0 };
	av_strerror(errNum, buf, sizeof(buf));
	cout << buf << endl;
	getchar();
	return -1;
}

static double r2d(AVRational r)
{
	return r.num == 0 || r.den == 0 ? 0. : (double)r.num / (double)r.den;
}

/*
��rtspЭ�������Դת��ΪrtmpЭ����з���
��������ͷ���͹�����ֱ�Ӿ���rtspЭ���ѹ����������
*/
int main(int argc, char *argv[])
{
	//char *inUrl = "test.flv";
	//���Ǻ�������ͷͨ��rtspЭ�������Դ  
	char *inUrl = "rtsp://test:test123456@192.168.1.64";
	char *outUrl = "rtmp://192.168.124.137/live";

	//��ʼ�����з�װ�ͽ��װ ��flv mp4 mov mp3,�������������ͽ�����
	av_register_all();

	//��ʼ�������
	avformat_network_init();

	/////////////////////////////////////////////////////////
	//1 ���ļ�,���װ
	//�����װ������
	/*
	AVFormatContext

	AVIOContext *pb;//IO������
	AVStream **streams;//��Ƶ��Ƶ��Ļ��
	int nb_streams;//��������

	AVStream
	AVRational time_base;
	AVCodecParameters *codecpar;(����Ƶ����)
	AVCodecContext *codec;
	*/
	AVFormatContext *ictx = NULL;
	
	//���������ʱ�����䷽ʽ���������ⶪ֡��������
	AVDictionary *opts = NULL;
	char key[] = "max_delay";
	char val[] = "500";//�����ʱ500ms
	av_dict_set(&opts, key, val, 0);
	char key2[] = "rtsp_transport";
	char val2[] = "tcp";
	av_dict_set(&opts, key2, val2, 0);

	//��rtsp���� ����ļ�ͷ
	int ret = avformat_open_input(&ictx, inUrl, 0, &opts);
	if (ret != 0)
	{
		return XError(ret);
	}

	cout << "open file " << inUrl << " Success." << endl;

	//��ȡ����Ƶ����Ϣ h264, flv
	ret = avformat_find_stream_info(ictx, 0);
	if (ret != 0)
	{
		return XError(ret);
	}

	//���avformat����Ϣ���ڶ�������Ϊ0��ʾ��ӡ���е�����Ϣ�����ĸ�������ʾ����
	av_dump_format(ictx, 0, inUrl, 0);

	///////////////////////////////////////////////////////////
	//���������������
	AVFormatContext *octx = NULL;
	ret = avformat_alloc_output_context2(&octx, 0, "flv", outUrl);
	if (!octx)
	{
		return XError(ret);
	}
	cout << "octx create success!" << endl;

	//���������
	//���������AVStream
	for (int i = 0; i < ictx->nb_streams; i++)
	{
		//���������
		AVStream* out = avformat_new_stream(octx, ictx->streams[i]->codec->codec);
		if (!out)
		{
			return XError(0);
		}

		//ע���� ����������Ϣ,����mp4
		//ret = avcodec_copy_context(out->codec, ictx->streams[i]->codec);
		ret = avcodec_parameters_copy(out->codecpar, ictx->streams[i]->codecpar);
		out->codec->codec_tag = 0;//���߱�����������룬ֱ��д�ļ�
	}

	//��ӡ�����ʽ����Ϣ����������ʾ����ĸ�ʽ
	av_dump_format(octx, 0, outUrl, 1);

	///////////////////////////////////////////////
	//rtmp����

	//��IO
	ret = avio_open(&octx->pb, outUrl, AVIO_FLAG_WRITE);
	if (!octx->pb)
	{
		return XError(ret);
	}
	//д��ͷ��Ϣ
	avformat_write_header(octx, 0);
	if (ret < 0)
	{
		return XError(ret);
	}
	cout << "avformat_write_header " << ret <<endl;

	//����ÿһ֡����
	AVPacket pkt;
	long long startTime = av_gettime();//��ȡ��ǰʱ���
	for (;;)
	{
		ret = av_read_frame(ictx, &pkt);
		if (ret != 0 || pkt.size <= 0)
		{
			continue;
		}
		cout << pkt.pts << " "<< flush;
		//����ת��pts dts  ��������������timebase���ܲ�һ�£������Ҫ����ת��,�������ת��Ϊ��������㣬���㷽ʽΪ���һ������
		AVRational itime = ictx->streams[pkt.stream_index]->time_base;
		AVRational otime = octx->streams[pkt.stream_index]->time_base;
		pkt.pts = av_rescale_q_rnd(pkt.pts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.dts = av_rescale_q_rnd(pkt.dts, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.duration = av_rescale_q_rnd(pkt.duration, itime, otime, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
		pkt.pos = -1;
	
		/*
		��������ԴΪrtsp���ݣ�����Ҫ������ʱ��������Ϊrtsp�������Ѿ�������ʱ�������̶�����֡�ʷ������ݹ���,
		av_read_frame���������������ȴ����ݷ���
		*/
#if 0   
		//��Ƶ֡
		if (ictx->streams[pkt.stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			AVRational tb = ictx->streams[pkt.stream_index]->time_base;
			//�Ѿ���ȥ��ʱ��
			long long now = av_gettime() - startTime;
			long long dts = 0;
			//���ڿ��������ٶȣ�ʹ�ý���ʱ����͵�ǰʱ������жԱ�
			dts = pkt.dts * (1000 * 1000 * r2d(tb));
			if (dts > now)
				av_usleep(dts - now);
		}
#endif

		//��������, ����������av_write_frame���Ը��ݻ�������Զ������ͣ����һ����Զ��ͷ�packet���ڴ�
		ret = av_interleaved_write_frame(octx, &pkt);
		if (ret < 0)
		{
			//XError(ret);
		}
		//av_packet_unref(&pkt);//�ͷ�֡�����ڴ�
	}

	cout << "file to rtmp test" << endl;
	getchar();

	return 0;
}