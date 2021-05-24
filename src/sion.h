#pragma once
#include <string>
#include <map>
#include <regex>
#include <array>
#include <vector>
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
    class Request;
    class Response;
    using std::array;
    using std::map;
    using std::pair;
    using std::string;
    using std::vector;

    class String : public string
    {
    public:
        String() {};
        ~String() {};
        template <class T>
        String(T&& arg) : string(std::forward<T>(arg)) {}

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
        // skip_empty 跳过空字符串，即不压入length==0的字符串
        vector<String> Split(String flag, int num = -1, bool skip_empty = true) const
        {
            vector<String> data_set;
            auto push_data = [&](String line) {
                if (line.length() != 0 || !skip_empty)
                {
                    data_set.push_back(line);
                }
            };
            auto pos = FindAll(flag, num);
            if (pos.size() == 0)
            {
                return vector<String>({ *this });
            }
            for (auto i = 0; i < pos.size() + 1; i++)
            {
                if (data_set.size() == num && pos.size() > num && num != -1)
                { // 满足数量直接截到结束
                    push_data(substr(pos[data_set.size()] + flag.size()));
                    break;
                }
                if (i == 0)
                { // 第一个数的位置不是0的话补上
                    push_data(substr(0, pos[0]));
                }
                else if (i != pos.size())
                {
                    int left = pos[i - 1] + flag.length();
                    int right = pos[i] - left;
                    push_data(substr(left, right));
                }
                else
                { // 最后一个标志到结束
                    push_data(substr(*(--pos.end()) + flag.size()));
                }
            }
            return data_set;
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
            for (auto& x : *this)
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
            vector<int> result;
            auto pos = find(flag);
            auto flag_offset = flag.length() == 0 ? 1 : flag.length();
            while (pos != -1 && result.size() != num)
            {
                result.push_back(pos);
                pos = find(flag, *(--result.end()) + flag_offset);
            }
            return result;
        }

        // 字符串替换
        // oldStr 被替换的字符串
        // newStr 新换上的字符串
        // count 替换次数，默认1，大于0时替换到足够次数或找不到旧字符串为止，小于0时替换到结束
        String& Replace(String old_str, String new_str, int count = 1)
        {
            if (count == 0)
            {
                return *this;
            }
            auto pos = find(old_str);
            if (pos == string::npos)
            {
                return *this;
            }
            replace(pos, old_str.length(), new_str);
            return Replace(old_str, new_str, count < 0 ? -1 : count - 1);
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
        addrinfo hints, * res;
        in_addr addr;
        int err;
        memset(&hints, 0, sizeof(addrinfo));
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_INET; // ipv4
        if ((err = getaddrinfo(hostname.c_str(), NULL, &hints, &res)) != 0)
        {
            Throw<std::runtime_error>("错误" + std::to_string(err) + String(gai_strerror(err)));
        }
        addr.s_addr = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr, str, sizeof(str));
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
        Header() {};
        ~Header() {};
        vector<pair<String, String>> data;
        Header& operator=(vector<pair<String, String>>&& other) noexcept
        {
            data = other;
            return *this;
        }
        // 添加一个键值对到头中
        void Add(String k, String v)
        {
            data.push_back({ k, v });
        }
        // 获取头的键所对应的所有值
        vector<String> GetValue(String key)
        {
            key = key.ToLowerCase();
            vector<String> res;
            for (auto& i : data)
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
        friend Request;
        bool is_chunked_ = false;       // 是否分块编码
        bool save_by_char_vec_ = false; // 是否使用字符数组保存，对于文本直接用字符串保存，其它用char数组
        int content_length_ = 0;        // 正文长度
        String source_;                 // 源响应报文
        String cookie_;
        String protocol_version_;
        String code_;
        String status_;
        String body_str_; // 响应体，对于文本直接保存这
        String char_set_;
        String content_type_;
        vector<char> body_char_vec_; // 响应体，对于二进制流保存在这
        Header response_header_;     // 响应头
    public:
        Response() {};
        ~Response() {};

        Response(String source) noexcept
        {
            source_ = source;
            ParseFromSource();
        }
        const String& Source()
        {
            return source_;
        }

        bool SaveByVec()
        {
            return this->save_by_char_vec_;
        }

        const String& ContentType()
        {
            return content_type_;
        }

        const String& Body()
        {
            return body_str_;
        }

        const vector<char>& BodyBin()
        {
            return body_char_vec_;
        }
        String HeaderValue(String k) { return response_header_.GetLastValue(k); };

        // 解析服务器发送过来的响应
        // pre_parse 预解析，如果为true且使用char保存而且不是chunked编码那么只解析头部
        void ParseFromSource(bool pre_parse = false)
        {
            // 头和首行一次就够
            if (response_header_.data.size() == 0)
            {
                HeaderAndFirstLineParse();
            }
            // 响应体，预解析的时候解析响应体
            // 关闭预解析，或者是非分块且为字符串的时候就行解析，因为这种情况需要获取头的长度
            if (!pre_parse || (!is_chunked_ && !save_by_char_vec_))
            {
                BodyStrParse();
            }
        }

    private:
        void HeaderAndFirstLineParse()
        {
            auto header_str = source_.substr(0, source_.find("\r\n\r\n"));
            auto data = String(header_str).Split("\r\n");
            if (data.size() == 0)
            {
                return;
            }
            // 第一行
            auto first_line = data[0].Split(" ", 2);
            check<std::runtime_error>(first_line.size() == 3, "解析错误\n" + source_);
            protocol_version_ = first_line[0].Trim();
            code_ = first_line[1].Trim();
            status_ = first_line[2].Trim();
            data.erase(data.begin());
            // 头
            for (auto& x : data)
            {
                auto pair = x.Split(":", 1);
                if (pair.size() == 2)
                {
                    response_header_.Add(pair[0].Trim().ToLowerCase(), pair[1].Trim());
                }
            }
            String content_len = HeaderValue("content-length");
            content_length_ = content_len != "" ? stoi(content_len) : content_length_;
            is_chunked_ = content_length_ == 0;
            cookie_ = HeaderValue("cookie");
            content_type_ = HeaderValue("content-type");
            if (content_type_ != "")
            { // Content-Type: text/html; charset=utf-8
                auto content_split = content_type_.Split(";", 1);
                auto type = content_split[0].Split("/", 1)[0].Trim();
                save_by_char_vec_ = type != "text" && type != "application"; // 解析看是文本还字节流
                if (content_split.size() != 1)                               // 是gbk的话已经转过一次了，不用再管
                {
                    char_set_ = content_split[1].Split("=", 1)[1].Trim();
                }
            }
        }

        void BodyStrParse()
        {
            if (save_by_char_vec_)
            {
                const auto& sc = body_char_vec_;
                if (sc.size() == 0 || !is_chunked_)
                {
                    return;
                }
                vector<char> pure_source_char;
                // 获取下一个\r\n的位置
                int crlf_pos = 0;
                auto get_next_crlf = [&](int leap) {
                    for (int i = crlf_pos + leap; i < (sc.size() - 1); i++)
                    {
                        if (sc[i] == '\r' && sc[i + 1] == '\n')
                        {
                            crlf_pos = i;
                            return i;
                        }
                    }
                    return -1;
                };
                int left = -2; // 这里-2是因为第一个数量是400\r\n这样的，而其它的是\r\n400\r\n。所以要对第一次进行补偿
                int right = get_next_crlf(0);
                while (left != -1 && right != -1)
                {
                    auto count = string(sc.begin() + 2 + left, sc.begin() + right); // 每个分块开头写的数量
                    if (count == "0")
                    {
                        break;
                    }                                            // 最后一个 0\r\n\r\n，退出
                    auto count_num = stoi(count, nullptr, 16);   // 那数量是16进制
                    auto chunked_start = sc.begin() + right + 2; // 每个分块正文的开始位置
                    pure_source_char.insert(pure_source_char.end(), chunked_start, chunked_start + count_num);
                    left = get_next_crlf(count_num); //  更新位置
                    right = get_next_crlf(1);
                }
                body_char_vec_ = pure_source_char;
                content_length_ = pure_source_char.size();
            }
            else
            {
                auto body_pos = source_.find("\r\n\r\n");
                if (body_pos == string::npos || body_pos == source_.length() - 4)
                {
                    return;
                }
                body_str_ = source_.substr(body_pos + 4);
                if (!is_chunked_)
                {
                    return;
                }
                const auto& rb = body_str_;
                String pure_str;
                // 获取下一个\r\n的位置
                int crlf_pos = 0;
                auto get_next_crlf = [&](int leap) {
                    crlf_pos = rb.find("\r\n", crlf_pos + leap);
                    return crlf_pos;
                };
                int left = -2; // 这里-2是因为第一个数量是400\r\n这样的，而其它的是\r\n400\r\n。所以要对第一次进行补偿
                int right = get_next_crlf(0);
                while (left != -1 && right != -1)
                {
                    auto count = string(rb.begin() + 2 + left, rb.begin() + right); // 每个分块开头写的数量
                    if (count == "0")
                    {
                        break;
                    }                                            // 最后一个 0\r\n\r\n，退出
                    auto count_num = stoi(count, nullptr, 16);   // 那数量是16进制
                    auto chunked_start = rb.begin() + right + 2; // 每个分块正文的开始位置
                    pure_str.insert(pure_str.end(), chunked_start, chunked_start + count_num);
                    left = get_next_crlf(count_num); //  更新位置
                    right = get_next_crlf(1);
                }
                body_str_ = pure_str;
                content_length_ = body_str_.length();
            }
        }
    };

    class Request
    {
        String source_;
        String method_;
        String path_;
        String protocol_;
        String ip_;
        String url_;
        String host_;
        String cookie_;
        String request_body_;
        String protocol_version_ = "HTTP/1.1";
        Header request_header_;

    public:
        Request() {};
        ~Request() {};
        int port_ = 80;

        Request& SetHttpMethod(sion::Method method)
        {
            switch (method)
            {
            case Method::Get:
                method_ = "GET";
                break;
            case Method::Post:
                method_ = "POST";
                break;
            case Method::Put:
                method_ = "PUT";
                break;
            case Method::Delete:
                method_ = "DELETE";
                break;
            }
            return *this;
        }

        Request& SetHttpMethod(String other)
        {
            method_ = other;
            return *this;
        }

        Request& SetUrl(String url)
        {
            url_ = url;
            return *this;
        }

        Request& SetCookie(String cookie)
        {
            cookie_ = cookie;
            return *this;
        }

        Request& SetBody(String body)
        {
            request_body_ = body;
            return *this;
        }

        Request& SetHeader(vector<pair<String, String>> header)
        {
            request_header_.data = header;
            return *this;
        }

        Request& SetHeader(String k, String v)
        {
            request_header_.Add(k, v);
            return *this;
        }

        Response Send(sion::Method method, String url)
        {
            SetHttpMethod(method);
            return Send(url);
        }

        Response Send() { return Send(url_); }
#ifndef SION_DISABLE_SSL
    private:
        Response SendBySSL(Socket socket)
        {
            SSL_library_init();
            auto method = TLS_method();
            auto context = SSL_CTX_new(method);
            auto ssl = SSL_new(context);
            SSL_set_fd(ssl, socket);
            SSL_connect(ssl);
            SSL_write(ssl, source_.c_str(), source_.length());
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
            check<std::invalid_argument>(method_.length(), "请求方法未定义");
            std::smatch m;
#ifndef SION_DISABLE_SSL
            std::regex url_parse(R"(^(http|https)://([\w.]*):?(\d*)(/?.*)$)");
            regex_match(url, m, url_parse);
            check<std::invalid_argument>(m.size() == 5, "url格式不对或者是用了除http,https外的协议");
            protocol_ = m[1];
            port_ = m[3].length() == 0 ? (protocol_ == "http" ? 80 : 443) : stoi(m[3]);
#else
            std::regex url_parse(R"(^(http)://([\w.]*):?(\d*)(/?.*)$)");
            regex_match(url, m, url_parse);
            check<std::invalid_argument>(m.size() == 5, "url格式不对或者是用了除http外的协议");
            protocol_ = m[1];
            port_ = m[3].length() == 0 ? 80 : stoi(m[3]);
#endif
            host_ = m[2];
            path_ = m[4].length() == 0 ? "/" : m[4].str();
            Socket socket = GetSocket();
            try
            {
                Connection(socket, host_);
                BuildRequestString();
                if (protocol_ == "http")
                {
                    send(socket, source_.c_str(), int(source_.length()), 0);
                    return ReadResponse(socket);
                }
#ifndef SION_DISABLE_SSL
                else // if (protocol_ == "https")
                {
                    return SendBySSL(socket);
                }
#endif
            }
            catch (const std::exception& e)
            {
#ifdef _WIN32
                WSACleanup();
#endif
                throw std::runtime_error(e.what());
            }
            throw std::runtime_error("");
        }

    private:
        void BuildRequestString()
        {
            request_header_.Add("Host", host_);
            request_header_.Add("Content-Length", std::to_string(request_body_.length()));
            if (cookie_.length())
            {
                request_header_.Add("Cookie", cookie_);
            }
            source_ = method_ + " " + path_ + " " + protocol_version_ + "\r\n";
            for (auto& x : request_header_.data)
            {
                source_ += x.first + ": " + x.second + "\r\n";
            }
            source_ += "\r\n";
            source_ += request_body_;
        }

        void Connection(Socket socket, String host)
        {
            in_addr sa;
            ip_ = host.HasLetter() ? GetIpByHost(host) : host;
#ifdef _WIN32
            check<std::invalid_argument>((InetPton(AF_INET, ip_.c_str(), &sa) != -1), "地址转换错误");
#else
            check<std::invalid_argument>((inet_pton(AF_INET, ip_.c_str(), &sa) != -1), "地址转换错误");
#endif
            sockaddr_in saddr;
            saddr.sin_family = AF_INET;
            saddr.sin_port = htons(port_);
            saddr.sin_addr = sa;
            if (::connect(socket, (sockaddr*)&saddr, sizeof(saddr)) != 0)
            {
                String err = "连接失败:\nHost:" + host_ + "\n";
                err += "Ip:" + ip_ + "\n";
#ifdef _WIN32
                err += "错误码：" + std::to_string(WSAGetLastError());
#else
                err += String("error str:") + strerror(errno);
#endif
                Throw<std::runtime_error>(err);
            }
        }
#ifndef SION_DISABLE_SSL
        Response ReadResponse(Socket socket, SSL* ssl = nullptr)
#else
        Response ReadResponse(Socket socket)
#endif
        {
            const int buf_size = 2048;
            array<char, buf_size> buf{ 0 };
            auto Read = [&]() {
                buf.fill(0);
                int status = 0;
                if (protocol_ == "http")
                {
                    status = recv(socket, buf.data(), buf_size - 1, 0);
                }
#ifndef SION_DISABLE_SSL
                else if (protocol_ == "https")
                {
                    status = SSL_read(ssl, buf.data(), buf_size - 1);
                }
#endif
                check<std::runtime_error>(status >= 0, "网络异常,Socket错误码：" + std::to_string(status));
                return status;
            };
            Response resp;
            // 读取解析头部信息
            auto read_count = Read();
            resp.source_ += buf.data();
            resp.ParseFromSource(true);
            auto is_close = resp.HeaderValue("connection") == "close";
            if (resp.code_[0] == '3' && resp.HeaderValue("location") != "") // 3xx
            {
                return resp;
            }

            if (resp.save_by_char_vec_)
            { // 把除头外多余的响应体部分移过去
                auto bodyPos = resp.source_.find("\r\n\r\n");
                auto startBody = buf.begin() + 4 + bodyPos;
                resp.body_char_vec_.insert(resp.body_char_vec_.end(), startBody, buf.begin() + read_count);
            }
            auto len_header = resp.source_.length() - resp.body_str_.length(); // 响应头长度
            // 检查是否接收完
            auto check_end = [&] {
                if (resp.save_by_char_vec_)
                {
                    if (resp.is_chunked_)
                    {
                        auto start = resp.body_char_vec_.begin() + resp.body_char_vec_.size() - 7;
                        return string(start, start + 7) == "\r\n0\r\n\r\n" || is_close;
                    }
                    else
                    {
                        return resp.body_char_vec_.size() == resp.content_length_;
                    }
                }
                else
                {
                    if (resp.is_chunked_)
                    {
                        return resp.source_.substr(resp.source_.length() - 7) == "\r\n0\r\n\r\n" || is_close;
                    }
                    else
                    {
                        return resp.source_.length() - len_header == resp.content_length_;
                    }
                }
            };
            // 循环读取接收
            while (!check_end())
            {
                read_count = Read();
                if (resp.save_by_char_vec_)
                {
                    resp.body_char_vec_.insert(resp.body_char_vec_.end(), buf.begin(), buf.begin() + read_count);
                }
                else
                {
                    resp.source_ += buf.data();
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
