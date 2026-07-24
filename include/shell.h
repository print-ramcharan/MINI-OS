#ifndef SHELL_H
#define SHELL_H

#define MAX_SCRIPT_LINE_LEN 64
#define MAX_SCRIPT_RECURSION 4
#define MAX_HISTORY 8

void shell_task(void);
int shell_run_script(const char *filename);
void history_push(const char *cmd);
void history_print(void);

#endif
