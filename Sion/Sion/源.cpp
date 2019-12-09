#include <iostream>
#define SION_DISABLE_SSL
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
			return Request()
				.SetUrl("http://127.0.0.1:7001/user")
				.SetHttpMethod(Method::Post)
				.SetBody(R"({"account":"zanllp","password":"ioflow@1598.auth"})")
				.SetCookie("csrfToken=4CUb9Rjk0dgRXZRZorAqbTm8")
				.SetHeader("Content-Type", "application/json; charset=utf-8")
				.SetHeader("x-csrf-token", "4CUb9Rjk0dgRXZRZorAqbTm8")
				.Send();
		}
		catch (const std::exception & e)
		{
			cout << e.what();
		}
	}).then([](Response resp) {
		cout << resp.ResponseBody << endl;
	});
	system("pause");
}