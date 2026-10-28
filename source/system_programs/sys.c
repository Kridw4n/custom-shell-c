#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>

static void print_windows_logo() {
    printf(
        "       .--.           .--.       \n"
        "     .'.:. '.       .' .:.'     \n"
        "    /:.   .:\\     /:.   .:\\    \n"
        "   |::.   .::|   |::.   .::|   \n"
        "    \\::.   .:/     \\::.   .:/    \n"
        "     '.'.:. .'       '. .:'     \n"
        "       '--'           '--'       \n"
        "      WINDOWS 10/11              \n\n"
    );
}

static void print_linux_logo() {
    printf(
        "     .--.     \n"
        "    |o_o |    \n"
        "    |:_/ |    \n"
        "   //   \\ \\   \n"
        "  (|     | )  \n"
        " /'\\_   _/`\\ \n"
        " \\___)=(___/  \n\n"
    );
}

void print_username_hostname() {
    char hostname[1024];
    gethostname(hostname, sizeof(hostname));
    struct passwd *pw = getpwuid(getuid());
    printf("%s@%s\n", pw->pw_name, hostname);
}

void print_os_info() {
    FILE *os = fopen("/etc/os-release", "r");
    if (!os) { perror("OS Info"); return; }
    char line[256];
    while (fgets(line, sizeof(line), os)) {
        if (strncmp(line, "PRETTY_NAME=", 12) == 0) {
            line[strcspn(line, "\n")] = '\0';
            printf("OS: %s\n", line + 13);
            break;
        }
    }
    fclose(os);
}

void print_kernel_info() {
    struct utsname buf;
    if (uname(&buf) != 0) { perror("uname"); return; }
    printf("Kernel: %s %s\n", buf.sysname, buf.release);
}

void print_uptime() {
    struct sysinfo info;
    sysinfo(&info);
    long u = info.uptime;
    int days    = u / 86400;
    int hours   = (u / 3600) % 24;
    int minutes = (u / 60) % 60;
    printf("Uptime: %dd %dh %dm\n", days, hours, minutes);
}

void print_shell() {
    char *shell = getenv("SHELL");
    printf("Shell: %s\n", shell ? shell : "Unknown");
}

void print_cpu_info() {
    FILE *cpu = fopen("/proc/cpuinfo", "r");
    if (!cpu) { perror("CPU Info"); return; }
    char line[256];
    while (fgets(line, sizeof(line), cpu)) {
        if (strncmp(line, "model name", 10) == 0) {
            char *c = strchr(line, ':');
            if (c) { printf("CPU: %s", c + 2); break; }
        }
    }
    fclose(cpu);
}

void print_mem_info() {
    struct sysinfo info;
    sysinfo(&info);
    long total = info.totalram / (1024 * 1024);
    long free  = info.freeram  / (1024 * 1024);
    long used  = total - free;
    printf("Memory: %ldMB / %ldMB\n", used, total);
}

int main(void) {
    printf("\n==== System Info (sys.c) ====\n\n");
#ifdef _WIN32
    print_windows_logo();
#else
    print_linux_logo();
#endif
    print_username_hostname();
    print_os_info();
    print_kernel_info();
    print_uptime();
    print_shell();
    print_cpu_info();
    print_mem_info();
    printf("\n=============================\n");
    return 0;
}
