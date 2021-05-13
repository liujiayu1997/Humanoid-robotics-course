#pragma once
//���õĴ������ӿڣ����ž��(jyzhang@sjtu.edu.cn)��д
//ʵ�����¹��ܣ�
//���������С��ع�ʱ�䡢��ƽ�⡢���㡢�����ж�...

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
			cout << "��ʼ��ͼ" << endl;
#endif
			if (GX_FRAME_STATUS_SUCCESS == objImageDataPointer->GetStatus()){
				if (GX_PIXEL_FORMAT_BAYER_RG8 == objImageDataPointer->GetPixelFormat())
					(*static_cast<void**>(data)) = objImageDataPointer->ConvertToRGB24(GX_BIT_0_7, GX_RAW2RGB_NEIGHBOUR, true);
				else if (GX_PIXEL_FORMAT_BAYER_RG12 == objImageDataPointer->GetPixelFormat())
					(*static_cast<void**>(data)) = objImageDataPointer->ConvertToRGB24(GX_BIT_4_11, GX_RAW2RGB_NEIGHBOUR, true);
#ifdef __debug__
				cout << "��ͼ�ɹ�" << endl;
#endif
			}
		}
	};


public:
	DahengCamera()  {};
	~DahengCamera() {};

	//�����ʼ��
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
					if (objFeatureControlPtr->IsWritable("TriggerMode")){//��ѯ�Ƿ��д
						objFeatureControlPtr->GetEnumFeature("TriggerMode")->SetValue("Off");//���õ�ǰֵ
					}
				}
				m_dvector.push_back(objDevicePtr);

			}
			cout << "��������������ʼ���������������豸��Ϣ" << endl;
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}

	}

	//����ͷ���Դ
	bool uninit(){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				m_dvector[i]->Close();
			}
			IGXFactory::GetInstance().Uninit();
			cout << "���ͷ����������Դ" << endl;
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//��ȡ�����豸��Ϣ
	void getDeviceInfo(){
		for (uint32_t i = 0; i < m_dinfovector.size(); i++)
		{
			cout << m_dinfovector[i].GetVendorName() << endl;
			cout << m_dinfovector[i].GetModelName() << endl;
			cout << m_dinfovector[i].GetDeviceID() << endl;
			cout << m_dinfovector[i].GetSN() << endl;
			//�����豸��Ϣ���IGXDeviceInfo�ӿ�
		}
	}

	//���㣬cameraIdx��ʶ��Ҫ�õڼ���������㣬Ĭ��Ϊ0
	bool capture(void* &data, int cameraIdx = 0){

		try{
			uint32_t nStreamNum = m_dvector[cameraIdx]->GetStreamCount();
#ifdef __debug__
			cout << "�豸" << cameraIdx << "������:" << nStreamNum << endl;
#endif
			if (nStreamNum > 0)
			{
				CGXStreamPointer objStreamPtr = m_dvector[cameraIdx]->OpenStream(0);
#ifdef __capturecallback__
				ICaptureEventHandler* pCaptureEventHandler = new CaptureEventHandler();
				//ע��ɼ��ص�����
				objStreamPtr->RegisterCaptureCallback(pCaptureEventHandler, &data);
#endif

				//������ͨ���ɼ�
				objStreamPtr->StartGrab();

				//���豸���Ϳ�������
				CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
				objFeatureControlPtr->GetCommandFeature("AcquisitionStart")->Execute();

#ifndef __capturecallback__
				CImageDataPointer objImageDataPtr;
				objImageDataPtr = objStreamPtr->GetImage(500);//��ʱʱ��ʹ��500ms���û����������趨
				if (objImageDataPtr->GetStatus() == GX_FRAME_STATUS_SUCCESS)
				{
#ifdef __debug__
					cout << objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue() << endl;
					cout << ("BayerRG8" == objFeatureControlPtr->GetEnumFeature("PixelFormat")->GetValue()) << endl;
#endif
					//��ͼ�ɹ�����������֡�����Խ���ͼ�������Կ����Զ���ͼ���ʽ
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
				//ͣ�ɡ�ע���ɼ��ص�����
				objFeatureControlPtr->GetCommandFeature("AcquisitionStop")->Execute();
				objStreamPtr->StopGrab();
#ifdef __capturecallback__
				objStreamPtr->UnregisterCaptureCallback();
				delete pCaptureEventHandler;
				pCaptureEventHandler = NULL;
#endif
				//�ر���ͨ��
				objStreamPtr->Close();
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//����ͼƬ�ֱ���
	bool setPictureResolution(int& width, int& height, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Width"))
			{
				if (objFeatureControlPtr->IsWritable("Width"))//��ѯ�Ƿ��д
					objFeatureControlPtr->GetIntFeature("Width")->SetValue(width);//���õ�ǰֵ
				if (objFeatureControlPtr->IsWritable("Height"))//��ѯ�Ƿ��д
					objFeatureControlPtr->GetIntFeature("Height")->SetValue(height);//���õ�ǰֵ
#ifdef __debug__
				cout << "�������" << cameraIdx << "�ķֱ���Ϊ" << width << "x" << height << endl;
#endif
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	bool getPictureResolution(int& width, int& height, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Width"))
			{
				if (objFeatureControlPtr->IsWritable("Width"))//��ѯ�Ƿ��д
					width = objFeatureControlPtr->GetIntFeature("Width")->GetValue();//���õ�ǰֵ
				if (objFeatureControlPtr->IsWritable("Height"))//��ѯ�Ƿ��д
					height = objFeatureControlPtr->GetIntFeature("Height")->GetValue();//���õ�ǰֵ
#ifdef __debug__
				cout << "���" << cameraIdx << "�ķֱ���Ϊ" << width << "x" << height << endl;
#endif
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//����ͼ���ʽ
	//BayerRG8�� BayerRG10
	bool setPictureFormat(string mode, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("PixelFormat"))
			{
				if (objFeatureControlPtr->IsWritable("PixelFormat"))//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("PixelFormat")->SetValue(mode.c_str());//���õ�ǰֵ
				cout << "�������" << cameraIdx << "�����ظ�ʽΪ" << mode << endl;
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//�����ع����
	bool setExposureTime(float t, int cameraIdx = 0){//����ʱĬ�Ϲر��Զ��ع�
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("ExposureTime"))
			{
				if (objFeatureControlPtr->IsWritable("ExposureAuto"))//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("ExposureAuto")->SetValue("Off");//�ر��Զ��ع�
				if (objFeatureControlPtr->IsWritable("ExposureTime")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetFloatFeature("ExposureTime")->SetValue(t);//���õ�ǰֵ
					cout << "�������" << cameraIdx << "���ع�ʱ��Ϊ" << t << "us" << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}
	bool setExposureAuto(string mode, int cameraIdx = 0){//mode: Off, Continuous, Once
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("ExposureAuto"))
			{
				if (objFeatureControlPtr->IsWritable("ExposureAuto")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("ExposureAuto")->SetValue(mode.c_str());//���õ�ǰֵ
					cout << "�������" << cameraIdx << "���Զ��ع�ģʽΪ" << mode << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//���ð�ƽ��
	bool setBalanceWhiteAuto(string mode, int cameraIdx = 0){//mode: Off, Continuous, Once
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("BalanceWhiteAuto"))
			{
				if (objFeatureControlPtr->IsWritable("BalanceWhiteAuto")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("BalanceWhiteAuto")->SetValue(mode.c_str());//���õ�ǰֵ
					cout << "�������" << cameraIdx << "���Զ���ƽ��ģʽΪ" << mode << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}
	bool setBalanceRatio(float r, float g, float b, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("BalanceWhiteAuto"))
			{
				if (objFeatureControlPtr->IsWritable("BalanceWhiteAuto")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("BalanceWhiteAuto")->SetValue("Off");//���õ�ǰֵ
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

				cout << "�������" << cameraIdx << "�İ�ƽ��ϵ��Ϊ" << endl
					<< "r = " << r << endl
					<< "g = " << g << endl
					<< "b = " << b << endl;
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	//��������
	bool setGainAuto(string mode, float min = 0, float max = 23.9, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("GainAuto"))
			{
				if (objFeatureControlPtr->IsWritable("GainAuto")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("GainAuto")->SetValue(mode.c_str());//���õ�ǰֵ
					cout << "�������" << cameraIdx << "���Զ��Զ�ģʽΪ" << mode << endl;
				}
				if ("Continuous" == mode || "Once" == mode){
					if (objFeatureControlPtr->IsWritable("AutoGainMin"))
						objFeatureControlPtr->GetFloatFeature("AutoGainMin")->SetValue(min);
					if (objFeatureControlPtr->IsWritable("AutoGainMax"))
						objFeatureControlPtr->GetFloatFeature("AutoGainMax")->SetValue(max);
					cout << "����Զ�������СֵΪ" << min << "�����ֵΪ" << max << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}
	bool setGain(float g, int cameraIdx = 0){
		try{
			CGXFeatureControlPointer objFeatureControlPtr = m_dvector[cameraIdx]->GetRemoteFeatureControl();
			if (objFeatureControlPtr->IsImplemented("Gain"))
			{
				if (objFeatureControlPtr->IsWritable("GainAuto"))//��ѯ�Ƿ��д
					objFeatureControlPtr->GetEnumFeature("GainAuto")->SetValue("Off");//�ر��Զ��ع�
				if (objFeatureControlPtr->IsWritable("Gain")){//��ѯ�Ƿ��д
					objFeatureControlPtr->GetFloatFeature("Gain")->SetValue(g);//���õ�ǰֵ
					cout << "�������" << cameraIdx << "������Ϊ" << g << "dB" << endl;
				}
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}


	//������������ļ�
	bool exportConfigFile(string filePathToName = "camera/default"){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				CGXStreamPointer objStreamPtr = m_dvector[i]->OpenStream(0);
				m_dvector[i]->ExportConfigFile((filePathToName + "_" + to_string(i)).c_str());
				//�ر���ͨ��
				objStreamPtr->Close();
			}
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	bool importConfigFile(string filePathToName, int cameraIdx){
		try{
			CGXStreamPointer objStreamPtr = m_dvector[cameraIdx]->OpenStream(0);
			m_dvector[cameraIdx]->ImportConfigFile(filePathToName.c_str());
			//�ر���ͨ��
			objStreamPtr->Close();
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}

	bool importConfigFile(string filePathToName = "camera/default"){
		try{
			for (uint32_t i = 0; i < m_dvector.size(); i++)
			{
				CGXStreamPointer objStreamPtr = m_dvector[i]->OpenStream(0);
				m_dvector[i]->ImportConfigFile((filePathToName + "_" + to_string(i)).c_str());
				//�ر���ͨ��
				objStreamPtr->Close();
			}
			return true;
		}
		catch (CGalaxyException& e){
			cout << "������: " << e.GetErrorCode() << endl;
			cout << "����������Ϣ: " << e.what() << endl;
			return false;
		}
	}


private:
	//�洢�豸��Ϣ�б�
	GxIAPICPP::gxdeviceinfo_vector m_dinfovector;
	vector<CGXDevicePointer> m_dvector;
	//�����ж��Ƿ��������
	//bool m_state;	
};