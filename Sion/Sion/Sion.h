#pragma once
#pragma warning(disable : 4267)
#pragma warning(disable : 4018)
#pragma warning(disable : 6031)
#include <winsock2.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include <map>
#include <regex>
#pragma comment(lib, "ws2_32.lib") //2

namespace Sion
{

	using namespace std;

	void Throw(string msg)
	{
		throw std::exception(msg.c_str());
	}

	void check(bool condition, const char* msg = "")
	{
		if (!condition)
		{
			Throw(msg);
		}
	}

	class MyString : public string
	{
	public:
		MyString() = default;
		~MyString() = default;
		template<class T>
		MyString(T&& arg) :string(forward<T>(arg))
		{

		}
		//使用字符串分割
		//flag 分割标志,返回的字符串向量会剔除,flag不要用char，会重载不明确
		//num 分割次数，默认0即分割到结束，例num=1,返回开头到flag,flag到结束size=2的字符串向量
		//skipEmpty 跳过空字符串，即不压入length==0的字符串
		vector<MyString> Split(MyString flag, int num = 0, bool skipEmpty = true)
		{
			vector<MyString> dataSet;
			auto PushData = [&dataSet, skipEmpty](MyString line) {
				if (line.length() != 0 || !skipEmpty)
				{
					dataSet.push_back(line);
				}
			};
			auto Pos = FindAll(flag, num != 0 ? num : -1);
			if (!Pos.size())
			{
				return dataSet;
			}
			for (int i = 0; i < Pos.size(); i++)
			{
				if (dataSet.size() == num && num != 0)
				{ //满足数量直接截到结束
					if (num == Pos.size())
					{
						PushData(substr(*(--Pos.end()) + flag.size()));
					}
					else
					{
						PushData(substr(Pos[dataSet.size()] + flag.size()));
					}
					break;
				}
				if (i == 0 && Pos[0] != 0)
				{ //第一个数的位置不是0的话补上
					PushData(substr(0, Pos[0]));
				}
				if (i != Pos.size() - 1)
				{
					int Left = Pos[i] + flag.length();
					int Right = Pos[i + 1] - Left;
					PushData(substr(Left, Right));
				}
				else
				{ //最后一个标志到结束
					PushData(substr(*(--Pos.end()) + flag.size()));
				}
			}
			return dataSet;
		}

		//清除前后的字符
		//target 需要清除的字符默认空格
		MyString Trim(char target = ' ')
		{
			auto left = find_first_not_of(target);
			if (left == string::npos)
			{
				return *this;
			}
			auto right = find_last_not_of(target);
			return substr(left, right - left + 1);
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

		//包含字母
		bool HasLetter()
		{
			for (auto x : *this)
			{
				if ((x >= 'a' && x <= 'z') ||
					(x >= 'A' && x <= 'Z'))
				{
					return true;
				}
			}
			return false;
		}

		//转换到gbk 中文显示乱码调用这个
		MyString ToGbk()
		{
			//由blog.csdn.net/u012234115/article/details/83186386 改过来
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
			if (wszGBK)
				delete[] wszGBK;
			if (szGBK)
				delete[] szGBK;
			return result;
		}

		//返回搜索到的所有位置
		//flag 定位标志
		//num 搜索数量，默认直到结束
		vector<int> FindAll(MyString flag, int num = -1)
		{
			vector<int> Result;
			auto Pos = find(flag);
			while (Pos != -1 && Result.size() != num)
			{
				Result.push_back(Pos);
				Pos = find(flag, *(--Result.end()) + 1);
			}
			return Result;
		}

		//字符串替换
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

	using Socket = SOCKET;

	enum MethodEnum
	{
		Get,
		Post,
		Put,
		Delete
	};

	MyString GetIpByHost(MyString hostname)
	{
		addrinfo hints, *res;
		in_addr addr;
		int err;
		memset(&hints, 0, sizeof(addrinfo));
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_family = AF_INET; //ipv4
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

	Socket GetSocket() //http请求不需要填端口
	{
		//初始化。,WSA windows异步套接字
		WSADATA inet_WsaData;					   //
		WSAStartup(MAKEWORD(2, 0), &inet_WsaData); //socket2.0版本
		if (LOBYTE(inet_WsaData.wVersion) != 2 || HIBYTE(inet_WsaData.wVersion) != 0)
		{ //高位字节指明副版本、低位字节指明主版本
			WSACleanup();
			return -1;
		}
		auto tcp_socket = socket(AF_INET, SOCK_STREAM, 0); //ipv4,tcp,tcp或udp该参数可为0
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
		void Add(pair<MyString, MyString> kv)
		{
			data.push_back(kv);
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
	public:
		// 响应报文
		MyString Source;
		MyString Cookie;
		MyString ProtocolVersion = "HTTP/1.1";
		MyString Code = "200";
		MyString Status = "ok";
		MyString Server = "CppTinyServer";
		MyString TransferEncoding;
		int ContentLength = 0;
		bool IsChunked = false;
		Header ResponseHeader;
		MyString ResponseBody;
		Response() = default;
		~Response() = default;

		Response(MyString source)
		{
#ifndef UNICODE
			Source = source.ToGbk();
#else
			Source = source;
#endif // !UNICODE
			ParseFromSource();
		}

		//解析服务器发送过来的响应
		void ParseFromSource()
		{
			auto ThrowError = [&] {
				MyString Msg = "解析错误\n" + Source;
				Throw(Msg.c_str());
			};
			auto HeaderStr = Source.substr(0, Source.find("\r\n\r\n"));
			auto data = MyString(HeaderStr).Split("\r\n");
			if (data.size() == 0)
			{
				return;
			}
			auto FirstLine = data[0].Split(" ", 2);
			if (FirstLine.size() != 3)
			{
				ThrowError();
			}
			ProtocolVersion = FirstLine[0].Trim();
			Code = FirstLine[1].Trim();
			Status = FirstLine[2].Trim();
			data.erase(data.begin());
			for (auto x : data)
			{
				auto pair = x.Split(":", 1);
				if (pair.size() == 2)
				{
					ResponseHeader.Add({ pair[0].Trim().ToLowerCase(),pair[1].Trim() });
				}
			}
			auto HeaderValue = [&](MyString k) { return ResponseHeader.GetLastValue(k); };
			MyString contentLen = HeaderValue("content-length");
			ContentLength = contentLen != "" ? stoi(contentLen) : ContentLength;
			Cookie = HeaderValue("cookie");
			Server = HeaderValue("server");
			TransferEncoding = HeaderValue("transfer-encoding");
			IsChunked = ContentLength == 0;
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
	};



	class Request
	{
	public:
		int port = 80;
		MyString Source;
		MyString Method;
		MyString Path;
		MyString ProtocolVersion = "HTTP/1.1";
		MyString IP;
		MyString Host;
		MyString Cookie;
		MyString RequestBody;
		Header RequestHeader;
		Request() = default;
		~Request() = default;

		void SetHttpMethod(MethodEnum method)
		{
			switch (method)
			{
			case MethodEnum::Get:
				Method = "GET";
				break;
			case MethodEnum::Post:
				Method = "POST";
				break;
			case MethodEnum::Put:
				Method = "PUT";
				break;
			case MethodEnum::Delete:
				Method = "DELETE";
				break;
			default:
				break;
			}
		}

		void SetHttpMethod(MyString other)
		{
			Method = other;
		}

		Response SendRequest(MyString url)
		{
			check(Method.length() != 0);
			regex urlParse(R"(^(http)://([\w.]*):?(\d*)(/?.*)$)");
			smatch m;
			regex_match(url, m, urlParse);
			check(m.size() == 5, "url格式不对或者是用了除http外的协议");
			Host = m[2];
			port = m[3].length() == 0 ? 80 : stoi(m[3]);
			Path = m[4].length() == 0 ? "/" : m[4].str();
			Socket socket = GetSocket();
			Connection(socket, Host);
			BuildRequestString();
			send(socket, Source.c_str(), int(Source.length()), 0);
			return Response(ReadResponse(socket));
		}

		Response SendRequest(MethodEnum method, MyString url)
		{
			SetHttpMethod(method);
			return SendRequest(url);
		}
	private:
		void BuildRequestString()
		{
			if (Cookie.length())
			{
				RequestHeader.Add({ "Cookie",Cookie });
			}
			RequestHeader.Add({ "Host",Host });
			if (RequestBody != "")
			{
				RequestHeader.Add({ "Content-Length",to_string(RequestBody.length()) });
			}
			Source = Method + " " + Path + " " + ProtocolVersion + "\r\n";
			for (auto x : RequestHeader.data)
			{
				Source += x.first + ":" + x.second + "\r\n";
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
			if (::connect(socket, (sockaddr*)& saddr, sizeof(saddr)) != 0)
			{
				Throw("连接失败错误码：" + to_string(WSAGetLastError()));
			}
		}

		MyString ReadResponse(Socket socket)
		{
			char buf[1024] = { 0 };
			Response resp;
			recv(socket, buf, sizeof(buf) - 1, 0);
			resp.Source += buf;
			resp.ParseFromSource();
			auto lenHeader = resp.Source.length() - resp.ResponseBody.length(); //响应头长度
			while (true)
			{ //接收到的字符少于缓冲区长度，接收结束,也有可能是错误或者连接关闭
				if (resp.IsChunked)
				{
					if (resp.Source.substr(resp.Source.length() - 7) == "\r\n0\r\n\r\n")
					{
						break;
					}
				}
				else
				{
					if (resp.Source.length() - lenHeader == resp.ContentLength)
					{
						break;
					}
				}
				memset(buf, 0, sizeof(buf));
				if (int num = recv(socket, buf, sizeof(buf) - 1, 0) < 0)
				{
					Throw("网络异常,Socket错误码：" + to_string(num));
				}
				resp.Source += buf;
			}
			closesocket(socket);
			return resp.Source;
		}
	};

	Response Fetch(MyString url, MethodEnum method = Get)
	{
		Request request;
		request.SetHttpMethod(method);
		return Response(request.SendRequest(url));
	}
} // namespace Sion