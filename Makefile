# CC=g++

# all:
# 	$(CC) -o client_c2 client_c2.cpp
# 	$(CC) -o server_c1 server_c1.cpp
# clean: 
# 	rm -rf client_c2 server_c1

CC=g++

all: client_c2 server_c1

# "$@" 代表的是目标文件client_c2，“$^”代表的是依赖的文件client_c2.cpp 
client_c2: client_c2.cpp
	$(CC) -o $@ $^

# “$<”代表的是依赖文件中的第一个 server_c1.cpp	
server_c1: server_c1.cpp
	$(CC) -o $@ $<

clean: 
	rm -rf client_c2 server_c1
