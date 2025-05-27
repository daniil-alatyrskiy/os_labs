#define _POSIX_C_SOURCE 200809L


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <fcntl.h>     // Для open()
#include <sys/types.h> // Для pid_t и других типов
#include <sys/wait.h>  // Для waitpid()
#include <errno.h>     // Для realpath()

#include "Myshell.h"

extern char **environ;

int main(int argc, char *argv[]) {
    char input[MAX_INPUT_SIZE];
    char *tokens[MAX_TOKENS];
    char *cmdArgs[MAX_TOKENS];
    char *inputFile = NULL;
    char *outputFile = NULL;
    int append = 0;      // 0 – перезапись, 1 – дозапись (>>)
    int background = 0;  // Флаг фонового выполнения

    /* Определяем полный путь к myshell */
    char shell_path[PATH_MAX];
    if (realpath(argv[0], shell_path) == NULL) {
        char cwd[PATH_MAX];
        if(getcwd(cwd, sizeof(cwd)) != NULL)
            snprintf(shell_path, sizeof(shell_path), "%s/%s", cwd, argv[0]);
        else
            strcpy(shell_path, "myshell");
    }
    /* Устанавливаем переменную среды shell */
    setenv("shell", shell_path, 1);

    /* Определяем источник ввода: интерактивный или batch-файл */
    FILE *inStream = stdin;
    if (argc == 2) {
        inStream = fopen(argv[1], "r");
        if (inStream == NULL) {
            perror("Ошибка открытия batch файла");
            exit(EXIT_FAILURE);
        }
    } else if (argc > 2) {
        fprintf(stderr, "Использование: %s [batchfile]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    while (1) {
        /* Вывод приглашения: путь к текущему каталогу */
        if (inStream == stdin) {
            char cwd[PATH_MAX];
            if(getcwd(cwd, sizeof(cwd)) != NULL)
                printf("%s> ", cwd);
            else
                printf("myshell> ");
        }

        if (fgets(input, sizeof(input), inStream) == NULL)
            break; // конец файла или ошибка ввода

        size_t len = strlen(input);
        if (len > 0 && input[len-1] == '\n')
            input[len-1] = '\0';

        if (strlen(input) == 0)
            continue;

        /* Разбиваем строку на токены по пробелам и табуляциям */
        int tokenCount = 0;
        char *token = strtok(input, " \t");
        while (token != NULL && tokenCount < MAX_TOKENS - 1) {
            tokens[tokenCount++] = token;
            token = strtok(NULL, " \t");
        }
        tokens[tokenCount] = NULL;

        /* Обработка токенов */
        int j = 0, i = 0;
        inputFile = NULL;
        outputFile = NULL;
        append = 0;
        background = 0;
        while (i < tokenCount) {
            if (strcmp(tokens[i], "<") == 0) {
                if (i+1 < tokenCount) {
                    inputFile = tokens[i+1];
                    i += 2;
                } else {
                    fprintf(stderr, "Ошибка: ожидается имя файла после '<'\n");
                    break;
                }
            } else if (strcmp(tokens[i], ">>") == 0) {
                if (i+1 < tokenCount) {
                    outputFile = tokens[i+1];
                    append = 1;
                    i += 2;
                } else {
                    fprintf(stderr, "Ошибка: ожидается имя файла после '>>'\n");
                    break;
                }
            } else if (strcmp(tokens[i], ">") == 0) {
                if (i+1 < tokenCount) {
                    outputFile = tokens[i+1];
                    append = 0;
                    i += 2;
                } else {
                    fprintf(stderr, "Ошибка: ожидается имя файла после '>'\n");
                    break;
                }
            } else if (strcmp(tokens[i], "&") == 0) {
                background = 1;
                i++;
            } else {
                cmdArgs[j++] = tokens[i++];
            }
        }
        cmdArgs[j] = NULL;

        if (cmdArgs[0] == NULL)
            continue;

        /* Обработка встроенных команд */
        if (strcmp(cmdArgs[0], "cd") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_cd(cmdArgs);
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "clr") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_clr();
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "dir") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_dir(cmdArgs);
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "environ") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_environ();
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "echo") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_echo(cmdArgs);
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "help") == 0) {
            int saved_stdin = dup(STDIN_FILENO);
            int saved_stdout = dup(STDOUT_FILENO);
            if (inputFile) {
                int fd = open(inputFile, O_RDONLY);
                if (fd < 0) perror("Ошибка открытия файла для ввода");
                else { dup2(fd, STDIN_FILENO); close(fd); }
            }
            if (outputFile) {
                int fd;
                if (append)
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                else
                    fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd < 0) perror("Ошибка открытия файла для вывода");
                else { dup2(fd, STDOUT_FILENO); close(fd); }
            }
            cmd_help();
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdin);
            close(saved_stdout);
        } else if (strcmp(cmdArgs[0], "pause") == 0) {
            printf("Нажмите Enter для продолжения...\n");
            fflush(stdout);
            while(getchar() != '\n');
        } else if (strcmp(cmdArgs[0], "quit") == 0 || strcmp(cmdArgs[0], "exit") == 0) {
            break;
        } else {
            /* Внешняя команда */
            pid_t pid = fork();
            if (pid < 0) {
                perror("Ошибка fork");
            } else if (pid == 0) {
                /* В дочернем процессе устанавливаем переменную parent */
                setenv("parent", shell_path, 1);
                /* Настраиваем перенаправление ввода */
                if (inputFile) {
                    int fd = open(inputFile, O_RDONLY);
                    if (fd < 0) {
                        perror("Ошибка открытия файла для ввода");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                    close(fd);
                }
                /* Настраиваем перенаправление вывода */
                if (outputFile) {
                    int fd;
                    if (append)
                        fd = open(outputFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
                    else
                        fd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) {
                        perror("Ошибка открытия файла для вывода");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    close(fd);
                }
                if (execvp(cmdArgs[0], cmdArgs) == -1) {
                    perror("Ошибка выполнения команды");
                }
                exit(EXIT_FAILURE);
            } else {
                /* Если команда не в фоне, ждём завершения */
                if (!background) {
                    int status;
                    waitpid(pid, &status, 0);
                } else {
                    printf("Команда запущена в фоне, PID: %d\n", pid);
                }
            }
        }
    }

    if (inStream != stdin)
        fclose(inStream);

    return 0;
}

