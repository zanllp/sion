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
			auto rec = chrono::system_clock::now();
			auto resp = Fetch("https://www.zanllp.cn/static/js/4.a50ac19e.chunk.js");
			cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - rec).count() << "ms" << endl;
			return resp;
		}
		catch (const std::exception & e)
		{
			cout << e.what();
		}
			return Response();
	}).then([](Response resp) {
			cout << resp.Source.length() << endl;
	}).then([]{
		try
		{
			auto rec = chrono::system_clock::now();
			auto resp = Request()
						.SetUrl("https://api.zanllp.cn/socket/push?descriptor=fHXMHCQfcgNHDq2P")
						.SetHttpMethod(Method::Post)
						.SetBody(R"({"data": 233333,"msg":"hello world!"})")
						.SetHeader("Content-Type", "application/json; charset=utf-8")
						.Send();
			cout << chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - rec).count() << "ms" << endl;
			return resp;
		}
		catch (const std::exception & e)
		{
			cout << e.what();
		}
		return Response();
	}).then([](Response resp) {
		cout << resp.ResponseBody << endl;
		exit(0);
	});
	this_thread::sleep_for(30s);
}