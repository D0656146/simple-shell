#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 256
#define MAX_CMD_LENGTH 256
#define MAX_SUBCMD_LENGTH 128
#define MAX_SUBCMD_NUM 32

int ReadParseCommand_RP(char command[MAX_CMD_LENGTH + 3],
                        char subcmds[MAX_SUBCMD_NUM][MAX_SUBCMD_LENGTH + 1]) {
    memset(command, 0, sizeof(char) * MAX_CMD_LENGTH + 3);
    memset(subcmds, 0, sizeof(char) * MAX_SUBCMD_NUM * (MAX_SUBCMD_LENGTH + 1));
    // read command
    if (!fgets(command, MAX_CMD_LENGTH + 3, stdin)) {
        printf("STDIN reaches EOF. \n");
        exit(EXIT_FAILURE);
    }
    // flush stdin if command is too long
    if (command[MAX_CMD_LENGTH + 1] != '\0') {
        printf("The command is too long. \n");
        char flusher;
        while ((flusher = getchar()) != '\n' && flusher != EOF)
            ;
        return 0;
    }
    // split command
    int num_subcmds = 0;
    const char* whitespaces = " \t\n\v\f\r";
    char* subcmd = strtok(command, whitespaces);
    for (; subcmd != NULL; num_subcmds++) {
        // too much sub commands
        if (num_subcmds >= MAX_SUBCMD_NUM) {
            printf("There are too much sub commands. \n");
            return 0;
        }
        strncpy(subcmds[num_subcmds], subcmd, MAX_SUBCMD_LENGTH + 1);
        // sub command is too long
        if (subcmds[num_subcmds][MAX_SUBCMD_LENGTH] != '\0') {
            printf("The sub command is too long. \n");
            return 0;
        }
        subcmd = strtok(NULL, whitespaces);
    }
    return num_subcmds;
}

void InputRedirection(char filename[]) {
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
    if (dup2(file, STDIN_FILENO) == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
    if (close(file) == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
}

void OutputRedirection(char filename[]) {
    // append to old file or create new file with rw-r--r--
    int file = open(filename,
                    O_WRONLY | O_CREAT | O_APPEND,
                    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (file == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
    if (dup2(file, STDOUT_FILENO) == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
    if (close(file) == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
}

void ChildProcess(int cmd_start, char subcmds[MAX_SUBCMD_NUM][MAX_SUBCMD_LENGTH + 1]) {
    // ignore SIGCHLD to prevent zombie processes
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        perror("");
        exit(EXIT_FAILURE);
    }
    // parse sub command started at index of cmd_start
    char* subcmds_ptr[MAX_SUBCMD_NUM];
    for (int c_ptr = 0; cmd_start < MAX_SUBCMD_NUM; cmd_start++, c_ptr++) {
        subcmds_ptr[c_ptr] = subcmds[cmd_start];
        // output redirection
        if (strcmp(subcmds[cmd_start], ">") == 0) {
            cmd_start++;
            OutputRedirection(subcmds[cmd_start]);
            c_ptr--;
            continue;
        }
        // input redirection
        if (strcmp(subcmds[cmd_start], "<") == 0) {
            cmd_start++;
            InputRedirection(subcmds[cmd_start]);
            c_ptr--;
            continue;
        }
        // pipeline
        if (strcmp(subcmds[cmd_start], "|") == 0) {
            int fd[2];
            if (pipe(fd) == -1) {
                perror("");
                exit(EXIT_FAILURE);
            }
            pid_t pid = fork();
            if (pid == -1) {
                perror("");
            } else if (pid == 0) {
                // child process
                if (close(fd[1]) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd[0], STDIN_FILENO) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                if (close(fd[0]) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                ChildProcess(cmd_start + 1, subcmds);
            } else {
                // parent process
                if (close(fd[0]) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd[1], STDOUT_FILENO) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                if (close(fd[1]) == -1) {
                    perror("");
                    exit(EXIT_FAILURE);
                }
                subcmds_ptr[c_ptr] = NULL;
                break;
            }
        }
        // end of command
        if (strlen(subcmds[cmd_start]) == 0 || strcmp(subcmds[cmd_start], "&") == 0) {
            subcmds_ptr[c_ptr] = NULL;
            break;
        }
    }
    if (execvp(subcmds_ptr[0], subcmds_ptr) == -1) {
        perror("");
        exit(EXIT_FAILURE);
    }
}

void RunShell() {
    printf("~ Simple Shell ~ \n");
    while (true) {
        // get and show current working directory
        char cwd[MAX_PATH_LENGTH];
        if (!getcwd(cwd, MAX_PATH_LENGTH)) {
            perror("");
            exit(EXIT_FAILURE);
        }
        printf("Shell: %s$ ", cwd);
        // read and parse command
        char command[MAX_CMD_LENGTH + 3];                     // for '\n', '\0' and check
        char subcmds[MAX_SUBCMD_NUM][MAX_SUBCMD_LENGTH + 1];  // for '\0'
        int num_subcmds = ReadParseCommand_RP(command, subcmds);
        if (num_subcmds == 0) {
            continue;
        }
        // cd
        if (strcmp(subcmds[0], "cd") == 0) {
            if (strlen(subcmds[1]) == 0) {
                printf("Path is not given. \n");
                continue;
            }
            if (chdir(subcmds[1]) == -1) {
                perror("");
            }
            continue;
        }
        // execute process
        // ignore SIGCHLD to prevent zombie processes
        if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
            perror("");
            exit(EXIT_FAILURE);
        }
        // fork and run
        pid_t pid = fork();
        if (pid == -1) {
            perror("");
        } else if (pid == 0) {
            ChildProcess(0, subcmds);
        } else {
            // parent process
            if (strcmp(subcmds[num_subcmds - 1], "&") != 0) {
                wait(NULL);
            }
        }
    }
}
