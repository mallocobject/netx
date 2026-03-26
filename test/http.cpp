#include "elog/logger.h"
#include "netx/async/async_main.hpp"
#include "netx/async/check_error.hpp"
#include "netx/async/event.hpp"
#include "netx/async/event_loop.hpp"
#include "netx/async/scheduled_task.hpp"
#include "netx/async/task.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <csignal>
#include <cstddef>
#include <iostream>
#include <list>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <queue>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace netx;

Task<> handle_client(int fd)
{
	char buf[1024];

	Event rd_ev{.fd = fd, .flags = EPOLLIN};
	Event wr_ev{.fd = fd, .flags = EPOLLOUT | EPOLLONESHOT};
	auto rd_awaiter = EventLoop::loop().wait_event(rd_ev);
	auto wr_awaiter = EventLoop::loop().wait_event(wr_ev);
	const char* response = "HTTP/1.1 200 OK\r\n"
						   "Content-Type: text/plain\r\n"
						   "Content-Length: 12\r\n"
						   "Connection: keep-alive\r\n\r\n"
						   "Hello, world";
	std::size_t total = strlen(response);

	while (true)
	{
		while (true)
		{
			ssize_t n = read(fd, buf, sizeof(buf));
			if (n > 0)
			{
				break;
			}
			else if (n == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
			{
				co_await rd_awaiter;
			}
			else
			{
				close(fd);
				co_return;
			}
		}

		std::size_t written = 0;
		while (written < total)
		{
			ssize_t w = write(fd, response + written, total - written);
			if (w > 0)
			{
				written += w;
			}
			else if (w == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
			{
				co_await wr_awaiter;
			}
			else
			{
				close(fd);
				co_return;
			}
		}
		// close(fd);
		// co_return;
	}
}

Task<> server_loop(int listen_fd)
{
	std::list<ScheduledTask<Task<>>> connections;
	Event ev{.fd = listen_fd, .flags = EPOLLIN};
	auto ev_awaiter = EventLoop::loop().wait_event(ev);
	while (true)
	{
		co_await ev_awaiter;
		while (true)
		{
			int conn_fd = accept4(listen_fd, nullptr, nullptr,
								  SOCK_NONBLOCK | SOCK_CLOEXEC);
			if (conn_fd == -1)
			{
				break;
			}
			int opt = 1;
			setsockopt(conn_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
			connections.emplace_back(handle_client(conn_fd));
		}

		if (connections.size() < 100) [[likely]]
		{
			continue;
		}
		for (auto iter = connections.begin(); iter != connections.end();)
		{
			if (iter->done())
			{
				iter->result(); //< consume result, such as throw exception
				iter = connections.erase(iter);
			}
			else
			{
				++iter;
			}
		}
	}
	co_return;
}

int main()
{
	::signal(SIGPIPE, SIG_IGN);

	int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = ::htons(8080);
	::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr.s_addr);

	int ret =
		::bind(listen_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
	::listen(listen_fd, SOMAXCONN);

	LOG_WARN << "Server listening on http://127.0.0.1:8080";

	async_main(server_loop(listen_fd));
}