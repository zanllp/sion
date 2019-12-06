#include<iostream>
#include"Sion.h"
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
		req.Header["Content-Type"] = R"({"id":null,"password":"zanllp_pw","account":"zanllp"})";
		cout << req.SendRequest(Post, "http://127.0.0.1:7001/api/login").ResponseBody << endl;;
	}
	catch (const std::exception& e)
	{
		cout << e.what();
	}
	system("pause");
}