
#ifndef SERVER_H_
#define SERVER_H_

#include <netinet/in.h>
#include <stdexcept>
#include <set>

class Server
{
public:
	void stop_server(); 
	void start_server();
private:
	//set: socketfd
	typedef std::set<int> socket_pool_t; 

protected:

	Server(int port)throw(std::runtime_error); 
	virtual ~Server(); 
	void _init_server()throw(std::runtime_error); 
	int _add_socket(int socketfd);
	int _delete_socket(int socketfd); 
	virtual int _parse_new_request(int socket, const struct sockaddr_in& client_addr)=0; 
	virtual int _parse_registed_request(int socket)=0; 
	virtual void _routine_check();
private:
	struct sockaddr_in _server_addr;
	int _port;
	int _epollfd; 
	int _server_socket;

	socket_pool_t _IOnode_socket_pool;
}; 

#endif
