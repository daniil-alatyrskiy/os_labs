#ifndef MYSHELL_H
#define MYSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#define MAX_INPUT_SIZE 1024
#define MAX_TOKENS 128

/* Прототипы встроенных команд */
void cmd_cd(char **args);
void cmd_clr();
void cmd_dir(char **args);
void cmd_environ();
void cmd_echo(char **args);
void cmd_help();

#endif /* MYSHELL_H */

