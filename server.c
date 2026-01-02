#include "server.h"
#include "auth.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>

/*
 * 课外技术一：网络编程 (Socket API)
 * 说明：使用POSIX Socket API实现HTTP服务器
 * - socket(): 创建套接字
 * - bind(): 绑定端口
 * - listen(): 监听连接
 * - accept(): 接受客户端连接
 * - send()/recv(): 发送/接收数据
 */

static int server_socket = -1;
static int running = 0;

/* 初始化服务器 */
int server_init(void) {
    struct sockaddr_in server_addr;
    
    // 创建socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        return 0;
    }
    
    // 设置socket选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        close(server_socket);
        return 0;
    }
    
    // 配置服务器地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // 绑定socket到端口
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_socket);
        return 0;
    }
    
    // 开始监听
    if (listen(server_socket, 10) < 0) {
        perror("Listen failed");
        close(server_socket);
        return 0;
    }
    
    printf("Server initialized on port %d\n", PORT);
    return 1;
}

/* 启动服务器 */
void server_start(void) {
    running = 1;
    printf("Server started. Access at http://localhost:%d/files\n", PORT);
    
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // 接受客户端连接
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                perror("Accept failed");
            }
            continue;
        }
        
        printf("Client connected from %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        // 处理客户端请求
        handle_client(client_socket);
        close(client_socket);
    }
}

/* 停止服务器 */
void server_stop(void) {
    running = 0;
    if (server_socket >= 0) {
        close(server_socket);
        server_socket = -1;
    }
    printf("Server stopped\n");
}

/*
 * 技术十三：Switch语句
 * 说明：使用switch根据HTTP方法和路径分发请求
 */
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));
    
    // 接收HTTP请求
    ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;
    }
    
    // 解析HTTP请求
    HttpRequest http_req;
    parse_http_request(buffer, &http_req);
    
    printf("Request: %d %s\n", http_req.method, http_req.path);
    
    // 根据路径分发请求
    if (strcmp(http_req.path, "/login") == 0 && http_req.method == POST) {
        handle_login(client_socket, http_req.body);
    }
    else if (strcmp(http_req.path, "/files") == 0) {
        switch (http_req.method) {
            case GET:
                handle_list_files(client_socket, http_req.token);
                break;
            case POST:
                // 从body中解析文件名和数据
                handle_upload_file(client_socket, http_req.token, "uploaded_file", 
                                  http_req.body, http_req.body_length);
                break;
            default:
                send_response(client_socket, 405, "text/plain", 
                            "Method Not Allowed", 18);
                break;
        }
    }
    else if (strncmp(http_req.path, "/files/", 7) == 0) {
        const char *filename = http_req.path + 7;
        
        switch (http_req.method) {
            case GET:
                handle_download_file(client_socket, http_req.token, filename);
                break;
            case DELETE:
                handle_delete_file(client_socket, http_req.token, filename);
                break;
            default:
                send_response(client_socket, 405, "text/plain", 
                            "Method Not Allowed", 18);
                break;
        }
    }
    else if (strcmp(http_req.path, "/users") == 0) {
        switch (http_req.method) {
            case GET:
                handle_list_users(client_socket, http_req.token);
                break;
            case POST:
                handle_create_user(client_socket, http_req.token, http_req.body);
                break;
            case DELETE:
                handle_delete_user(client_socket, http_req.token, http_req.body);
                break;
            default:
                send_response(client_socket, 405, "text/plain", 
                            "Method Not Allowed", 18);
                break;
        }
    }
    else if (strcmp(http_req.path, "/") == 0 || strcmp(http_req.path, "/index.html") == 0) {
        // 提供 HTML 客戶端介面
        FILE *html_file = fopen("client.html", "r");
        if (!html_file) {
            const char *msg = "client.html not found";
            send_response(client_socket, 404, "text/plain", msg, strlen(msg));
            return;
        }
        
        // 讀取整個 HTML 文件
        fseek(html_file, 0, SEEK_END);
        long file_size = ftell(html_file);
        fseek(html_file, 0, SEEK_SET);
        
        char *html_content = malloc(file_size + 1);
        if (!html_content) {
            fclose(html_file);
            const char *msg = "Memory allocation failed";
            send_response(client_socket, 500, "text/plain", msg, strlen(msg));
            return;
        }
        
        fread(html_content, 1, file_size, html_file);
        html_content[file_size] = '\0';
        fclose(html_file);
        
        // 發送 HTML 響應
        send_response(client_socket, 200, "text/html; charset=utf-8", html_content, file_size);
        free(html_content);
    }
    else {
        const char *msg = "Welcome to File Server!\n\n"
                         "Available endpoints:\n"
                         "POST /login - Login with username and password\n"
                         "GET /files - List all files\n"
                         "GET /files/<filename> - Download a file\n"
                         "POST /files - Upload a file\n"
                         "DELETE /files/<filename> - Delete a file (admin only)\n"
                         "GET /users - List all users (admin only)\n"
                         "POST /users - Create a new user (admin only)\n"
                         "DELETE /users - Delete a user (admin only)\n";
        send_response(client_socket, 200, "text/plain", msg, strlen(msg));
    }
}

/*
 * 技术十四：字符串处理和Pointer操作
 * 说明：解析HTTP请求头，提取方法、路径、token等信息
 */
void parse_http_request(const char *request, HttpRequest *http_req) {
    char *line_end;
    char first_line[512];
    
    memset(http_req, 0, sizeof(HttpRequest));
    
    // 提取第一行 (例如: GET /files HTTP/1.1)
    line_end = strstr(request, "\r\n");
    if (line_end) {
        int len = line_end - request;
        if (len > sizeof(first_line) - 1) len = sizeof(first_line) - 1;
        strncpy(first_line, request, len);
        first_line[len] = '\0';
    } else {
        strncpy(first_line, request, sizeof(first_line) - 1);
    }
    
    // 解析方法和路径
    char method[16];
    sscanf(first_line, "%s %s", method, http_req->path);
    
    if (strcmp(method, "GET") == 0) http_req->method = GET;
    else if (strcmp(method, "POST") == 0) http_req->method = POST;
    else if (strcmp(method, "DELETE") == 0) http_req->method = DELETE;
    else http_req->method = UNKNOWN;
    
    // 查找Authorization header
    const char *auth_header = strstr(request, "Authorization: Bearer ");
    if (auth_header) {
        auth_header += 22;  // 跳过 "Authorization: Bearer "
        sscanf(auth_header, "%63s", http_req->token);
    }
    
    // 查找请求体
    const char *body_start = strstr(request, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        int body_len = strlen(body_start);
        if (body_len > sizeof(http_req->body) - 1) {
            body_len = sizeof(http_req->body) - 1;
        }
        memcpy(http_req->body, body_start, body_len);
        http_req->body_length = body_len;
    }
}

/* 发送HTTP响应 */
void send_response(int socket, int status_code, const char *content_type, 
                   const char *body, int body_length) {
    char header[512];
    const char *status_text;
    
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 201: status_text = "Created"; break;
        case 401: status_text = "Unauthorized"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        case 500: status_text = "Internal Server Error"; break;
        default: status_text = "Unknown"; break;
    }
    
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "\r\n",
             status_code, status_text, content_type, body_length);
    
    send(socket, header, strlen(header), 0);
    if (body && body_length > 0) {
        send(socket, body, body_length, 0);
    }
}

/*
 * 课外技术二：文件系统操作 (POSIX File/Directory API)
 * 说明：使用opendir, readdir, stat等函数操作文件系统
 */
void handle_list_files(int socket, const char *token) {
    // 验证token
    const char *username = auth_verify_token(token);
    if (!username) {
        send_response(socket, 401, "text/plain", "Unauthorized", 12);
        return;
    }
    
    DIR *dir = opendir(FILES_DIR);
    if (!dir) {
        send_response(socket, 500, "text/plain", "Cannot open files directory", 27);
        return;
    }
    
    char response[BUFFER_SIZE];
    strcpy(response, "Available files:\n\n");
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 获取文件信息
        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s%s", FILES_DIR, entry->d_name);
        
        struct stat file_stat;
        if (stat(filepath, &file_stat) == 0) {
            char file_info[256];
            snprintf(file_info, sizeof(file_info), "  - %s (Size: %lld bytes)\n", 
                    entry->d_name, (long long)file_stat.st_size);
            strcat(response, file_info);
        }
    }
    
    closedir(dir);
    
    if (strlen(response) == 18) {  // 只有标题，没有文件
        strcat(response, "  (No files available)\n");
    }
    
    send_response(socket, 200, "text/plain", response, strlen(response));
}

/*
 * 技术十五：文件读写操作
 * 说明：使用fopen, fread, fwrite等函数进行文件I/O
 */
void handle_download_file(int socket, const char *token, const char *filename) {
    // 验证token
    const char *username = auth_verify_token(token);
    if (!username) {
        send_response(socket, 401, "text/plain", "Unauthorized", 12);
        return;
    }
    
    // 构建文件路径
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_DIR, filename);
    
    // 打开文件
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        send_response(socket, 404, "text/plain", "File not found", 14);
        return;
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // 读取文件内容
    char *file_data = (char*)malloc(file_size);
    if (!file_data) {
        fclose(file);
        send_response(socket, 500, "text/plain", "Memory allocation failed", 24);
        return;
    }
    
    size_t read_size = fread(file_data, 1, file_size, file);
    fclose(file);
    
    if (read_size != file_size) {
        free(file_data);
        send_response(socket, 500, "text/plain", "Failed to read file", 19);
        return;
    }
    
    // 发送文件
    send_response(socket, 200, "application/octet-stream", file_data, file_size);
    free(file_data);
    
    printf("File '%s' downloaded by user '%s'\n", filename, username);
}

void handle_upload_file(int socket, const char *token, const char *filename, 
                        const char *data, int data_length) {
    // 验证token
    const char *username = auth_verify_token(token);
    if (!username) {
        send_response(socket, 401, "text/plain", "Unauthorized", 12);
        return;
    }
    
    // 从body中解析文件名（简化版本，实际应该解析multipart/form-data）
    // 这里假设body格式为: filename=test.txt&data=...
    char actual_filename[256] = "uploaded_file.txt";
    const char *filename_param = strstr(data, "filename=");
    if (filename_param) {
        filename_param += 9;
        const char *end = strchr(filename_param, '&');
        if (end) {
            int len = end - filename_param;
            if (len > sizeof(actual_filename) - 1) len = sizeof(actual_filename) - 1;
            strncpy(actual_filename, filename_param, len);
            actual_filename[len] = '\0';
        }
    }
    
    // 构建文件路径
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_DIR, actual_filename);
    
    // 写入文件
    FILE *file = fopen(filepath, "wb");
    if (!file) {
        send_response(socket, 500, "text/plain", "Failed to create file", 21);
        return;
    }
    
    // 查找实际数据部分
    const char *file_data = strstr(data, "data=");
    if (file_data) {
        file_data += 5;
        fwrite(file_data, 1, data_length - (file_data - data), file);
    } else {
        fwrite(data, 1, data_length, file);
    }
    
    fclose(file);
    
    char msg[256];
    snprintf(msg, sizeof(msg), "File '%s' uploaded successfully", actual_filename);
    send_response(socket, 201, "text/plain", msg, strlen(msg));
    
    printf("File '%s' uploaded by user '%s'\n", actual_filename, username);
}

void handle_delete_file(int socket, const char *token, const char *filename) {
    // 验证token
    const char *username = auth_verify_token(token);
    if (!username) {
        send_response(socket, 401, "text/plain", "Unauthorized", 12);
        return;
    }
    
    // 检查是否为管理员
    if (!auth_is_admin_token(token)) {
        send_response(socket, 403, "text/plain", "Forbidden: Admin only", 21);
        return;
    }
    
    // 构建文件路径
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "%s%s", FILES_DIR, filename);
    
    // 删除文件
    if (unlink(filepath) != 0) {
        send_response(socket, 404, "text/plain", "File not found", 14);
        return;
    }
    
    char msg[256];
    snprintf(msg, sizeof(msg), "File '%s' deleted successfully", filename);
    send_response(socket, 200, "text/plain", msg, strlen(msg));
    
    printf("File '%s' deleted by admin '%s'\n", filename, username);
}

void handle_login(int socket, const char *body) {
    char username[64] = {0};
    char password[64] = {0};
    
    // 解析username和password (格式: username=admin&password=admin123)
    const char *user_param = strstr(body, "username=");
    const char *pass_param = strstr(body, "password=");
    
    if (user_param && pass_param) {
        user_param += 9;
        const char *user_end = strchr(user_param, '&');
        if (user_end) {
            int len = user_end - user_param;
            if (len > sizeof(username) - 1) len = sizeof(username) - 1;
            strncpy(username, user_param, len);
        }
        
        pass_param += 9;
        const char *pass_end = strchr(pass_param, '&');
        int len = pass_end ? (pass_end - pass_param) : strlen(pass_param);
        if (len > sizeof(password) - 1) len = sizeof(password) - 1;
        strncpy(password, pass_param, len);
    }
    
    // 尝试登录
    char *token = auth_login(username, password);
    if (!token) {
        send_response(socket, 401, "text/plain", "Invalid credentials", 19);
        return;
    }
    
    // 只返回純 token，方便前端使用
    send_response(socket, 200, "text/plain", token, strlen(token));
    free(token);
}

void handle_create_user(int socket, const char *token, const char *body) {
    // 验证是否为管理员
    if (!auth_is_admin_token(token)) {
        send_response(socket, 403, "text/plain", "Forbidden: Admin only", 21);
        return;
    }
    
    char username[64] = {0};
    char password[64] = {0};
    
    // 解析参数
    const char *user_param = strstr(body, "username=");
    const char *pass_param = strstr(body, "password=");
    
    if (user_param && pass_param) {
        user_param += 9;
        const char *user_end = strchr(user_param, '&');
        if (user_end) {
            int len = user_end - user_param;
            if (len > sizeof(username) - 1) len = sizeof(username) - 1;
            strncpy(username, user_param, len);
        }
        
        pass_param += 9;
        const char *pass_end = strchr(pass_param, '&');
        int len = pass_end ? (pass_end - pass_param) : strlen(pass_param);
        if (len > sizeof(password) - 1) len = sizeof(password) - 1;
        strncpy(password, pass_param, len);
    }
    
    if (db_create_user(username, password, 0)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "User '%s' created successfully", username);
        send_response(socket, 201, "text/plain", msg, strlen(msg));
    } else {
        send_response(socket, 500, "text/plain", "Failed to create user", 21);
    }
}

void handle_delete_user(int socket, const char *token, const char *body) {
    // 验证是否为管理员
    if (!auth_is_admin_token(token)) {
        send_response(socket, 403, "text/plain", "Forbidden: Admin only", 21);
        return;
    }
    
    char username[64] = {0};
    
    // 解析username
    const char *user_param = strstr(body, "username=");
    if (user_param) {
        user_param += 9;
        const char *user_end = strchr(user_param, '&');
        int len = user_end ? (user_end - user_param) : strlen(user_param);
        if (len > sizeof(username) - 1) len = sizeof(username) - 1;
        strncpy(username, user_param, len);
    }
    
    if (db_delete_user(username)) {
        char msg[128];
        snprintf(msg, sizeof(msg), "User '%s' deleted successfully", username);
        send_response(socket, 200, "text/plain", msg, strlen(msg));
    } else {
        send_response(socket, 500, "text/plain", "Failed to delete user", 21);
    }
}

void handle_list_users(int socket, const char *token) {
    // 验证是否为管理员
    if (!auth_is_admin_token(token)) {
        send_response(socket, 403, "text/plain", "Forbidden: Admin only", 21);
        return;
    }
    
    char *users = db_list_users();
    if (users) {
        send_response(socket, 200, "text/plain", users, strlen(users));
        free(users);
    } else {
        send_response(socket, 500, "text/plain", "Failed to list users", 20);
    }
}
