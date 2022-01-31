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
        sion::ThreadPool tl;
        sion::String ms_url = "https://visualstudio.microsoft.com/zh-hans/msdn-platforms/";
        tl.Run();
        for (size_t i = 0; i < 16; i++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            tl.Send([=]() { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); },
                    [](sion::Response resp) { std::cout << resp.Header().Get("server")<<"  "<<std::this_thread::get_id() << std::endl; });
        }
    }
    FetchHeader();
    // FetchChunkedHtml();
    DownloadChunkedFile();
    return 1;
}
