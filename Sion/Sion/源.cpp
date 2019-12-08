#include<iostream>
#include"Sion.h"
#define SION_DISABLE_SSL 
using namespace Sion;
using namespace std;
int main()
{

	try
	{
		// Get
		cout << Fetch("http://www.baidu.com").ResponseBody << endl;;
		// Post
		Request req;
		req.RequestHeader = {
			{ "Content-Type","application/json; charset=utf-8" },
			{ "x-csrf-token","MXx8phRwFI3wd08blRXoWe58" },
		};
		req.Cookie = "csrfToken=MXx8phRwFI3wd08blRXoWe58";
		req.RequestBody = R"({"account":"zanllp","password":"zanllp_pw"})";
		cout << req.SendRequest(Post,"http://127.0.0.1:7001/user" ).ResponseBody << endl;
	}
	catch (const std::exception& e)
	{
		cout << e.what();
	}
	system("pause");
}