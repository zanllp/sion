# Sion
## 特性
* Sion是一个轻量级简单易用的c++ http客户端
* 仅单头文件700行，自带std::string的扩展
* 跨平台，支持linux，win, mac...
* 支持文本及二进制的响应体
* 支持分块(chunked)的传输编码
* 支持http,https请求。_https需要安装openssl(推荐使用[vcpkg](https://github.com/microsoft/vcpkg)),如果不需要可以使用 #define SION_DISABLE_SSL 关闭_
## 用法
直接复制[sion.h](src/sion.h)到自己的项目下`include`
### 最普通的GET请求
```cpp
auto resp = Fetch("https://api.zanllp.cn/plugin");
cout << resp.Body() << endl;
cout << resp.HeaderValue("content-type") <<endl;
```
### 链式调用及POST请求
```CPP
auto resp = Request()
			.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
			.SetHttpMethod(Method::Post)
			.SetBody(R"({"data": 233333,"msg":"hello world!"})")
			.SetHeader("Content-Type", "application/json; charset=utf-8")
			.Send();
```
### 使用Sion下载文件
Sion对于Content-Type头是text和application的报文的响应体会使用字符串保存，通过Body()获取。

例如这些头
* Content-Type: text/html; charset=utf-8
* content-type: application/javascript
* Content-Type: application/json; charset=utf-8

而对于其它的默认保存在BodyBin()中,具体保存在哪可以可以通过Response::SaveByVec判断

例如这些头
* content-type: image/vnd.microsoft.icon
* content-type: image/gif
* content-type: image/svg+xml
```cpp
auto resp = Fetch("https://static.zanllp.cn/94da3a6b32e0ddcad844aca6a8876da2ecba8cb3c7094c3ad10996b28311e4b50ab455ee3d6d55fb50dc4e3c62224f4a20a4ddb1.gif");
ofstream file(R"(滑稽.gif)", ios::binary);
auto& bin = resp.BodyBin();
file.write(bin.data(), bin.size());

// 当然也支持分块编码传输的
auto resp = sion::Fetch("http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx");
std::ofstream file(R"(分块.jpeg)", std::ios::binary);
auto& bin = resp.BodyBin();
file.write(bin.data(), bin.size());
```
### 异步请求
SION是cpp客户端所使用的，在之前用过c#的HttpClient,py的request,js的Fetch，所以会受到这几种的影响，在写之前决定使用阻塞io+异步库的方式。具体怎么搞看[例子](./Sion/Sion/源.cpp)

### [其它](./Sion/Sion/源.cpp)


# Doc
## Fetch
~~~cpp
// 静态请求方法
Response Fetch(String url, Method method = Get, vector<pair<String, String>> header = {}, String body = "");
~~~

## String
该类继承std::string，用法基本一致，拓展了几个函数

~~~cpp

//使用字符串分割
//flag 分割标志,返回的字符串向量会剔除,flag不要用char，会重载不明确
//num 分割次数，默认0即分割到结束，例num=1,返回开头到flag,flag到结束size=2的字符串向量
//skip_empty 跳过空字符串，即不压入length==0的字符串
std::vector<String> Split(String flag, int num = -1, bool skip_empty = true);

//清除前后的字符
//target 需要清除的字符默认空格空格姬换行符
String Trim(String target = " \n\r");

//包含字母
bool HasLetter();

// 转成小写，返回一个新的字符串
String ToLowerCase();

// 转成大写，返回一个新的字符串
String ToUpperCase();

//返回搜索到的所有位置
//flag 定位标志
//num 搜索数量，默认直到结束
std::vector<int> FindAll(String flag, int num = -1);

//字符串替换，会修改原有的字符串，而不是返回新的
String& Replace(String old_str, String new_str);
~~~

## Response
该类用来处理请求响应
```cpp
bool SaveByVec();
vector<char> BodyBin();
String Body();
String ContentType();
String HeaderValue(String k)；// 获取响应体的值（最后一个）
```

## Request
该类用来处理发送请求
~~~cpp
//支持链式设置属性
Request& SetHttpMethod(String other) ;
Request& SetUrl(String url);
Request& SetCookie(String cookie);
Request& SetBody(String body);
Request& SetHeader(vector<pair<String, String>> header);
Request& SetHeader(String k, String v);

//发送请求
Response Send(String url);
Response Send(Method method, String url);
~~~

## Header
响应头
```cpp
// 添加一个键值对到头中
void Add(String k, String v);

// 获取头的键所对应的所有值
vector<String> GetValue(String key);

// 获取头的键所对应的值，以最后一个为准
String GetLastValue(String key);
```
