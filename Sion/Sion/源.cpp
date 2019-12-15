#include <iostream>
#include <fstream>
#include <functional>
//#define SION_DISABLE_SSL
#include "Sion.h"
#include <ppltasks.h>
#pragma warning(disable : 26444)
using namespace Sion;
using namespace std;
using namespace concurrency;


void ErrorBoundaries(task<void> task)
{
	try
	{
		task.wait();
	}
	catch (const std::exception & e)
	{
		cout << "ERR!:  " << e.what() << endl;
	}
}

task<void> RestTask()
{
	// ʹ��pplʵ���첽������sion��Ϊ����io
	return create_task([] {cout << "-- REST����ʼ" << endl; })
		.then(([] { return Fetch("https://api.zanllp.cn/plugin"); }))
		.then([](Response resp) {cout << "-- ��Ӧͷ Content-Type: " << resp.HeaderValue("content-type") << endl; })
		.then(([]
			{
				return Request()
					.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
					.SetHttpMethod(Method::Post)
					.SetBody(R"({"data": 233333,"msg":"hello world!"})")
					.SetHeader("Content-Type", "application/json; charset=utf-8")
					.Send();
			}))
		.then([](Response resp) { cout << "-- ��Ӧ��: " << resp.ResponseBody << endl; })
		.then(ErrorBoundaries); // ��������ʱ���ܲ������쳣
}

task<void> DownloadTask()
{
	auto DownloadChunkedFile = []
	{
		auto resp = Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
		ofstream file(R"(�ֿ�.jpeg)", ios::binary);
		file.write(resp.SourceChar.data(), resp.SourceChar.size() * sizeof(char));
	};
	auto DownloadHuaji = []
	{
		auto resp = Fetch("https://static.zanllp.cn/94da3a6b32e0ddcad844aca6a8876da2ecba8cb3c7094c3ad10996b28311e4b50ab455ee3d6d55fb50dc4e3c62224f4a20a4ddb1.gif");
		ofstream file(R"(����.gif)", ios::binary);
		file.write(resp.SourceChar.data(), resp.SourceChar.size() * sizeof(char));
	};
	return create_task([] { cout << "��������ʼ" << endl; })
		.then(DownloadChunkedFile)
		.then([] { cout << "�ֿ��ļ��������" << endl; })
		.then(DownloadHuaji)
		.then([] { cout << "����.gif�������" << endl; })
		.then(ErrorBoundaries);
}


int main()
{
	vector<task<void>> tasks{ DownloadTask(),RestTask() };
	when_all(tasks.begin(), tasks.end()).wait();
}