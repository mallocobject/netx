#include "netx/rpc/server.hpp"
#include <csignal>
#include <string>

using namespace netx::rpc;

struct Point
{
	double x;
	double y;
};

struct User
{
	std::uint32_t id;
	std::string name;
	Point loc;
};

int main()
{
	RpcServer::server()
		.listen("127.0.0.1", 8080)
		.bind("EchoUser", [](const User& usr) { return usr; })
		.bind("AddPoint",
			  [](Point a, Point b) { return Point{a.x + b.x, a.y + b.y}; })
		.loop(4)
		.start();
}