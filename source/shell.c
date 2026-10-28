#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <stdarg.h>

// Function to print colored output based on user preference
void print_usercolor(const char *format, ...) {
    const char *color = getenv("TEXT_COLOR");
    if (!color) color = "white";

    const char *color_code = COLOR_WHITE;
    if (strcmp(color, "red") == 0) color_code = COLOR_RED;
    else if (strcmp(color, "green") == 0) color_code = COLOR_GREEN;
    else if (strcmp(color, "yellow") == 0) color_code = COLOR_YELLOW;
    else if (strcmp(color, "blue") == 0) color_code = COLOR_BLUE;
    else if (strcmp(color, "magenta") == 0) color_code = COLOR_MAGENTA;
    else if (strcmp(color, "cyan") == 0) color_code = COLOR_CYAN;

    va_list args;
    va_start(args, format);
    printf("%s", color_code);
    vprintf(format, args);
    printf("%s", COLOR_RESET);
    va_end(args);
}

void print_errorcolor(const char *format, ...) {
    const char *color = getenv("ERROR_COLOR");
    if (!color) color = "red";  // default error color

    const char *color_code = COLOR_RED;
    if (strcmp(color, "green") == 0) color_code = COLOR_GREEN;
    else if (strcmp(color, "yellow") == 0) color_code = COLOR_YELLOW;
    else if (strcmp(color, "blue") == 0) color_code = COLOR_BLUE;
    else if (strcmp(color, "magenta") == 0) color_code = COLOR_MAGENTA;
    else if (strcmp(color, "cyan") == 0) color_code = COLOR_CYAN;
    else if (strcmp(color, "white") == 0) color_code = COLOR_WHITE;

    va_list args;
    va_start(args, format);
    printf("%s", color_code);
    vprintf(format, args);
    printf("%s", COLOR_RESET);
    va_end(args);
}

// Function to display the shell prompt
char *type_prompt() {
    const char *color = getenv("PROMPT_COLOR");
    const char *style = getenv("PROMPT_STYLE");

    if (!color) color = "green";
    if (!style) style = "$$";

    const char *color_code = COLOR_GREEN;
    if (strcmp(color, "red") == 0) color_code = COLOR_RED;
    else if (strcmp(color, "yellow") == 0) color_code = COLOR_YELLOW;
    else if (strcmp(color, "blue") == 0) color_code = COLOR_BLUE;
    else if (strcmp(color, "magenta") == 0) color_code = COLOR_MAGENTA;
    else if (strcmp(color, "cyan") == 0) color_code = COLOR_CYAN;
    else if (strcmp(color, "white") == 0) color_code = COLOR_WHITE;

    char *prompt = malloc(256);
    if (!prompt) {
        print_usercolor( "Failed to allocate memory for prompt\n");
        exit(1);
    }
    snprintf(prompt, 256, "%s%s %s", color_code, style, COLOR_RESET);
    return prompt;
}

// Function to read a command from the user input
void read_command(char **cmd) {
  // Define a character array to store the command line input
  // char line[MAX_LINE];
  // Use readline to read the command line input from the user
  char *prompt = type_prompt();
  char *line = readline(prompt);
  free(prompt);
  // Initialize i to keep track of the number of parsed arguments
  int i = 0;
  // Array to hold pointers to the parsed command arguments
  char *array[MAX_ARGS], *command_token;

  // If the user entered an empty line, return without processing
  if (!line || strlen(line) == 0) {
    free(line);
    return;
  }

  // Add cmd to history
  add_history(line);

  // If only a newline character was entered, return without processing
  if (strlen(line) == 1) {
    free(line);
    return;
  }

  // Use strtok to parse the first token (word) of the command
  command_token = strtok(line, " \n");

  // Continue parsing the line into words and store them in the array
  while (command_token != NULL) {
    array[i++] = strdup(command_token);  // Duplicate the token and store it
    command_token = strtok(NULL, " \n"); // Get the next token
  }

  // Copy the parsed command and its parameters to the cmd array
  for (int j = 0; j < i; j++) {
    cmd[j] = array[j];
  }

  // Null-terminate the cmd array to mark the end of arguments
  cmd[i] = NULL;

  // Free the original line buffer
  free(line);
}

// Function to display the cpu usage by each command
void print_resource_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_CHILDREN, &usage) == 0) {
        // How long the cpu spends in user mode
        print_usercolor("User CPU time: %ld.%06ld sec\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
        // How long the cpu spends in system mode (kernal-side resources)
        print_usercolor("System CPU time: %ld.%06ld sec\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
        // Basically peak RAM usage
        print_usercolor("Max resident set size: %ld KB\n", usage.ru_maxrss);
        // Load missing pages from RAM (not in process's page table)
        print_usercolor("Page faults (no I/O): %ld\n", usage.ru_minflt);
        // Require to access the disk to load missing pages
        print_usercolor("Page faults (requiring I/O): %ld\n", usage.ru_majflt);
        // Track slow processes and excessive context switching
        print_usercolor("Voluntary context switches: %ld\n", usage.ru_nvcsw);
        print_usercolor("Involuntary context switches: %ld\n", usage.ru_nivcsw);
    } else {
        print_errorcolor("getrusage: %s\n", strerror(errno));
    }
}

// Load customisation of user
void load_user_preferences() {
    const char *profile_path = ".cseshell_profile";
    FILE *file = fopen(profile_path, "r");
    if (!file) {
        // Profile doesn't exist yet, no error
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) == 0 || line[0] == '#') continue;

        char *equal_sign = strchr(line, '=');
        if (!equal_sign) continue;
        *equal_sign = '\0';
        char *key = line;
        char *value = equal_sign + 1;

        setenv(key, value, 1); // store as environment variable
    }

    fclose(file);
}

void save_user_preferences() {
    FILE *fp = fopen(".cseshell_profile", "w");
    if (!fp) {
        const char *err_color = getenv("ERROR_COLOR");
        if (err_color) {
            printf("\033[31mFailed to open .cseshell_profile for writing\n\033[0m");
        } else {
            fprintf(stderr, "Failed to open .cseshell_profile for writing\n");
        }
        return;
    }

    for (int i = 0; i < valid_pref_count; i++) {
        const char *key = valid_pref_keys[i];
        const char *value = getenv(key);
        if (value) {
            fprintf(fp, "%s=%s\n", key, value);
        }
    }

    fclose(fp);
}

/*** This is array of functions, with argument char ***/
int (*builtin_command_func[])(char **) = {
    &shell_cd,     // builtin_command_func[0]: cd
    &shell_help,   // builtin_command_func[1]: help
    &shell_exit,   // builtin_command_func[2]: exit
    &shell_usage,  // builtin_command_func[3]: usage
    &list_env,     // builtin_command_func[4]: env
    &set_env_var,  // builtin_command_func[5]: setenv
    &unset_env_var, // builtin_command_func[6]: unsetenv
    &shell_clear,
    &set_user_pref
};

// Helper function to figure out how many builtin commands are supported by the shell
int num_builtin_functions(){
    return sizeof(builtin_commands) / sizeof(char *);
}

// Tab completion: built-in + ./bin executables
char *command_generator(const char *text, int state) {
  static int list_index, len;
  static DIR *dir;
  static struct dirent *entry;
  static int phase = 0;

  if (!state) {
    list_index = 0;
    len = strlen(text);
    dir = opendir("./bin");
    phase = 0;
  }

  // Phase 0: Built-in commands
  if (phase == 0) {
    while (list_index < num_builtin_functions()) {
      if (strncmp(builtin_commands[list_index], text, len) == 0) {
        return strdup(builtin_commands[list_index++]);
      }
      list_index++;
    }
    phase = 1;
    list_index = 0;
  }

  // Phase 1: ./bin/ executables
  if (phase == 1 && dir) {
    while ((entry = readdir(dir)) != NULL) {
      if ((entry->d_type == DT_REG || entry->d_type == DT_LNK) &&
          strncmp(entry->d_name, text, len) == 0) {
        return strdup(entry->d_name);
      }
    }
    closedir(dir);
    dir = NULL;
  }

  return NULL;
}

char **cseshell_completion(const char *text, int start, int end) {
  (void)end;
  rl_attempted_completion_over = 1;

  if (start == 0) {
    return rl_completion_matches(text, command_generator);
  } else {
    return rl_completion_matches(text, rl_filename_completion_function);
  }
}


int start() {
    const char *filePath = ".cseshellrc"; // Path to RC file

    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        print_errorcolor("Failed to open file: %s\n", strerror(errno));
        print_usercolor("Tried to open: %s\n", filePath);
        return EXIT_FAILURE;
    }

    char line[1024];
    while (fgets(line, sizeof(line), file)) {
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';

        // Skip empty lines
        if (strlen(line) == 0) continue;

        // Handle environment variable assignment
        if (strncmp(line, "PATH=", 5) == 0) {
            if (setenv("PATH", line + 5, 1) != 0) {
                print_errorcolor("setenv failed: %s\n", strerror(errno));
            }
            continue;
        }

        pid_t pid = fork();
        if (pid == -1) {
            print_errorcolor("fork failed: %s\n", strerror(errno));
            fclose(file);
            return EXIT_FAILURE;
        } else if (pid == 0) {
            // Child process
            char *args[MAX_ARGS];
            int i = 0;
            char *token = strtok(line, " ");
            while (token != NULL && i < MAX_ARGS - 1) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            // Try to execute the command
            execvp(args[0], args);

            // If execvp fails, print custom error message
            print_errorcolor("Command %s not found\n", args[0]);
            _exit(EXIT_FAILURE);
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    }

    fclose(file);
    return EXIT_SUCCESS;
}


int main(void) {
    char *cmd[MAX_ARGS];
    int status;
    pid_t pid;
    int continue_shell = 1;

    rl_attempted_completion_function = cseshell_completion;

    load_user_preferences();
    start();

    while (continue_shell) {
        read_command(cmd);
        if (!cmd[0]) continue;

        int builtin = 0;

        // Check if it is a builtin command
        for (int command_index = 0; command_index < num_builtin_functions(); command_index++) {
            if (strcmp(cmd[0], builtin_commands[command_index]) == 0) {
                continue_shell = (*builtin_command_func[command_index])(cmd);
                builtin = 1;
                break;
            }
        }

        // If it was a builtin, skip fork
        if (!builtin) {
            pid = fork();

            if (pid < 0) {
                print_errorcolor("Fork failed: %s\n", strerror(errno));
                exit(1);
            }

            if (pid == 0) {
                // Child process
                execvp(cmd[0], cmd);

                // If exec fails, display in user color
                print_errorcolor("Command %s not found\n", cmd[0]);
                exit(1);
            } else {
                // Parent waits
                if (waitpid(pid, &status, WUNTRACED) == -1) {
                    print_errorcolor("waitpid failed: %s\n", strerror(errno));
                }
            }
            print_resource_usage();
        }


    }

    save_user_preferences();

    return 0;
}
