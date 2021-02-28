#include <iostream>
#include <fstream>
// #define SION_DISABLE_SSL
#include "sion.h"
#include <ppltasks.h>
#pragma warning(disable : 26444)
using namespace sion;
using namespace std;
using namespace concurrency;

void ErrorBoundaries(task<void> task)
{
    try
    {
        task.wait();
    }
    catch (const std::exception &e)
    {
        cout << "ERR!:  " << e.what() << endl;
    }
}

task<void> RestTask()
{
    // 使用ppl实现异步操作，sion内为阻塞io
    return create_task([] { cout << "-- REST任务开始" << endl; })
        .then(([] { return Fetch("https://api.zanllp.cn/plugin"); }))
        .then([](Response resp) { cout << "-- 响应头 Content-Type: " << resp.HeaderValue("content-type") << endl; })
        .then(([] {
            return Request()
                .SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
                .SetHttpMethod(Method::Post)
                .SetBody(R"({"data": 233333,"msg":"hello world!"})")
                .SetHeader("Content-Type", "application/json; charset=utf-8")
                .Send();
        }))
        .then([](Response resp) { cout << "-- 响应体: " << resp.BodyStr << endl; })
        .then([] { return Fetch("https://www.themepark.com.cn/"); })
        .then([](Response resp) {
            ofstream file(R"(chunked.html)", ios::binary);
            file.write(resp.BodyStr.data(), resp.ContentLength * sizeof(char));
        })
        .then(ErrorBoundaries); // 捕获运行时可能产生的异常
}

task<void> DownloadTask()
{
    auto DownloadChunkedFile = [] {
        auto resp = Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
        ofstream file(R"(分块.jpeg)", ios::binary);
        file.write(resp.BodyCharVec.data(), resp.BodyCharVec.size() * sizeof(char));
    };
    auto DownloadHuaji = [] {
        auto resp = Fetch("https://static.zanllp.cn/94da3a6b32e0ddcad844aca6a8876da2ecba8cb3c7094c3ad10996b28311e4b50ab455ee3d6d55fb50dc4e3c62224f4a20a4ddb1.gif");
        ofstream file(R"(滑稽.gif)", ios::binary);
        file.write(resp.BodyCharVec.data(), resp.BodyCharVec.size() * sizeof(char));
    };
    return create_task([] { cout << "下载任务开始" << endl; })
        .then(DownloadChunkedFile)
        .then([] { cout << "分块文件下载完成" << endl; })
        .then(DownloadHuaji)
        .then([] { cout << "滑稽.gif下载完成" << endl; })
        .then(ErrorBoundaries);
}

int main()
{
    vector<task<void>> tasks{DownloadTask(), RestTask()};
    when_all(tasks.begin(), tasks.end()).wait();
}