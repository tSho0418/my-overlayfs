/* whiteout.c */
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include "simple_overlay.h"

/* カーネルの ovl_whiteout() に相当
   character device major=0, minor=0 を作成 */
int ovl_create_whiteout(const char *path) {
    /* mknod(path, S_IFCHR|0000, 0) */
    // ホワイトアウト作成
    if (mknod(path, S_IFCHR | 0000, 0) != 0) {
        perror("mknod whiteout");
        return -1;
    }
    printf("whiteout created: %s\n", path);
    return 0;
}

/* ファイル削除の全体フロー */
int ovl_unlink(const ovl_fs_t *ofs,
               const char *relpath)
{
    ovl_entry_t oe;
    char upper_wh[512];

    snprintf(upper_wh, sizeof(upper_wh),
             "%s/%s", ofs->upper_root, relpath);

    if (ovl_lookup(ofs, relpath, &oe) != 0)
        return -1; /* 存在しないファイル */

    switch (oe.type) {
    case OVL_TYPE_UPPER:
        /* upperのみ → そのまま削除 */
        return unlink(oe.upper_path);

    case OVL_TYPE_LOWER:
        /* lowerのみ → whiteoutを作る */
        return ovl_create_whiteout(upper_wh);

    case OVL_TYPE_MERGE:
        /* 両方 (dir) → upperを消してwhiteout */
        unlink(oe.upper_path); /* or rmdir */
        return ovl_create_whiteout(upper_wh);
    }
    return -1;
}
