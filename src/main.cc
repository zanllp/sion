#include "sion.h"
#include <chrono>
#include <fstream>
#include <iostream>

void FetchHeader()
{

    auto resp = sion::Fetch("https://github.com/zanllp/sion");
    std::cout << resp.StrBody().substr(0, 100) << "..." << std::endl << "cache-control: " << resp.Header().Get("cache-control");
    for (auto&& i : resp.Header().Data())
    {
        std::cout << "k:" << i.first << "\tv:" << i.second << std::endl;
    }
}

void FetchChunkedHtml()
{
    auto resp = sion::Fetch("https://visualstudio.microsoft.com/zh-hans/msdn-platforms/");
    std::cout << resp.StrBody().substr(0, 100) << "..." << std::endl;
}

void DownloadChunkedFile()
{
    auto resp = sion::Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
    std::ofstream file(R"(分块.jpeg)", std::ios::binary);
    file.write(resp.Body().data(), resp.Body().size());
}

int main()
{
    {
        sion::Async async;
        sion::String ms_url = "https://github.com/zanllp/sion";
        async.Start();

        {
            auto id = async.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); });

            auto pkg = async.Await(id);
            std::cout << pkg.resp.Header().Data().size()<< pkg.err_msg << std::endl;
        }
        for (size_t i = 0; i < 100; i++)
        {
            async.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); });
        }
    }

    // FetchHeader();
    // FetchChunkedHtml();
    // DownloadChunkedFile();
    return 1;
}
