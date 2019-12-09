# Sion
* Sion是一个轻量级简单易用的c++ http客户端，仅单头文件450行，自带std::string的扩展
* Sion仅支持http，https需要openssl，不大可能单文件实现，ssl版本移步[MyHttpClient](https://github.com/zanllp/MyHttpClient)，暂不支持长连接，职业前端不定时手痒填坑。
### 例子
~~~cpp
#include<iostream>
#define SION_DISABLE_SSL 
#include"Sion.h"
using namespace Sion;
using namespace std;
int main()
{

	try
	{
		// Get
		cout << Fetch("http://www.baidu.com").Source << endl;;
		// Post
		auto resp = Request()
			.SetUrl("http://127.0.0.1:7001/user")
			.SetHttpMethod(Post)
			.SetBody(R"({"account":"zanllp","password":"zanllp_pw"})")
			.SetCookie("csrfToken=4CUb9Rjk0dgRXZRZorAqbTm8")
			.SetHeader("Content-Type", "application/json; charset=utf-8")
			.SetHeader("x-csrf-token", "4CUb9Rjk0dgRXZRZorAqbTm8")
			.SendRequest();
		cout << resp.ResponseBody << endl;
	}
	catch (const std::exception & e)
	{
		cout << e.what();
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
std::vector<MyString> Split(MyString flag, int num = 0, bool skipEmpty = true);

//清除前后的字符
//target 需要清除的字符默认空格
MyString Trim(MyString target = " ");

//包含字母
bool HasLetter();

//转换到gbk 中文显示乱码调用这个
MyString ToGbk();

//返回搜索到的所有位置
//flag 定位标志
//num 搜索数量，默认直到结束
std::vector<int> FindAll(MyString flag, int num = -1);

//字符串替换
MyString& Replace(MyString oldStr, MyString newStr);
~~~

## Response
该类用来处理请求响应
## Request
该类用来处理发送请求
~~~cpp
//设置请求方法 
void SetHttpMethod(Method method);
void SetHttpMethod(MyString other);

//发送请求
Response SendRequest(MyString url);
Response SendRequest(Method method, MyString url);
~~~
## Fetch
~~~cpp
// 静态请求方法
Response Fetch(MyString url, Method method = Get, vector<pair<MyString, MyString>> header = {}, MyString body = "");
~~~
