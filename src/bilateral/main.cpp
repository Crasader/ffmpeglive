#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#pragma comment(lib, "opencv_world320d.lib")
using namespace std;
using namespace cv;

/*
opencv˫���˲��㷨����
*/
int main(int argc, char argv[])
{
	Mat src = imread("001.jpg");//��ȡ����ͼ������
	if (!src.data)
	{
		cout << "open file failed!" << endl;
		getchar();
		return -1;
	}

	namedWindow("src");
	moveWindow("src", 100, 100);
	imshow("src", src);//��ʾһ��ͼ������

	Mat image;
	int d = 3;

	namedWindow("image");
	moveWindow("image", 600, 100);

	for (;;)
	{
		long long b = getTickCount();

		/*
		�˲�����ͼ����ͼ�����Ӿ�������������㡣��Bilateral Filter����ʮ�־����һ���˲�����
		����Ҫ����������һ��ͻ�����ص㣬���Ƕ�ͼ�����ƽ��ʱ���ܽ��б�Ե������
		��Bilateral Flter�����������Ҫ����Ϊ����ƽ���˲�ʱͬʱ���������ؼ�ļ��ξ����ɫ�ʾ��롣
		��ͼ������˲�����һ����Ȩƽ����������̣��˲���ͼ���е�ÿ�����ص㶼������ԭͼ���иõ������ڶ�����ص�ֵ�ļ�Ȩƽ����
		��ͬ���˲�����������Ĳ������Ȩֵ��ͬ��
		
		˫���˲���˫�ߵ���˼��ͬʱ���������ߣ����أ����������߷ֱ��ǿռ����ֵ������Ŀռ�����ָ���ǿռ�λ�ù�ϵ��
		���ݾ������λ�õľ����Զ�����費ͬ�ļ�Ȩֵ��ԭ��͸�˹�˲�һ������ֵ����ָ��������Χ�ڵ����ز�ֵ������˲���ϵ����
		�����ڦ�-��β��ֵ�˲�����ȥ���ٷ���Ϊ������Сֵ�����֮��ʣ�����صľ�ֵ��Ϊ�˲�������
		˫���˲��ǽ��ͼ��Ŀռ��ڽ��Ⱥ�����ֵ���ƶȵ�һ�����д���ͬʱ���ǿ�����Ϣ�ͻҶ������ԣ��ﵽ����ȥ���Ŀ�ģ����м򵥡��ǵ������ֲ����ص㡣
		˫���˲���ͼ�������������Ź㷺��Ӧ�ã�����ȥ�롢ȥ�����ˡ��������Ƶȵȡ�

		˫���˲�����  ������Ե��ƽ���˲���  
		��һ������ΪԴͼ�񣬵ڶ�������ΪĿ��ͼ��
		����sigmaColorԽ�󣬾ͱ�����������������Խ������ɫ�ᱻ��ϵ�һ�𣬲����ϴ�������ɫ����sigmaSpaceԽ��
		�����ԽԶ�����ػ��kernel���ĵ����ز���Ӱ�죬�Ӷ�ʹ������������㹻���Ƶ���ɫ��ȡ��ͬ����ɫ
		*/
		bilateralFilter(src, image, d, d*2, d/2);

		/*
		GetTickcount�����������شӲ���ϵͳ��������ǰ�����ļ�ʱ������
		getTickFrequency����������ÿ��ļ�ʱ������
		*/
		double sec = (double)(getTickCount() - b)/(double)getTickFrequency();
		cout << "d = "<< d <<"sec is " << sec << endl;

		imshow("image", image);//��ʾһ��ͼ������

		int k = waitKey(0);//���޵ȴ���������
		if (k == 'd')
		{
			d += 2;
		}
		else if (k == 'f')
		{
			d -= 2;
		}
		else
		{
			break;
		}
	}
	
	waitKey(0);

	return 0;
}