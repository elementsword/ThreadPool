# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -Wall -Wextra -std=c++11 -g

# 目标可执行文件
TARGET = app

# 源文件和头文件路径
SRCDIR = main log task threadpool
INCLUDE = -Ilog

# 找到所有源文件
SRC = $(foreach dir, $(SRCDIR), $(wildcard $(dir)/*.cpp))

# 目标文件
OBJ = $(SRC:.cpp=.o)

# 依赖 log4cpp
LIBS = -llog4cpp

# 生成可执行文件
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

# 编译所有 .cpp 文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

# 清理
clean:
	rm -f $(OBJ) $(TARGET)

# 运行
run: $(TARGET)
	./$(TARGET)
