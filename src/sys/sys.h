#ifndef SYS_H
#define SYS_H

typedef void (*callback_func)(void);

void sys_init();
void sys_run(callback_func update_callback);

#endif // SYS_H
