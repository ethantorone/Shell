#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <ctype.h>


#define BUFF_SIZE 1024

int input_redirection, output_redirection, append, in, out;

char * input_redirection_file;
char * output_redirection_file;

char * cmd;
char * delims = " \t\r\n\a";


/**
 * Places the file after the '>' as the new output redirection file and
 * the file after the '<' as the new input refirection file.
 */
void tokenise_redirect_input_output(char *cmd_exec) {
    char *io_token[100];
    char *new_cmd_exec1;
    //dupes string as to not modify original
    new_cmd_exec1 = strdup(cmd_exec);

    int m = 1;
    //snags filename that's after the '<'
    io_token[0] = strtok(new_cmd_exec1, "<");
    //snags filename that's after the '>'
    while ((io_token[m] = strtok(NULL, ">")) != NULL)
        m++;

    //gets rid of whitespace
    io_token[1] = strtok(io_token[1], delims);
    io_token[2] = strtok(io_token[2], delims);

    //copies down file names
    input_redirection_file = strdup(io_token[1]);
    output_redirection_file = strdup(io_token[2]);

    cmd = strdup(io_token[0]);

}

/**
 * Places the file after the '<' as the new input redirection file.
 */
void tokenise_redirect_input(char *cmd_exec) {
    char *i_token[100];
    char *new_cmd_exec1;
    //dupes string as to not modify original
    new_cmd_exec1 = strdup(cmd_exec);

    int m = 1;
    //snags filename that's after the '<'
    i_token[0] = strtok(new_cmd_exec1, "<");
    while ((i_token[m] = strtok(NULL, "<")) != NULL)
        m++;

    //gets rid of whitespace and copies down file name
    i_token[1] = strtok(i_token[1], delims);
    input_redirection_file = strdup(i_token[1]);

    cmd = strdup(i_token[0]);
}

/**
 * Places the file after the '>' as the new output redirection file.
 */
void tokenise_redirect_output(char * cmd_exec) {

    if (strstr(cmd_exec, ">>")) {
        append = 1;
    }

    char *o_token[100];
    char *new_cmd_exec1;
    //dupes string as to not modify original
    new_cmd_exec1 = strdup(cmd_exec);

    //snags filename that's after the '>'
    int m = 1;
    o_token[0]=strtok(new_cmd_exec1, ">");
    while((o_token[m] = strtok(NULL, ">"))!=NULL)
        m++;

    //gets rid of whitespace and copies down file names
    o_token[1]=strtok(o_token[1], delims);
    output_redirection_file = strdup(o_token[1]);

    cmd = strdup(o_token[0]);
}

/**
 * Sets stdin and stdout back to normal.
 */
void reset_io() {
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);

    fflush(stdin);
    fflush(stdout);
}

/**
 * Checks if input/output redirection is flagged on and then redirects
 * input/output with dup2();
 */
void redirect_io() {
    int input_fd, output_fd;
    //if output redirection is flagged on, redirects output
    if (output_redirection == 1) {
        if (append == 1) {
            output_fd = open(output_redirection_file, O_APPEND | O_CREAT | O_RDWR);
        } else {
            output_fd = creat(output_redirection_file, 0644);
        }
        if (output_fd < 0) {
            fprintf(stderr, "Failed to open %s for writing\n", output_redirection_file);
            reset_io();
            output_redirection = 0;
            return;;
        }
        dup2(output_fd, 1);
        close(output_fd);
        append = 0;
        output_redirection=0;
    }
    //if input redirection is flagged on, redirects input
    if (input_redirection == 1) {
        input_fd = open(input_redirection_file,O_RDONLY, 0);
        if (input_fd < 0) {
            fprintf(stderr, "Failed to open %s for reading\n", input_redirection_file);
            reset_io();
            input_redirection = 0;
            return;
        }
        dup2(input_fd, 0);
        close(input_fd);
        input_redirection=0;
    }
}


/**
 * Changes the CWD to the specified directory
 */
void changeDir(char * dir) {
    char path[100] = "./";
    strncat(path, dir, strlen(dir));
    if (chdir(path) == -1) perror("Directory not found\n");
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

        exit(EXIT_SUCCESS);
    } else if (pid < 0) {
        // Error forking
        perror("fork");
    } else {
        // Parent process
        do {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }
}

/**
 * Either executes a exit, cd, or an executable.
 */
void launch(int argc, char * argv[]) {
    /*
    printf("%d\n", argc);
    for (int i = 0; argv[i] != NULL; i++) {
        printf("%s\t", argv[i]);
    }
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
    char  buffer[BUFF_SIZE];
    int rres = read(STDIN_FILENO, buffer, BUFF_SIZE);
    //printf("%d: %s\n", rres, buffer);
    buffer[rres] = '\0';

    cmd = strdup(buffer);
    //checks if io redirection is necessary, flags them 'on', and copies file name(s).
    if (strchr(buffer, '<') && strchr(buffer, '>')) {
        input_redirection = 1;
        output_redirection = 1;
        tokenise_redirect_input_output(cmd);
    } else if (strchr(buffer, '<')) {
        input_redirection=1;
        tokenise_redirect_input(cmd);
    } else if (strchr(buffer, '>')) {
        output_redirection=1;
        tokenise_redirect_output(cmd);
    }
    redirect_io();
    int i = 0, tokSize = 64;

    //can initially hold 64 tokens
    char ** tokens = malloc(tokSize * sizeof(char*));
    char * token;

    //if tokens == 0, it does not have a memory address
    if (!tokens) {
        perror("allocation error\n");
        exit(EXIT_FAILURE);
    }

    //takes the strign up until one of the delims, then returns the start of that string
    token = strtok(cmd, delims);

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
    char * home = getenv("HOME");
    int hl = strlen(home);
    getcwd(path, 100);

    if (strstr(path, home) != NULL) {
        char temp[100] = "~";
        strncat(temp, path + hl, strlen(path));
        strcpy(path, temp);
    }

    strncat(prompt, path, strlen(path));
    strncat(prompt, sp, 2);
    printf("%s", prompt);
}

/**
 * Main process loop. Calls printPrompt() and readCommand(),
 * then sets stdin back to keyboard and stdout back to the terminal.
 */
int main(int argc, char * argv[]) {
    setbuf(stdout, NULL);
    chdir(getenv("HOME"));
    in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
    while(1) {
        printPrompt();
        readCommand();
        reset_io();
    }
}
