# Sion
# 特性
* Sion是一个轻量级简单易用的c++ http客户端
* 仅单个头文件，自带std::string的扩展
* 跨平台，支持linux,win, mac...
* 有着良好的异步支持，可以选择以自己喜欢的方式发送异步请求，callback，await，事件循环，etc.
* 支持文本及二进制的响应体
* 支持分块(chunked)的传输编码
* 支持http,https请求。_https需要安装openssl(推荐使用[vcpkg](https://github.com/microsoft/vcpkg)),如果不需要可以使用 #define SION_DISABLE_SSL 关闭_
# 用法
## 导入
直接复制[sion.h](src/sion.h)到自己的项目下`include`
## 最普通的GET请求
```cpp
auto resp = sion::Fetch(ms_url);
std::cout << resp.StrBody() << std::endl;
std::cout << resp.Header().Get("content-type") << std::endl;
```
## 链式调用及POST请求
```CPP
auto resp = Request()
			.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
			.SetHttpMethod(Method::Post)
			.SetBody(R"({"data": 233333,"msg":"hello world!"})")
			.SetHeader("Content-Type", "application/json; charset=utf-8")
			.Send();
```

## 异步请求
sion在此版本后对异步增加了巨大的支持，相对于之前的版本依旧保持了同步api+异步库的方式，尽可能保持简单轻量，尽管性能不是很理想但性能从来就不是sion的最优先考虑的东西。

sion的异步支持依赖于sion内置的Async线程池，函数可参考[Async](#Async)。

### 先启动线程池线程池
```cpp
sion::Async async_thread_pool;

async_thread_pool.Start(); // 启动线程池，创建子线程，子线程接受任务开始工作
```

### 添加请求到线程池，并接受响应
sion::Async处理异步有3种方式
1. 使用回调
```cpp
async_thread_pool.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); },
                          [](sion::AsyncResponse async_resp) { std::cout << "AsyncCallback " << async_resp.resp.Status() << std::endl; });
```
2. 使用await
在无回调时提交任务到线程池会返回给你一个id，通过这个id我们可以使用await在当前线程上等待请求完成
```cpp
auto id = async_thread_pool.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); });
        auto pkg = async_thread_pool.Await(id);
        std::cout << "AsyncAwait " << pkg.resp.Header().Data().size() << pkg.err_msg << std::endl;
// 你可以给await添加超时时间，如果超时会抛出AsyncAwaitTimeout
try
{
    auto id = async_thread_pool.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); });
    auto pkg = async_thread_pool.Await(id, 10);
}
catch (const std::exception& e)
{
    std::cerr << e.what() << '\n';
}
```

3. 使用GetAvailableResponse
这种方式是通过不断获取调取此函数来获取想要的响应，主要还是方便与事件循环集成。
```cpp
 const int num = 100;
    for (size_t i = 0; i < num; i++)
    {
        async_thread_pool.Run([=] { return sion::Request().SetUrl(ms_url).SetHttpMethod(sion::Method::Get); });
    }
    int i = 0;
    while (i <= num)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto async_resps = async_thread_pool.GetAvailableResponse();
        if (async_resps.size())
        {
            i += async_resps.size();
            std::cout << "AsyncGetAvailableResponse got resp size:" << async_resps.size() << std::endl;
        }
    }
```
### AsyncResponse
上面几种方式都会返回AsyncResponse，通过这个可以获取异步请求的响应体，id，以及错误信息

## 更多用法
[参考](./src/main.cc)
# 类&函数定义
## Fetch
~~~cpp
// 静态请求方法
Response Fetch(String url, Method method = Get, vector<pair<String, String>> header = {}, String body = "");
~~~

## Response
该类用来处理请求响应
```cpp
// 返回接收到的响应体
const vector<char>& Body();
// 将接收到的响应体转成字符串，如果你能确保接收到的数据是字符串，可以直接使用这个
String StrBody();
String Code();
String Status();
int ContentLength();
const Header& Header()；// 获取响应头 参考 [Header](#Header)
```

## Request
该类用来处理发送请求
~~~cpp
//支持链式设置属性
Request& SetHttpMethod(String other) ;
Request& SetUrl(String url);
Request& SetBody(String body);
Request& SetHeader(vector<pair<String, String>> header);
Request& SetHeader(String k, String v);

//发送请求
Response Send(String url);
Response Send(Method method, String url);
~~~

## Header
响应头和请求头
```cpp
// 添加一个键值对到头中
void Add(String k, String v);

// 获取头的键所对应的所有值
vector<String> GetAll(String key);

// 获取头的键所对应的值，以最后一个为准
String Get(String key);

//删除目标键值对，返回一个bool指示操作是否成功
bool Remove(String key);

// 删除所有的键值对
void RemoveAll(String key)

// 返回数据本体
const vector<pair<String, String>>& Data();
```
## Async
该类依赖处理sion的异步请求，内置了个小型的线程池，提供了callback，await，事件循环三种方式让你来处理异步的请求。线程池会在析构时退出启动的所有线程。

```cpp
// 设置线程池的线程数量，即能够同时请求或者是运行请求回调的数量, 如果进行比较重的回调可以设置成std::thread::hardware_concurrency()，以减少上下文切换开销
Async& SetThreadNum(int num);

// 设置线程池是否以阻塞当前线程的方式运行
Async& SetBlock(bool wait);

// 设置await是否在接收到包含错误消息的响应时抛异常
Async& SetThrowIfHasErrMsg(bool op);

// 启动线程池
void Start();

// 添加一个请求到线程池的任务队列, 返回一个请求的id,id可用于await或者是查找对应的请求
uint Run(std::function<Request()> fn);

// 添加一个请求和回调到线程池的任务队列,在请求完成后会在请求的线程执行回调
void Run(std::function<Request()> fn, std::function<void(AsyncResponse)> cb);

//在当前线程等待直到目标请求可用。id:Async::Run返回的id,timeout_ms 超时时间单位毫秒，默认永不超时
AsyncResponse Await(uint id, uint timeout_ms);

// 获得当前可用的响应，如果你添加一个请求到线程池的任务队列，且没有回调或者使用await取获取，那么可用通过这种方式获取。适用场合是一般是在事件循环里面
vector<AsyncResponse>  GetAvailableResponse();
```


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
