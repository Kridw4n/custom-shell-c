#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

int main() {
    char *backup_path = getenv("BACKUP_DIR");
    if (!backup_path) {
        fprintf(stderr, "Error: BACKUP_DIR environment variable is not set.\n");
        return 1;
    }

    struct stat st;
    if (stat(backup_path, &st) != 0) {
        perror("Error accessing BACKUP_DIR path");
        return 1;
    }

    // Reserve enough space for "/archive/backup-YYYYMMDDHHMMSS.zip"
    char project_dir[PATH_MAX - 64];
    if (getcwd(project_dir, sizeof(project_dir)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    char archive_dir[PATH_MAX - 32];
    snprintf(archive_dir, sizeof(archive_dir), "%s/archive", project_dir);
    mkdir(archive_dir, 0777);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char datetime[32];
    strftime(datetime, sizeof(datetime), "%Y%m%d%H%M%S", t);

    const char *slash = strrchr(backup_path, '/');
    const char *name = slash ? slash + 1 : backup_path; // not used

    // zip_path now guaranteed to be safe due to capped sizes above
    char zip_path[PATH_MAX];
    if (snprintf(zip_path, sizeof(zip_path), "%s/backup-%s.zip", archive_dir, datetime) >= sizeof(zip_path)) {
        fprintf(stderr, "Error: zip_path truncated.\n");
        return 1;
    }

    char zip_cmd[PATH_MAX * 2];
    if (snprintf(zip_cmd, sizeof(zip_cmd), "zip -r '%s' '%s'", zip_path, backup_path) >= sizeof(zip_cmd)) {
        fprintf(stderr, "Error: zip_cmd truncated.\n");
        return 1;
    }

    printf("Creating backup of '%s' at '%s'\n", backup_path, zip_path);

    int result = system(zip_cmd);
    if (result != 0) {
        fprintf(stderr, "Error creating zip file.\n");
        return 1;
    }

    printf("Backup created successfully.\n");
    return 0;
}
