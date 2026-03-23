#ifndef RAC_RPC_CLIENT_HPP
#define RAC_RPC_CLIENT_HPP

#include "rac/net/inet_addr.hpp"
#include "rac/net/socket.hpp"
#include "rac/net/stream.hpp"
#include <string>
namespace rac
{
// RpcClient client("127.0.0.1", 8080);
// User result = client.call<User>("Echo", User{123, "Alice", {1,2}});

class RpcClient
{
  public:
	explicit RpcClient(const InetAddr& sock_addr)
		: stream_(Socket::socket(nullptr), sock_addr)
	{
	}

	RpcClient(const std::string& ip, std::uint16_t port)
		: RpcClient(InetAddr{ip, port})
	{
	}

	template <typename Ret, typename... Args>
	Task<Ret> call(const std::string& method_name, Args&&... args)
	{
	}

  private:
	Stream stream_;
};
} // namespace rac

#endif