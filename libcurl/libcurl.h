#pragma once
//https://blog.csdn.net/xsy29000/article/details/103181267
#define CURL_STATICLIB

#include "curl/curl.h"
#include <time.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <string>


#define error(args) do{ \
						printf("[%s] - %s:%s:%d-----%s, %s\n" , "error", __FILE__, __FUNCTION__,  __LINE__, strerror(errno), args); \
} while(0)



namespace libcurl {

	class curl_http_send
	{
	public:
		curl_http_send();
		~curl_http_send();

		// desc: ÿ�ν����������ݵĻص�����(�����ݳ��ȳ���ʱ��δ����һ�ν��ճɹ�)
		// param: contents/�������� size/ÿ��Ԫ�صĴ�С nmemb/Ԫ�صĸ��� pcurl/Ϊ�û��Զ���ָ�� 
		// return: �������ݵ�����
		static size_t receive_data(void* contents, size_t size, size_t nmemb, void* pcurl);

		static size_t download_data(void* contents, size_t size, size_t nmemb, void* pcurl);

		// desc: �ϴ����ؽ��ȵĻص�����
		// param: clientp/Ϊ�û��Զ���ָ�� dltotal/Ϊ��Ҫ���ص����� dlnow/��ǰ���ص����� ultotal/Ϊ��Ҫ�ϴ������� ulnow/��ǰ�ϴ�������
		// return: ����0���˺������ط�0ֵ�����жϴ���

		//�ϴ�ר�õĻص�
		static int upload_progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
		
		static int progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

		// desc: ���ձ�ͷ���ݵĻص�����
		// param: buffer/�������� size/ÿ��Ԫ�صĴ�С nitems/Ԫ�صĸ��� userdata/Ϊ�û��Զ���ָ�� 
		// return: ���ձ�ͷ���ݵ�����
		static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

		// desc: httpget����
		// param: strUrl/�������ݵĵ�ַ nTimeout/��ʱʱ��
		// return: ��������
		std::string HttpGet(const std::string& strUrl, const long nTimeout = 10);
		std::string HttpUpload(const std::string& strUrl, const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout = 10);
		std::string HttpDownload(const std::string& strUrl, const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout = 10);

		// desc: httppost����
		// param: strUrl/�������ݵĵ�ַ send_data/���͵����� nTimeout/��ʱʱ��
		// return: ��������
		std::string HttpPost(const std::string& strUrl, const char* send_data, const long nTimeout = 10);


	private:
		curl_http_send(curl_http_send&&);
		curl_http_send(const curl_http_send&);
		curl_http_send& operator=(curl_http_send&&);
		curl_http_send& operator=(const curl_http_send&);

	private:
		volatile double buf_size;				//���յ����ݵ��ܴ�С
		std::string data;							//���յ�����
	};
}
