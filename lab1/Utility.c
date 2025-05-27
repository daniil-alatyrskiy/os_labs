#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include "Myshell.h"

/* Встроенная команда: cd */
void cmd_cd(char **args) {
    if (args[1] == NULL) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("Ошибка getcwd");
    } else {
        if (chdir(args[1]) != 0)
            perror("Ошибка cd");
        else {
            char cwd[PATH_MAX];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
                setenv("PWD", cwd, 1);
            else
                perror("Ошибка getcwd");
        }
    }
}

/* Встроенная команда: clr */
void cmd_clr() {
    system("clear");
}

/* Встроенная команда: dir */
void cmd_dir(char **args) {
    char *directory = (args[1] == NULL) ? "." : args[1];
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        perror("Ошибка dir");
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
        printf("%s\n", entry->d_name);
    closedir(dir);
}

/* Встроенная команда: environ */
void cmd_environ() {
    extern char **environ;
    for (char **env = environ; *env != NULL; env++)
        printf("%s\n", *env);
}

/* Встроенная команда: echo */
void cmd_echo(char **args) {
    for (int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);
        if (args[i+1] != NULL)
            printf(" ");
    }
    printf("\n");
}

/* Встроенная команда: help */
void cmd_help() {
    printf("myshell - простой интерпретатор командной строки\n");
    printf("Поддерживаемые команды:\n");
    printf("  cd <directory>    : смена текущего каталога или вывод текущего каталога, если аргумент отсутствует\n");
    printf("  clr               : очистка экрана\n");
    printf("  dir <directory>   : вывод содержимого каталога\n");
    printf("  environ           : вывод всех переменных окружения\n");
    printf("  echo <comment>    : вывод комментария\n");
    printf("  help              : вывод справки\n");
    printf("  pause             : приостановка оболочки до нажатия клавиши Enter\n");
    printf("  quit или exit     : выход из оболочки\n");
    printf("\nДополнительные возможности:\n");
    printf("  Перенаправление ввода/вывода ('<', '>', '>>')\n");
    printf("  Фоновое выполнение (символ '&' в конце команды)\n");
}
