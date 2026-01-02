#include "auth.h"
#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 * 技术六：Array（数组）- Session管理
 * 说明：使用数组来存储多个用户session
 * 这里定义了一个Session数组，最多可以同时管理100个用户会话
 */
static Session sessions[MAX_SESSIONS];
static int session_count = 0;

/* 
 * 初始化认证系统
 * 技术七：For Loop（for循环）
 * 说明：使用for循环初始化所有session
 */
void auth_init(void) {
    for (int i = 0; i < MAX_SESSIONS; i++) {
        memset(&sessions[i], 0, sizeof(Session));
    }
    session_count = 0;
    printf("Auth system initialized\n");
}

/*
 * 生成随机token
 * 技术八：递归（Recursion）概念和随机数生成
 * 说明：使用随机数生成session token
 */
static void generate_token(char *token) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    
    // 使用当前时间作为随机数种子
    srand((unsigned int)time(NULL) + session_count);
    
    for (int i = 0; i < SESSION_TOKEN_LENGTH; i++) {
        int index = rand() % (sizeof(charset) - 1);
        token[i] = charset[index];
    }
    token[SESSION_TOKEN_LENGTH] = '\0';
}

/*
 * 查找空闲的session slot
 * 技术九：While Loop（while循环）
 * 说明：使用while循环查找第一个空闲的session位置
 */
static int find_free_session_slot(void) {
    int i = 0;
    while (i < MAX_SESSIONS) {
        if (sessions[i].username[0] == '\0') {
            return i;
        }
        i++;
    }
    return -1;  // 没有空闲slot
}

/*
 * 用户登录
 * 技术十：Pointer（指针）- 返回动态分配的字符串
 */
char* auth_login(const char *username, const char *password) {
    // 验证用户名和密码
    if (!db_verify_user(username, password)) {
        fprintf(stderr, "Invalid username or password\n");
        return NULL;
    }
    
    // 查找空闲的session slot
    int slot = find_free_session_slot();
    if (slot == -1) {
        fprintf(stderr, "Too many active sessions\n");
        return NULL;
    }
    
    // 创建新session
    Session *session = &sessions[slot];
    generate_token(session->token);
    strncpy(session->username, username, sizeof(session->username) - 1);
    session->is_admin = db_is_admin(username);
    session->created_at = time(NULL);
    session_count++;
    
    // 动态分配内存返回token
    char *token = (char*)malloc(SESSION_TOKEN_LENGTH + 1);
    if (token) {
        strcpy(token, session->token);
    }
    
    printf("User '%s' logged in successfully (Admin: %s)\n", 
           username, session->is_admin ? "Yes" : "No");
    
    return token;
}

/*
 * 验证token
 * 技术十一：Do-While Loop（do-while循环）
 * 说明：使用do-while循环查找匹配的session
 */
const char* auth_verify_token(const char *token) {
    if (!token || strlen(token) != SESSION_TOKEN_LENGTH) {
        return NULL;
    }
    
    int i = 0;
    do {
        if (sessions[i].username[0] != '\0' && 
            strcmp(sessions[i].token, token) == 0) {
            // 检查session是否过期（24小时）
            time_t now = time(NULL);
            if (difftime(now, sessions[i].created_at) > 86400) {
                // Session过期，清除
                memset(&sessions[i], 0, sizeof(Session));
                session_count--;
                return NULL;
            }
            return sessions[i].username;
        }
        i++;
    } while (i < MAX_SESSIONS);
    
    return NULL;
}

/*
 * 检查token对应的用户是否为管理员
 */
int auth_is_admin_token(const char *token) {
    if (!token || strlen(token) != SESSION_TOKEN_LENGTH) {
        return 0;
    }
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].username[0] != '\0' && 
            strcmp(sessions[i].token, token) == 0) {
            return sessions[i].is_admin;
        }
    }
    
    return 0;
}

/*
 * 登出
 */
void auth_logout(const char *token) {
    if (!token) return;
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (strcmp(sessions[i].token, token) == 0) {
            printf("User '%s' logged out\n", sessions[i].username);
            memset(&sessions[i], 0, sizeof(Session));
            session_count--;
            return;
        }
    }
}

/*
 * 清理过期的sessions
 * 技术十二：For Loop和时间处理
 * 说明：遍历所有session，清理超过24小时的session
 */
void auth_cleanup_sessions(void) {
    time_t now = time(NULL);
    int cleaned = 0;
    
    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (sessions[i].username[0] != '\0') {
            if (difftime(now, sessions[i].created_at) > 86400) {
                memset(&sessions[i], 0, sizeof(Session));
                session_count--;
                cleaned++;
            }
        }
    }
    
    if (cleaned > 0) {
        printf("Cleaned %d expired sessions\n", cleaned);
    }
}
