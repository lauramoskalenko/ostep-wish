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

void do_path(char *args[], int count) {
    for (int i = 0; i < path_count; i++) {
        free(path_dirs[i]);
    }
    path_count = 0;

    for (int i = 1; i < count; i++) {
        if (path_count < MAX_PATHS) {
            path_dirs[path_count] = strdup(args[i]);
            path_count++;
        }
    }
}

void do_cd(char *args[], int count) {
    if (count != 2) {
        print_error();
        return;
    }
    if (chdir(args[1]) != 0) {
        print_error();
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];
    FILE *input = stdin;
    int interactive = 1;

    if (argc > 2) {
        print_error();
        exit(1);
    }
    if (argc == 2) {
        input = fopen(argv[1], "r");
        if (input == NULL) {
            print_error();
            exit(1);
        }
        interactive = 0; 
    }

    path_dirs[0] = strdup("/bin");
    path_count = 1;

    while (1) {
        if (interactive) {
            printf("wish> ");
            fflush(stdout);
        }

        ssize_t nread = getline(&line, &len, input);
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
            if (count != 1) {
                print_error();
            } else {
                free(line);
                exit(0);
            }
        } else if (strcmp(args[0], "cd") == 0) {
            do_cd(args, count);
        } else if (strcmp(args[0], "path") == 0) {
            do_path(args, count);
        } else {
            run_command(args);
        }
    }

    return 0;
}