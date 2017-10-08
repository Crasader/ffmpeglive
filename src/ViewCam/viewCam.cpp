#include <opencv2/highgui.hpp>
#include <string>
#include <iostream>

using namespace cv;
using namespace std;

#pragma comment(lib, "opencv_world320d.lib")

/*
opencv����rtsp��������ͷ�Ͳ���ϵͳ����ͷ
*/
int main()
{
	VideoCapture cam;
	string url = "rtsp://test:test123456@192.168.1.64";//���������
	namedWindow("video");

	//if (cam.open(url))
	if (cam.open(0))//�������Ϊ0�������ڲ�ʶ������������Դ�usb�������������ͷ
	{
		cout << "open cam success!" << endl;
	}
	else
	{
		cout << "open cam failed!" << endl;
		waitKey(0);
		return -1;
	}

	Mat frame;
	for (;;)
	{
		cam.read(frame);//read�����Ѿ������˽��벢ͼ��ת��
		imshow("video", frame);//��ʾͼ��
		waitKey(1);
	}
	
	getchar();

	return 0;
}