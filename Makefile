# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -Wall -Wextra -std=c++11 -g

# 目标可执行文件
TARGET = app

# 源文件和头文件路径
ClientSRCDIR = mainClient Client log json tools openssl mysqlPool
ServerSRCDIR = mainServer log task threadpool Server json mysqlPool clientInfo openssl tools
INCLUDE = -Ilog 

# 找到所有源文件
CLIENT_SRC = $(foreach dir, $(ClientSRCDIR), $(wildcard $(dir)/*.cpp))
SERVER_SRC = $(foreach dir, $(ServerSRCDIR), $(wildcard $(dir)/*.cpp))

# 目标文件
CLIENT_OBJ = $(CLIENT_SRC:.cpp=.o)
SERVER_OBJ = $(SERVER_SRC:.cpp=.o)

# 依赖 log4cpp mysql opensll
LIBS = -llog4cpp -lmysqlcppconn -lcrypto -lssl 
# 目标可执行文件
CLIENT_TARGET = client
SERVER_TARGET = server

# 生成 client 可执行文件
$(CLIENT_TARGET): $(CLIENT_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 生成 server 可执行文件
$(SERVER_TARGET): $(SERVER_OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译所有 .cpp 文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# 清理
clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) $(CLIENT_TARGET) $(SERVER_TARGET)


# 默认目标（构建 client 和 server）
all: $(CLIENT_TARGET) $(SERVER_TARGET)
