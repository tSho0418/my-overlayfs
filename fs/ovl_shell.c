/* ovl_shell.c — 統合デモ */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "simple_overlay.h"

static ovl_fs_t g_ofs;

void cmd_ls(const char *dir) {
    merge_entry_t entries[256];
    int n = ovl_readdir(&g_ofs, dir ? dir : ".",
                         entries, 256);
    printf("--- merged ls [%s] ---\n", dir ? dir : ".");
    for (int i = 0; i < n; i++)
        printf("  [%s] %s\n",
               entries[i].is_upper ? "U" : "L",
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
    /* execlp cat real — 実際のファイルを読む */
    execlp("cat", "cat", real, NULL);
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

int main(int argc, char **argv) {
    strcpy(g_ofs.lower_root, "/tmp/ovl/lower");
    strcpy(g_ofs.upper_root, "/tmp/ovl/upper");
    strcpy(g_ofs.work_root,  "/tmp/ovl/work");

    char line[256], cmd[64], arg1[256], arg2[256];
    while (printf("my-ovl> "),
           fgets(line, sizeof(line), stdin)) {
        int n = sscanf(line,
                  "%63s %255s %255s", cmd, arg1, arg2);
        if      (!strcmp(cmd,"ls"))
            cmd_ls(n>1 ? arg1 : NULL);
        else if (!strcmp(cmd,"cat"))
            cmd_cat(arg1);
        else if (!strcmp(cmd,"write"))
            cmd_write(arg1, arg2);
        else if (!strcmp(cmd,"rm"))
            ovl_unlink(&g_ofs, arg1);
    }
}
