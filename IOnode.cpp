/*
 * IOnode.c
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include "IOnode.h"
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

IOnode::block::block(unsigned long size, int block_id) throw(std::bad_alloc):size(size),block_id(block_id),data(NULL)
{
	data=malloc(size);
	if( NULL == data)
	{
	   throw std::bad_alloc(); 
	}
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):size(src.size),block_id(src.block_id),data(src.data){};

IOnode::IOnode(std::string my_ip, std::string master_ip,  int master_port) throw(std::runtime_error):
	_ip(my_ip),
	_node_id(-1),
	_blocks(block_info()),  
	_files(file_info()), 
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY), 
	_master_port(MASTER_PORT), 
	_node_server_socket(-1), 
	_master_socket(-1) 
{
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	try
	{
		_init_server(); 
	}
	catch(std::runtime_error& e)
	{
		_unregist();
		throw;
	}
}

void IOnode::_init_server() throw(std::runtime_error)
{
	memset(&_node_server_addr, 0, sizeof(_node_server_addr));
	_node_server_addr.sin_family = AF_INET; 
	_node_server_addr.sin_addr.s_addr = htons(INADDR_ANY); 
	_node_server_addr.sin_port = htons(_master_port); 
	if( 0 > (_node_server_socket = socket(PF_INET, SOCK_STREAM, 0)))
	{
		throw std::runtime_error("Create Socket Failed");  
	}
	if(0 != bind(_node_server_socket, (struct sockaddr*)&_node_server_addr, sizeof(_node_server_addr)))
	{
		throw std::runtime_error("Server Bind Port Failed"); 
	}
	if(0 != listen(_node_server_socket, MAX_QUEUE))
	{
		throw std::runtime_error("Server Listen PORT ERROR");  
	}
	
	std::cout << "Start IO node Server" << std::endl; 
}	

IOnode::~IOnode()
{
	if(-1 != _node_server_socket)
	{
		close(_node_server_socket); 
	}
	_unregist(); 
	delete &_blocks; 
}

int IOnode::_regist(std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	memset(&_master_addr, 0, sizeof(_master_addr)); 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(MASTER_CONN_PORT); 
	
	_master_socket = socket(PF_INET,  SOCK_STREAM, 0); 
	if( 0 > _master_socket)
	{
		//perror("Create Socket Failed"); 
		throw std::runtime_error("Create Socket Failed"); 
	}

	if(bind(_master_socket,  (struct sockaddr*)&_master_conn_addr, sizeof(_master_conn_addr)))
	{
		//perror("Client Bind Port Failed");
		throw std::runtime_error("Client Bind Port Failed");  
	}
	memset(&_master_addr, 0,  sizeof(_master_addr)); 
	_master_addr.sin_family = AF_INET; 
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		//perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}

	_master_addr.sin_port = htons(_master_port); 
	if( connect(_master_socket,  (struct sockaddr*)&_master_addr,  sizeof(_master_addr)))
	{
		throw std::runtime_error("Can Not Connect To master");  
	}
	send(_master_socket, &REGIST, sizeof(REGIST),  0);
	char id[sizeof(int)]; 
	recv(_master_socket, id,  sizeof(id), 0);
	return atoi(id); 
}

void IOnode::_unregist()
{
	if(-1 != _master_socket)
	{
		send(_master_socket,  &UNREGIST,  sizeof(UNREGIST),  0);
		close(_master_socket); 
		_master_socket = -1; 
	}
}

int IOnode::_insert_block(unsigned long size) throw(std::bad_alloc)
{
	if( MAX_BLOCK_NUMBER < _current_block_number)
	{
		throw std::bad_alloc(); 
	}
	int id = _current_block_number++; 
	try
	{
		_blocks.insert(std::make_pair(id,  block(size, id))); 
	}
	catch(std::bad_alloc)
	{
		throw; 
	}
	return id; 
}

void IOnode::_delete_block(int block_id)
{
	block_info::iterator it; 
	if((it = _blocks.find(block_id))  !=  _blocks.end())
	{
		delete &(it->second); 
		_blocks.erase(it); 
	}
}
