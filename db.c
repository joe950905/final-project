#include "db.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static sqlite3 *db = NULL;

/* 
 * 技术一：Pointer（指针）
 * 说明：使用指针来管理SQLite数据库连接
 * 这里使用静态指针db来维护全局数据库连接，避免重复打开关闭
 */
sqlite3* db_get_connection(void) {
    return db;
}

/*
 * 技术二：读写档（文件操作）
 * 说明：通过SQLite对数据库文件进行读写操作
 * 这个函数初始化数据库，创建users表，并插入默认管理员账号
 */
int db_init(const char *db_path) {
    char *err_msg = NULL;
    int rc;
    
    // 打开数据库文件
    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }
    
    // 创建users表的SQL语句
    const char *sql = 
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT UNIQUE NOT NULL,"
        "password TEXT NOT NULL,"
        "is_admin INTEGER DEFAULT 0);";
    
    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    // 检查是否已存在管理员账号
    sqlite3_stmt *stmt;
    const char *check_admin = "SELECT COUNT(*) FROM users WHERE username = 'admin';";
    rc = sqlite3_prepare_v2(db, check_admin, -1, &stmt, NULL);
    
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            
            // 如果没有管理员账号，创建默认管理员
            if (count == 0) {
                printf("Creating default admin account (username: admin, password: admin123)\n");
                db_create_user("admin", "admin123", 1);
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }
    
    // 检查是否已存在 guest 账号
    const char *check_guest = "SELECT COUNT(*) FROM users WHERE username = 'guest';";
    rc = sqlite3_prepare_v2(db, check_guest, -1, &stmt, NULL);
    
    if (rc == SQLITE_OK) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            int count = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            
            // 如果没有 guest 账号，创建默认 guest
            if (count == 0) {
                printf("Creating default guest account (username: guest, password: guest)\n");
                db_create_user("guest", "guest", 0);
            }
        } else {
            sqlite3_finalize(stmt);
        }
    }
    
    printf("Database initialized successfully\n");
    return 1;
}

/*
 * 技术三：Array（数组）和 String 操作
 * 说明：使用字符数组存储SQL查询语句和处理字符串
 */
int db_create_user(const char *username, const char *password, int is_admin) {
    char *err_msg = NULL;
    char sql[512];  // 使用数组存储SQL语句
    
    // 使用snprintf安全地构建SQL语句
    snprintf(sql, sizeof(sql), 
             "INSERT INTO users (username, password, is_admin) VALUES ('%s', '%s', %d);",
             username, password, is_admin);
    
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to create user: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    printf("User '%s' created successfully\n", username);
    return 1;
}

/*
 * 技术四：While Loop（while循环）
 * 说明：使用while循环处理SQL查询结果
 */
int db_verify_user(const char *username, const char *password) {
    sqlite3_stmt *stmt;
    char sql[512];
    
    snprintf(sql, sizeof(sql),
             "SELECT password FROM users WHERE username = '%s';",
             username);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
        return 0;
    }
    
    // 使用while循环遍历查询结果
    int verified = 0;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *stored_password = (const char*)sqlite3_column_text(stmt, 0);
        if (strcmp(stored_password, password) == 0) {
            verified = 1;
            break;
        }
    }
    
    sqlite3_finalize(stmt);
    return verified;
}

/*
 * 检查用户是否为管理员
 */
int db_is_admin(const char *username) {
    sqlite3_stmt *stmt;
    char sql[512];
    
    snprintf(sql, sizeof(sql),
             "SELECT is_admin FROM users WHERE username = '%s';",
             username);
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return 0;
    }
    
    int is_admin = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        is_admin = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return is_admin;
}

/*
 * 删除用户
 */
int db_delete_user(const char *username) {
    // 防止删除管理员账号
    if (strcmp(username, "admin") == 0) {
        fprintf(stderr, "Cannot delete default admin account\n");
        return 0;
    }
    
    char *err_msg = NULL;
    char sql[512];
    
    snprintf(sql, sizeof(sql),
             "DELETE FROM users WHERE username = '%s';",
             username);
    
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to delete user: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 0;
    }
    
    printf("User '%s' deleted successfully\n", username);
    return 1;
}

/*
 * 技术五：动态内存分配（malloc/realloc）
 * 说明：动态分配内存来存储用户列表
 */
char* db_list_users(void) {
    sqlite3_stmt *stmt;
    const char *sql = "SELECT username, is_admin FROM users;";
    
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        return NULL;
    }
    
    // 动态分配初始内存
    size_t buffer_size = 1024;
    char *result = (char*)malloc(buffer_size);
    if (!result) {
        sqlite3_finalize(stmt);
        return NULL;
    }
    
    strcpy(result, "Users:\n");
    
    // 遍历所有用户
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char *username = (const char*)sqlite3_column_text(stmt, 0);
        int is_admin = sqlite3_column_int(stmt, 1);
        
        char line[256];
        snprintf(line, sizeof(line), "  - %s %s\n", 
                 username, is_admin ? "(Admin)" : "");
        
        // 如果缓冲区不够，重新分配更大的内存
        if (strlen(result) + strlen(line) + 1 > buffer_size) {
            buffer_size *= 2;
            char *new_result = (char*)realloc(result, buffer_size);
            if (!new_result) {
                free(result);
                sqlite3_finalize(stmt);
                return NULL;
            }
            result = new_result;
        }
        
        strcat(result, line);
    }
    
    sqlite3_finalize(stmt);
    return result;
}

void db_close(void) {
    if (db) {
        sqlite3_close(db);
        db = NULL;
        printf("Database closed\n");
    }
}
