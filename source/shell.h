#include <limits.h> // For PATH_MAX
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>

#define MAX_LINE 1024
#define MAX_ARGS 64
#define BIN_PATH "./bin/"
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"
#define PROFILE_FILE ".cseshell_profile"

void print_usercolor(const char *format, ...);
void print_errorcolor(const char *format, ...);

const char *builtin_commands[] = {
    "cd",    // Changes the current directory of the shell to the specified path. If no path is given, it defaults to the user's home directory.
    "help",  //  List all builtin commands in the shell
    "exit",  // Exits the shell
    "usage", // Provides a brief usage guide for the shell and its built-in command
    "env", // Lists all the environment variables currently set in the shell
    "setenv", // Sets or modifies an environment variable for this shell session
    "unsetenv", // Removes an environment variable from the shell
    "clear", // Clears the cmd
    "setpref"
    };

/*
Handler of each shell builtin function
*/
// Change directory
int shell_cd(char **args) {
    if (args[1] == NULL) {
        print_errorcolor("cd: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0) {
            print_errorcolor("cd: %s\n", strerror(errno));
        }
    }
    return 1;
}

// Print help
int shell_help(char **args) {
    (void)args;
    print_usercolor("Simple Shell\n");
    print_usercolor("Built-in commands:\n");

    int count = sizeof(builtin_commands) / sizeof(char *);
    for (int i = 0; i < count; i++) {
        print_usercolor("  %s\n", builtin_commands[i]);
    }

    print_usercolor("\nUse 'usage <command>' to see specific usage info.\n");
    return 1;
}

int shell_exit(char **args){
    (void)args;
    return 0;
}

// Print usage
int shell_usage(char **args) {
    if (args[1] == NULL) {
        print_usercolor("Command not given. Type usage <command>.\n");
        return 1;
    }

    if (strcmp(args[1], "cd") == 0) {
        print_usercolor("Type: cd directory_name to change the current working directory of the shell\n");
    } else if (strcmp(args[1], "help") == 0) {
        print_usercolor("Type: help for supported commands\n");
    } else if (strcmp(args[1], "exit") == 0) {
        print_usercolor("Type: exit to terminate the shell gracefully\n");
    } else if (strcmp(args[1], "usage") == 0) {
        print_usercolor("Type: usage cd/help/exit\n");
    } else if (strcmp(args[1], "env") == 0) {
        print_usercolor("Type: env to list all registered env variables\n");
    } else if (strcmp(args[1], "setenv") == 0) {
        print_usercolor("Type: setenv ENV=VALUE to set a new env variable\n");
    } else if (strcmp(args[1], "unsetenv") == 0) {
        print_usercolor("Type: unsetenv ENV to remove this env from the list of env variables\n");
    } else if (strcmp(args[1], "clear") == 0) {
        print_usercolor("Type: clear to clear your terminal\n");
    } else if (strcmp(args[1], "setpref") == 0) {
        print_usercolor("Type: set your preferred text colors and prompt style\n");
    } else {
        print_usercolor("The command you gave: %s, is not part of CSEShell's builtin command\n", args[1]);
    }

    return 1;
}


// List environment variables
int list_env(char **args) {
    (void)args;
    extern char **environ;
    for (char **env = environ; *env != 0; env++) {
        print_usercolor("%s\n", *env);
    }
    return 1;
}

// Set environment variable
int set_env_var(char **args) {
    if (args[1] == NULL) {
        print_errorcolor("setenv: expected VAR=VALUE\n");
        return 1;
    }

    // Find '=' character
    char *equal_sign = strchr(args[1], '=');
    if (equal_sign == NULL) {
        print_errorcolor("setenv: format must be VAR=VALUE\n");
        return 1;
    }

    // Split into variable and value
    *equal_sign = '\0';            // Null-terminate VAR part
    char *var = args[1];           // Left of '='
    char *value = equal_sign + 1;  // Right of '='  with quotes included

    // Keep quotes as-is
    if (setenv(var, value, 1) != 0) {
        print_errorcolor("setenv: %s\n", strerror(errno));
    }

    return 1;
}


// Unset environment variable
int unset_env_var(char **args) {
    if (args[1] == NULL) {
        print_errorcolor("unsetenv: expected VAR\n");
    } else {
        if (unsetenv(args[1]) != 0) {
            print_errorcolor("unsetenv: %s\n", strerror(errno));
        }
    }
    return 1;
}

int shell_clear(char **args) {
    (void)args;
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
    return 1;  // Return 1 to continue shell
}

// Helper function of setting user preference
const char *valid_pref_keys[] = {
    "PROMPT_COLOR",
    "PROMPT_STYLE",
    "TEXT_COLOR",
    "ERROR_COLOR"
};
const int valid_pref_count = sizeof(valid_pref_keys) / sizeof(valid_pref_keys[0]);

int is_valid_pref_key(const char *key) {
    for (int i = 0; i < valid_pref_count; i++) {
        if (strcmp(key, valid_pref_keys[i]) == 0) {
            return 1; // valid
        }
    }
    return 0; // invalid
}

void to_uppercase(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

const char *valid_value[] = {
    "reset",
    "red",
    "green",
    "yellow",
    "blue",
    "magenta",
    "cyan",
    "white",
    "bold"
};
const int valid_value_count = sizeof(valid_value) / sizeof(valid_value[0]);

int is_valid_value(const char *value) {
    for (int i = 0; i < valid_value_count; i++) {
        if (strcmp(value, valid_value[i]) == 0) {
            return 1; // valid
        }
    }
    return 0; // invalid
}

// Main function to set user preference
int set_user_pref(char **args) {
    if (args[1] == NULL) {
        print_errorcolor("setpref: expected KEY=VALUE\n");
        return 1;
    }

    char *equal_sign = strchr(args[1], '=');
    if (!equal_sign) {
        print_usercolor("setpref: format must be KEY=VALUE\n");
        return 1;
    }

    *equal_sign = '\0';
    char *key = args[1];
    char *value = equal_sign + 1;

    to_uppercase(key);

    if (!is_valid_pref_key(key)) {
        print_usercolor("setpref: invalid preference key: %s\n", key);
        return 1;
    }

    if (strstr(key, "COLOR") != NULL){
        if (!is_valid_value(value)) {
            print_usercolor("setpref: invalid preference value: %s\n", value);
            return 1;
        }
    }


    // Update environment variable immediately
    if (setenv(key, value, 1) != 0) {
        print_errorcolor("setenv failed: %s\n", strerror(errno));
        return 1;
    }

    FILE *file = fopen(PROFILE_FILE, "r");
    FILE *temp = fopen(".cseshell_profile_tmp", "w");
    if (!temp) {
        print_errorcolor("Failed to open temporary file: %s\n", strerror(errno));
        if (file) fclose(file);
        return 1;
    }

    char line[256];
    int updated = 0;

    if (file) {
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = 0;
            if (strlen(line) == 0 || line[0] == '#') {
                fprintf(temp, "%s\n", line); // preserve comments/empty lines
                continue;
            }

            char line_copy[256];
            strncpy(line_copy, line, sizeof(line_copy));
            char *eq = strchr(line_copy, '=');
            if (!eq) {
                fprintf(temp, "%s\n", line);
                continue;
            }

            *eq = '\0'; // null-terminate key
            char *file_key = line_copy;

            // Convert file_key to uppercase for case-insensitive match
            to_uppercase(file_key);

            if (strcmp(file_key, key) == 0) {
                fprintf(temp, "%s=%s\n", key, value); // overwrite key
                updated = 1;
            } else {
                fprintf(temp, "%s\n", line); // keep original line
            }
        }
        fclose(file);
    }

    if (!updated) {
        fprintf(temp, "%s=%s\n", key, value); // append new key if not found
    }

    fclose(temp);

    if (rename(".cseshell_profile_tmp", PROFILE_FILE) != 0) {
        print_errorcolor("Failed to update profile file: %s\n", strerror(errno));
        return 1;
    }

    print_usercolor("%s=%s\n", key, value);
    print_usercolor("Preference %s=%s saved and applied.\n", key, value);
    return 1;
}
