#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void launch(int argc, char * argv[]) {
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
            perror("lsh");
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
    printf("%s\n", prompt);
}


int main(int argc, char * argv[]) {
    setbuf(stdout, NULL);
    printPrompt();
}
