#pragma once

enum XFilterType
{
	XBILATERAL//˫���˲�
};

//������ͷ�ļ���ֱ����������
namespace cv
{
	class Mat;
}

#include <string>
#include <map>

class XFilter
{
public:
	static XFilter* Get(XFilterType t = XBILATERAL);
	virtual bool Filter(cv::Mat *src, cv::Mat* des) = 0;//�˲�������������Ҫ������д��ÿ���˲�����Ҫʹ��
	virtual bool Set(std::string key, double value);

	virtual ~XFilter();

protected:
	std::map<std::string, double> paras;//ӳ��

	XFilter();
};

