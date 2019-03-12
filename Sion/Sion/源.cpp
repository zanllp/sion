#include<iostream>
#include"Sion.h"

MyString PostTest()
{
	Sion::Request request;
	request.Header["Content-Type"] = "application/json; charset=utf-8";
	request.RequestBody = R"({"id":null,"password":"zanllp_pw","account":"zanllp"})";
	Sion::Response response = request.SendRequest(Sion::Post, "http://127.0.0.1:5000/api/auth");
	response.ParseFromSource();
	return response.ResponseBody;
}

MyString ChunkedTest()
{
	Sion::Response response = Sion::Request::StaticRequest(Sion::Get, "http://zanllp.cn");
	response.ParseFromSource(true);
	return response.ResponseBody;
}

void ShowHeader()
{
	Sion::Request request;
	Sion::Response response=request.SendRequest(Sion::Get, "http://www.baidu.com");
	response.ParseFromSource(true);
	for (auto x : response.Header)
	{
		std::cout << x.first << "   " << x.second << std::endl;
	}
}

int main()
{
	try
	{
		ShowHeader();
		std::cout << ChunkedTest() <<std::endl;
		std::cout << PostTest() << std::endl;
	}
	catch (const std::exception &e)
	{
		std::cout << e.what() << std::endl;
	}
	system("pause");
}