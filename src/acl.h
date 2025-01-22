#ifndef ACL_H
#define ACL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define USER_MAX_LENGTH 6 * 2
#define MAX_USERS 1000

struct access_control_list {
	char users[MAX_USERS][USER_MAX_LENGTH];
	size_t user_count;
	const char *file_path;
};

void acl_load(struct access_control_list *acl, const char *file_path);
void acl_save(struct access_control_list *acl);
void acl_append_user(struct access_control_list *acl, const char *user);
void acl_remove_user(struct access_control_list *acl, const char *user);
bool acl_has_user(struct access_control_list *acl, const char *user);
uint32_t acl_hash(struct access_control_list *acl);
void acl_print(struct access_control_list *acl);

#endif // ACL_H
