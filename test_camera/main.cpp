#include "DahengCamera.h"
#include <opencv2/core/mat.hpp>
#include "robotControler.h"

int main()
{
	DahengCamera m_cam;
	if (!m_cam.init()) {
		std::cout << "�����ʼ��ʧ��" << std::endl;
		return 1;
	}
	//if (!m_cam.importConfigFile("�滻Ϊ��������ļ�·��", 0)) {
	//	std::cout << "�����ʼ��ʧ�ܣ���ʼ�������ļ�Ϊ" << m_config.Read<std::string>("CameraConfigFile") << std::endl;
	//	return false;
	//}

	void* data = nullptr;
	int width = 0, height = 0;
	if (
		(!m_cam.capture(data)) ||
		(!m_cam.getPictureResolution(width, height))
		)
	{
		std::cout << "ͼƬ����ʧ��" << std::endl;
	}


	cv::Mat img;
	cv::Mat(height, width, CV_8UC3, data).copyTo(img);


	robotControler robot;
	robot.connectRobot();
	float x, y, z;
	robot.getPoint(x, y, z);
	robot.movePoint(1, x, y + 5, z);
	robot.openAirsource(true);
	Sleep(1000);
	robot.openAirsource(false);
	Sleep(1000);
	robot.closeAirsource();
	robot.disconnectRobot();
	return 0;
}