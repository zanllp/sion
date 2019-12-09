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