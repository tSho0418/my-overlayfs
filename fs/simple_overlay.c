/* simple_overlay.c (Phase 2 実装) */
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include "simple_overlay.h"

/* whiteoutの判定: character device で rdev==0 */
int is_whiteout(const char *path) {
    struct stat st;
    if (lstat(path, &st) != 0) return 0;
    return S_ISCHR(st.st_mode) && st.st_rdev == 0;
}

/* カーネルの ovl_lookup() に相当 */
int ovl_lookup(const ovl_fs_t *ofs,
               const char *relpath,
               ovl_entry_t *out)
{
    char upper[512], lower[512];
    struct stat st;

    snprintf(upper, sizeof(upper),
             "%s/%s", ofs->upper_root, relpath);
    snprintf(lower, sizeof(lower),
             "%s/%s", ofs->lower_root, relpath);

    memset(out, 0, sizeof(*out));

    /* upperを先にチェック */
    int upper_exists = (lstat(upper, &st) == 0);
    int lower_exists = (lstat(lower, &st) == 0);

    /* whiteout なら「削除済み」を返す */
    if (upper_exists && is_whiteout(upper)) {
        out->is_whiteout = 1;
        return -1; /* ENOENT 相当 */
    }

    /* パス型を決定してパスをセット */
    if (upper_exists) {
        strcpy(out->upper_path, upper);
        out->type = lower_exists ? OVL_TYPE_MERGE
                                 : OVL_TYPE_UPPER;
    } else if (lower_exists) {
        strcpy(out->lower_path, lower);
        out->type = OVL_TYPE_LOWER;
    } else {
        return -1; /* 存在しない */
    }
    return 0;
}
