/* simple_overlay.h */
#pragma once
#include <stddef.h>
#include <linux/limits.h>

/* カーネルの ovl_path_type に相当 */
typedef enum {
    OVL_TYPE_LOWER = 0,   /* lowerのみに存在 */
    OVL_TYPE_UPPER = 1,   /* upperのみに存在 */
    OVL_TYPE_MERGE = 2,   /* 両方 (ディレクトリ) */
} ovl_path_type_t;

/* カーネルの ovl_entry に相当 */
typedef struct {
    char lower_path[PATH_MAX]; /* lower側の実パス */
    char upper_path[PATH_MAX]; /* upper側の実パス (NULLなら未コピー) */
    ovl_path_type_t type;
    int  is_whiteout;     /* 削除マーク */
} ovl_entry_t;

/* カーネルの ovl_fs に相当 (マウント情報) */
typedef struct {
    char lower_root[PATH_MAX];
    char upper_root[PATH_MAX];
    char work_root[PATH_MAX];
} ovl_fs_t;

typedef struct {
  char name[NAME_MAX + 1];
  int is_upper;
  int is_whiteout;
} merge_entry_t;

/* 名前からovl_entryを解決する (lookup) */
int ovl_lookup(const ovl_fs_t *ofs,
               const char  *relpath,
               ovl_entry_t  *out);

int ovl_copy_up(const ovl_fs_t *ofs,
                const char     *relpath);

int is_whiteout(const char *path);

int ovl_create_whiteout(const char *path);

int ovl_unlink(const ovl_fs_t *ofs, const char *relpath);

int ovl_readdir(const ovl_fs_t *ofs,
                const char *reldir,
                merge_entry_t *out,
                int max);
