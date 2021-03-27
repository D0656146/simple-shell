#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_PATH_LENGTH 256
#define MAX_CMD_LENGTH 256
#define MAX_SUBCMD_LENGTH 128
#define MAX_SUBCMD_NUM 32

bool read_parse_command_RP(char command[MAX_CMD_LENGTH + 2],
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

void run_shell() {
    printf("~ Simple Shell ~ \n");
    while (true) {
        // get and show current working directory
        char cwd[MAX_PATH_LENGTH];
        if (!getcwd(cwd, MAX_PATH_LENGTH)) {
            printf("Failed to get working diretory. \n");
            exit(EXIT_FAILURE);
        }
        printf("Shell: %s$ ", cwd);
        // read and parse command
        char command[MAX_CMD_LENGTH + 2];                     // for '\n' and '\0'
        char subcmds[MAX_SUBCMD_NUM][MAX_SUBCMD_LENGTH + 1];  // for '\0'
        if (!read_parse_command_RP(command, subcmds)) {
            continue;
        }
        // cd
        if (strcmp(subcmds[0], "cd") == 0) {
            if (strlen(subcmds[1]) == 0) {
                printf("Path is not given. \n");
                continue;
            }
            if (chdir(subcmds[1]) == -1) {
                printf("Failed to change directory to %s \n", subcmds[1]);
            }
            continue;
        }
    }
}
