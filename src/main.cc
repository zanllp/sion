
#include "assert.h"
#include "sion.h"
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>

sion::Async async_thread_pool;

const sion::String url = "https://github.com/zanllp/sion";

void PrintFuncCalled(const char* str) { std::cout << "\033[32m------" << str << "-------\033[0m " << std::endl; }

void FetchHeader()
{

    PrintFuncCalled("FetchHeader");
    auto resp = sion::Fetch(url);
    std::cout << resp.StrBody().Trim().substr(0, 100) << "..." << std::endl << "cache-control: " << resp.GetHeader().Get("cache-control");
    for (auto&& i : resp.GetHeader().Data())
    {
        std::cout << "k:" << i.first << "\tv:" << i.second.substr(0, 20) << std::endl;
    }
    assert(resp.Code() == "200");
}

void FetchChunkedHtml()
{
    PrintFuncCalled("FetchChunkedHtml");
    auto resp = sion::Fetch(url);
    std::cout << resp.StrBody().Trim().substr(0, 100) << "..." << std::endl;
    assert(resp.Code() == "200");
    {
        auto resp = sion::Fetch(url, sion::Method::Delete);
        assert(resp.Code() != "200");
    }
}

void DownloadChunkedFile()
{
    PrintFuncCalled("DownloadChunkedFile");
    auto resp = sion::Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
    std::ofstream file("chunked.jpeg", std::ios::binary);
    file.write(resp.Body().data(), resp.Body().size());
    assert(resp.Code() == "200" && resp.Body().size() != 0);
}

void PostRequest()
{
    PrintFuncCalled("PostRequest");
    auto resp = sion::Request()
                    .SetBody(R"({ "hello": "world" })")
                    .SetHeader("Content-type", "application/json")
                    .SetUrl("http://www.httpbin.org/post")
                    .SetHttpMethod("POST")
                    .Send();
    std::cout << resp.StrBody() << std::endl;
    assert(resp.Code() == "200");
}

void AsyncGetAvailableResponse()
{
    PrintFuncCalled("AsyncGetAvailableResponse");
    const int num = 10;
    for (size_t i = 0; i < num; i++)
    {
        async_thread_pool.Run([=] { return sion::Request().SetUrl(url).SetHttpMethod(sion::Method::Get); });
    }
    int i = 0;
    while (i <= num)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto async_resps = async_thread_pool.GetAvailableResponse();
        if (async_resps.size())
        {
            i += async_resps.size();
            std::cout << "AsyncGetAvailableResponse got resp size:" << async_resps.size() << std::endl;
        }
    }
}

void AsyncAwait()
{
    PrintFuncCalled("AsyncAwait");
    {
        auto id = async_thread_pool.Run([=] { return sion::Request().SetUrl(url).SetHttpMethod(sion::Method::Get); });
        auto pkg = async_thread_pool.Await(id);
        std::cout << "AsyncAwait " << pkg.resp.GetHeader().Data().size() << pkg.err_msg << std::endl;
        assert(pkg.resp.Code() == "200");
    }
    {
        try
        {
            auto id = async_thread_pool.Run([=] { return sion::Request().SetUrl(url).SetHttpMethod(sion::Method::Get); });
            auto pkg = async_thread_pool.Await(id, 10);
            assert(pkg.err_msg.find("timeout") != -1);
        }
        catch (const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
    }
}

void AsyncCallback()
{
    PrintFuncCalled("AsyncCallback");
    async_thread_pool.Run([=] { return sion::Request().SetUrl(url).SetHttpMethod(sion::Method::Get); },
                          [](sion::AsyncResponse async_resp) {
                              std::cout << "AsyncCallback " << async_resp.resp.Status() << std::endl;
                              assert(async_resp.resp.Code() == "200");
                          });
}

void RequestWithProxy()
{
    PrintFuncCalled("RequestWithProxy");
    std::smatch m;
    std::regex url_parse(R"(^http://([\w.]+):(\d+)$)");
    std::string proxy_env(std::getenv("http_proxy"));
    std::regex_match(proxy_env, m, url_parse);
    int port = std::stoi(m[2]);
    std::string ip = m[1];
    sion::HttpProxy proxy{port, ip}; // 请求代理服务器，只支持http
    // 如果身份验证自己加个header https://developer.mozilla.org/zh-CN/docs/Web/HTTP/Headers/Proxy-Authorization
    auto resp = sion::Request().SetProxy(proxy).SetUrl(url).SetHttpMethod("GET").Send();
    std::cout << "server: " << resp.GetHeader().Get("server") << std::endl;
    std::cout << resp.StrBody() << std::endl;
}

sion::Payload::Binary LoadFile(sion::String path, sion::String type)
{
    std::fstream file(path, std::ios_base::in);
    assert(file.good());
    sion::Payload::Binary bin;
    bin.file_name = path;
    bin.type = type;
    const int size = 1000;
    std::array<char, size> buf;
    while (file.good())
    {
        file.read(buf.data(), size - 1);
        bin.data.insert(bin.data.end(), buf.begin(), buf.begin() + file.gcount());
    }
    return bin;
}

void PostBinaryData()
{
    PrintFuncCalled("PostBinaryData - Formdata");
    auto file = LoadFile("chunked.jpeg", "image/jpeg");
    {

        sion::Payload::FormData form;
        form.Append("helo", "world");
        form.Append("hello2", "world");
        form.Append("file", file);
        form.Append("agumi", "script runtime");
        form.Remove("agumi");
        auto req = sion::Request().SetUrl("http://www.httpbin.org/post").SetBody(form).SetHttpMethod("POST").Send();
        // std::cout << "post binary data with FormData  " << req.Code() << req.StrBody();
        assert(req.Code() == "200");
    }

    PrintFuncCalled("PostBinaryData - Binary");
    {
        auto req = sion::Request()
                       .SetUrl("http://www.httpbin.org/post")
                       .SetBody(file)
                       .SetHttpMethod("POST")
                       .Send();
        // std::cout << "post binary data with Binary  " << req.Code() << req.StrBody();
        assert(req.Code() == "200");
    }
}
int main()
{
    async_thread_pool.Start();
    // RequestWithProxy();
    FetchHeader();
    FetchChunkedHtml();
    DownloadChunkedFile();
    PostRequest();
    AsyncAwait();
    AsyncCallback();
    AsyncGetAvailableResponse();
    PostBinaryData();
    std::cout << "TestPassed;All ok" << std::endl;
    return 1;
}
