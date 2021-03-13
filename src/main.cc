#include "sion.h"
#include <fstream>
#include <iostream>
int main()
{
    auto resp = sion::Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
    std::ofstream file(R"(分块.jpeg)", std::ios::binary);
    auto& bin = resp.BodyBin();
    file.write(bin.data(), bin.size() * sizeof(char));
    return 1;
}