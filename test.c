#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        return 1;
    }

    if (pid == 0) {
        // Child process
        printf("Child (PID %d): Hello from the child!\n", getpid());
        fflush(stdout);     // Ensure the buffer is flushed
        _exit(0);           // Exit without running atexit handlers
    } else {
        // Parent process
        wait(NULL);         // Wait for the child to finish
        printf("Parent (PID %d): Child has terminated.\n", getpid());
    }

    return 0;
}
