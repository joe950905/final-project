#ifndef SERVER_H
#define SERVER_H

#define PORT 7979
#define BUFFER_SIZE 8192
#define FILES_DIR "./files/"

/* HTTP请求类型 */
typedef enum {
    GET,
    POST,
    DELETE,
    UNKNOWN
} HttpMethod;

/* HTTP请求结构 */
typedef struct {
    HttpMethod method;
    char path[256];
    char token[64];
    char body[BUFFER_SIZE];
    int body_length;
} HttpRequest;

/* 初始化服务器 */
int server_init(void);

/* 启动服务器 */
void server_start(void);

/* 停止服务器 */
void server_stop(void);

/* 处理客户端请求 */
void handle_client(int client_socket);

/* 解析HTTP请求 */
void parse_http_request(const char *request, HttpRequest *http_req);

/* 发送HTTP响应 */
void send_response(int socket, int status_code, const char *content_type, 
                   const char *body, int body_length);

/* 处理文件列表请求 */
void handle_list_files(int socket, const char *token);

/* 处理文件下载请求 */
void handle_download_file(int socket, const char *token, const char *filename);

/* 处理文件上传请求 */
void handle_upload_file(int socket, const char *token, const char *filename, 
                        const char *data, int data_length);

/* 处理文件删除请求 */
void handle_delete_file(int socket, const char *token, const char *filename);

/* 处理登录请求 */
void handle_login(int socket, const char *body);

/* 处理用户管理请求 */
void handle_create_user(int socket, const char *token, const char *body);
void handle_delete_user(int socket, const char *token, const char *body);
void handle_list_users(int socket, const char *token);

#endif
