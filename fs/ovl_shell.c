/* ovl_shell.c — 統合デモ */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "simple_overlay.h"

static ovl_fs_t g_ofs;

void cmd_ls(const char *dir) {
    merge_entry_t entries[256];
    int n = ovl_readdir(&g_ofs, dir ? dir : ".",
                         entries, 256);
    printf("--- merged ls [%s] ---\n", dir ? dir : ".");
    for (int i = 0; i < n; i++)
        printf("  [%s] %s\n",
               entries[i].is_upper ? "Upper" : "Lower",
               entries[i].name);
}

void cmd_cat(const char *relpath) {
    ovl_entry_t oe;
    if (ovl_lookup(&g_ofs, relpath, &oe) != 0) {
        printf("No such file: %s\n", relpath);
        return;
    }
    const char *real = oe.type == OVL_TYPE_LOWER
                      ? oe.lower_path : oe.upper_path;

    pid_t pid = fork();
    if (pid == 0) {
        execlp("cat", "cat", real, NULL);
        perror("execlp");   /* execlp が失敗したときだけ到達 */
        _exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    } else {
        perror("fork");
    }
}

void cmd_touch(const char *relpath) {
    ovl_entry_t oe;
    if (ovl_lookup(&g_ofs, relpath, &oe) == 0) {
        printf("Already exists: %s\n", relpath);
        return;
    }

    char upper_path[PATH_MAX];
    snprintf(upper_path, sizeof(upper_path),
             "%s/%s", g_ofs.upper_root, relpath);

    /* O_CREAT で空ファイルを作る */
    int fd = open(upper_path, O_CREAT | O_WRONLY, 0644);
    if (fd < 0) { perror("touch"); return; }
    close(fd);

    printf("Created (empty): %s → upper\n", relpath);
}

void cmd_write(const char *relpath, const char *text) {
    ovl_entry_t oe;
    ovl_lookup(&g_ofs, relpath, &oe);

    /* lowerのファイルならcopy-upが必要 */
    if (oe.type == OVL_TYPE_LOWER)
        ovl_copy_up(&g_ofs, relpath);

    char upper_path[512];
    snprintf(upper_path, sizeof(upper_path),
             "%s/%s", g_ofs.upper_root, relpath);
    FILE *f = fopen(upper_path, "w");
    fprintf(f, "%s\n", text);
    fclose(f);
    printf("Written to upper: %s\n", relpath);
}

void print_help(void) {
    puts("commands:");
    puts("  ls [dir]            マージ結果を表示");
    puts("  cat <file>          ファイルを表示");
    puts("  touch <file>        空ファイルを作成");
    puts("  write <file> <text> ファイルに書き込み(copy-up)");
    puts("  rm <file>           削除(whiteout)");
    puts("  help                このヘルプ");
    puts("  exit / quit         終了");
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [lowerdir upperdir workdir]\n"
        "  引数なしの場合は /tmp/ovl/{lower,upper,work} を使用\n",
        prog);
}

int main(int argc, char **argv) {
    if (argc == 1){
      strcpy(g_ofs.lower_root, "/tmp/ovl/lower");
      strcpy(g_ofs.upper_root, "/tmp/ovl/upper");
      strcpy(g_ofs.work_root,  "/tmp/ovl/work");
    }else if(argc == 4){
      strncpy(g_ofs.lower_root, argv[1], PATH_MAX - 1);
      strncpy(g_ofs.upper_root, argv[2], PATH_MAX - 1);
      strncpy(g_ofs.work_root,  argv[3], PATH_MAX - 1);
      g_ofs.lower_root[PATH_MAX - 1] = '\0';
      g_ofs.upper_root[PATH_MAX - 1] = '\0';
      g_ofs.work_root[PATH_MAX - 1] = '\0';
    }else {
      usage(argv[0]);
      return 1;
    }

    printf("=== simple overlayfs shell ===\n");
    printf("  lower : %s\n", g_ofs.lower_root);
    printf("  upper : %s\n", g_ofs.upper_root);
    printf("  work  : %s\n", g_ofs.work_root);

    char line[256], cmd[64], arg1[256], arg2[256];
    while (printf("my-ovl> "), fgets(line, sizeof(line), stdin)) {
        cmd[0] = arg1[0] = arg2[0] = '\0';
        int n = sscanf(line, "%63s %255s %255s", cmd, arg1, arg2);
        if (n < 1) continue;

        if      (!strcmp(cmd, "ls"))
            cmd_ls(n > 1 ? arg1 : NULL);
        else if (!strcmp(cmd, "cat") && n > 1)
            cmd_cat(arg1);
        else if (!strcmp(cmd, "touch") && n > 1)
            cmd_touch(arg1);
        else if (!strcmp(cmd, "write") && n > 2)
            cmd_write(arg1, arg2);
        else if (!strcmp(cmd, "rm") && n > 1)
            ovl_unlink(&g_ofs, arg1);
        else if (!strcmp(cmd, "help"))
            print_help();
        else if (!strcmp(cmd, "exit") || !strcmp(cmd, "quit"))
            break;
        else
            printf("unknown or incomplete command: %s "
                   "('help' 参照)\n", cmd);
    }
    return 0;
}
