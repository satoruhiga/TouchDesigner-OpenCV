#pragma once

#ifndef verify
	#ifdef NDEBUG
		#define verify(expression) expression
	#else
		#define verify(expression) assert( 0 != (expression) )
	#endif
#endif

#include <string>
#include <sstream>
#include <functional>

#include <assert.h>

#include <nanomsg/nn.h>
#include <nanomsg/pubsub.h>

namespace NanomsgIO
{
	class Publisher
	{
	public:
		int sock;

		Publisher()
			: sock(nn_socket(AF_SP, NN_PUB))
		{
			verify(sock >= 0);

			int size = 1024 * 1024 * 10; // 10MB
			verify(nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDBUF, &size, sizeof(int)) >= 0);
			verify(nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVBUF, &size, sizeof(int)) >= 0);
		}

		~Publisher()
		{
			nn_close(sock);
			nn_shutdown(sock, 0);
			sock = 0;
		}

		bool bind(const std::string& url)
		{
			return nn_bind(sock, url.c_str()) >= 0;
		}

		bool send(const void* data, int size)
		{
			int bytes = nn_send(sock, data, size, NN_DONTWAIT);
			return bytes == size;
		}
	};

	class Subscriber
	{
	public:
		int sock;

		Subscriber(const std::string& url)
			: sock(nn_socket(AF_SP, NN_SUB))
		{
			verify(sock >= 0);
			verify(nn_errno() >= 0);

			verify(nn_setsockopt(sock, NN_SUB, NN_SUB_SUBSCRIBE, "", 0) >= 0);
			verify(nn_errno() >= 0);

			int size = 1024 * 1024 * 10; // 10MB
			verify(nn_setsockopt(sock, NN_SOL_SOCKET, NN_SNDBUF, &size, sizeof(int)) >= 0);
			verify(nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVBUF, &size, sizeof(int)) >= 0);

			size = 1024 * 1024 * 100; // 100MB
			verify(nn_setsockopt(sock, NN_SOL_SOCKET, NN_RCVMAXSIZE, &size, sizeof(int)) >= 0);

			verify(nn_connect(sock, url.c_str()) >= 0);
			verify(nn_errno() >= 0);
		}

		~Subscriber()
		{
			if (sock)
			{
				nn_shutdown(sock, 0);
				sock = 0;
			}
		}

		bool recv(const std::function<void(const char*, int size)>& callback)
		{
			char *buf = nullptr;
			int ret = nn_recv(sock, &buf, NN_MSG, NN_DONTWAIT);

			if (ret < 0)
				return false;

			const int bytes = ret;
			callback(buf, bytes);
			nn_freemsg(buf);

			return true;
		}
	};
}