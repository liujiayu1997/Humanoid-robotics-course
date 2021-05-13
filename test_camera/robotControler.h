#pragma once
/*!
* \file robotControler,h
* \brief �������˻����˿γ�Dobot Magician�γ̻����˿���
* \author Jiayu Liu
* \data 2021.5.11
* \email email liujiayu@sjtu.edu.cn
*/

#pragma comment(lib, "./include/DobotDll.lib")
#include "./include/DobotDll.h"
#include <iostream>
using namespace std;

/// <summary>
/// ����Dobot Magician�������˶����ƣ���������
/// </summary>
class robotControler
{
public:
	/// <summary>
	/// \brief �������ӻ�����
	/// </summary>
	void connectRobot();

	/// <summary>
	/// \brief ���ڶϿ�������
	/// </summary>
	void disconnectRobot();

	/// <summary>
	/// \brief ���ڻ����˵�PTP�˶�ģʽ����
	/// </summary>
	/// <param name="ptpMode">
	/// ptpMode = 0 JUMPģʽ
	/// ptpMode = 1 MoveJģʽ
	/// ptpMode = 2 MoveLģʽ
	/// </param>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="z"></param>
	void movePoint(uint8_t ptpMode, float x, float y, float z);

	/// <summary>
	/// \brief ���ڻ�ȡ�����˵�ǰλ��
	/// </summary>
	/// <param name="x"></param>
	/// <param name="y"></param>
	/// <param name="z"></param>
	void getPoint(float& x, float& y, float& z);

	/// <summary>
	/// ����Դ
	/// </summary>
	void openAirsource(bool status);

	/// <summary>
	/// �ر���Դ
	/// </summary>
	void closeAirsource();

	/// <summary>
	/// ��ʼ����е��
	/// </summary>
	void initRobot();
private:
	bool robot_status = false;
	bool air_source_status = false;
};

