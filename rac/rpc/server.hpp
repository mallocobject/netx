#ifndef RAC_RPC_SERVER_HPP
#define RAC_RPC_SERVER_HPP

#include "rac/net/inet_addr.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include "rac/rpc/dispatcher.hpp"
#include <concepts>
#include <cstdint>
#include <utility>
namespace rac
{
// RpcServer server("127.0.0.1", 8080);
// server.bind("Echo", [](const User& u) { return u; });
// server.start();
class RpcServer
{
  public:
	explicit RpcServer(const InetAddr& sock_addr)
		: stream_(Socket::socket(nullptr), sock_addr)
	{
	}

	RpcServer(const std::string& ip, std::uint16_t port)
		: RpcServer(InetAddr{ip, port})
	{
	}

	template <typename Func> void bind(const std::string& method_name, Func&& f)
	{
	}

  private:
	Stream stream_;
	RpcDispatcher dispatcher_{};
};
} // namespace rac

#endif