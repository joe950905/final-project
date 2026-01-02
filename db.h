#ifndef DB_H
#define DB_H

#include <sqlite3.h>

/* 数据库初始化 */
int db_init(const char *db_path);

/* 创建用户 - 返回1表示成功，0表示失败 */
int db_create_user(const char *username, const char *password, int is_admin);

/* 验证用户 - 返回1表示成功，0表示失败 */
int db_verify_user(const char *username, const char *password);

/* 检查用户是否为管理员 - 返回1表示是管理员，0表示不是 */
int db_is_admin(const char *username);

/* 删除用户 - 返回1表示成功，0表示失败 */
int db_delete_user(const char *username);

/* 列出所有用户 - 返回用户列表字符串（需要调用者释放内存） */
char* db_list_users(void);

/* 关闭数据库 */
void db_close(void);

/* 获取全局数据库连接 */
sqlite3* db_get_connection(void);

#endif
