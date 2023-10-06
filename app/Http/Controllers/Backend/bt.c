#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int reverse_order = 0;
int sort_by_time = 0;
int sort_by_access_time = 0;
int show_size_in_blocks = 0;
int show_all = 0;
int show_numeric_ids = 0;
int human_readable_size = 0;
int mark_file_types = 0;
int use_kibibytes = 0;  // Thêm biến toàn cục để kiểm tra tùy chọn -k

void list_directory(const char *path, int show_hidden, int long_format, int list_dir_only, int show_inode);

int main(int argc, char *argv[]) {
    int show_hidden = 0;
    int long_format = 0;
    int list_dir_only = 0;
    int show_inode = 0;
    const char *path = ".";

    int opt;
    while ((opt = getopt(argc, argv, "AaldirtsunhFk")) != -1) {  // Thêm 'k' vào danh sách tùy chọn
        switch (opt) {
            case 'A':
                show_hidden = 1;
                break;
            case 'a':
                show_all = 1;
                break;
            case 'l':
                long_format = 1;
                break;
            case 'd':
                list_dir_only = 1;
                break;
            case 'i':
                show_inode = 1;
                break;
            case 'r':
                reverse_order = 1;
                break;
            case 't':
                sort_by_time = 1;
                break;
            case 'u':
                sort_by_access_time = 1;
                break;
            case 's':
                show_size_in_blocks = 1;
                break;
            case 'n':
                show_numeric_ids = 1;
                break;
            case 'h':
                human_readable_size = 1;
                break;
            case 'F':
                mark_file_types = 1;
                break;
            case 'k':  // Xử lý tùy chọn '-k' để hiển thị kích thước dưới dạng ký hiệu đề cập
                use_kibibytes = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-AaldirtsunhFk] [directory]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        path = argv[optind];
    }

    list_directory(path, show_hidden, long_format, list_dir_only, show_inode);

    return EXIT_SUCCESS;
}

int compare_names(const void *a, const void *b) {
    return reverse_order ? -strcmp((const char *)a, (const char *)b) : strcmp((const char *)a, (const char *)b);
}

int compare_time(const void *a, const void *b) {
    const char filename_a = *(const char *)a;
    const char filename_b = *(const char *)b;

    struct stat stat_a, stat_b;
    if (stat(filename_a, &stat_a) == -1 || stat(filename_b, &stat_b) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    if (sort_by_access_time) {
        return reverse_order ? difftime(stat_b.st_atime, stat_a.st_atime) : difftime(stat_a.st_atime, stat_b.st_atime);
} else {
        return reverse_order ? difftime(stat_b.st_mtime, stat_a.st_mtime) : difftime(stat_a.st_mtime, stat_b.st_mtime);
    }
}

char *human_readable_size_str(off_t size) {
    static const char *suffixes[] = {"B", "KB", "MB", "GB", "TB"};
    int i = 0;
    double dsize = (double)size;

    while (dsize >= 1024 && i < sizeof(suffixes) / sizeof(suffixes[0]) - 1) {
        dsize /= 1024;
        i++;
    }

    char *result = (char *)malloc(10);
    snprintf(result, 10, "%.1f %s", dsize, suffixes[i]);
    return result;
}

char *abbreviated_size_str(off_t size) {
    static const char *suffixes[] = {"", "K", "M", "G", "T"};
    int i = 0;
    double dsize = (double)size;

    while (dsize >= 1024 && i < sizeof(suffixes) / sizeof(suffixes[0]) - 1) {
        dsize /= 1024;
        i++;
    }

    char *result = (char *)malloc(10);
    snprintf(result, 10, "%.1f%s", dsize, suffixes[i]);
    return result;
}

void list_directory(const char *path, int show_hidden, int long_format, int list_dir_only, int show_inode) {
    if (!list_dir_only) {
        DIR *dir;
        struct dirent *entry;
        struct stat file_info;

        dir = opendir(path);
        if (dir == NULL) {
            perror("opendir");
            exit(EXIT_FAILURE);
        }

        int num_entries = 0;
        char **entries = NULL;

        while ((entry = readdir(dir)) != NULL) {
            if (!show_all && !show_hidden && entry->d_name[0] == '.') {
                continue;
            }

            char full_path[PATH_MAX];
            snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

            if (stat(full_path, &file_info) == -1) {
                perror("stat");
                continue;
            }

            if (long_format) {
                char time_str[80];
                strftime(time_str, sizeof(time_str), "%b %d %H:%M", localtime(&file_info.st_mtime));

                if (show_inode) {
                    printf("%lu ", (unsigned long)file_info.st_ino);
                }

                if (show_size_in_blocks) {
                    printf("%ld ", (long)file_info.st_blocks);
                }

                if (show_numeric_ids) {
                    printf("%ld %ld ", (long)file_info.st_uid, (long)file_info.st_gid);
                } else {
                    struct passwd *pwd = getpwuid(file_info.st_uid);
                    struct group *grp = getgrgid(file_info.st_gid);

                    printf("%s %s ", (pwd != NULL) ? pwd->pw_name : "", (grp != NULL) ? grp->gr_name : "");
                }

                if (human_readable_size) {
                    char *size_str = abbreviated_size_str(file_info.st_size);
                    printf("%s %s\n", size_str, entry->d_name);
                    free(size_str);
                } else {
                    printf("%ld %s %s", (long)file_info.st_size, time_str, entry->d_name);

                    if (mark_file_types) {
struct stat file_info;
                        if (stat(full_path, &file_info) != -1) {
                            if (S_ISDIR(file_info.st_mode)) {
                                printf("/");
                            } else if (file_info.st_mode & S_IXUSR || file_info.st_mode & S_IXGRP || file_info.st_mode & S_IXOTH) {
                                printf("*");
                            }
                        }
                    }
                    printf("\n");
                }
            } else {
                num_entries++;
                entries = (char **)realloc(entries, num_entries * sizeof(char *));
                entries[num_entries - 1] = strdup(entry->d_name);
            }
        }

        closedir(dir);

        if (sort_by_time) {
            qsort(entries, num_entries, sizeof(char *), compare_time);
        } else if (!reverse_order) {
            qsort(entries, num_entries, sizeof(char *), compare_names);
        } else {
            qsort(entries, num_entries, sizeof(char *), compare_names);
            for (int i = 0; i < num_entries / 2; i++) {
                char *temp = entries[i];
                entries[i] = entries[num_entries - 1 - i];
                entries[num_entries - 1 - i] = temp;
            }
        }

        for (int i = 0; i < num_entries; i++) {
            printf("%s", entries[i]);

            if (mark_file_types) {
                char full_path[PATH_MAX];
                snprintf(full_path, sizeof(full_path), "%s/%s", path, entries[i]);
                struct stat file_info;
                if (stat(full_path, &file_info) != -1) {
                    if (S_ISDIR(file_info.st_mode)) {
                        printf("/");
                    } else if (file_info.st_mode & S_IXUSR || file_info.st_mode & S_IXGRP || file_info.st_mode & S_IXOTH) {
                        printf("*");
                    }
                }
            }

            printf("\n");
            free(entries[i]);
        }
        free(entries);
    } else {
        printf("%s\n", path);
    }
}
