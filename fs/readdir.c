/* readdir.c (ユーザー空間版) */
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <linux/limits.h>
#include "simple_overlay.h"

#define MAX_ENTRIES 1024

// typedef struct {
//     char  name[NAME_MAX + 1];
//     int   is_upper;
//     int   is_whiteout;
// } merge_entry_t;

static int name_seen(merge_entry_t *tbl, int n,
                     const char *name) {
    for (int i = 0; i < n; i++)
        if (strcmp(tbl[i].name, name) == 0)
            return 1;
    return 0;
}


/* カーネルの ovl_iterate() に相当 */
int ovl_readdir(const ovl_fs_t *ofs,
                const char *reldir,
                merge_entry_t *out, int max)
{
    char upper[PATH_MAX], lower[PATH_MAX];
    struct dirent *de;
    int count = 0;
    char entry_path[PATH_MAX];

    snprintf(upper, sizeof(upper),
             "%s/%s", ofs->upper_root, reldir);
    snprintf(lower, sizeof(lower),
             "%s/%s", ofs->lower_root, reldir);

    /* upper を読む */
    DIR *ud = opendir(upper);
    if (ud) {
        while ((de = readdir(ud)) && count < max) {
            if (de->d_name[0] == '.') continue;
            snprintf(entry_path, sizeof(entry_path),
                     "%s/%s", upper, de->d_name);
            out[count].is_upper = 1;
            out[count].is_whiteout = is_whiteout(entry_path);
            strcpy(out[count].name, de->d_name);
            count++;
        }
        closedir(ud);
    }

    /* lower を読む(重複・whiteout除外) */
    DIR *ld = opendir(lower);
    if (ld) {
        while ((de = readdir(ld)) && count < max) {
            if (de->d_name[0] == '.') continue;
            /* 既に upper にある名前はスキップ */
            if (name_seen(out, count, de->d_name))
                continue;
            out[count].is_upper = 0;
            out[count].is_whiteout = 0;
            strcpy(out[count].name, de->d_name);
            count++;
        }
        closedir(ld);
    }

    /* whiteoutエントリを結果から除外 */
    int final = 0;
    for (int i = 0; i < count; i++)
        if (!out[i].is_whiteout)
            out[final++] = out[i];

    return final;
}
