#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <map>
#include <queue>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#ifdef _WIN32
#include <WS2tcpip.h>
#include <Windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif // WIN32
#ifndef SION_DISABLE_SSL
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
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

class Error : public std::exception
{
  private:
    std::string msg;

  public:
    Error() {}
    Error(std::string msg) : msg(msg) {}
    ~Error() {}
    const char* what() const noexcept { return msg.c_str(); }
};

class AsyncAwaitTimeout : public Error
{
  public:
    AsyncAwaitTimeout(std::string msg) : Error(msg) {}
};

class String : public string
{
  public:
    String(){};
    ~String(){};
    template <class T> String(T&& arg) : string(std::forward<T>(arg)) {}

    String(int arg) : string(std::to_string(arg)) {}

    String(unsigned long arg) : string(std::to_string(arg)) {}

    String(double arg) : string(std::to_string(arg)) {}

    String(bool arg) { (*this) = arg ? "true" : "false"; }

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
        auto push_data = [&](String line)
        {
            if (line.length() != 0 || !skip_empty)
            {
                data_set.push_back(line);
            }
        };
        auto pos = FindAll(flag, num);
        if (pos.size() == 0)
        {
            return vector<String>({*this});
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
            if ((x >= 'a' && x <= 'z') || (x >= 'A' && x <= 'Z'))
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

template <typename ExceptionType = Error> void Throw(String msg = "") { throw ExceptionType(msg); }

template <typename ExceptionType = Error>
void check(
    bool condition, String msg = "", std::function<void()> recycle = [] {})
{
    if (!condition)
    {
        recycle();
        Throw<ExceptionType>(msg);
    }
}
using Socket = int;
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
        Throw("错误" + std::to_string(err) + String(gai_strerror(err)));
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
        Throw("wsa初始化错误");
    }
#endif
    auto tcp_socket = socket(AF_INET, SOCK_STREAM, 0); // ipv4,tcp,tcp或udp该参数可为0
    return tcp_socket;
}

class Header
{
    vector<pair<String, String>> data;

  public:
    Header(){};
    ~Header(){};
    const auto& Data() const { return data; }
    bool Remove(String key)
    {
        for (size_t i = 0; i < data.size(); i++)
        {
            if (data[i].first == key)
            {
                data.erase(data.begin() + i);
                return true;
            }
        }
        return false;
    }
    void RemoveAll(String key)
    {
        while (Remove(key))
        {
        }
    }
    Header(const vector<pair<String, String>>& other) noexcept { data = other; }
    // 添加一个键值对到头中
    void Add(String k, String v) { data.push_back({k, v}); }
    // 获取头的键所对应的所有值
    vector<String> GetAll(String key) const
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
    String Get(String key) const
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
    bool is_chunked_ = false;  // 是否分块编码
    int content_length_ = 0;   // 正文长度
    std::vector<char> source_; // 源响应报文
    String protocol_version_;
    String code_;
    String status_;
    vector<char> body_;      // 响应体
    Header response_header_; // 响应头
  public:
    Response(){};
    ~Response(){};

    Response(std::vector<char> source) noexcept
    {
        source_ = source;

        ParseHeader();
        ParseBody();
    }

    const vector<char>& Body() const { return body_; }
    const String Code() const { return code_; };
    const String Status() const { return status_; };
    const int ContentLength() const { return content_length_; };
    String StrBody() { return body_.size() == 0 ? "" : std::string(body_.data(), 0, body_.size()); }
    const auto& Header() const { return response_header_; };

  private:
    size_t resp_body_start_pos_ = -1;

    String Sourse2Str() { return source_.size() == 0 ? "" : std::string(source_.data(), 0, source_.size()); }

    bool CanParseHeader()
    {
        String body_separator = "\r\n\r\n";
        auto buf_str = Sourse2Str();
        if (resp_body_start_pos_ == -1 && buf_str.find(body_separator) != -1)
        {
            resp_body_start_pos_ = buf_str.find(body_separator) + 4;
        }
        return resp_body_start_pos_ != std::string::npos;
    }

    void ParseHeader()
    {
        String buf_str = Sourse2Str();
        auto header_str = buf_str.substr(0, resp_body_start_pos_);
        auto data = String(header_str).Split("\r\n");
        if (data.size() == 0)
        {
            return;
        }
        // 第一行
        auto first_line = data[0].Split(" ", 2);
        check(first_line.size() == 3, "解析错误\n" + buf_str);
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
        String content_len = response_header_.Get("content-length");
        content_length_ = content_len != "" ? stoi(content_len) : content_length_;
        is_chunked_ = response_header_.Get("Transfer-Encoding") == "chunked";
        body_.insert(body_.end(), source_.begin() + resp_body_start_pos_, source_.end());
    }

    void ParseBody()
    {
        const auto& sc = body_;
        if (sc.size() == 0 || !is_chunked_)
        {
            return;
        }
        vector<char> pure_source_char;
        // 获取下一个\r\n的位置
        int crlf_pos = 0;
        auto get_next_crlf = [&](int leap)
        {
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
            auto count_num = stoi(count, nullptr, 16);                      // 那数量是16进制
            if (count_num == 0)                                             // 最后一个 0\r\n\r\n，退出
            {
                break;
            }
            auto chunked_start = sc.begin() + right + 2; // 每个分块正文的开始位置
            pure_source_char.insert(pure_source_char.end(), chunked_start, chunked_start + count_num);
            left = get_next_crlf(count_num + 2); //  更新位置lpv
            right = get_next_crlf(1);
        }
        body_ = pure_source_char;
        content_length_ = pure_source_char.size();
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
    String request_body_;
    String protocol_version_ = "HTTP/1.1";
    Header request_header_;

  public:
    Request(){};
    ~Request(){};
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

    Request& SetBody(String body)
    {
        request_body_ = body;
        return *this;
    }

    Request& SetHeader(vector<pair<String, String>> header)
    {
        request_header_ = header;
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
        auto ssl_ctx = SSL_CTX_new(method);
        auto ssl = SSL_new(ssl_ctx);
        check(ssl != nullptr && ssl_ctx != nullptr, "openssl初始化异常");
        SSL_set_fd(ssl, socket);
        SSL_connect(ssl);
        SSL_write(ssl, source_.c_str(), source_.length());
        auto resp = ReadResponse(socket, ssl);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ssl_ctx);
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
            Throw(e.what());
        }
        assert(false);
    }

  private:
    void BuildRequestString()
    {
        request_header_.Add("Host", host_);
        request_header_.Add("Content-Length", std::to_string(request_body_.length()));
        source_ = method_ + " " + path_ + " " + protocol_version_ + "\r\n";
        for (auto& x : request_header_.Data())
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
            Throw(err);
        }
    }
#ifndef SION_DISABLE_SSL
    Response ReadResponse(Socket socket, SSL* ssl = nullptr)
#else
    Response ReadResponse(Socket socket)
#endif
    {
        const int buf_size = 2048;
        array<char, buf_size> buf{0};
        auto Read = [&]()
        {
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
            check(status >= 0, "网络异常,Socket错误码：error:" + String(strerror(errno)));
            return status;
        };
        Response resp;
        auto read_count = 0;
        // 读取解析头部信息
        while (true)
        {
            read_count = Read();
            if (read_count > 0)
            {
                resp.source_.insert(resp.source_.end(), buf.data(), buf.data() + read_count);
            }
            if (resp.CanParseHeader())
            {
                break;
            }
        }

        resp.ParseHeader();
        // 检查是否接收完
        auto check_end = [&]
        {
            const auto& body = resp.Body();
            if (resp.is_chunked_)
            {
                if (body.size() < 7)
                {
                    return false;
                }
                auto chunked_end_offset = body.size() - 4;
                auto chunked_end_iter = body.begin() + chunked_end_offset;
                auto chunked_end = string(chunked_end_iter, chunked_end_iter + 4);
                if (chunked_end != "\r\n\r\n")
                {
                    return false;
                }
                auto chunked_start_offset = chunked_end_offset - 1;
                // 有些不是\r\n0\r\n\r\n 而是\r\n000000\r\n\r\n
                for (auto& i = chunked_start_offset; i >= 2; i--)
                {
                    auto r = body[i];
                    if (r != '0')
                    {
                        break;
                    }
                    if (body[i - 1] == '\n' && body[i - 2] == '\r')
                    {
                        return true;
                    }
                }
            }
            else
            {
                return body.size() == resp.content_length_;
            }
            return false;
        };
        // 循环读取接收
        while (!check_end())
        {
            read_count = Read();
            resp.body_.insert(resp.body_.end(), buf.begin(), buf.begin() + read_count);
        }
#ifdef _WIN32
        closesocket(socket);
        WSACleanup();
#else
        close(socket);
#endif
        resp.ParseBody();
        return resp;
    }
};

Response Fetch(String url, Method method = Method::Get, vector<pair<String, String>> header = {}, String body = "")
{
    return Request().SetUrl(url).SetHttpMethod(method).SetHeader(header).SetBody(body).Send();
}
enum AsyncResponseReceiveMode
{
    callback,
    queue,
    future
};

struct AsyncResponse
{
    Response resp;
    uint32_t id;
    String err_msg;
};

struct AsyncPackage
{
    Request request;
    std::function<void(AsyncResponse)> callback;
    uint32_t id;
    AsyncResponseReceiveMode received_mode;
};

class Async
{
    int thread_num_ = 6;
    std::queue<AsyncPackage> queue_;
    std::mutex m_;
    std::condition_variable cv_;
    std::condition_variable waiting_resp_cv_;
    std::map<int, std::thread> threads_;
    bool running_ = false;
    bool throw_if_has_err_msg = false;
    std::atomic_bool stopped_ = false;
    std::atomic_uint incr_id = 0;
    bool is_block_ = false;
    std::mutex waiting_resp_queue_mutex_;
    std::vector<AsyncResponse> waiting_handle_response_;

  public:
    ~Async()
    {
        stopped_ = true;
        cv_.notify_all();
        std::unique_lock<std::mutex> lk(m_);
        auto& ts = threads_;
        cv_.wait(lk, [&] { return ts.empty(); }); // 等待所有线程退出
    }
    Async& SetThreadNum(int num)
    {
        thread_num_ = num;
        return *this;
    }
    Async& SetBlock(bool wait)
    {
        is_block_ = wait;
        return *this;
    }
    Async& SetThrowIfHasErrMsg(bool op)
    {
        throw_if_has_err_msg = op;
        return *this;
    }

    auto GetTargetResp(uint32_t id)
    {
        auto& queue = waiting_handle_response_;
        for (size_t i = 0; i < queue.size(); i++)
        {
            auto& target = queue[i];
            if (target.id == id)
            {
                auto r = queue[i];
                if (throw_if_has_err_msg && r.err_msg.size())
                {
                    Throw(r.err_msg);
                }
                queue.erase(queue.begin() + i);
                return r;
            }
        }
        assert(false);
    }

    auto Await(uint32_t id, uint32_t timeout_ms = 0)
    {
        std::unique_lock<std::mutex> lk(waiting_resp_queue_mutex_);
        auto& queue = waiting_handle_response_;
        auto check = [&]
        {
            for (auto& i : queue)
            {
                if (i.id == id)
                {
                    return true;
                }
            }
            return false;
        };
        if (check())
        {
            return GetTargetResp(id);
        }
        if (timeout_ms != 0)
        {
            waiting_resp_cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms), check);
            if (!check())
            {
                Throw<AsyncAwaitTimeout>("await timeout");
            }
        }
        else
        {
            waiting_resp_cv_.wait(lk, check);
        }
        return GetTargetResp(id);
    }

    void Start()
    {
        check<std::logic_error>(!running_, "一个线程池实例只能start一次");
        for (size_t i = 0; i < thread_num_; i++)
        {
            threads_[i] = std::thread([&, i] { AsyncLoop(i); });
        }
        running_ = true;
        if (is_block_)
        {
            for (auto& i : threads_)
            {
                i.second.join();
            }
        }
        else
        {
            for (auto& i : threads_)
            {
                i.second.detach();
            }
        }
    }

    auto Run(std::function<Request()> fn)
    {
        auto id = ++incr_id;
        {
            std::lock_guard<std::mutex> lock(m_);
            queue_.push(AsyncPackage{.request = fn(), .id = id, .received_mode = AsyncResponseReceiveMode::queue});
        }
        cv_.notify_one();
        return id;
    }

    auto Run(std::function<Request()> fn, std::function<void(AsyncResponse)> cb)
    {
        auto id = ++incr_id;
        {
            std::lock_guard<std::mutex> lock(m_);
            queue_.push(AsyncPackage{.callback = cb, .request = fn(), .received_mode = AsyncResponseReceiveMode::callback});
        }
        cv_.notify_one();
        return id;
    }

    auto GetAvailableResponse()
    {
        std::lock_guard<std::mutex> m(waiting_resp_queue_mutex_);
        auto available_resp_queue = waiting_handle_response_;
        waiting_handle_response_ = {};
        return available_resp_queue;
    }

  private:
    void AsyncLoop(int id)
    {
        AsyncPackage pkg;
        while (!stopped_.load())
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [&] { return !queue_.empty() || stopped_.load(); });
            if (stopped_.load())
            {
                break;
            }
            pkg = queue_.front();
            queue_.pop();
            lk.unlock(); // 提前解锁，不等析构再解，防止notify_one拉起后又马上阻塞
            cv_.notify_one();
            Response resp;
            String err_msg;
            if (stopped_.load())
            {
                break;
            }
            try
            {
                resp = pkg.request.Send();
            }
            catch (const std::exception& e)
            {
                err_msg = e.what();
            }
            if (stopped_.load())
            {
                break;
            }
            AsyncResponse resp_pkg{.id = pkg.id, .resp = resp, .err_msg = err_msg};
            if (pkg.received_mode == AsyncResponseReceiveMode::queue)
            {
                std::lock_guard<std::mutex> m(waiting_resp_queue_mutex_);
                waiting_handle_response_.push_back(resp_pkg);
                waiting_resp_cv_.notify_all();
            }
            else if (pkg.received_mode == AsyncResponseReceiveMode::callback)
            {
                pkg.callback(resp_pkg);
            }
        }
        {
            std::lock_guard<std::mutex> m(m_);
            threads_.erase(id);
            cv_.notify_one();
        }
    }
};

} // namespace sion
