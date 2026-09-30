#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 64
#define MAX_PATHS 64
#define MAX_CMDS 64

// список папок, где ищем программы
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

pid_t run_command(char *args[], char *outfile) {
    char full_path[1024];

    if (find_executable(args[0], full_path, sizeof(full_path)) == 0) {
        print_error();
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        print_error();
        return -1;
    }

    if (pid == 0) {
        if (outfile != NULL) {
            int fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                print_error();
                exit(1);
            }
            dup2(fd, STDOUT_FILENO); 
            dup2(fd, STDERR_FILENO);
            close(fd);
        }
        execv(full_path, args);
        print_error();
        exit(1);
    }

    return pid;
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

void handle_command(char *cmd, char *line, pid_t pids[], int *npids) {
    char *args[MAX_ARGS];
    char *files[MAX_ARGS];
    char *outfile = NULL;

    char *gt = strchr(cmd, '>');
    if (gt != NULL) {
        *gt = '\0';
        char *right = gt + 1;

        if (strchr(right, '>') != NULL) {
            print_error();
            return;
        }

        int fcount = parse_line(right, files);
        if (fcount != 1) {
            print_error();
            return;
        }
        outfile = files[0];
    }

    int count = parse_line(cmd, args);

    if (count == 0) {
        if (outfile != NULL) {
            print_error();
        }
        return;
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
        pid_t pid = run_command(args, outfile);
        if (pid > 0 && *npids < MAX_CMDS) {
            pids[*npids] = pid;
            (*npids)++;
        }
    }
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
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

        pid_t pids[MAX_CMDS];
        int npids = 0;

        char *rest = line;
        char *cmd;
        while ((cmd = strsep(&rest, "&")) != NULL) {
            handle_command(cmd, line, pids, &npids);
        }

        for (int i = 0; i < npids; i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }

    return 0;
}
