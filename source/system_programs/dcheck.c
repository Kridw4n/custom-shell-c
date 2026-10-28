#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *fp;
    char buffer[1024];
    int count = 0;

    // Open the command pipeline
    fp = popen("ps -efj | grep dspawn | grep -Ev 'tty|pts' | grep -v grep", "r");
    if (fp == NULL) {
        perror("popen failed");
        return EXIT_FAILURE;
    }

    // Count matching lines (each line = one daemon process)
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        count++;
    }

    pclose(fp);

    // Output result
    printf("Number of live dspawn daemons: %d\n", count);
    return 0;
}
