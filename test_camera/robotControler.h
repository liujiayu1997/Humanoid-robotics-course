#pragma once
/*!
* \file robotControler,h
* \brief 用于类人机器人课程Dobot Magician课程机器人控制
* \author Jiayu Liu
* \data 2021.5.11
* \email email liujiayu@sjtu.edu.cn
*/

#pragma comment(lib, "./include/DobotDll.lib")
#include "./include/DobotDll.h"
#include <iostream>
using namespace std;

/// <summary>
/// 用于Dobot Magician机器人运动控制，气阀控制
/// </summary>
class robotControler
{
public:
	/// <summary>
	/// \brief 用于连接机器人
	/// </summary>
	void connectRobot();

	/// <summary>
	/// \brief 用于断开机器人
	/// </summary>
	void disconnectRobot();

	/// <summary>
	/// \brief 用于机器人的PTP运动模式控制
	/// </summary>
	/// <param name="ptpMode">
	/// ptpMode = 0 JUMP模式
	/// ptpMode = 1 MoveJ模式
	/// ptpMode = 2 MoveL模式
	/// </param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="z"></param>
	void movePoint(uint8_t ptpMode, float x, float y, float z);

	/// <summary>
	/// \brief 用于获取机器人当前位姿
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="z"></param>
	void getPoint(float& x, float& y, float& z);

	/// <summary>
	/// 打开气源
	/// </summary>
	void openAirsource(bool status);

	/// <summary>
	/// 关闭气源
	/// </summary>
	void closeAirsource();

	/// <summary>
	/// 初始化机械臂
	/// </summary>
	void initRobot();
private:
	bool robot_status = false;
	bool air_source_status = false;
};

