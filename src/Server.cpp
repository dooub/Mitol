//
// Created by helidium on 3/22/17.
//

#include <ctime>
#include "Server.h"

static uv_loop_t *loop;
static std::unordered_map<int, uv_poll_t *> polls;
std::string MNS::Server::currTime = "";
std::map<int, std::string> MNS::Server::response_msgs;

void MNS::Server::onConnect(uv_poll_t *handle, int status, int events) {
	//printf("Acception socket"); fflush(stdout);
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	if (data) {
		const MNS::Server *server = data->server;
		socklen_t addr_size = 0;
		sockaddr_in sadr;
		int csock = accept4(data->fd, (sockaddr *) &sadr, &addr_size, SOCK_NONBLOCK);

		int y_int = 1;
		//setsockopt(csock, SOL_SOCKET, SO_KEEPALIVE, &y_int, sizeof(int));
		setsockopt(csock, IPPROTO_TCP, TCP_NODELAY, &y_int, sizeof(int));
		//setsockopt(csock, IPPROTO_TCP, TCP_QUICKACK, &y_int, sizeof(int));

		uv_poll_t *socket_poll_h = (uv_poll_t *) malloc(sizeof(uv_poll_t));
		socket_poll_h->data = new MNS::SocketData(socket_poll_h, csock, MNS::SOCKET_TYPE::PEER, data->server);

		uv_poll_init(loop, socket_poll_h, csock);
		uv_poll_start(socket_poll_h, UV_READABLE, MNS::Server::onReadData);

		polls[csock] = socket_poll_h;

		if (server && server->onHttpConnectionHandler) {
			server->onHttpConnectionHandler(static_cast<MNS::SocketData *>(socket_poll_h->data));
		}
	}
}

void MNS::Server::onClose(uv_handle_t *handle) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);

	polls.erase(data->fd);
	close(data->fd);
	delete (data);
	free(handle);
}

void MNS::Server::onReadData(uv_poll_t *handle, int status, int events) {
	ssize_t requestLen = 0;
	ssize_t bytesRead = 0;
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);
	MNS::Server *server = data->server;

	// Todo: Offset and grow buffer in case of larger requests
	// Todo: Mark request state as reading socket
	while ((bytesRead = recv(data->fd, data->request->getBuffer(), 1024, 0)) >
	       0) { // WHILE DATA READ:: POTENTIAL ERROR SIZE OF READ DATA
		requestLen += bytesRead;
	}

	if (requestLen <= 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
		} else {
			uv_poll_stop(handle);
			uv_close((uv_handle_t *) handle, MNS::Server::onClose);
		}
	} else {
		if (!data->request->Parse(requestLen)) {
			data->response->startResponse();

			if (server->onHttpRequestHandler) {
				server->onHttpRequestHandler(data);
			} else {
				fflush(stdout);

				uv_poll_stop(handle);
				uv_close((uv_handle_t *) handle, MNS::Server::onClose);
			}
		} else {
			printf("Parse ERROR! Closing socket!\n");
		}
	}
}

void MNS::Server::onWriteData(uv_poll_t *handle, int status, int events) {
	MNS::SocketData *data = static_cast<MNS::SocketData *>(handle->data);

	if (data) {
		MNS::Response *response = data->response;

		ssize_t numSent = send(data->fd, response->getBuffer(), response->getBufferLen(), 0);
		if (numSent == (int) response->getBufferLen()) {
			response->clear();
			uv_poll_start(handle, UV_READABLE, onReadData);
		}
	}
}

MNS::Server::Server() {
	MNS::Server::response_msgs[200] = "200 OK\r\n";
	MNS::Server::response_msgs[301] = "301 Redirect\r\n";
	MNS::Server::response_msgs[404] = "404 Not Found\r\n";
	MNS::Server::response_msgs[500] = "500 Internal Server Error\r\n";

	loop = uv_default_loop();
}

void MNS::Server::listen(int port) {
	listeningSocket = Socket::createListening(port);

	if (listeningSocket != -1) {
		// Start the timer
		uv_timer_t *timer_h = new uv_timer_t;
		uv_timer_init(loop, timer_h);
		uv_timer_start(timer_h, MNS::Server::onSecondTimer, 0, 1000);

		// Register the listening socket
		uv_poll_t *listening_poll_h = new uv_poll_t;
		listening_poll_h->data = new SocketData(listening_poll_h, listeningSocket, SOCKET_TYPE::LISTENING, this);

		uv_poll_init(loop, listening_poll_h, listeningSocket);

		uv_poll_start(listening_poll_h, UV_READABLE, onConnect);
		polls[listeningSocket] = listening_poll_h;
	}
}

void MNS::Server::run() {
	uv_run(loop, UV_RUN_DEFAULT);
}

void MNS::Server::stop() {
	for (auto it = polls.begin(); it != polls.end(); it++) {
		MNS::SocketData *data = (MNS::SocketData *) it->second->data;
		it->second->data = NULL;

		uv_poll_stop(it->second);
		free(it->second);
		close(it->first);
		delete (data);
	}

	polls.clear();
}

void MNS::Server::onSecondTimer(uv_timer_t *handle) {
	char strt[100];
	std::time_t t = std::time(NULL);
	std::strftime(strt, 100, "%a, %e %b %Y %H:%M:%S GMT\0", std::gmtime(&t));

	MNS::Server::currTime = strt;
}

void MNS::Server::onHttpConnection(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpConnectionHandler = callback;
}

void MNS::Server::onHttpRequest(const std::function<void(MNS::SocketData *)> &callback) {
	this->onHttpRequestHandler = callback;
}

MNS::Server::~Server() {
}