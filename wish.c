#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_PATHS 64

char *path_dirs[MAX_PATHS];
int path_count = 0;

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int parse_line(char *line, char *args[]) {
    int count = 0;
    char *rest = line;
    char *token;

    while ((token = strsep(&rest, " \t")) != NULL) {
        if (token[0] == '\0') {
            continue; 
        }
        if (count < MAX_ARGS - 1) {
            args[count] = token;
            count++;
        }
    }
    args[count] = NULL; 
    return count;
}

int find_executable(char *cmd, char *result, size_t size) {
    for (int i = 0; i < path_count; i++) {
        snprintf(result, size, "%s/%s", path_dirs[i], cmd);
        if (access(result, X_OK) == 0) {
            return 1;
        }
    }
    return 0;
}

void run_command(char *args[]) {
    char full_path[1024];

    if (find_executable(args[0], full_path, sizeof(full_path)) == 0) {
        print_error();
        return;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return;
    }

    if (pid == 0) {
        execv(full_path, args);
        print_error();
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];

    path_dirs[0] = "/bin";
    path_count = 1;

    while (1) {
        printf("wish> ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) {
            free(line);
            exit(0);
        }

        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        int count = parse_line(line, args);

        if (count == 0) {
            continue;
        }

        if (strcmp(args[0], "exit") == 0) {
            free(line);
            exit(0);
        }

        run_command(args);
    }

    return 0;
}