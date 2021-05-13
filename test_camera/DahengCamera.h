#pragma once
//自用的大恒相机接口，由张景瑜(jyzhang@sjtu.edu.cn)编写
//实现以下功能：
//获得相机序列、曝光时间、白平衡、拍摄、错误判断...

//#define __debug__
//#define __capturecallback__ //TODO 

#include "IGXFactory.h"
#include "IGXDevice.h"
#include "GalaxyException.h"

#include <fstream>

#include <string>
#include <iostream>
#include <vector>

using namespace std;


class DahengCamera{
	class CaptureEventHandler : public ICaptureEventHandler
	{
	public:
		void DoOnImageCaptured(CImageDataPointer& objImageDataPointer, void* data){
#ifdef __debug__
			cout << "开始采图" << endl;
#endif
			if (GX_FRAME_STATUS_SUCCESS == objImageDataPointer->GetStatus()){
				if (GX_PIXEL_FORMAT_BAYER_RG8 == objImageDataPointer->GetPixelFormat())
					(*static_cast<void**>(data)) = objImageDataPointer->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
				else if (GX_PIXEL_FORMAT_BAYER_RG12 == objImageDataPointer->GetPixelFormat())
					(*static_cast<void**>(data)) = objImageDataPointer->ConvertToRGB24(GX_BIT_4_11, GX_RAW2RGB_NEIGHBOUR, true);
#ifdef __debug__
				cout << "采图成功" << endl;
#endif
			}
		}
	};


public:
	DahengCamera()  {};
	~DahengCamera() {};

	//相机初始化
	bool init(){
		try{
			IGXFactory::GetInstance().Init();
			IGXFactory::GetInstance().UpdateDeviceList(1000, m_dinfovector);
			for (uint32_t i = 0; i < m_dinfovector.size(); i++)
			{
				CGXDevicePointer objDevicePtr;
				GxIAPICPP::gxstring strSN = m_dinfovector[i].GetSN();
				GxIAPICPP::gxstring strUserID = m_dinfovector[i].GetUserID();
				//GxIAPICPP::gxstringstrMAC = m_dinfovector[0].GetMAC();
				//GxIAPICPP::gxstringstrIP = m_dinfovector[0].GetIP();
				objDevicePtr = IGXFactory::GetInstance().OpenDeviceBySN(strSN, GX_ACCESS_EXCLUSIVE);
				CGXFeatureControlPointer objFeatureControlPtr = objDevicePtr->GetRemoteFeatureControl();
				if (objFeatureControlPtr->IsImplemented("TriggerMode"))
				{
					if (objFeatureControlPtr->IsWritable("TriggerMode")){//查询是否可写
						objFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("Off");//设置当前值
					}
				}
				m_dvector.push_back(objDevicePtr);

			}
			cout << "已完成相机驱动初始化，并更新所有设备信息" << endl;
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}

	}

	//相机释放资源
	bool uninit(){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				m_dvector[i]->Close();
			}
			IGXFactory::GetInstance().Uninit();
			cout << "已释放相机驱动资源" << endl;
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//获取所有设备信息
	void getDeviceInfo(){
		for (uint32_t i = 0; i < m_dinfovector.size(); i++)
		{
			cout << m_dinfovector[i].GetVendorName() << endl;
			cout << m_dinfovector[i].GetModelName() << endl;
			cout << m_dinfovector[i].GetDeviceID() << endl;
			cout << m_dinfovector[i].GetSN() << endl;
			//更多设备信息详见IGXDeviceInfo接口
		}
	}

	//拍摄，cameraIdx标识了要用第几个相机拍摄，默认为0
	bool capture(void* &data, int cameraIdx = 0){

		try{
			uint32_t nStreamNum = m_dvector[cameraIdx]->GetStreamCount();
#ifdef __debug__
			cout << "设备" << cameraIdx << "流个数:" << nStreamNum << endl;
#endif
			if (nStreamNum > 0)
			{
				CGXStreamPointer objStreamPtr = m_dvector[cameraIdx]->OpenStream(0);
#ifdef __capturecallback__
				ICaptureEventHandler* pCaptureEventHandler = new CaptureEventHandler();
				//注册采集回调函数
				objStreamPtr->RegisterCaptureCallback(pCaptureEventHandler, &data);
#endif

				//开启流通道采集
				objStreamPtr->StartGrab();

				//给设备发送开采命令
				CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
				objFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();

#ifndef __capturecallback__
				CImageDataPointer objImageDataPtr;
				objImageDataPtr = objStreamPtr->GetImage(500);//超时时间使用500ms，用户可以自行设定
				if (objImageDataPtr->GetStatus() == GX_FRAME_STATUS_SUCCESS)
				{
#ifdef __debug__
					cout << objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue() << endl;
					cout << ("BayerRG8" == objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue()) << endl;
#endif
					//采图成功而且是完整帧，可以进行图像处理，可以考虑自定义图像格式
					size_t size = objImageDataPtr->GetWidth() * objImageDataPtr->GetHeight() * 3;
					if (data){
						free(data);
						data = nullptr;
					}
					data = malloc(size);
					if (0 == ("BayerRG8" == objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue()))
						memcpy(data, objImageDataPtr->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, false), size);
					else if (0 == ("BayerRG12" == objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue()))
						memcpy(data, objImageDataPtr->ConvertToRGB24(GX_BIT_4_11, GX_RAW2RGB_NEIGHBOUR, false), size);
				}
#endif
				//停采、注销采集回调函数
				objFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
				objStreamPtr->StopGrab();
#ifdef __capturecallback__
				objStreamPtr->UnregisterCaptureCallback();
				delete pCaptureEventHandler;
				pCaptureEventHandler = NULL;
#endif
				//关闭流通道
				objStreamPtr->Close();
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//设置图片分辨率
	bool setPictureResolution(int& width, int& height, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Width"))
			{
				if (objFeatureControlPtr->IsWritable("Width"))//查询是否可写
					objFeatureControlPtr->GetIntFeature("Width")->SetValue(width);//设置当前值
				if (objFeatureControlPtr->IsWritable("Height"))//查询是否可写
					objFeatureControlPtr->GetIntFeature("Height")->SetValue(height);//设置当前值
#ifdef __debug__
				cout << "设置相机" << cameraIdx << "的分辨率为" << width << "x" << height << endl;
#endif
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	bool getPictureResolution(int& width, int& height, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Width"))
			{
				if (objFeatureControlPtr->IsWritable("Width"))//查询是否可写
					width = objFeatureControlPtr->GetIntFeature("Width")->GetValue();//设置当前值
				if (objFeatureControlPtr->IsWritable("Height"))//查询是否可写
					height = objFeatureControlPtr->GetIntFeature("Height")->GetValue();//设置当前值
#ifdef __debug__
				cout << "相机" << cameraIdx << "的分辨率为" << width << "x" << height << endl;
#endif
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//设置图像格式
	//BayerRG8， BayerRG10
	bool setPictureFormat(string mode, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("PixelFormat"))
			{
				if (objFeatureControlPtr->IsWritable("PixelFormat"))//查询是否可写
					objFeatureControlPtr->GetEnumFeature("PixelFormat")->SetValue(mode.c_str());//设置当前值
				cout << "设置相机" << cameraIdx << "的像素格式为" << mode << endl;
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//设置曝光参数
	bool setExposureTime(float t, int cameraIdx = 0){//调用时默认关闭自动曝光
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("ExposureTime"))
			{
				if (objFeatureControlPtr->IsWritable("ExposureAuto"))//查询是否可写
					objFeatureControlPtr->GetEnumFeature("ExposureAuto")->SetValue("Off");//关闭自动曝光
				if (objFeatureControlPtr->IsWritable("ExposureTime")){//查询是否可写
					objFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(t);//设置当前值
					cout << "设置相机" << cameraIdx << "的曝光时间为" << t << "us" << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}
	bool setExposureAuto(string mode, int cameraIdx = 0){//mode: Off, Continuous, Once
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("ExposureAuto"))
			{
				if (objFeatureControlPtr->IsWritable("ExposureAuto")){//查询是否可写
					objFeatureControlPtr->GetEnumFeature("ExposureAuto")->SetValue(mode.c_str());//设置当前值
					cout << "设置相机" << cameraIdx << "的自动曝光模式为" << mode << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//设置白平衡
	bool setBalanceWhiteAuto(string mode, int cameraIdx = 0){//mode: Off, Continuous, Once
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("BalanceWhiteAuto"))
			{
				if (objFeatureControlPtr->IsWritable("BalanceWhiteAuto")){//查询是否可写
					objFeatureControlPtr->GetEnumFeature("BalanceWhiteAuto")->SetValue(mode.c_str());//设置当前值
					cout << "设置相机" << cameraIdx << "的自动白平衡模式为" << mode << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}
	bool setBalanceRatio(float r, float g, float b, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("BalanceWhiteAuto"))
			{
				if (objFeatureControlPtr->IsWritable("BalanceWhiteAuto")){//查询是否可写
					objFeatureControlPtr->GetEnumFeature("BalanceWhiteAuto")->SetValue("Off");//设置当前值
				}
				if (objFeatureControlPtr->IsWritable("BalanceRatioSelector"))
					objFeatureControlPtr->GetEnumFeature("BalanceRatioSelector")->SetValue("Red");
				if (objFeatureControlPtr->IsWritable("BalanceRatio"))
					objFeatureControlPtr->GetFloatFeature("BalanceRatio")->SetValue(r);

				if (objFeatureControlPtr->IsWritable("BalanceRatioSelector"))
					objFeatureControlPtr->GetEnumFeature("BalanceRatioSelector")->SetValue("Green");
				if (objFeatureControlPtr->IsWritable("BalanceRatio"))
					objFeatureControlPtr->GetFloatFeature("BalanceRatio")->SetValue(g);

				if (objFeatureControlPtr->IsWritable("BalanceRatioSelector"))
					objFeatureControlPtr->GetEnumFeature("BalanceRatioSelector")->SetValue("Blue");
				if (objFeatureControlPtr->IsWritable("BalanceRatio"))
					objFeatureControlPtr->GetFloatFeature("BalanceRatio")->SetValue(b);

				cout << "设置相机" << cameraIdx << "的白平衡系数为" << endl
					<< "r = " << r << endl
					<< "g = " << g << endl
					<< "b = " << b << endl;
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	//设置增益
	bool setGainAuto(string mode, float min = 0, float max = 23.9, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("GainAuto"))
			{
				if (objFeatureControlPtr->IsWritable("GainAuto")){//查询是否可写
					objFeatureControlPtr->GetEnumFeature("GainAuto")->SetValue(mode.c_str());//设置当前值
					cout << "设置相机" << cameraIdx << "的自动自动模式为" << mode << endl;
				}
				if ("Continuous" == mode || "Once" == mode){
					if (objFeatureControlPtr->IsWritable("AutoGainMin"))
						objFeatureControlPtr->GetFloatFeature("AutoGainMin")->SetValue(min);
					if (objFeatureControlPtr->IsWritable("AutoGainMax"))
						objFeatureControlPtr->GetFloatFeature("AutoGainMax")->SetValue(max);
					cout << "相机自动增益最小值为" << min << "，最大值为" << max << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}
	bool setGain(float g, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Gain"))
			{
				if (objFeatureControlPtr->IsWritable("GainAuto"))//查询是否可写
					objFeatureControlPtr->GetEnumFeature("GainAuto")->SetValue("Off");//关闭自动曝光
				if (objFeatureControlPtr->IsWritable("Gain")){//查询是否可写
					objFeatureControlPtr->GetFloatFeature("Gain")->SetValue(g);//设置当前值
					cout << "设置相机" << cameraIdx << "的增益为" << g << "dB" << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}


	//导出相机配置文件
	bool exportConfigFile(string filePathToName = "camera/default"){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				CGXStreamPointer objStreamPtr = m_dvector[i]->OpenStream(0);
				m_dvector[i]->ExportConfigFile((filePathToName + "_" + to_string(i)).c_str());
				//关闭流通道
				objStreamPtr->Close();
			}
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	bool importConfigFile(string filePathToName, int cameraIdx){
		try{
			CGXStreamPointer objStreamPtr = m_dvector[cameraIdx]->OpenStream(0);
			m_dvector[cameraIdx]->ImportConfigFile(filePathToName.c_str());
			//关闭流通道
			objStreamPtr->Close();
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}

	bool importConfigFile(string filePathToName = "camera/default"){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				CGXStreamPointer objStreamPtr = m_dvector[i]->OpenStream(0);
				m_dvector[i]->ImportConfigFile((filePathToName + "_" + to_string(i)).c_str());
				//关闭流通道
				objStreamPtr->Close();
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "错误码: " << e.GetErrorCode() << endl;
			cout << "错误描述信息: " << e.what() << endl;
			return false;
		}
	}


private:
	//存储设备信息列表
	GxIAPICPP::gxdeviceinfo_vector m_dinfovector;
	vector<CGXDevicePointer> m_dvector;
	//用于判断是否拍摄完成
	//bool m_state;	
};