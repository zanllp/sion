#pragma once
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
#ifdef _WIN32
#include <WS2tcpip.h>
#include <winsock2.h>
#include <Windows.h>
#pragma comment(lib, "ws2_32.lib") 
#else _linux
#endif // WIN32 
#ifndef SION_DISABLE_SSL 
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif // !SION_DISABLE_SSL 
namespace Sion
{

	using std::string;
	using std::pair;
	using std::vector;
	using std::array;

	class MyString : public string
	{
	public:
		MyString() = default;
		~MyString() = default;
		template<class T>
		MyString(T&& arg) :string(std::forward<T>(arg)) { }

		// 使用字符串分割
		// flag 分割标志,返回的字符串向量会剔除,flag不要用char，会重载不明确
		// num 分割次数，默认0即分割到结束，例num=1,返回开头到flag,flag到结束size=2的字符串向量
		// skipEmpty 跳过空字符串，即不压入length==0的字符串
		vector<MyString> Split(MyString flag, int num = 0, bool skipEmpty = true)
		{
			vector<MyString> dataSet;
			auto PushData = [&](MyString line)
			{
				if (line.length() != 0 || !skipEmpty)
				{
					dataSet.push_back(line);
				}
			};
			auto Pos = FindAll(flag, num != 0 ? num : -1);
			if (Pos.size() == 0) { return { *this }; }
			for (int i = 0; i < Pos.size() + 1; i++)
			{
				if (dataSet.size() == num && Pos.size() > num&& num != 0)
				{	// 满足数量直接截到结束
					PushData(substr(Pos[dataSet.size()] + flag.size()));
					break;
				}
				if (i == 0)
				{	// 第一个数的位置不是0的话补上
					PushData(substr(0, Pos[0]));
				}
				else if (i != Pos.size())
				{
					int Left = Pos[i - 1] + flag.length();
					int Right = Pos[i] - Left;
					PushData(substr(Left, Right));
				}
				else
				{	// 最后一个标志到结束
					PushData(substr(*(--Pos.end()) + flag.size()));
				}
			}
			return dataSet;
		}

		// 清除前后的字符
		// target 需要清除的字符默认空格
		MyString Trim(MyString target = " ")
		{
			auto left = find_first_not_of(target);
			if (left == string::npos) { return *this; }
			auto right = find_last_not_of(target);
			return substr(left, right - left + target.length());
		}

		MyString ToLowerCase()
		{
			MyString res = *this;
			for (auto& i : res)
			{
				if (i >= 'A' && i <= 'Z')
				{
					i += 'a' - 'A';
				}
			}
			return res;
		}

		MyString ToUpperCase()
		{
			MyString res = *this;
			for (auto& i : res)
			{
				if (i >= 'a' && i <= 'z')
				{
					i -= 'a' - 'A';
				}
			}
			return res;
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

		// 转换到gbk 中文显示乱码调用这个
		MyString ToGbk()
		{
#ifdef _WIN32
			// 由blog.csdn.net/u012234115/article/details/83186386 改过来
			auto src_str = c_str();
			int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
			wchar_t* wszGBK = new wchar_t[len + 1];
			memset(wszGBK, 0, len * 2 + 2);
			MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);
			len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
			char* szGBK = new char[len + 1];
			memset(szGBK, 0, len + 1);
			WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);
			MyString result(szGBK);
			if (wszGBK) delete[] wszGBK;
			if (szGBK) delete[] szGBK;
			return result;
#else
#endif // _WIN32
		}

		// 返回搜索到的所有位置
		// flag 定位标志
		// num 搜索数量，默认直到结束
		vector<int> FindAll(MyString flag, int num = -1)
		{
			vector<int> Result;
			auto Pos = find(flag);
			while (Pos != -1 && Result.size() != num)
			{
				Result.push_back(Pos);
				Pos = find(flag, *(--Result.end()) + flag.length());
			}
			return Result;
		}

		// 字符串替换
		MyString& Replace(MyString oldStr, MyString newStr)
		{
			int pos = find(oldStr);
			if (pos != -1)
			{
				replace(pos, oldStr.length(), newStr);
			}
			return *this;
		}
	};

	void Throw(MyString msg = "") { throw std::exception(msg.c_str()); }

	void check(bool condition, MyString msg = "")
	{
		if (!condition)
		{
			Throw(msg);
		}
	}
	using Socket = SOCKET;

	enum class Method { Get, Post, Put, Delete };

	MyString GetIpByHost(MyString hostname)
	{
		addrinfo hints, * res;
		in_addr addr;
		int err;
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET; // ipv4
		if ((err = getaddrinfo(hostname.c_str(), NULL, &hints, &res)) != 0)
		{
			Throw("错误" + err + MyString(gai_strerror(err)));
		}
		addr.s_addr = ((sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
		char str[INET_ADDRSTRLEN];
		auto ptr = inet_ntop(AF_INET, &addr, str, sizeof(str));
		freeaddrinfo(res);
		return str;
	}

	Socket GetSocket()
	{
		// 初始化。,WSA windows异步套接字
		WSADATA inet_WsaData;					   //
		WSAStartup(MAKEWORD(2, 0), &inet_WsaData); // socket2.0版本
		if (LOBYTE(inet_WsaData.wVersion) != 2 || HIBYTE(inet_WsaData.wVersion) != 0)
		{	// 高位字节指明副版本、低位字节指明主版本
			WSACleanup();
			return -1;
		}
		auto tcp_socket = socket(AF_INET, SOCK_STREAM, 0); // ipv4,tcp,tcp或udp该参数可为0
		return tcp_socket;
	}

	class Header
	{
	public:
		Header() = default;
		~Header() = default;
		vector<pair<MyString, MyString>> data;
		Header& operator=(vector<pair<MyString, MyString>>&& other) noexcept
		{
			data = other;
			return *this;
		}
		void Add(MyString k, MyString v)
		{
			data.push_back({ k,v });
		}
		vector<MyString> GetValue(MyString key)
		{
			vector<MyString> res;
			for (auto& i : data)
			{
				if (i.first == key)
				{
					res.push_back(i.second);
				}
			}
			return res;
		}
		MyString GetLastValue(MyString key)
		{
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

	class Response
	{
	private:
		
	public:
		bool IsChunked = false;
		bool SaveByChar = false; // 对于文本直接用字符串保存，其它用char数组
		int ContentLength = 0;
		MyString Source; // 响应报文
		MyString Cookie;
		MyString ProtocolVersion;
		MyString Code;
		MyString Status;
		MyString ResponseBody;
		vector<char> SourceChar;
		Header ResponseHeader;
		Response() = default;
		~Response() = default;
		MyString CharSet = "utf-8";

#ifndef UNICODE
		Response(MyString source, bool convert2gbk = true) noexcept
		{
			if (convert2gbk)
			{
				CharSet = "gbk";
				Source = source.ToGbk(); 
			} 
			else
			{
				Source = source;
			}
			ParseFromSource();
		}
#else
		Response(MyString source) noexcept
		{
			Source = source; 
			ParseFromSource();
		}
#endif // !UNICODE
		MyString HeaderValue(MyString k) { return ResponseHeader.GetLastValue(k.ToLowerCase()); };

		// 解析服务器发送过来的响应
		// PreParse 预解析，如果为true且使用char保存而且不是chunked编码那么只解析头部
		void ParseFromSource(bool PreParse = false)
		{
			auto HeaderStr = Source.substr(0, Source.find("\r\n\r\n"));
			auto data = MyString(HeaderStr).Split("\r\n");
			if (data.size() == 0) { return; }
			// 第一行
			auto FirstLine = data[0].Split(" ", 2);
			check(FirstLine.size() == 3, "解析错误\n" + Source);
			ProtocolVersion = FirstLine[0].Trim();
			Code = FirstLine[1].Trim();
			Status = FirstLine[2].Trim();
			data.erase(data.begin());
			// 头
			for (auto x : data)
			{
				auto pair = x.Split(":", 1);
				if (pair.size() == 2)
				{
					ResponseHeader.Add(pair[0].Trim().ToLowerCase(), pair[1].Trim());
				}
			}
			MyString contentLen = HeaderValue("content-length");
			ContentLength = contentLen != "" ? stoi(contentLen) : ContentLength;
			Cookie = HeaderValue("cookie");
			IsChunked = ContentLength == 0;
			auto ContentType = HeaderValue("content-type");
			if (ContentType != "")
			{	// Content-Type: text/html; charset=utf-8
				auto ContentSplit = ContentType.Split(";", 1);
				auto type = ContentSplit[0].Split("/", 1)[0].Trim();
				SaveByChar = type != "text" && type != "application"; // 解析看是文本还字节流
				if (ContentSplit.size() != 1 && CharSet != "gbk") // 是gbk的话已经转过一次了，不用再管
				{
					CharSet = ContentSplit[1].Split("=", 1)[1].Trim();
				}
				if (PreParse && SaveByChar && !IsChunked) { return; }
			}
			// 响应体
			if (SaveByChar)
			{
				if (SourceChar.size() == 0) { return; }
				const auto& sc = SourceChar;
				vector<char> PureSouceChar;
				// 获取下一个\r\n的位置
				int NRpos = 0;
				auto GetNextNR = [&](int leap = 0)
				{
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
				int Right = GetNextNR();
				while (Left != -1 && Right != -1)
				{
					auto count = string(sc.begin() + 2 + Left, sc.begin() + Right); // 每个分块开头写的数量
					if (count.length() == 0) { break; } // 最后一个 0\r\n\r\n，退出
					auto countNum = stoi(count, nullptr, 16); // 那数量是16进制
					auto chunkedStart = sc.begin() + Right + 2; // 每个分块正文的开始位置
					PureSouceChar.insert(PureSouceChar.end(), chunkedStart, chunkedStart + countNum);
					Left = GetNextNR(countNum); //  更新位置
					Right = GetNextNR(1);
				}
				SourceChar = PureSouceChar;
			}
			else
			{
				auto bodyPos = Source.find("\r\n\r\n");
				if (bodyPos != -1 && bodyPos != Source.length() - 4)
				{
					ResponseBody = Source.substr(bodyPos + 4);
					if (IsChunked)
					{
						auto Pos = ResponseBody.FindAll("\r\n");
						MyString ResponseBodyClear;
						for (int i = 0; i < (Pos.size() - 1) / 2; i++)
						{
							int Left = Pos[i * 2] + 2;
							int Right = Pos[i * 2 + 1] - Left;
							ResponseBodyClear += ResponseBody.substr(Left, Right);
						};
						ResponseBody = ResponseBodyClear;
					}
				}
			}
		}
	};

	class Request
	{
	public:
		int port = 80;
		MyString Source;
		MyString Method;
		MyString Path;
		MyString Protocol;
		MyString IP;
		MyString Url;
		MyString Host;
		MyString Cookie;
		MyString RequestBody;
		MyString ProtocolVersion = "HTTP/1.1";
		Header RequestHeader;
		Request() = default;
		~Request() = default;

		Request& SetHttpMethod(Sion::Method method)
		{
			switch (method)
			{
			case Method::Get: Method = "GET"; break;
			case Method::Post: Method = "POST"; break;
			case Method::Put: Method = "PUT"; break;
			case Method::Delete: Method = "DELETE"; break;
			}
			return *this;
		}

		Request& SetHttpMethod(MyString other) { Method = other; return *this; }

		Request& SetUrl(MyString url) { Url = url; return *this; }

		Request& SetCookie(MyString cookie) { Cookie = cookie; return *this; }

		Request& SetBody(MyString body) { RequestBody = body; return *this; }

		Request& SetHeader(vector<pair<MyString, MyString>> header) { RequestHeader.data = header; return *this; }

		Request& SetHeader(MyString k, MyString v) { RequestHeader.Add(k, v); return *this; }

		Response Send(Sion::Method method, MyString url) { SetHttpMethod(method); return Send(url); }

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
		Response Send(MyString url)
		{
			check(Method.length(), "请求方法未定义");
			std::smatch m;
#ifndef SION_DISABLE_SSL 
			std::regex urlParse(R"(^(http|https)://([\w.]*):?(\d*)(/?.*)$)");
			regex_match(url, m, urlParse);
			check(m.size() == 5, "url格式不对或者是用了除http,https外的协议");
			Protocol = m[1];
			port = m[3].length() == 0 ? (Protocol == "http" ? 80 : 443) : stoi(m[3]);
#else
			std::regex urlParse(R"(^(http)://([\w.]*):?(\d*)(/?.*)$)");
			regex_match(url, m, urlParse);
			check(m.size() == 5, "url格式不对或者是用了除http外的协议");
			Protocol = m[1];
			port = m[3].length() == 0 ? 80 : stoi(m[3]);
#endif
			Host = m[2];
			Path = m[4].length() == 0 ? "/" : m[4].str();
			Socket socket = GetSocket();
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

	private:
		void BuildRequestString()
		{
			if (Cookie.length())
			{
				RequestHeader.Add("Cookie", Cookie);
			}
			RequestHeader.Add("Host", Host);
			if (RequestBody.length())
			{
				RequestHeader.Add("Content-Length", std::to_string(RequestBody.length()));
			}
			Source = Method + " " + Path + " " + ProtocolVersion + "\r\n";
			for (auto x : RequestHeader.data)
			{
				Source += x.first + ": " + x.second + "\r\n";
			}
			Source += "\r\n";
			Source += RequestBody;
		}

		void Connection(Socket socket, MyString host)
		{
			in_addr sa;
			IP = host.HasLetter() ? GetIpByHost(host) : host;
#ifdef UNICODE
			WCHAR wcIP[256];
			memset(wcIP, 0, sizeof(wcIP));
			MultiByteToWideChar(CP_ACP, 0, IP.c_str(), IP.length() + 1, wcIP, sizeof(wcIP) / sizeof(wcIP[0]));
			check(InetPton(AF_INET, wcIP, &sa) != -1, "地址转换错误");
#else
			check((InetPton(AF_INET, IP.c_str(), &sa) != -1), "地址转换错误");
#endif
			sockaddr_in saddr;
			saddr.sin_family = AF_INET;
			saddr.sin_port = htons(port);
			saddr.sin_addr = sa;
			if (::connect(socket, (sockaddr*)&saddr, sizeof(saddr)) != 0)
			{
				Throw("连接失败错误码：" + std::to_string(WSAGetLastError()));
			}
		}
#ifndef SION_DISABLE_SSL 
		Response ReadResponse(Socket socket, SSL* ssl = nullptr)
#else
		Response ReadResponse(Socket socket)
#endif
		{
			const int bufSize = 1024;
			array<char, bufSize> buf{ 0 };
			auto Read = [&]()
			{
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
				check(status >= 0, "网络异常,Socket错误码：" + std::to_string(status));
				return status;
			};
			Response resp;
			// 读取解析头部信息
			Read();
			resp.Source += buf.data();
			resp.ParseFromSource(true);
			if (resp.SaveByChar)
			{	// 把除头外多余的响应体部分移过去
				auto bodyPos = resp.Source.find("\r\n\r\n");
				auto startBody = buf.begin() + 4 + bodyPos;
				resp.SourceChar.insert(resp.SourceChar.end(), startBody, --buf.end());
			}
			auto lenHeader = resp.Source.length() - resp.ResponseBody.length(); // 响应头长度
			// 检查是否接收完
			auto CheckEnd = [&]
			{
				if (resp.SaveByChar)
				{
					if (resp.IsChunked)
					{
						auto start = resp.SourceChar.begin() + resp.SourceChar.size() - 7;
						return string(start, start + 7) == "\r\n0\r\n\r\n";
					}
					else
					{
						return resp.SourceChar.size() == resp.ContentLength;
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
				auto num = Read();
				if (resp.SaveByChar)
				{
					resp.SourceChar.insert(resp.SourceChar.end(), buf.begin(), buf.begin() + num);
				}
				else
				{
					resp.Source += buf.data();
				}
			}
			closesocket(socket);
			if (resp.SaveByChar && resp.IsChunked)
			{	// 字节流且是分块编码的清除下
				resp.ParseFromSource();
			}
			return resp.SaveByChar ? resp : Response(resp.Source); // 如果是字节流直接移动不需要转码，不是返回源让构造器再转码解析一次
		}
	};

	Response Fetch(MyString url, Method method = Method::Get, vector<pair<MyString, MyString>> header = {}, MyString body = "")
	{
		return Request().SetUrl(url).SetHttpMethod(method).SetHeader(header).SetBody(body).Send();
	}
} // namespace Sion