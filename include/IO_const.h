#ifndef IO_CONST_H_
#define IO_CONST_H_

const int MASTER_PORT = 9000; 
const int MASTER_CONN_PORT = 9001; 
const int IONODE_PORT = 8000; 

const size_t MEMORY = 10000000000; 
const int MAX_BLOCK_NUMBER = 10; 
const int MAX_FILE_NUMBER = 1000; 
const int MAX_QUEUE = 100; 
const int BLOCK_SIZE = 1000; 
const int MAX_NODE_NUMBER = 10; 
const int LENGTH_OF_LISTEN_QUEUE = 10; 
const int MAX_SERVER_BUFFER = 1000; 
const int MAX_COMMAND_SIZE = 1000; 
const int MAX_CONNECT_TIME = 10;
const int MAX_QUERY_LENGTH = 100; 
const int WAIT_INTERVAL = -1;//30;
const int WAIT_TIME_SEC =1;
const int WAIT_TIME_NSEC = 0;
const int PARALLEL_IO_NUMBER = 10;

const int LAN_THROUGHPUT = 1;
const int WAN_THROUGHPUT = 1;
const int IONODE_THROUGHPUT = 1;
const int LAN=1;
const int WAN=1;

const bool CLEAN = false;
const bool DIRTY = true;
const int FAILURE = -1;
const int SUCCESS = 0;

const int CLOSE_PIPE = 0;
const int REGIST = 1; 
const int UNREGIST = 2;
const int PRINT_NODE_INFO = 3; 
const int GET_FILE_INFO = 4; 
const int OPEN_FILE = 5;
const int READ_FILE = 6;
const int WRITE_FILE = 7;
const int FLUSH_FILE = 8;
const int CLOSE_FILE = 9;
const int UNRECOGNISTED=10;
const int GET_FILE_META = 11;
const int IO_COMM_REGIST = 12;

const int SERVER_SHUT_DOWN = 20; 
const int I_AM_SHUT_DOWN = 21;
const int READ_FINISHED = 22;
const int WRITE_FINISHED = 24;
const int ARE_YOU_ALIVE = 25;
const int I_AM_ALIVE = 26;
const int IO_FINISHED = 27;
const int WAN_IO_FINISHED = 28;
const int LAN_IO_FINISHED = 29;

const int TOO_MANY_FILES =50;
const int FILE_NOT_FOUND = 51;
const int UNKNOWN_ERROR = 52;

const size_t KB=1000;
const size_t MB=KB*1000;
const size_t GB=MB*1000;

#endif
