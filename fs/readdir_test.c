/* readdir_test.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "simple_overlay.h"

/* readdir.c で定義した型と関数の宣言 */
typedef struct {
    char name[256];
    int  is_upper;
    int  is_whiteout;
} merge_entry_t;

int ovl_readdir(const ovl_fs_t *ofs,
                const char     *reldir,
                merge_entry_t  *out,
                int             max);

/* ── ヘルパー: マージ結果を表示 ────────────── */
static void print_merged(const ovl_fs_t *ofs,
                         const char     *label)
{
    merge_entry_t entries[256];
    int n = ovl_readdir(ofs, ".", entries, 256);

    printf("\n[%s] merged ls:\n", label);
    if (n == 0) {
        printf("  (empty)\n");
        return;
    }
    for (int i = 0; i < n; i++) {
        printf("  [%s] %s\n",
               entries[i].is_upper ? "U" : "L",
               entries[i].name);
    }
    printf("  合計 %d エントリ\n", n);
}

/* ── ケース1: 重複排除の確認 ────────────────
 *
 *   lower/both.txt と upper/both.txt が同名で存在する
 *   → merged では1つだけ表示 (upper 優先)
 * ─────────────────────────────────────────── */
static void test_dedup(const ovl_fs_t *ofs)
{
    puts("\n════ ケース1: 重複排除 ════");

    system("echo 'from lower' > /tmp/ovl/lower/lower_only.txt");
    system("echo 'from lower' > /tmp/ovl/lower/both.txt");
    system("echo 'from upper' > /tmp/ovl/upper/upper_only.txt");
    system("echo 'from upper' > /tmp/ovl/upper/both.txt");

    puts("lower の内容:");
    system("ls /tmp/ovl/lower/");
    puts("upper の内容:");
    system("ls /tmp/ovl/upper/");

    print_merged(ofs, "重複排除後");

    puts("\n期待: both.txt は [U] で1つだけ表示される");
}

/* ── ケース2: whiteout の確認 ───────────────
 *
 *   upper に lower_only.txt の whiteout を置く
 *   → merged では lower_only.txt が見えなくなる
 * ─────────────────────────────────────────── */
static void test_whiteout(const ovl_fs_t *ofs)
{
    puts("\n════ ケース2: whiteout ════");

    /* whiteout 作成前 */
    print_merged(ofs, "whiteout 前");

    /* upper に whiteout を置く (要 root) */
    puts("\nupper に lower_only.txt の whiteout を作成...");
    int ret = system(
        "mknod /tmp/ovl/upper/lower_only.txt c 0 0 2>/dev/null");
    if (ret != 0) {
        puts("  mknod 失敗 (root 権限が必要です)");
        puts("  手動で実行: sudo mknod /tmp/ovl/upper/lower_only.txt c 0 0");
        return;
    }

    puts("upper の内容 (whiteout 確認):");
    system("ls -la /tmp/ovl/upper/");

    /* whiteout 作成後 */
    print_merged(ofs, "whiteout 後");

    puts("\n期待: lower_only.txt が表示されなくなる");
}

/* ── main ──────────────────────────────────── */
int main(void) {
    ovl_fs_t ofs;
    strcpy(ofs.lower_root, "/tmp/ovl/lower");
    strcpy(ofs.upper_root, "/tmp/ovl/upper");
    strcpy(ofs.work_root,  "/tmp/ovl/work");

    /* テスト前にクリーンアップ */
    system("rm -f /tmp/ovl/upper/* /tmp/ovl/lower/*");

    test_dedup(&ofs);
    test_whiteout(&ofs);

    return 0;
}
