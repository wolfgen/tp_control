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

		// desc: 每次接收数据内容的回调函数(当数据长度长的时候，未必能一次接收成功)
		// param: contents/数据内容 size/每个元素的大小 nmemb/元素的个数 pcurl/为用户自定义指针 
		// return: 接收数据的总数
		static size_t receive_data(void* contents, size_t size, size_t nmemb, void* pcurl);

		static size_t download_data(void* contents, size_t size, size_t nmemb, void* pcurl);

		// desc: 上传下载进度的回调函数
		// param: clientp/为用户自定义指针 dltotal/为需要下载的总数 dlnow/当前下载的总数 ultotal/为需要上传的总数 ulnow/当前上传的总数
		// return: 返回0，此函数返回非0值将会中断传输

		//上传专用的回调
		static int upload_progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);
		
		static int progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow);

		// desc: 接收报头数据的回调函数
		// param: buffer/数据内容 size/每个元素的大小 nitems/元素的个数 userdata/为用户自定义指针 
		// return: 接收报头数据的总数
		static size_t header_callback(char* buffer, size_t size, size_t nitems, void* userdata);

		// desc: httpget请求
		// param: strUrl/申请数据的地址 nTimeout/超时时间
		// return: 返回数据
		std::string HttpGet(const std::string& strUrl, const long nTimeout = 10);
		std::string HttpUpload(const std::string& strUrl, const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout = 10);
		std::string HttpDownload(const std::string& strUrl, const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout = 10);

		// desc: httppost请求
		// param: strUrl/申请数据的地址 send_data/发送的数据 nTimeout/超时时间
		// return: 返回数据
		std::string HttpPost(const std::string& strUrl, const char* send_data, const long nTimeout = 10);


	private:
		curl_http_send(curl_http_send&&);
		curl_http_send(const curl_http_send&);
		curl_http_send& operator=(curl_http_send&&);
		curl_http_send& operator=(const curl_http_send&);

	private:
		volatile double buf_size;				//接收到数据的总大小
		std::string data;							//接收到数据
	};
}
