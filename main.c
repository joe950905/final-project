#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "server.h"
#include "auth.h"
#include "db.h"

/*
 * File Server - 最终项目
 * 
 * 这个项目实现了一个完整的文件服务器，包含以下功能：
 * 1. 用户认证系统（登录/登出）
 * 2. 文件管理（上传/下载/列表/删除）
 * 3. 用户管理（管理员可以创建/删除用户）
 * 4. SQLite数据库存储用户信息
 * 5. HTTP服务器处理客户端请求
 * 
 * 技术要点：
 * - Array, Pointer, 循环(for/while/do-while), Switch
 * - 文件读写操作
 * - 动态内存分配
 * - 自定义库（db.h/c, auth.h/c, server.h/c）
 * - SQLite数据库（课外技术一）
 * - Socket网络编程（课外技术二）
 */

volatile sig_atomic_t shutdown_requested = 0;

/* 
 * 信号处理函数
 * 用于优雅地关闭服务器（Ctrl+C）
 */
void signal_handler(int signum) {
    if (signum == SIGINT) {
        printf("\n\nShutdown signal received...\n");
        shutdown_requested = 1;
        server_stop();
    }
}

/*
 * 打印使用说明
 */
void print_usage(void) {
    printf("\n=================================================\n");
    printf("         File Server - Usage Guide\n");
    printf("=================================================\n\n");
    
    printf("Default Admin Account:\n");
    printf("  Username: admin\n");
    printf("  Password: admin123\n\n");
    
    printf("Available Endpoints:\n");
    printf("  POST   http://localhost:7878/login\n");
    printf("         Body: username=admin&password=admin123\n\n");
    
    printf("  GET    http://localhost:7878/files\n");
    printf("         List all available files\n");
    printf("         Header: Authorization: Bearer <token>\n\n");
    
    printf("  GET    http://localhost:7878/files/<filename>\n");
    printf("         Download a specific file\n");
    printf("         Header: Authorization: Bearer <token>\n\n");
    
    printf("  POST   http://localhost:7878/files\n");
    printf("         Upload a file\n");
    printf("         Header: Authorization: Bearer <token>\n");
    printf("         Body: filename=test.txt&data=<file_content>\n\n");
    
    printf("  DELETE http://localhost:7878/files/<filename>\n");
    printf("         Delete a file (Admin only)\n");
    printf("         Header: Authorization: Bearer <token>\n\n");
    
    printf("  GET    http://localhost:7878/users\n");
    printf("         List all users (Admin only)\n");
    printf("         Header: Authorization: Bearer <token>\n\n");
    
    printf("  POST   http://localhost:7878/users\n");
    printf("         Create a new user (Admin only)\n");
    printf("         Header: Authorization: Bearer <token>\n");
    printf("         Body: username=newuser&password=pass123\n\n");
    
    printf("  DELETE http://localhost:7878/users\n");
    printf("         Delete a user (Admin only)\n");
    printf("         Header: Authorization: Bearer <token>\n");
    printf("         Body: username=newuser\n\n");
    
    printf("Example with curl:\n");
    printf("  # Login\n");
    printf("  curl -X POST http://localhost:7878/login \\\n");
    printf("       -d 'username=admin&password=admin123'\n\n");
    
    printf("  # List files (replace TOKEN with your token)\n");
    printf("  curl http://localhost:7878/files \\\n");
    printf("       -H 'Authorization: Bearer TOKEN'\n\n");
    
    printf("  # Upload file\n");
    printf("  curl -X POST http://localhost:7878/files \\\n");
    printf("       -H 'Authorization: Bearer TOKEN' \\\n");
    printf("       -d 'filename=hello.txt&data=Hello World!'\n\n");
    
    printf("=================================================\n\n");
}

int main(int argc, char *argv[]) {
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║        File Server - Final Project           ║\n");
    printf("║              Starting up...                   ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    // 注册信号处理函数
    signal(SIGINT, signal_handler);
    
    // 初始化数据库
    printf("Initializing database...\n");
    if (!db_init("users.db")) {
        fprintf(stderr, "Failed to initialize database\n");
        return 1;
    }
    
    // 初始化认证系统
    printf("Initializing authentication system...\n");
    auth_init();
    
    // 初始化服务器
    printf("Initializing server...\n");
    if (!server_init()) {
        fprintf(stderr, "Failed to initialize server\n");
        db_close();
        return 1;
    }
    
    // 打印使用说明
    print_usage();
    
    // 启动服务器（阻塞）
    printf("Starting server...\n");
    server_start();
    
    // 清理资源
    printf("\nCleaning up...\n");
    auth_cleanup_sessions();
    db_close();
    
    printf("\n");
    printf("╔═══════════════════════════════════════════════╗\n");
    printf("║           Server Shutdown Complete            ║\n");
    printf("║              Goodbye! 😉                      ║\n");
    printf("╚═══════════════════════════════════════════════╝\n\n");
    
    return 0;
}
