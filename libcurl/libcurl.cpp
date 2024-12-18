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

		//�����յ�����д��data.txt�м�¼
		//f_p = fopen("data.txt", "a+");
		//if (!f_p) {
		//	error("data.txt open failure\n");
		//}
		//fwrite(contents, size * nmemb, 1, f_p);
		//fclose(f_p);

		//�����յ�������д��data��
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
		//�ڱ�ͷ�л�ȡ���ݵ��ܳ���
		char* buf = NULL;
		char* buf_right = NULL;
		buf = strtok_s(buffer, ":", &buf_right);

		//���˴ζ�ȡҪ���ĳ��Ȼ�ȡ�������������ý�����
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

		//����url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//���ò�����SIGPIPE�ź�
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//���ó�ʱ
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//���ÿ���������
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//�������ڽ������ݵĻص�����
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//���ô��ݸ����պ������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//�������ڽ�����ʾ�Ļص�����
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, curl_http_send::progress_callback);
		//���ô��ݸ�������ʾ�������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, this);
		//�������ڽ��ձ�ͷ���ݵĻص�����
		curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//���ô��ݸ����ձ�ͷ�������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, this);

		//��������
		res = curl_easy_perform(pCURL);

		//���res != CURLE_OK����ʧ��
		if (res != CURLE_OK) {
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//��ȡ���յ�������Status,���Ϊ200���ʾ���շ�����������������
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

		//����url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//���ò�����SIGPIPE�ź�
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//���ó�ʱ
		curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//���ÿ���������
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//�������ڽ������ݵĻص�����
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//���ô��ݸ����պ������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//�������ڽ�����ʾ�Ļص�����
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, progress_callback);
		//���ô��ݸ�������ʾ�������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, progress_callback_ctx);
		//�������ڽ��ձ�ͷ���ݵĻص�����
		curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//���ô��ݸ����ձ�ͷ�������Զ���ָ��
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

		//��������
		res = curl_easy_perform(pCURL);

		//���res != CURLE_OK����ʧ��
		if (res != CURLE_OK) {
			std::cout << res;
		//	printf(res);
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//��ȡ���յ�������Status,���Ϊ200���ʾ���շ�����������������
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



		//����url
		curl_easy_setopt(pCURL, CURLOPT_URL, strUrl.c_str());
		//���ò�����SIGPIPE�ź�
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//���ó�ʱ
		curl_easy_setopt(pCURL, CURLOPT_CONNECTTIMEOUT, 5);
		curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//���ÿ���������
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//�������ڽ������ݵĻص�����
		curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::download_data);
		//���ô��ݸ����պ������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, fp);
		//�������ڽ�����ʾ�Ļص�����
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, progress_callback);
		//���ô��ݸ�������ʾ�������Զ���ָ��
		curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, progress_callback_ctx);



		//��������
		res = curl_easy_perform(pCURL);

		//���res != CURLE_OK����ʧ��
		if (res != CURLE_OK) {
			std::cout << res;
			//	printf(res);
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//��ȡ���յ�������Status,���Ϊ200���ʾ���շ�����������������
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
		//���÷��͵�����Ϊjson����
		headers = curl_slist_append(headers, "content-type:application/json");

		if (headers == NULL) {
			curl_slist_free_all(headers);
			error("curl_slist_append error");
			return "";
		}

		//����Ҫhttpͷ��Ϣ
		ret = curl_easy_setopt(pCURL, CURLOPT_HTTPHEADER, headers);
		//���ò�����SIGPIPE�ź�
		curl_easy_setopt(pCURL, CURLOPT_NOSIGNAL, 1L);
		//���ÿ���������
		curl_easy_setopt(pCURL, CURLOPT_NOPROGRESS, 0L);
		//����Ҫ���͵�����
		ret = curl_easy_setopt(pCURL, CURLOPT_POSTFIELDS, send_data);
		//���ó�ʱ
		ret = curl_easy_setopt(pCURL, CURLOPT_TIMEOUT, nTimeout);
		//�������ڽ����������ݵĻص�����
		ret = curl_easy_setopt(pCURL, CURLOPT_WRITEFUNCTION, curl_http_send::receive_data);
		//���ô��ݸ����պ������ݵ��Զ���ָ��
		ret = curl_easy_setopt(pCURL, CURLOPT_WRITEDATA, this);
		//�������ڽ�����ʾ�Ļص�����
		ret = curl_easy_setopt(pCURL, CURLOPT_PROGRESSFUNCTION, curl_http_send::progress_callback);
		//���ô��ݸ�������ʾ�������Զ���ָ��
		ret = curl_easy_setopt(pCURL, CURLOPT_PROGRESSDATA, this);
		//�������ڽ��ձ�ͷ���ݵĻص�����
		ret = curl_easy_setopt(pCURL, CURLOPT_HEADERFUNCTION, curl_http_send::header_callback);
		//���ô��ݸ����ձ�ͷ�������Զ���ָ��
		ret = curl_easy_setopt(pCURL, CURLOPT_HEADERDATA, this);


		res = curl_easy_perform(pCURL);
		curl_slist_free_all(headers);

		//���res != CURLE_OK����ʧ��
		if (res != CURLE_OK) {
			error("curl_easy_perform error");
			data.clear();
		}
		else {
			//��ȡ���յ�������Status,���Ϊ200���ʾ���շ�����������������
			long response_code;
			curl_easy_getinfo(pCURL, CURLINFO_RESPONSE_CODE, &response_code);

			char* contentType = { 0 };
			CURLcode return_code;
			return_code = curl_easy_getinfo(pCURL, CURLINFO_CONTENT_TYPE, &contentType);
			if ((CURLE_OK == return_code) && contentType)
				std::cout << "������ļ�����:" << contentType << std::endl;

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
