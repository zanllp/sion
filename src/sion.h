#define SION_DISABLE_SSL
#pragma once
#pragma warning(disable : 4267)
#pragma warning(disable : 4244)
#pragma warning(disable : 6031)
#pragma warning(disable : 26451)
#pragma warning(disable : 26444)
#include <string>
#include <map>
#include <regex>
#include <array>
#include <vector>
#include <codecvt>
#include <locale>
#include <functional>
#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#endif // WIN32
#ifndef SION_DISABLE_SSL
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif // !SION_DISABLE_SSL
namespace sion
{
    using std::array;
    using std::map;
    using std::pair;
    using std::string;
    using std::vector;

    class String : public string
    {
    public:
        String(){};
        virtual ~String(){};
        template <class T>
        String(T &&arg) : string(std::forward<T>(arg)) {}

        String(int arg) : string(std::to_string(arg)) {}

        String(unsigned long arg) : string(std::to_string(arg)) {}

        String(double arg) : string(std::to_string(arg)) {}

        String(bool arg)
        {
            (*this) = arg ? "true" : "false";
        }

        String(char arg)
        {
            (*this) = " ";
            (*this)[0] = arg;
        }

        // 使用字符串分割
        // flag 分割标志,返回的字符串向量会剔除,flag不要用char，会重载不明确
        // num 分割次数，默认-1即分割到结束，例num=1,返回开头到flag,flag到结束size=2的字符串向量
        // skipEmpty 跳过空字符串，即不压入length==0的字符串
        vector<String> Split(String flag, int num = -1, bool skipEmpty = true) const
        {
            vector<String> dataSet;
            auto PushData = [&](String line) {
                if (line.length() != 0 || !skipEmpty)
                {
                    dataSet.push_back(line);
                }
            };
            auto Pos = FindAll(flag, num);
            if (Pos.size() == 0)
            {
                return vector<String>({*this});
            }
            for (auto i = 0; i < Pos.size() + 1; i++)
            {
                if (dataSet.size() == num && Pos.size() > num && num != -1)
                { // 满足数量直接截到结束
                    PushData(substr(Pos[dataSet.size()] + flag.size()));
                    break;
                }
                if (i == 0)
                { // 第一个数的位置不是0的话补上
                    PushData(substr(0, Pos[0]));
                }
                else if (i != Pos.size())
                {
                    int Left = Pos[i - 1] + flag.length();
                    int Right = Pos[i] - Left;
                    PushData(substr(Left, Right));
                }
                else
                { // 最后一个标志到结束
                    PushData(substr(*(--Pos.end()) + flag.size()));
                }
            }
            return dataSet;
        }
        // 清除前后的字符
        // target 需要清除的字符默认空格
        String Trim(String empty_set = " \n\r") const
        {
            int len = length();
            int left = 0;
            while (left < len && IncludeSym(empty_set, (*this)[left]))
            {
                left++;
            }
            if (left >= len)
            {
                return *this;
            }
            int right = len - 1;
            while (right > 0 && IncludeSym(empty_set, (*this)[right]))
            {
                right--;
            }
            return substr(left, right - left + 1);
        }

        String ToLowerCase()
        {
            String s = *this;
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
            return s;
        }

        String ToUpperCase()
        {
            String s = *this;
            std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::toupper(c); });
            return s;
        }

        static bool IncludeSym(String syms, char sym)
        {
            for (auto i : syms)
            {
                if (i == sym)
                {
                    return true;
                }
            }
            return false;
        }

        // 包含字母
        bool HasLetter()
        {
            for (auto &x : *this)
            {
                if ((x >= 'a' && x <= 'z') ||
                    (x >= 'A' && x <= 'Z'))
                {
                    return true;
                }
            }
            return false;
        }
        // 返回搜索到的所有位置
        // flag 定位标志
        // num 搜索数量，默认直到结束
        vector<int> FindAll(String flag, int num = -1) const
        {
            vector<int> Result;
            auto Pos = find(flag);
            auto flag_offset = flag.length() == 0 ? 1 : flag.length();
            while (Pos != -1 && Result.size() != num)
            {
                Result.push_back(Pos);
                Pos = find(flag, *(--Result.end()) + flag_offset);
            }
            return Result;
        }

        // 字符串替换
        // oldStr 被替换的字符串
        // newStr 新换上的字符串
        // count 替换次数，默认1，大于0时替换到足够次数或找不到旧字符串为止，小于0时替换到结束
        String &Replace(String oldStr, String newStr, int count = 1)
        {
            if (count == 0)
            {
                return *this;
            }
            int pos = find(oldStr);
            if (pos == string::npos)
            {
                return *this;
            }
            replace(pos, oldStr.length(), newStr);
            return Replace(oldStr, newStr, count < 0 ? -1 : count - 1);
        }
    };

    template <typename ExceptionType>
    void Throw(String msg = "") { throw ExceptionType(msg.c_str()); }

    template <typename ExceptionType = std::exception>
    void check(
        bool condition, String msg = "", std::function<void()> recycle = [] {})
    {
        if (!condition)
        {
            recycle();
            Throw<ExceptionType>(msg);
        }
    }
#ifdef _WIN32
    using Socket = SOCKET;
#else
    using Socket = int;
#endif

    String GetIpByHost(String hostname)
    {
        addrinfo hints, *res;
        in_addr addr;
        int err;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_INET; // ipv4
        if ((err = getaddrinfo(hostname.c_str(), NULL, &hints, &res)) != 0)
        {
            Throw<std::runtime_error>("错误" + err + String(gai_strerror(err)));
        }
        addr.s_addr = ((sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
        char str[INET_ADDRSTRLEN];
        auto ptr = inet_ntop(AF_INET, &addr, str, sizeof(str));
        freeaddrinfo(res);
        return str;
    }

    Socket GetSocket()
    {
#ifdef _WIN32
        // 初始化。,WSA windows异步套接字
        WSADATA inet_WsaData;                      //
        WSAStartup(MAKEWORD(2, 0), &inet_WsaData); // socket2.0版本
        if (LOBYTE(inet_WsaData.wVersion) != 2 || HIBYTE(inet_WsaData.wVersion) != 0)
        { // 高位字节指明副版本、低位字节指明主版本
            WSACleanup();
            return -1;
        }
#endif
        auto tcp_socket = socket(AF_INET, SOCK_STREAM, 0); // ipv4,tcp,tcp或udp该参数可为0
        return tcp_socket;
    }

    class Header
    {
    public:
        Header(){};
        ~Header(){};
        vector<pair<String, String>> data;
        Header &operator=(vector<pair<String, String>> &&other) noexcept
        {
            data = other;
            return *this;
        }
        // 添加一个键值对到头中
        void Add(String k, String v)
        {
            data.push_back({k, v});
        }
        // 获取头的键所对应的所有值
        vector<String> GetValue(String key)
        {
            key = key.ToLowerCase();
            vector<String> res;
            for (auto &i : data)
            {
                if (i.first == key)
                {
                    res.push_back(i.second);
                }
            }
            return res;
        }
        // 获取头的键所对应的值，以最后一个为准
        String GetLastValue(String key)
        {
            key = key.ToLowerCase();
            for (int i = data.size() - 1; i >= 0; i--)
            {
                if (data[i].first == key)
                {
                    return data[i].second;
                }
            }
            return "";
        }
    };

    enum class Method
    {
        Get,
        Post,
        Put,
        Delete
    };

    class Response
    {

    public:
        bool IsChunked = false;     // 是否分块编码
        bool SaveByCharVec = false; // 是否使用字符数组保存，对于文本直接用字符串保存，其它用char数组
        int ContentLength = 0;      // 正文长度
        String Source;              // 源响应报文
        String Cookie;
        String ProtocolVersion;
        String Code;
        String Status;
        String BodyStr; // 响应体，对于文本直接保存这
        String CharSet;
        vector<char> BodyCharVec; // 响应体，对于二进制流保存在这
        Header ResponseHeader;    // 响应头
        Response(){};
        ~Response(){};

        Response(String source) noexcept
        {
            Source = source;
            ParseFromSource();
        }
        String HeaderValue(String k) { return ResponseHeader.GetLastValue(k); };

        // 解析服务器发送过来的响应
        // PreParse 预解析，如果为true且使用char保存而且不是chunked编码那么只解析头部
        void ParseFromSource(bool PreParse = false)
        {
            // 头和首行一次就够
            if (ResponseHeader.data.size() == 0)
            {
                HeaderAndFirstLineParse();
            }
            // 响应体，预解析的时候解析响应体
            // 关闭预解析，或者是非分块且为字符串的时候就行解析，因为这种情况需要获取头的长度
            if (!PreParse || (!IsChunked && !SaveByCharVec))
            {
                BodyStrParse();
            }
        }

    private:
        void HeaderAndFirstLineParse()
        {
            auto HeaderStr = Source.substr(0, Source.find("\r\n\r\n"));
            auto data = String(HeaderStr).Split("\r\n");
            if (data.size() == 0)
            {
                return;
            }
            // 第一行
            auto FirstLine = data[0].Split(" ", 2);
            check<std::runtime_error>(FirstLine.size() == 3, "解析错误\n" + Source);
            ProtocolVersion = FirstLine[0].Trim();
            Code = FirstLine[1].Trim();
            Status = FirstLine[2].Trim();
            data.erase(data.begin());
            // 头
            for (auto &x : data)
            {
                auto pair = x.Split(":", 1);
                if (pair.size() == 2)
                {
                    ResponseHeader.Add(pair[0].Trim().ToLowerCase(), pair[1].Trim());
                }
            }
            String contentLen = HeaderValue("content-length");
            ContentLength = contentLen != "" ? stoi(contentLen) : ContentLength;
            IsChunked = ContentLength == 0;
            Cookie = HeaderValue("cookie");
            auto ContentType = HeaderValue("content-type");
            if (ContentType != "")
            { // Content-Type: text/html; charset=utf-8
                auto ContentSplit = ContentType.Split(";", 1);
                auto type = ContentSplit[0].Split("/", 1)[0].Trim();
                SaveByCharVec = type != "text" && type != "application"; // 解析看是文本还字节流
                if (ContentSplit.size() != 1)                            // 是gbk的话已经转过一次了，不用再管
                {
                    CharSet = ContentSplit[1].Split("=", 1)[1].Trim();
                }
            }
        }

        void BodyStrParse()
        {
            if (SaveByCharVec)
            {
                const auto &sc = BodyCharVec;
                if (sc.size() == 0 || !IsChunked)
                {
                    return;
                }
                vector<char> PureSouceChar;
                // 获取下一个\r\n的位置
                int NRpos = 0;
                auto GetNextNR = [&](int leap) {
                    for (int i = NRpos + leap; i < (sc.size() - 1); i++)
                    {
                        if (sc[i] == '\r' && sc[i + 1] == '\n')
                        {
                            NRpos = i;
                            return i;
                        }
                    }
                    return -1;
                };
                int Left = -2; // 这里-2是因为第一个数量是400\r\n这样的，而其它的是\r\n400\r\n。所以要对第一次进行补偿
                int Right = GetNextNR(0);
                while (Left != -1 && Right != -1)
                {
                    auto count = string(sc.begin() + 2 + Left, sc.begin() + Right); // 每个分块开头写的数量
                    if (count == "0")
                    {
                        break;
                    }                                           // 最后一个 0\r\n\r\n，退出
                    auto countNum = stoi(count, nullptr, 16);   // 那数量是16进制
                    auto chunkedStart = sc.begin() + Right + 2; // 每个分块正文的开始位置
                    PureSouceChar.insert(PureSouceChar.end(), chunkedStart, chunkedStart + countNum);
                    Left = GetNextNR(countNum); //  更新位置
                    Right = GetNextNR(1);
                }
                BodyCharVec = PureSouceChar;
                ContentLength = PureSouceChar.size();
            }
            else
            {
                auto bodyPos = Source.find("\r\n\r\n");
                if (bodyPos == string::npos || bodyPos == Source.length() - 4)
                {
                    return;
                }
                BodyStr = Source.substr(bodyPos + 4);
                if (!IsChunked)
                {
                    return;
                }
                const auto &rb = BodyStr;
                String pureStr;
                // 获取下一个\r\n的位置
                int NRpos = 0;
                auto GetNextNR = [&](int leap) {
                    NRpos = rb.find("\r\n", NRpos + leap);
                    return NRpos;
                };
                int Left = -2; // 这里-2是因为第一个数量是400\r\n这样的，而其它的是\r\n400\r\n。所以要对第一次进行补偿
                int Right = GetNextNR(0);
                while (Left != -1 && Right != -1)
                {
                    auto count = string(rb.begin() + 2 + Left, rb.begin() + Right); // 每个分块开头写的数量
                    if (count == "0")
                    {
                        break;
                    }                                           // 最后一个 0\r\n\r\n，退出
                    auto countNum = stoi(count, nullptr, 16);   // 那数量是16进制
                    auto chunkedStart = rb.begin() + Right + 2; // 每个分块正文的开始位置
                    pureStr.insert(pureStr.end(), chunkedStart, chunkedStart + countNum);
                    Left = GetNextNR(countNum); //  更新位置
                    Right = GetNextNR(1);
                }
                BodyStr = pureStr;
                ContentLength = BodyStr.length();
            }
        }
    };

    class Request
    {
    public:
        int port = 80;
        String Source;
        String Method;
        String Path;
        String Protocol;
        String IP;
        String Url;
        String Host;
        String Cookie;
        String RequestBody;
        String ProtocolVersion = "HTTP/1.1";
        Header RequestHeader;
        Request(){};
        ~Request(){};

        Request &SetHttpMethod(sion::Method method)
        {
            switch (method)
            {
            case Method::Get:
                Method = "GET";
                break;
            case Method::Post:
                Method = "POST";
                break;
            case Method::Put:
                Method = "PUT";
                break;
            case Method::Delete:
                Method = "DELETE";
                break;
            }
            return *this;
        }

        Request &SetHttpMethod(String other)
        {
            Method = other;
            return *this;
        }

        Request &SetUrl(String url)
        {
            Url = url;
            return *this;
        }

        Request &SetCookie(String cookie)
        {
            Cookie = cookie;
            return *this;
        }

        Request &SetBody(String body)
        {
            RequestBody = body;
            return *this;
        }

        Request &SetHeader(vector<pair<String, String>> header)
        {
            RequestHeader.data = header;
            return *this;
        }

        Request &SetHeader(String k, String v)
        {
            RequestHeader.Add(k, v);
            return *this;
        }

        Response Send(sion::Method method, String url)
        {
            SetHttpMethod(method);
            return Send(url);
        }

        Response Send() { return Send(Url); }
#ifndef SION_DISABLE_SSL
    private:
        Response SendBySSL(Socket socket)
        {
            SSL_library_init();
            auto method = TLSv1_1_client_method();
            auto context = SSL_CTX_new(method);
            auto ssl = SSL_new(context);
            SSL_set_fd(ssl, socket);
            SSL_connect(ssl);
            SSL_write(ssl, Source.c_str(), Source.length());
            auto resp = ReadResponse(socket, ssl);
            SSL_shutdown(ssl);
            SSL_free(ssl);
            SSL_CTX_free(context);
            return resp;
        }
#endif
    public:
        Response Send(String url)
        {
            check<std::invalid_argument>(Method.length(), "请求方法未定义");
            std::smatch m;
#ifndef SION_DISABLE_SSL
            std::regex urlParse(R"(^(http|https)://([\w.]*):?(\d*)(/?.*)$)");
            regex_match(url, m, urlParse);
            check<std::invalid_argument>(m.size() == 5, "url格式不对或者是用了除http,https外的协议");
            Protocol = m[1];
            port = m[3].length() == 0 ? (Protocol == "http" ? 80 : 443) : stoi(m[3]);
#else
            std::regex urlParse(R"(^(http)://([\w.]*):?(\d*)(/?.*)$)");
            regex_match(url, m, urlParse);
            check<std::invalid_argument>(m.size() == 5, "url格式不对或者是用了除http外的协议");
            Protocol = m[1];
            port = m[3].length() == 0 ? 80 : stoi(m[3]);
#endif
            Host = m[2];
            Path = m[4].length() == 0 ? "/" : m[4].str();
            Socket socket = GetSocket();
            try
            {
                Connection(socket, Host);
                BuildRequestString();
                if (Protocol == "http")
                {
                    send(socket, Source.c_str(), int(Source.length()), 0);
                    return ReadResponse(socket);
                }
#ifndef SION_DISABLE_SSL
                else // if (Protocol == "https")
                {
                    return SendBySSL(socket);
                }
#endif
            }
            catch (const std::exception &e)
            {
#ifdef _WIN32
                WSACleanup();
#endif
                throw e;
            }
        }

    private:
        void BuildRequestString()
        {
            RequestHeader.Add("Host", Host);
            RequestHeader.Add("Content-Length", std::to_string(RequestBody.length()));
            if (Cookie.length())
            {
                RequestHeader.Add("Cookie", Cookie);
            }
            Source = Method + " " + Path + " " + ProtocolVersion + "\r\n";
            for (auto &x : RequestHeader.data)
            {
                Source += x.first + ": " + x.second + "\r\n";
            }
            Source += "\r\n";
            Source += RequestBody;
        }

        void Connection(Socket socket, String host)
        {
            in_addr sa;
            IP = host.HasLetter() ? GetIpByHost(host) : host;
#ifdef _WIN32
            check<std::invalid_argument>((InetPton(AF_INET, IP.c_str(), &sa) != -1), "地址转换错误");
#else
            check<std::invalid_argument>((inet_pton(AF_INET, IP.c_str(), &sa) != -1), "地址转换错误");
#endif
            sockaddr_in saddr;
            saddr.sin_family = AF_INET;
            saddr.sin_port = htons(port);
            saddr.sin_addr = sa;
            if (::connect(socket, (sockaddr *)&saddr, sizeof(saddr)) != 0)
            {
#ifdef _WIN32
                std::string err = "连接失败错误码：" + std::std::to_string(WSAGetLastError());
#else
                std::string err = "连接失败";
#endif
                Throw<std::runtime_error>(err);
            }
        }
#ifndef SION_DISABLE_SSL
        Response ReadResponse(Socket socket, SSL *ssl = nullptr)
#else
        Response ReadResponse(Socket socket)
#endif
        {
            const int bufSize = 2048;
            array<char, bufSize> buf{0};
            auto Read = [&]() {
                buf.fill(0);
                int status = 0;
                if (Protocol == "http")
                {
                    status = recv(socket, buf.data(), bufSize - 1, 0);
                }
#ifndef SION_DISABLE_SSL
                else if (Protocol == "https")
                {
                    status = SSL_read(ssl, buf.data(), bufSize - 1);
                }
#endif
                check<std::runtime_error>(status >= 0, "网络异常,Socket错误码：" + std::to_string(status));
                return status;
            };
            Response resp;
            // 读取解析头部信息
            auto ReadCount = Read();
            resp.Source += buf.data();
            resp.ParseFromSource(true);
            if (resp.SaveByCharVec)
            { // 把除头外多余的响应体部分移过去
                auto bodyPos = resp.Source.find("\r\n\r\n");
                auto startBody = buf.begin() + 4 + bodyPos;
                resp.BodyCharVec.insert(resp.BodyCharVec.end(), startBody, buf.begin() + ReadCount);
            }
            auto lenHeader = resp.Source.length() - resp.BodyStr.length(); // 响应头长度
            // 检查是否接收完
            auto CheckEnd = [&] {
                if (resp.SaveByCharVec)
                {
                    if (resp.IsChunked)
                    {
                        auto start = resp.BodyCharVec.begin() + resp.BodyCharVec.size() - 7;
                        return string(start, start + 7) == "\r\n0\r\n\r\n";
                    }
                    else
                    {
                        return resp.BodyCharVec.size() == resp.ContentLength;
                    }
                }
                else
                {
                    if (resp.IsChunked)
                    {
                        return resp.Source.substr(resp.Source.length() - 7) == "\r\n0\r\n\r\n";
                    }
                    else
                    {
                        return resp.Source.length() - lenHeader == resp.ContentLength;
                    }
                }
            };
            // 循环读取接收
            while (!CheckEnd())
            {
                ReadCount = Read();
                if (resp.SaveByCharVec)
                {
                    resp.BodyCharVec.insert(resp.BodyCharVec.end(), buf.begin(), buf.begin() + ReadCount);
                }
                else
                {
                    resp.Source += buf.data();
                }
            }
#ifdef _WIN32
            closesocket(socket);
            WSACleanup();
#else
            close(socket);
#endif
            resp.ParseFromSource();
            return resp;
        }
    };

    Response Fetch(String url, Method method = Method::Get, vector<pair<String, String>> header = {}, String body = "")
    {
        return Request().SetUrl(url).SetHttpMethod(method).SetHeader(header).SetBody(body).Send();
    }
} // namespace sion