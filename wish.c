#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_error() {
    char error_message[30] = "An error has occurred\n";
    write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char *argv[]) {
    char *line = NULL;
    size_t len = 0;

    while (1) {
        printf("wish> ");
        fflush(stdout);

        ssize_t nread = getline(&line, &len, stdin);
        if (nread == -1) {
            // EOF (Ctrl+D)
            free(line);
            exit(0);
        }

        if (line[nread - 1] == '\n') {
            line[nread - 1] = '\0';
        }

        if (strcmp(line, "exit") == 0) {
            free(line);
            exit(0);
        }
    }

    return 0;
}