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