#ifndef AUTH_H
#define AUTH_H

#include <time.h>

#define MAX_SESSIONS 100
#define SESSION_TOKEN_LENGTH 32

/* Session结构体 */
typedef struct {
    char token[SESSION_TOKEN_LENGTH + 1];
    char username[64];
    int is_admin;
    time_t created_at;
} Session;

/* 初始化认证系统 */
void auth_init(void);

/* 用户登录，返回session token（需要调用者释放内存） */
char* auth_login(const char *username, const char *password);

/* 验证session token，返回用户名（不需要释放） */
const char* auth_verify_token(const char *token);

/* 检查token对应的用户是否为管理员 */
int auth_is_admin_token(const char *token);

/* 登出 */
void auth_logout(const char *token);

/* 清理过期的sessions */
void auth_cleanup_sessions(void);

#endif
