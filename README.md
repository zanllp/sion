# Sion
* Sion是一个轻量级的c++ http客户端，仅单头文件450行，自带std::string的扩展
* Sion is a lightweight C + + HTTP Client, with only one header file 450 lines, with its own std::string extension. 
* Sion由Myhttp(暂未取名)删除服务器相关部分代码而来，仅能作为HttpClient使用。仅支持http协议，本来是打算加入https的，发现加入后根本不能单头文件就实现支持http,https的client。https，ws还是放在体积大点的myHttp吧。
### 例子
~~~cpp
#include<iostream>
#include"Sion.h"

MyString PostTest()
{
	Sion::Request request;
	request.Header["Content-Type"] = "application/json; charset=utf-8";
	request.RequestBody = R"({"id":null,"password":"zanllp_pw","account":"zanllp"})";
	Sion::Response response = request.SendRequest(Sion::Post, "http://127.0.0.1:5000/api/auth");
	response.ParseFromSource();
	return response.ResponseBody;
}

MyString ChunkedTest()
{
	Sion::Response response = Sion::Request::StaticRequest(Sion::Get, "http://zanllp.cn");
	response.ParseFromSource(true);
	return response.ResponseBody;
}

void ShowHeader()
{
	Sion::Request request;
	Sion::Response response=request.SendRequest(Sion::Get, "http://www.baidu.com");
	response.ParseFromSource(true);
	for (auto x : response.Header)
	{
		std::cout << x.first << "   " << x.second << std::endl;
	}
}

int main()
{
	try
	{
		ShowHeader();
		std::cout << ChunkedTest() <<std::endl;
		std::cout << PostTest() << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	system("pause");
}
~~~
## MyString
该类继承std::string，用法基本一致，拓展了几个函数

~~~cpp

//使用字符串分割
//flag 分割标志,返回的字符串向量会剔除,flag不要用char，会重载不明确
//num 分割次数，默认0即分割到结束，例num=1,返回开头到flag,flag到结束size=2的字符串向量
//skipEmpty 跳过空字符串，即不压入length==0的字符串
std::vector<MyString> Split(MyString flag, int num = 0, bool skipEmpty = true)

//清除前后的字符
//target 需要清除的字符默认空格
MyString Trim(char target = ' ')

//包含字母
bool HasLetter()

//转换到gbk 中文显示乱码调用这个
MyString ToGbk()

//返回搜索到的所有位置
//flag 定位标志
//num 搜索数量，默认直到结束
std::vector<int> FindAll(MyString flag, int num = -1)

//字符串替换
MyString& Replace(MyString oldStr, MyString newStr)
~~~

## Response
该类用来处理请求响应
~~~cpp
//解析服务器发送过来的响应 获得响应源后需手动调用一次
MyString ParseFromSource(bool ConverToGbk = false)
~~~
## Request
该类用来处理发送请求
~~~cpp
//设置请求方法 
void SetHttpMethod(MethodEnum method, MyString other = "")

//发送请求
MyString SendRequest(MyString url)
MyString SendRequest(MethodEnum method, MyString url)
MyString static StaticRequest(MethodEnum method, MyString url)
~~~
