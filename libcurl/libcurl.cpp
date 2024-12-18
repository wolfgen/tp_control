#include "libcurl.h"
#include <stdlib.h>
#include <string.h>

namespace libcurl {

	curl_http_send::curl_http_send() :buf_size(0) {
	}

	curl_http_send::~curl_http_send()
	{
	}

	size_t curl_http_send::receive_data(void* contents, size_t size, size_t nmemb, void* pcurl)
	{
		FILE* f_p;
		curl_http_send* curl_p = (curl_http_send*)pcurl;

		//将接收到数据写到data.txt中记录
		//f_p = fopen("data.txt", "a+");
		//if (!f_p) {
		//	error("data.txt open failure\n");
		//}
		//fwrite(contents, size * nmemb, 1, f_p);
		//fclose(f_p);

		//将接收到的数据写入data中
		curl_p->data.append((char*)contents, size * nmemb);

		return size * nmemb;
	}


	size_t curl_http_send::download_data(void* ptr, size_t size, size_t nmemb, void* stream)
	{
		size_t written = 0;
		
		if (stream)
			written = fwrite(ptr, size, nmemb, (FILE*)stream);

		return written;
	}

	int curl_http_send::upload_progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
	{
		/*std::cout << to_string(dltotal) << endl;
		std::cout << to_string(dlnow) << endl;
		std::cout << to_string(ultotal) << endl;
		std::cout << to_string(ulnow) << endl;
		std::cout << to_string(ulnow / ultotal) << endl;*/

		//std::cout << to_string(ultotal) << endl;
		//std::cout << to_string(ulnow) << endl;
		//std::cout << ".........................." << endl;

		if (ultotal) {
			std::cout << "uploading_progress : " << std::to_string(ulnow / ultotal * 100) << "%" << std::endl;

		}
		//printf("333|||");
		curl_http_send* curl_p = (curl_http_send*)clientp;
		if (curl_p->buf_size)
			printf("\r[%u%%]", (unsigned int)((dlnow / curl_p->buf_size) * (double)100));
		return 0;
	}

	int curl_http_send::progress_callback(void* clientp, double dltotal, double dlnow, double ultotal, double ulnow)
	{ 

		//printf("333|||");
		curl_http_send* curl_p = (curl_http_send*)clientp;
		if (curl_p->buf_size)
			printf("\r[%u%%]", (unsigned int)((dlnow / curl_p->buf_size) * (double)100));
		return 0;
	}

	size_t curl_http_send::header_callback(char* buffer, size_t size, size_t nitems, void* userdata) {
		curl_http_send* curl_p = (curl_http_send*)userdata;
		//在报头中获取数据的总长度
		char* buf = NULL;
		char* buf_right = NULL;
		buf = strtok_s(buffer, ":", &buf_right);

		//将此次读取要读的长度获取出来，用于设置进度条
		if (!strncmp("data-length", buf, sizeof("data-length"))) {
			curl_p->buf_size = strtod(buf_right, NULL);
		}

		return size * nitems;
	}

	std::string curl_http_send::HttpGet(const std::string& strUrl, const long nTimeout)
	{
		data.clear();
		CURLcode res;
		CURL* pCURL = curl_easy_init();

		if (!pCURL) {
			error("curl_easy_init error");
			curl_easy_cleanup(pCURL);
			return "";
		}

		//设置url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//设置不忽略SIGPIPE信号
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//设置超时
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//设置开启进度条
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//设置用于接收数据的回调函数
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//设置传递给接收函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//设置用于进度显示的回调函数
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, curl_http_send::progress_callback);
		//设置传递给进度显示函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, this);
		//设置用于接收报头数据的回调函数
		curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//设置传递给接收报头函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, this);

		//发送数据
		res = curl_easy_perform(pCURL);

		//如果res != CURLE_OK则发送失败
		if (res != CURLE_OK) {
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//获取接收到的数据Status,如果为200则表示接收发正常，其余则不正常
			long response_code;
			curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &response_code);

			if (response_code == 200) {
				//printf("Data status normal\n");
			}
			else {
				//printf("Data status abnormal\n");
			}
		}

		curl_easy_cleanup(pCURL);
		return data;
	}

	std::string curl_http_send::HttpUpload(const std::string& strUrl,const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout)
	{
	
		printf(file_path.c_str());
		data.clear();
		CURLcode res;
		CURL* pCURL = curl_easy_init();

		if (!pCURL) {
			error("curl_easy_init error");
			curl_easy_cleanup(pCURL);
			return "";
		}

		//设置url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//设置不忽略SIGPIPE信号
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//设置超时
		curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//设置开启进度条
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//设置用于接收数据的回调函数
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//设置传递给接收函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//设置用于进度显示的回调函数
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, progress_callback);
		//设置传递给进度显示函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, progress_callback_ctx);
		//设置用于接收报头数据的回调函数
		curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//设置传递给接收报头函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, this);



		// test_begin

		struct curl_slist* header_list = NULL;
		header_list = curl_slist_append(header_list, "Content-Type:multipart/form-data");
		curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, header_list);

		curl_mime* form = NULL;
		form = curl_mime_init(pCURL);
		curl_mimepart* file_field = NULL;
		file_field = curl_mime_addpart(form);
		curl_mime_name(file_field, "file");
		curl_mime_filedata(file_field,file_path.c_str());
		curl_easy_setopt(pCURL,CURLOPT_MIMEPOST,form);
		// test_end

		//发送数据
		res = curl_easy_perform(pCURL);

		//如果res != CURLE_OK则发送失败
		if (res != CURLE_OK) {
			std::cout << res;
		//	printf(res);
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//获取接收到的数据Status,如果为200则表示接收发正常，其余则不正常
			long response_code;
			curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &response_code);

			if (response_code == 200) {
				//printf("Data status normal\n");
			}
			else {
				//printf("Data status abnormal\n");
			}
		}

		if (header_list)
			curl_slist_free_all(header_list);

		if (form)
			curl_mime_free(form);

		curl_easy_cleanup(pCURL);

		return data;
	}

	std::string curl_http_send::HttpDownload(const std::string& strUrl, const std::string& file_path, curl_progress_callback progress_callback, void* progress_callback_ctx, const long nTimeout /*= 10*/)
	{

		printf(file_path.c_str());
		data.clear();
		CURLcode res;
		CURL* pCURL = curl_easy_init();

		FILE* fp = NULL;

		if (!pCURL) {
			error("curl_easy_init error");
			curl_easy_cleanup(pCURL);
			return "";
		}

		fp = fopen(file_path.c_str(), "wb");



		//设置url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//设置不忽略SIGPIPE信号
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//设置超时
		curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//设置开启进度条
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//设置用于接收数据的回调函数
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::download_data);
		//设置传递给接收函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, fp);
		//设置用于进度显示的回调函数
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, progress_callback);
		//设置传递给进度显示函数的自定义指针
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, progress_callback_ctx);



		//发送数据
		res = curl_easy_perform(pCURL);

		//如果res != CURLE_OK则发送失败
		if (res != CURLE_OK) {
			std::cout << res;
			//	printf(res);
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//获取接收到的数据Status,如果为200则表示接收发正常，其余则不正常
			long response_code;
			curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &response_code);

			if (response_code == 200) {
				//printf("Data status normal\n");
			}
			else {
				//printf("Data status abnormal\n");
			}
		}

		curl_easy_cleanup(pCURL);

		if(fp)
			fclose(fp);

		return data;
	}

	std::string curl_http_send::HttpPost(const std::string& strUrl, const char* send_data, const long nTimeout)
	{
		data.clear();
		CURLcode res;

		CURL* pCURL = curl_easy_init();
		struct curl_slist* headers = NULL;

		if (!pCURL) {
			error("curl_easy_init error");
			curl_easy_cleanup(pCURL);
			return "";
		}

		this->buf_size = 0;
		CURLcode ret;
		ret = curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, 10);
		ret = curl_easy_setopt(pCURL, CURLOPT_POST, 1L);
		//设置发送的数据为json数据
		headers = curl_slist_append(headers, "content-type:application/json");

		if (headers == NULL) {
			curl_slist_free_all(headers);
			error("curl_slist_append error");
			return "";
		}

		//设置要http头信息
		ret = curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headers);
		//设置不忽略SIGPIPE信号
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//设置开启进度条
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//设置要发送的数据
		ret = curl_easy_setopt(pCURL, CURLOPT_POSTFIELDS, send_data);
		//设置超时
		ret = curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//设置用于接收数据内容的回调函数
		ret = curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//设置传递给接收函数内容的自定义指针
		ret = curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//设置用于进度显示的回调函数
		ret = curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, curl_http_send::progress_callback);
		//设置传递给进度显示函数的自定义指针
		ret = curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, this);
		//设置用于接收报头数据的回调函数
		ret = curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//设置传递给接收报头函数的自定义指针
		ret = curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, this);


		res = curl_easy_perform(pCURL);
		curl_slist_free_all(headers);

		//如果res != CURLE_OK则发送失败
		if (res != CURLE_OK) {
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//获取接收到的数据Status,如果为200则表示接收发正常，其余则不正常
			long response_code;
			curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &response_code);

			char* contentType = { 0 };
			CURLcode return_code;
			return_code = curl_easy_getinfo(pCURL, CURLINFO_CONTENT_TYPE, &contentType);
			if ((CURLE_OK == return_code) && contentType)
				std::cout << "请求的文件类型:" << contentType << std::endl;

			if (response_code == 200) {
				//printf("Data status normal\n");
			}
			else {
				//printf("Data status abnormal\n");
			}
		}
		curl_easy_cleanup(pCURL);
		return data;
	}
}
