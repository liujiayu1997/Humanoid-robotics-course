#include "DahengCamera.h"
#include <opencv2/core/mat.hpp>
#include "robotControler.h"

int main()
{
	DahengCamera m_cam;
	if (!m_cam.init()) {
		std::cout << "相机初始化失败" << std::endl;
		return 1;
	}
	//if (!m_cam.importConfigFile("替换为相机配置文件路径", 0)) {
	//	std::cout << "相机初始化失败，初始化配置文件为" << m_config.Read<std::string>("CameraConfigFile") << std::endl;
	//	return false;
	//}

	void* data = nullptr;
	int width = 0, height = 0;
	if (
		(!m_cam.capture(data)) ||
		(!m_cam.getPictureResolution(width, height))
		)
	{
		std::cout << "图片拍摄失败" << std::endl;
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