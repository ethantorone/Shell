#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>


#define BUFF_SIZE 1024

char * input_redirection_file;
char * output_redirection_file;

char  buffer[BUFF_SIZE];

char* skipwhite(char* s) {
  while (isspace(*s)) ++s;
  return s;
}


/**
 *
 */
void tokenise_redirect_input_output(char *cmd_exec) {
    char *io_token[100];
    char *new_cmd_exec1;
    //new_cmd_exec1 = strdup(buffer);
    new_cmd_exec1 = buffer;
    int m = 1;
    io_token[0] = strtok(new_cmd_exec1, "<");
    while ((io_token[m] = strtok(NULL, ">")) != NULL)
        m++;

    io_token[1] = skipwhite(io_token[1]);
    io_token[2] = skipwhite(io_token[2]);

    input_redirection_file = strdup(io_token[1]);
    output_redirection_file = strdup(io_token[2]);

}

/**
 *
 */
void tokenise_redirect_input(char *cmd_exec) {
    char *i_token[100];
    char *new_cmd_exec1;
    //new_cmd_exec1 = strdup(buffer);
    new_cmd_exec1 = buffer;

    int m = 1;
    i_token[0] = strtok(new_cmd_exec1, "<");
    while ((i_token[m] = strtok(NULL, "<")) != NULL)
        m++;

    i_token[1] = skipwhite(i_token[1]);
    input_redirection_file = strdup(i_token[1]);
}

/**
 *
 */
void tokenise_redirect_output(char * cmd_exec) {
    char *o_token[100];
    char *new_cmd_exec1;
    //new_cmd_exec1 = strdup(buffer);
    new_cmd_exec1 = buffer;

    int m = 1;
    o_token[0]=strtok(new_cmd_exec1, ">");
    while((o_token[m] = strtok(NULL, ">"))!=NULL)
        m++;

    o_token[1]=skipwhite(o_token[1]);
    output_redirection_file = strdup(o_token[1]);

}

/**
 * Changes the CWD to the specified directory
 */
void changeDir(char * dir) {
    char path[100] = "./";
    strncat(path, dir, strlen(dir));
    chdir(path);
}

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
            perror("exec");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("fork");
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
    } else if (strcmp(argv[0], "cd") == 0) {
        changeDir(argv[1]);
    } else {
        execute(argc, argv);
    }

}

/**
 * Reads a line from standard input and parses it into an array of c-strings
 */
void readCommand() {

    int input_fd, output_fd, input_redirection, output_redirection;
    int rres = read(STDIN_FILENO, buffer, BUFF_SIZE);
    //printf("%d: %s\n", rres, buffer);
    buffer[rres] = '\0';

    char * redirect  = strdup(buffer);

    if (strchr(buffer, '<') && strchr(buffer, '>')) {
        input_redirection = 1;
        output_redirection = 1;
        tokenise_redirect_input_output(redirect);
    }
    else if (strchr(buffer, '<')) {
        input_redirection=1;
        tokenise_redirect_input(redirect);
    }
    else if (strchr(buffer, '>')) {
        output_redirection=1;
        tokenise_redirect_output(redirect);
    }
    if (output_redirection == 1) {
        output_fd= creat(output_redirection_file, 0644);
        if (output_fd < 0) {
            fprintf(stderr, "Failed to open %s for writing\n", output_redirection_file);
            exit(EXIT_FAILURE);
        }
        dup2(output_fd, 1);
        close(output_fd);
        output_redirection=0;
    }
    if (input_redirection  == 1) {
        input_fd=open(input_redirection_file,O_RDONLY, 0);
        if (input_fd < 0) {
            fprintf(stderr, "Failed to open %s for reading\n", input_redirection_file);
            exit(EXIT_FAILURE);
        }
        dup2(input_fd, 0);
        close(input_fd);
        input_redirection=0;
    }

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

    int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
    while(1) {
        printPrompt();
        readCommand();

        dup2(in, STDIN_FILENO);
        dup2(out, STDOUT_FILENO);

        fflush(stdin);
        fflush(stdout);
    }
}
