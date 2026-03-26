#ifndef NETX_RPC_SERVER_HPP
#define NETX_RPC_SERVER_HPP

#include "netx/async/async_main.hpp"
#include "netx/async/check_error.hpp"
#include "netx/async/event.hpp"
#include "netx/async/event_loop.hpp"
#include "netx/async/task.hpp"
#include "netx/net/inet_addr.hpp"
#include "netx/net/scheduler.hpp"
#include "netx/net/socket.hpp"
#include "netx/net/stream.hpp"
#include "netx/rpc/dispatcher.hpp"
#include <cstddef>
#include <cstdint>
#include <latch>
#include <thread>
#include <utility>
#include <vector>
namespace netx
{
namespace rpc
{
// RpcServer server("127.0.0.1", 8080);
// server.bind("Echo", [](const User& u) { return u; });
// server.start();

namespace async = netx::async;
namespace net = netx::net;
class RpcServer
{
	RpcServer() : stream_(async::checkError(net::Socket::socket(nullptr)))
	{
	}

  public:
	static RpcServer& server()
	{
		static RpcServer rpc_server{};
		return rpc_server;
	}

	RpcServer(RpcServer&&) = delete;

	RpcServer& listen(const net::InetAddr& sock_addr)
	{
		stream_.bind(sock_addr);
		int listen_fd = stream_.fd();
		net::Socket::setReuseAddr(listen_fd, true);
		async::checkError(net::Socket::bind(listen_fd, sock_addr, nullptr));
		async::checkError(net::Socket::listen(listen_fd, nullptr));

		return *this;
	}

	RpcServer& listen(const std::string& ip, std::uint16_t port)
	{
		return listen(net::InetAddr{ip, port});
	}

	RpcServer& loop(std::size_t loop_count = 1)
	{
		if (loop_count < 1)
		{
			LOG_FATAL << "loop count must more then 1";
			exit(EXIT_FAILURE);
		}
		loop_count_ = loop_count;
		return *this;
	}

	template <typename Func>
	RpcServer& bind(const std::string& method_name, Func&& f)
	{
		dispatcher_.bind(method_name, std::forward<Func>(f));
		return *this;
	}

	void start()
	{
		std::latch start_latch{static_cast<std::ptrdiff_t>(loop_count_)};
		for (size_t idx = 0; idx < loop_count_; idx++)
		{
			loops_.emplace_back(
				[this, &start_latch]
				{
					net::Scheduler scheduler{};
					schedulers_ptr_.push_back(&scheduler);
					async_main(scheduler.schedulerLoop(start_latch));
				});
		}
		start_latch.wait();

		LOG_WARN << "RPC Server listening on "
				 << stream_.sock_addr().to_formatted_string();

		async_main(serverLoop());
	}

  private:
	async::Task<> handleClient(int conn_fd);

	async::Task<> serverLoop();

  private:
	net::Stream stream_;
	RpcDispatcher dispatcher_{};
	std::size_t loop_count_{0};

	std::vector<net::Scheduler*> schedulers_ptr_;
	std::vector<std::jthread> loops_;
};

inline async::Task<> RpcServer::handleClient(int conn_fd)
{
	net::Stream s{conn_fd};

	while (true)
	{
		while (s.read_buffer()->readableBytes() < sizeof(RpcHeaderWire))
		{
			auto rd_buf = co_await s.read();
			if (!rd_buf)
			{
				LOG_ERROR << "Connection closed by peer before reading "
							 "header in fd "
						  << stream_.fd();
				co_return;
			}
		}

		RpcHeader h = s.read_buffer()->peekRpcHeader();
		std::size_t total_len = h.header_len + h.body_len;

		while (s.read_buffer()->readableBytes() < total_len)
		{
			auto rd_buf = co_await s.read();
			if (!rd_buf)
			{
				LOG_ERROR << "Connection closed by peer before reading "
							 "body in fd "
						  << stream_.fd();
				co_return;
			}
		}

		s.read_buffer()->retrieve(h.header_len);
		std::size_t readable_before = s.read_buffer()->readableBytes();

		std::string method_name;
		DeserializeTraits<std::string>::deserialize(s.read_buffer(),
													&method_name);

		// LOG_DEBUG << "Client requesting method: " << method_name;

		std::size_t consumed =
			readable_before - s.read_buffer()->readableBytes();
		std::uint32_t args_limit = h.body_len - consumed;

		std::size_t size_before = s.write_buffer()->readableBytes();

		dispatcher_.dispatch(method_name, s.read_buffer(), s.write_buffer(),
							 args_limit);

		h.body_len = static_cast<uint32_t>(s.write_buffer()->readableBytes() -
										   size_before);
		s.write_buffer()->prependRpcHeader(h);

		co_await s.write();
	}
}

inline async::Task<> RpcServer::serverLoop()
{
	// std::list<ScheduledTask<Task<>>> connections;
	int listen_fd = stream_.fd();
	async::Event ev{.fd = listen_fd, .flags = EPOLLIN};
	auto ev_awaiter = async::EventLoop::loop().wait_event(ev);
	static std::size_t lucky_boy = 0;

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

			assert(schedulers_ptr_.size() >= 1);
			auto lucky = schedulers_ptr_[lucky_boy++ % schedulers_ptr_.size()];
			lucky->push(handleClient(conn_fd));
			lucky->wakeup();

			// int epfd = epfds_[lucky_boy++ % epfds_.size()];

			// auto* info = new HandleInfo{.boostrap_fd = conn_fd};

			// epoll_event ev{.events = EPOLLIN | EPOLLONESHOT,
			// 			   .data{.ptr = info}};

			// checkError(epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev));
			// connections.emplace_back(handleClient(conn_fd));
		}

		// if (connections.size() < 100) [[likely]]
		// {
		// 	continue;
		// }
		// for (auto iter = connections.begin(); iter != connections.end();)
		// {
		// 	if (iter->done())
		// 	{
		// 		iter->result(); //< consume result, such as throw exception
		// 		iter = connections.erase(iter);
		// 	}
		// 	else
		// 	{
		// 		++iter;
		// 	}
		// }
	}
	co_return;
}
} // namespace rpc
} // namespace netx

#endif