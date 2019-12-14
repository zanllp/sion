#include <iostream>
#include <fstream>
// #define SION_DISABLE_SSL
#include "Sion.h"
#include <ppltasks.h>
using namespace Sion;
using namespace std;

void RestTask()
{

	
}

void DownloadTask()
{
	concurrency::create_task([] { cout << "下载任务开始" << endl; })
		.then([]
			{
				auto resp = Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
				ofstream file(R"(chunkedimage.jpeg)", ios::binary);
				file.write(resp.SourceChar.data(), resp.SourceChar.size() * sizeof(char));
			})
		.then([] { cout << "滑稽.gif下载完成" << endl; })
				.then([]
					{
						auto resp = Fetch("https://static.zanllp.cn/94da3a6b32e0ddcad844aca6a8876da2ecba8cb3c7094c3ad10996b28311e4b50ab455ee3d6d55fb50dc4e3c62224f4a20a4ddb1.gif");
						ofstream file(R"(滑稽.gif)", ios::binary);
						file.write(resp.SourceChar.data(), resp.SourceChar.size() * sizeof(char));
					})
				.then([] { cout << "分块文件下载完成" << endl; });
}


int main()
{
	cout<<Request()
		.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
		.SetHttpMethod(Method::Post)
		.SetBody(R"({"data": 233333,"msg":"hello world!"})")
		.SetHeader("Content-Type", "application/json; charset=utf-8")
		.Send().Source;
	DownloadTask();
	//RestTask();
	system("pause");

}