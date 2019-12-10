# Sion
* Sion是一个轻量级简单易用的c++ http客户端
* 仅单头文件500行，自带std::string的扩展
* 支持http,https请求。https需要安装openssl(推荐使用[vcpkg](https://github.com/microsoft/vcpkg)),如果不需要https 可以使用 #define SION_DISABLE_SSL 关闭
### 例子
~~~cpp
#include <iostream>
// #define SION_DISABLE_SSL
#include "Sion.h"
#include <ppltasks.h>
using namespace Sion;
using namespace std;
int main()
{
	// 使用ppl实现异步操作，sion内为阻塞io
	concurrency::create_task([]() {
		try
		{
			return Fetch("https://api.zanllp.cn/plugin");
		}
		catch (const std::exception & e)
		{
			cout << e.what();
		}
		return Response();
	}).then([](Response resp) {
		cout << resp.ResponseBody << endl;
	}).then([]() {
		try
		{
			return Request()
				.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
				.SetHttpMethod(Method::Post)
				.SetBody(R"({"data": 233333,"msg":"hello world!"})")
				.SetHeader("Content-Type", "application/json; charset=utf-8")
				.Send();
		}
		catch (const std::exception & e)
		{
			cout << e.what();
		}
		return Response();
	}).then([](Response resp) {
			cout << resp.ResponseBody << endl;
	}).then([] {
		exit(0);
	});
	this_thread::sleep_for(10s);
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
//支持链式设置属性
Request& SetHttpMethod(MyString other) ;
Request& SetUrl(MyString url);
Request& SetCookie(MyString cookie);
Request& SetBody(MyString body);
Request& SetHeader(vector<pair<MyString, MyString>> header);
Request& SetHeader(MyString k, MyString v);
//发送请求
Response Send(MyString url);
Response Send(Method method, MyString url);
~~~
## Fetch
~~~cpp
// 静态请求方法
Response Fetch(MyString url, Method method = Get, vector<pair<MyString, MyString>> header = {}, MyString body = "");
~~~
