/*
 * IOnode.cpp
 *
 *  Created on: Aug 8, 2014
 *      Author: xtq
 */

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <aio.h>
#include <memory>

#include "include/IOnode.h"
#include "include/IO_const.h"
#include "include/Communication.h"

IOnode::block::block(off_t start_point, size_t size, bool dirty_flag, size_t &total_memory) throw(std::bad_alloc):
	size(size),
	data(NULL),
	start_point(start_point),
	dirty_flag(dirty_flag)
{
	if(size > total_memory)
	{
		throw std::bad_alloc();
	}
	data=malloc(size);
	if( NULL == data)
	{
	   throw std::bad_alloc();
	}
	total_memory-=size;
	return;
}

IOnode::block::~block()
{
	free(data);
}

IOnode::block::block(const block & src):size(src.size),data(src.data), start_point(src.start_point){};

IOnode::lio_info::lio_info(ssize_t file_no, int master_socket, int *fd, int nitems, off_t start_point, void *buffer, int mode, size_t nbytes):
	nitems(nitems),
	iolist(new aiocb*[nitems]),
	master_socket(master_socket),
	file_no(file_no)
{
	size_t step=(nbytes+nitems-1)/nitems, offset=start_point;
	void *buf=buffer;
	for(int i=0;i<nitems;++i)
	{
		struct aiocb *cb=new aiocb;
		memset(cb, 0, sizeof(aiocb));
		cb->aio_fildes=fd[i];
		cb->aio_offset=offset;
		cb->aio_buf=buf;
		cb->aio_nbytes=step+offset<nbytes?step:nbytes-offset;
		cb->aio_lio_opcode=mode;
		iolist[i]=cb;
	}
}

IOnode::lio_info::~lio_info()
{
	for(int i=0;i<nitems;++i)
	{
		close(iolist[i]->aio_fildes);
		delete iolist[i];
	}
	delete[] iolist;
}

IOnode::requested_blocks::requested_blocks(void *buffer, off_t start_point, size_t size):
	buffer(buffer),
	start_point(start_point),
	size(size)
{}

IOnode::aio_info::aio_info(io_blocks_t* block_set, aio_fun io_fun):
	block_set(block_set),
	present_block(block_set->begin()),
	io_fun(io_fun)
{
	memset(&cb, 0, sizeof(cb));
}

IOnode::aio_info::~aio_info()
{
	delete block_set;
}

IOnode::IOnode(const std::string& master_ip,  int master_port) throw(std::runtime_error):
	Server(IONODE_PORT), 
	Client(), 
	_node_id(-1),
	_files(file_blocks_t()),
	_file_path(file_path_t()),
	_current_block_number(0), 
	_MAX_BLOCK_NUMBER(MAX_BLOCK_NUMBER), 
	_memory(MEMORY),
	_master_port(MASTER_PORT)
{
	memset(&_master_conn_addr, 0, sizeof(_master_conn_addr));
	memset(&_master_addr, 0, sizeof(_master_addr));
	pthread_spin_init(&_master_comm_lock, PTHREAD_PROCESS_SHARED);
	if(-1  ==  (_node_id=_regist(master_ip,  master_port)))
	{
		throw std::runtime_error("Get Node Id Error"); 
	}
	try
	{
		Server::_init_server();
	}
	catch(std::runtime_error& e)
	{
		//_unregist();
		throw;
	}
}

IOnode::~IOnode()
{
	_send_blocks_to_other();
	Send(_master_socket, I_AM_SHUT_DOWN);
	close(_master_socket);
	close(_master_io_comm_socket);
	pthread_spin_destroy(&_master_comm_lock);
	for(file_blocks_t::iterator it=_files.begin();
			_files.end()!=it;++it)
	{
		for(block_info_t::iterator block_it=it->second.begin();
				it->second.end()!=block_it;++block_it)
		{
			delete block_it->second;
		}
	}
}

void IOnode::_send_blocks_to_other()
{}

int IOnode::_regist(const std::string& master_ip, int master_port) throw(std::runtime_error)
{ 
	_master_conn_addr.sin_family = AF_INET;
	_master_conn_addr.sin_addr.s_addr = htons(INADDR_ANY);
	_master_conn_addr.sin_port = htons(MASTER_CONN_PORT);

	_master_addr.sin_family = AF_INET;
	_master_addr.sin_port = htons(_master_port);
	if( 0  ==  inet_aton(master_ip.c_str(), &_master_addr.sin_addr))
	{
		perror("Server IP Address Error"); 
		throw std::runtime_error("Server IP Address Error");
	}
	try
	{
		_master_socket=Client::_connect_to_server(_master_conn_addr, _master_addr); 
	}
	catch(std::runtime_error &e)
	{
		throw;
	}
	printf("regist IOnode to master\n");
	Send(_master_socket, REGIST);
	Send(_master_socket, _memory);
	int id=-1;
	Recv(_master_socket, id);
	Server::_add_socket(_master_socket);
	int open;
	Recv(_master_socket, open);
	if(-1 != id)
	{
		_master_io_comm_socket=Client::_connect_to_server(_master_conn_addr, _master_addr);
		Send(_master_io_comm_socket, IO_COMM_REGIST);
		Send(_master_io_comm_socket, id);
	}
	printf("finish node registeration\n");
	return id; 
}

int IOnode::_parse_new_request(int sockfd, const struct sockaddr_in& client_addr)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
/*	case SERVER_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN; break; */
	case READ_FILE:
		_send_data(sockfd);break;
	case WRITE_FILE:
		_receive_data(sockfd);break;
/*	dafault:
		break; */
	}
	return ans; 
}

//request from master
int IOnode::_parse_registed_request(int sockfd)
{
	int request, ans=SUCCESS; 
	Recv(sockfd, request); 
	switch(request)
	{
	case OPEN_FILE:
		_open_file(sockfd);break;
	case READ_FILE:
		_read_file(sockfd);break;
	case WRITE_FILE:
		_write_file(sockfd);break;
/*	case I_AM_SHUT_DOWN:
		ans=SERVER_SHUT_DOWN;break;*/
	case FLUSH_FILE:
		_write_back_file(sockfd);break;
	case CLOSE_FILE:
		_close_file(sockfd);break;
/*	default:
		break; */
	}
	return ans;  
}

IOnode::io_blocks_t* IOnode::_get_io_blocks(block_info_t &blocks, off_t start_point, size_t size)
{
	io_blocks_t *request_block_set=new io_blocks_t();
	size_t end_point=start_point + size;
	for(block_info_t::iterator it=blocks.begin();it!=blocks.end();++it)
	{
		block *present_block=it->second;
		size_t block_end_point = present_block->start_point + present_block->size;
		if(start_point < block_end_point)
		{
			continue;
		}
		if(end_point > present_block->start_point)
		{
			break;
		}
		size_t io_start_point=present_block->start_point > start_point?present_block->start_point:start_point;
		void *buf=present_block->start_point > start_point?present_block->data:present_block->data+start_point-present_block->start_point;
		size_t io_end_point=block_end_point>end_point?end_point-io_start_point:block_end_point-io_start_point;
		struct requested_blocks block(buf, io_start_point, io_end_point);
		request_block_set->push_back(block);
	}
	return request_block_set;
}

int IOnode::_send_data(int sockfd)
{
	ssize_t file_no;
	off_t start_point;
	size_t size;
	Recv(sockfd, file_no);
	Recv(sockfd, start_point);
	Recv(sockfd, size); 
	try
	{
		Send(sockfd, SUCCESS);
		_add_aio_job(_files.at(file_no), sockfd, start_point, size, aio_write);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(sockfd, FILE_NOT_FOUND);
		close(sockfd);
		return FAILURE;
	}
}

int IOnode::_open_file(int sockfd)
{
	ssize_t file_no; 
	int flag=0;
	char *path_buffer=NULL; 
	Recv(sockfd, file_no);
	Recv(sockfd, flag);
	Recvv(sockfd, &path_buffer); 
	off_t start_point;
	size_t block_size;
	block_info_t *blocks=NULL; 
	Recv(sockfd, start_point); 
	Recv(sockfd, block_size); 
	if(_files.end() != _files.find(file_no))
	{
		blocks=&(_files.at(file_no)); 
	}
	else
	{
		blocks=&(_files[file_no]); 
	}
	blocks->insert(std::make_pair(start_point, new block(start_point, block_size, CLEAN, _memory)));
	if(_file_path.end() == _file_path.find(file_no))
	{
		_file_path.insert(std::make_pair(file_no, std::string(path_buffer)));
	}
	delete path_buffer; 
	return SUCCESS; 
}

int IOnode::_read_file(int sockfd)
{
	off_t start_point=0;
	size_t size=0;
	ssize_t file_no=0;
	Recv(sockfd, file_no);
	Recv(sockfd, start_point);
	Recv(sockfd, size);
	const std::string& path=_file_path.at(file_no);
	block_info_t &file=_files.at(file_no);
	for(block_info_t::iterator it=file.find(start_point);
			it != file.end();++it)
	{
		_read_from_storage(file_no, path, it->second);
	}
	return SUCCESS;
}

int IOnode::_read_from_storage(const ssize_t &file_no, const std::string& path, const block* block_data)throw(std::runtime_error)
{
	int parallel_io_number=_get_parallel_number(block_data->size);
	std::unique_ptr<int[]> fd(new int[parallel_io_number]);
	for(int i=0;i<parallel_io_number;++i)
	{
		fd[i]=open(path.c_str(),O_RDONLY);
		if( -1 == fd[i])
		{
			perror("Open File");
			throw std::runtime_error("Open File Error\n");
		}
	}
	return _add_lio_job(file_no, fd.get(), parallel_io_number, block_data->start_point, block_data->data, LIO_READ, block_data->size);
}

IOnode::block* IOnode::_buffer_block(off_t start_point, size_t size)throw(std::runtime_error)
{
	try
	{
		block *new_block=new block(start_point, size, DIRTY, _memory); 
		return new_block; 
	}
	catch(std::bad_alloc &e)
	{
		throw std::runtime_error("Memory Alloc Error"); 
	}
}

int IOnode::_write_file(int clientfd)
{
	ssize_t file_no;
	off_t start_point;
	size_t size;
	char *file_path=NULL;
	Recv(clientfd, file_no);
	Recvv(clientfd, &file_path);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	try
	{
		block_info_t &blocks=_files.insert(std::make_pair(file_no, block_info_t())).first->second;
		blocks.insert(std::make_pair(start_point, new block(start_point, size, DIRTY, _memory)));
		if(_file_path.end() == _file_path.find(file_no))
		{
			_file_path.insert(std::make_pair(file_no, std::string(file_path)));
		}
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		return FILE_NOT_FOUND;
	}
}

int IOnode::_receive_data(int clientfd)
{
	ssize_t file_no;
	off_t start_point;
	size_t size;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	try
	{
		Send(clientfd, SUCCESS);
		_add_aio_job(_files.at(file_no), clientfd, start_point, size, aio_read);
		return SUCCESS;
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FILE_NOT_FOUND);
		close(clientfd);
		return FAILURE;
	}
}

int IOnode::_write_back_file(int clientfd)
{
	ssize_t file_no;
	off_t start_point;
	size_t size;
	Recv(clientfd, file_no);
	Recv(clientfd, start_point);
	Recv(clientfd, size);
	try
	{
		block_info_t &blocks=_files.at(file_no);
		block* _block=blocks.at(start_point);
		Send(clientfd, SUCCESS);
		const std::string &path=_file_path.at(file_no);
		_write_to_storage(file_no, path, _block);
	}
	catch(std::out_of_range &e)
	{
		Send(clientfd, FILE_NOT_FOUND);
		return FAILURE;
	}
	return SUCCESS;
}

int IOnode::_write_to_storage(const ssize_t &file_no, const std::string& path, const block* block_data)throw(std::runtime_error)
{
	int parallel_io_number=_get_parallel_number(block_data->size);
	std::unique_ptr<int[]> fd(new int[parallel_io_number]);
	for(int i=0;i<parallel_io_number;++i)
	{
		fd[i]=open(path.c_str(),O_WRONLY|O_CREAT|O_SYNC);
		if( -1 == fd[i])
		{
			perror("Open File");
			throw std::runtime_error("Open File Error\n");
		}
	}
	return _add_lio_job(file_no, fd.get(), parallel_io_number, block_data->start_point, block_data->data, LIO_WRITE, block_data->size);
}

int IOnode::_flush_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	std::string &path=_file_path.at(file_no);
	block_info_t &blocks=_files.at(file_no);
	for(block_info_t::iterator it=blocks.begin();
			it != blocks.end();++it)
	{
		block* _block=it->second;
		if(DIRTY == _block->dirty_flag)
		{
			_write_to_storage(file_no, path, _block);
			_block->dirty_flag=CLEAN;
		}
	}
	//Send(sockfd, WRITE_FINISHED);
	//Send(sockfd, file_no);
	return SUCCESS;
}

int IOnode::_close_file(int sockfd)
{
	ssize_t file_no;
	Recv(sockfd, file_no);
	std::string &path=_file_path.at(file_no);
	block_info_t &blocks=_files.at(file_no);
	for(block_info_t::iterator it=blocks.begin();
			it != blocks.end();++it)
	{
		block* _block=it->second;
		if(DIRTY == _block->dirty_flag)
		{
			_write_to_storage(file_no, path, _block);
		}
		delete _block;
	}
	_files.erase(file_no);
	_file_path.erase(file_no);
	//Send(sockfd, WRITE_FINISHED);
	//Send(sockfd, file_no);
	return SUCCESS;
}

int IOnode::_get_parallel_number(size_t file_size)
{
	if(file_size/PARALLEL_IO_NUMBER < MB)
	{
		return (file_size+MB-1)/MB;
	}
	else
	{
		return PARALLEL_IO_NUMBER;
	}
}

int IOnode::_add_lio_job(ssize_t file_no, int *fd, int nitems, off_t start_point, void *buffer, int mode, size_t nbytes)
{
	lio_info *info=new lio_info(file_no, _master_io_comm_socket, fd, nitems, start_point, buffer, mode, nbytes);
	struct sigevent event;
	memset(&event, 0, sizeof(event));
	event.sigev_notify=SIGEV_THREAD;
	event.sigev_value.sival_ptr=info;
	event.sigev_notify_function=_lio_thread_fun;
	if(0 != lio_listio(LIO_NOWAIT, info->iolist, nitems, &event))
	{
		perror("lio_list");
		return FAILURE;
	}
	return SUCCESS;
}

void IOnode::_lio_thread_fun(union sigval info)
{
	lio_info * thread_info=reinterpret_cast<lio_info*>(info.sival_ptr);
	bool flag=false;
	for(int i=0;i<thread_info->nitems;++i)
	{
		ssize_t ret=aio_return(thread_info->iolist[i]);
		thread_info->iolist[i]->aio_offset+=ret;
		thread_info->iolist[i]->aio_buf+=ret;
		thread_info->iolist[i]->aio_nbytes-=ret;
		if(0 != thread_info->iolist[i]->aio_nbytes)
		{
			flag=true;
		}
	}
	if(flag)
	{
		//IO unfinished reissue IO request
		struct sigevent event;
		memset(&event, 0, sizeof(event));
		event.sigev_notify=SIGEV_THREAD;
		event.sigev_value.sival_ptr=info.sival_ptr;
		event.sigev_notify_function=_lio_thread_fun;
		lio_listio(LIO_NOWAIT, thread_info->iolist, thread_info->nitems, &event);
	}
	else
	{
		//IO finished
		//let master know and close all fd, delete aio_info class
		pthread_spin_lock(&thread_info->lock);
		Send(thread_info->master_socket, WAN_IO_FINISHED);
		Send(thread_info->master_socket, thread_info->file_no);
		pthread_spin_unlock(&thread_info->lock);
		delete thread_info;	
	}
}

void IOnode::_aio_thread_fun(union sigval info)
{
	aio_info *thread_info=reinterpret_cast<aio_info*>(info.sival_ptr);
	ssize_t ret=aio_return(&thread_info->cb);
	thread_info->cb.aio_nbytes-=ret;
	if(0 == thread_info->cb.aio_nbytes)
	{
		++thread_info->present_block;
		//io next block
		if(thread_info->block_set->end() != thread_info->present_block)
		{
			int fd=thread_info->cb.aio_fildes;
			memset(&thread_info->cb, 0, sizeof(struct sigevent));
			thread_info->cb.aio_fildes=fd;
			thread_info->cb.aio_offset=thread_info->present_block->start_point;
			thread_info->cb.aio_buf=thread_info->present_block->buffer;
			thread_info->cb.aio_nbytes=thread_info->present_block->size;
			thread_info->io_fun(&thread_info->cb);
		}
		//io finished
		else
		{
			close(thread_info->cb.aio_fildes);
		}
	}
	//current block io unfinished
	else
	{
		thread_info->cb.aio_offset+=ret;
		thread_info->cb.aio_nbytes-=ret;
		thread_info->io_fun(&thread_info->cb);
	}
}

int IOnode::_add_aio_job(block_info_t &file_block, int fd, off_t start_point, size_t size, aio_fun io_fun)
{
	io_blocks_t *block_set=_get_io_blocks(file_block, start_point, size);
	aio_info *info=new aio_info(block_set, io_fun);
	info->cb.aio_fildes=fd;
	info->cb.aio_offset=info->present_block->start_point;
	info->cb.aio_buf=info->present_block->buffer;
	info->cb.aio_nbytes=info->present_block->size;
	memset(&info->cb.aio_sigevent, 0, sizeof(struct sigevent));
	info->cb.aio_sigevent.sigev_notify=SIGEV_THREAD;
	info->cb.aio_sigevent.sigev_value.sival_ptr=info;
	info->cb.aio_sigevent.sigev_notify_function=_aio_thread_fun;
	if(0 == info->io_fun(&info->cb))
	{
		perror("aio_fun");
		return FAILURE;
	}
	return SUCCESS;
}
