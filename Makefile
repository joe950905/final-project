# File Server Makefile
# 编译配置文件

# 编译器
CC = gcc

# 编译选项
# -Wall: 显示所有警告
# -Wextra: 显示额外警告
# -g: 包含调试信息
# -O2: 优化级别2
CFLAGS = -Wall -Wextra -g -O2

# 链接库
# -lsqlite3: 链接SQLite3库
# -lpthread: 链接POSIX线程库（SQLite需要）
LDFLAGS = -lsqlite3 -lpthread

# 目标可执行文件
TARGET = fileserver

# 源文件
SOURCES = main.c server.c auth.c db.c

# 目标文件（.o文件）
OBJECTS = $(SOURCES:.c=.o)

# 头文件
HEADERS = server.h auth.h db.h

# 默认目标：编译整个项目
all: $(TARGET)
	@echo "Build complete! Run with: ./$(TARGET)"

# 链接目标文件生成可执行文件
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "✓ $(TARGET) created successfully"

# 编译规则：从.c文件生成.o文件
%.o: %.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	@echo "Cleaning up..."
	rm -f $(OBJECTS) $(TARGET) users.db
	@echo "✓ Cleanup complete"

# 清理并重新编译
rebuild: clean all

# 运行程序
run: $(TARGET)
	@echo "Starting File Server..."
	./$(TARGET)

# 检查依赖
check-deps:
	@echo "Checking dependencies..."
	@which $(CC) > /dev/null || (echo "Error: gcc not found" && exit 1)
	@echo "  ✓ gcc found"
	@pkg-config --exists sqlite3 || (echo "Error: sqlite3 not found. Install with: brew install sqlite3" && exit 1)
	@echo "  ✓ sqlite3 found"
	@echo "All dependencies satisfied!"

# 安装依赖（macOS）
install-deps-mac:
	@echo "Installing dependencies for macOS..."
	brew install sqlite3
	@echo "✓ Dependencies installed"

# 安装依赖（Ubuntu/Debian）
install-deps-ubuntu:
	@echo "Installing dependencies for Ubuntu/Debian..."
	sudo apt-get update
	sudo apt-get install -y build-essential libsqlite3-dev
	@echo "✓ Dependencies installed"

# 创建测试文件
test-setup:
	@echo "Setting up test files..."
	@mkdir -p files
	@echo "Hello from test file!" > files/test.txt
	@echo "This is sample data" > files/sample.txt
	@echo "✓ Test files created in files/ directory"

# 显示帮助信息
help:
	@echo "File Server Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all              - Compile the project (default)"
	@echo "  clean            - Remove compiled files"
	@echo "  rebuild          - Clean and recompile"
	@echo "  run              - Compile and run the server"
	@echo "  check-deps       - Check if dependencies are installed"
	@echo "  install-deps-mac - Install dependencies on macOS"
	@echo "  install-deps-ubuntu - Install dependencies on Ubuntu"
	@echo "  test-setup       - Create test files"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make              # Compile the project"
	@echo "  make run          # Compile and run"
	@echo "  make clean        # Clean up"

# 声明伪目标（不是真实文件）
.PHONY: all clean rebuild run check-deps install-deps-mac install-deps-ubuntu test-setup help
