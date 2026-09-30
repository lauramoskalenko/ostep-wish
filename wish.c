#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 64

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

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;
    char *args[MAX_ARGS];

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

        for (int i = 0; i < count; i++) {
            printf("args[%d] = '%s'\n", i, args[i]);
        }
    }

    return 0;
}