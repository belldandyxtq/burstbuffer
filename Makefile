SERVER_LIB = server
CLIENT_LIB = client
MASTER_LIB = Master
IONODE_LIB = IOnode
USER_LIB = userclient
QUERY_LIB = query

MASTER = Master
IONODE = IOnode
SERVER = Server
CLIENT = Client
QUERY = Query_Client
USER = User_Client
CONST = IO_const

MASTER_MAIN = master_main
IONODE_MAIN = node_main
USER_MAIN = user_main
USER_CLIENT = user_client

INCLUDE = include
SRC = src
LIB = lib
BIN = bin

CC = g++
LD = ld
#FLAG = -O3 -Wall
FLAG = -g -Wall
LIBFLAG = -shared -fPIC
C11 = -std=c++0x
LIB_PATH = -L./ -L./lib

vpath %.h ./include
vpath %.so ./lib
vpath %.cpp ./src

run:
	@echo 'run make Master'
	@echo 'or make IOnode'
	@echo 'or make User_main'
	@echo 'or make User_client'

lib$(SERVER_LIB).so: $(SERVER).cpp $(SERVER).h $(CONST).h
	$(CC) $(C11) $(LIBFLAG) $(FLAG) -I./ -o lib$(SERVER_LIB).so $(SRC)/$(SERVER).cpp

lib$(CLIENT_LIB).so: $(CLIENT).cpp $(CLIENT).h $(CONST).h
	$(CC) $(LIBFLAG) $(FLAG) -I./ -o lib$(CLIENT_LIB).so $(SRC)/$(CLIENT).cpp

lib$(QUERY_LIB).so: $(QUERY).cpp $(QUERY).h $(CONST).h
	$(CC) $(LIBFLAG) $(FLAG) -I./ -o lib$(QUERY_LIB).so $(SRC)/$(QUERY).cpp

lib$(USER_LIB).so: $(USER).cpp $(USER).h $(CONST).h
	$(CC) $(LIBFLAG) $(FLAG) -I./ -lrt -o lib$(USER_LIB).so $(SRC)/$(USER).cpp

lib$(MASTER_LIB).so: $(MASTER_LIB).cpp $(MASTER_LIB).h $(CONST).h
	$(CC) $(C11) $(LIBFLAG) $(FLAG) -I./ -o lib$(MASTER_LIB).so $(SRC)/$(MASTER).cpp

lib$(IONODE_LIB).so: $(IONODE).cpp $(IONODE).h $(CONST).h
	$(CC) $(LIBFLAG) $(C11) $(FLAG) -I./ -lrt -o lib$(IONODE_LIB).so $(SRC)/$(IONODE).cpp

$(MASTER_MAIN): lib$(SERVER_LIB).so lib$(MASTER_LIB).so $(MASTER_MAIN).cpp $(CONST).h
	$(CC) $(C11) $(FLAG) -I./ $(LIB_PATH) -o $(MASTER_MAIN) $(SRC)/$(MASTER_MAIN).cpp -lrt -l$(SERVER_LIB) -l$(MASTER_LIB)

$(IONODE_MAIN): lib$(SERVER_LIB).so lib$(IONODE_LIB).so lib$(CLIENT_LIB).so $(IONODE).cpp  $(CONST).h
	$(CC) $(FLAG) -I./ $(LIB_PATH) -o $(IONODE_MAIN) $(SRC)/$(IONODE_MAIN).cpp -lrt -l$(SERVER_LIB) -l$(CLIENT_LIB) -l$(IONODE_LIB)

$(USER_MAIN): lib$(CLIENT_LIB).so lib$(QUERY_LIB).so $(USER_MAIN).cpp $(CONST).h
	$(CC) $(FLAG) -I./ $(LIB_PATH) -o $(USER_MAIN) $(SRC)/$(USER_MAIN).cpp -l$(CLIENT_LIB) -l$(QUERY_LIB)

$(USER_CLIENT): lib$(CLIENT_LIB).so lib$(USER_LIB).so $(USER_CLIENT).cpp $(CONST).h
	$(CC) $(FLAG) -I./ $(LIB_PATH) -o $(USER_CLIENT) $(SRC)/$(USER_CLIENT).cpp -lrt -l$(CLIENT_LIB) -l$(USER_LIB)

.PHONY:
Master:$(MASTER_MAIN)
	mkdir -p bin
	mkdir -p lib
	mv $(MASTER_MAIN) bin
	mv *.so lib

IOnode:$(IONODE_MAIN)
	mkdir -p lib
	mkdir -p bin
	mv $(IONODE_MAIN) bin
	mv *.so lib

User_main:$(USER_MAIN)
	mkdir -p lib
	mkdir -p bin
	mv $(USER_MAIN) bin
	mv *.so lib

User_client:$(USER_CLIENT)
	mkdir -p lib
	mkdir -p bin
	mv $(USER_CLIENT) bin
	mv *.so lib

clean:
	rm $(BIN)/*
	rm $(LIB)/*
