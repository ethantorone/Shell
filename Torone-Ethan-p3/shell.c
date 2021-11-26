#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#define BUFF_SIZE 1024


/**
 * Executes a process using fork(2) and exec(3)
 *
 * @param argc the length of argv, or the number of arguments
 * @param argv the arguments
 */
void execute(int argc, char * argv[]) {
    /*
    for (int i = 0; i < argc; i++) {
        printf("%s\n", argv[i]);
    }
    */


    pid_t pid = -1;
    int status;

    pid = fork();

    if (pid == 0) {
        // Child process
        if (execvp(argv[0], argv) == -1) {
            perror("");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("lsh");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
    //puts("Hello!");

}

/**
 *
 */
void launch(int argc, char * argv[]) {
    /*
    printf("%d\n", argc);
    for (int i = 0; argv[i] != NULL; i++) {
        printf("%s\t", argv[i]);
    }
    puts("");
    */
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }

}

/**
 * Reads a line from standard input and parses it into an array of c-strings
 */
void readCommand() {
    char  buffer[BUFF_SIZE];
    int rres = read(STDIN_FILENO, buffer, BUFF_SIZE);
    //printf("%d: %s\n", rres, buffer);
    buffer[rres] = '\0';

    int i = 0, tokSize = 64;

    //can initially hold 64 tokens
    char ** tokens = malloc(tokSize * sizeof(char*));
    char * token;
    char * delims = " \t\r\n\a";

    //if tokens == 0, it does not have a memory address
    if (!tokens) {
        perror("allocation error\n");
        exit(EXIT_FAILURE);
    }

    //takes the strign up until one of the delims, then returns the start of that string
    token = strtok(buffer, delims);

    //loops through buffer to each token,
    while (token != NULL) {
        tokens[i] = token;
        //puts(token);
        i++;

        //there are more tokens than tokSize, size of tokens* must be increased
        if (i >= tokSize) {
            tokSize += tokSize;
            tokens = realloc(tokens, tokSize * sizeof(char*));

            //if tokens == 0, then it does not have a memory address
            if (!tokens) {
                perror("allocation error\n");
                exit(EXIT_FAILURE);
            } //if
        } //if

        //consecutive calls only require NULL as a parameter, returns the next token
        token = strtok(NULL, delims);
    } //while
    tokens[i] = NULL; //safeguard, whatever is after the last token is NULL
    launch(i, tokens);

    free(tokens);
}

/**
 * Prints "1730sh:(pwd)$ " to standard output
 */
void printPrompt() {
    char prompt[100] = "1730sh:", path[100], sp[2] = "$ ";
    getcwd(path, 100);
    if (strlen(path) >= 20) {
        char temp[100] = "~";
        strncat(temp, path + 18, strlen(path));
        strcpy(path, temp);
    }
    strncat(prompt, path, strlen(path));
    strncat(prompt, sp, 2);
    printf("%s", prompt);
}


int main(int argc, char * argv[]) {
    setbuf(stdout, NULL);
    chdir("/home/");
    while(1) {
        printPrompt();
        readCommand();
    }
}
