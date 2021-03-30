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

bool ReadParseCommand_RP(char command[MAX_CMD_LENGTH + 3],
                         char subcmds[MAX_SUBCMD_NUM][MAX_SUBCMD_LENGTH + 1]) {
    memset(command, 0, sizeof(char) * MAX_CMD_LENGTH + 2);
    memset(subcmds, 0, sizeof(char) * MAX_SUBCMD_NUM * (MAX_SUBCMD_LENGTH + 1));
    // read command
    if (!fgets(command, MAX_CMD_LENGTH + 3, stdin)) {
        printf("STDIN reaches EOF. \n");
        exit(EXIT_FAILURE);
    }
    // no command
    if (command[0] == '\n') {
        return false;
    }
    // flush stdin if command is too long
    if (command[MAX_CMD_LENGTH + 1] != '\0') {
        printf("The command is too long. \n");
        char flusher;
        while ((flusher = getchar()) != '\n' && flusher != EOF)
            ;
        return false;
    }
    // split command
    const char* whitespaces = " \t\n\v\f\r";
    char* subcmd = strtok(command, whitespaces);
    for (int num_commands = 0; subcmd != NULL; num_commands++) {
        // too much sub commands
        if (num_commands >= MAX_SUBCMD_NUM) {
            printf("There are too much sub commands. \n");
            return false;
        }
        strncpy(subcmds[num_commands], subcmd, MAX_SUBCMD_LENGTH + 1);
        // sub command is too long
        if (subcmds[num_commands][MAX_SUBCMD_LENGTH] != '\0') {
            printf("The sub command is too long. \n");
            return false;
        }
        subcmd = strtok(NULL, whitespaces);
    }
    return true;
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
        if (!ReadParseCommand_RP(command, subcmds)) {
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
            // child process
            char* subcmds_ptr[MAX_SUBCMD_NUM];
            for (int c_arr = 0, c_ptr = 0; c_arr < MAX_SUBCMD_NUM; c_arr++, c_ptr++) {
                subcmds_ptr[c_ptr] = subcmds[c_arr];
                // output redirection
                if (strcmp(subcmds[c_arr], ">") == 0) {
                    c_arr++;
                    OutputRedirection(subcmds[c_arr]);
                    c_ptr--;
                    continue;
                }
                // input redirection
                if (strcmp(subcmds[c_arr], "<") == 0) {
                    c_arr++;
                    InputRedirection(subcmds[c_arr]);
                    c_ptr--;
                    continue;
                }
                // end of command
                if (strlen(subcmds[c_arr]) == 0) {
                    subcmds_ptr[c_ptr] = NULL;
                    break;
                }
            }
            if (execvp(subcmds_ptr[0], subcmds_ptr) == -1) {
                perror("");
                exit(EXIT_FAILURE);
            }
        } else {
            // parent process
            wait(NULL);
        }
    }
}
