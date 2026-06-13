/* copy_up_test.c */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "simple_overlay.h"

static const char *type_name(ovl_path_type_t t) {
    switch (t) {
    case OVL_TYPE_LOWER: return "LOWER only";
    case OVL_TYPE_UPPER: return "UPPER only";
    case OVL_TYPE_MERGE: return "MERGE (both)";
    }
    return "unknown";
}

static void run_test(const ovl_fs_t *ofs,
                     const char     *label,
                     const char     *relpath)
{
    ovl_entry_t oe;
    int ret = ovl_lookup(ofs, relpath, &oe);

    printf("\n[TEST] %s\n", label);
    printf("  relpath : %s\n", relpath);

    if (ret != 0) {
        if (oe.is_whiteout)
            printf("  result  : WHITEOUT (deleted)\n");
        else
            printf("  result  : NOT FOUND\n");
        return;
    }

    printf("  type    : %s\n", type_name(oe.type));

    /* どのパスが「実体」として使われるかを表示 */
    if (oe.type == OVL_TYPE_LOWER)
        printf("  real    : %s  ← lower を直接読む\n", oe.lower_path);
    else
        printf("  real    : %s  ← upper を使う\n",  oe.upper_path);
}


int main(void) {
    ovl_fs_t ofs;
    strcpy(ofs.lower_root, "/tmp/ovl/lower");
    strcpy(ofs.upper_root, "/tmp/ovl/upper");
    strcpy(ofs.work_root,  "/tmp/ovl/work");

    puts("=== ovl_lookup テスト ===");

    /* ケース1: lower にだけある */
    run_test(&ofs, "lower only",  "lower_only.txt");

    /* ケース2: upper にだけある */
    run_test(&ofs, "upper only",  "upper_only.txt");

    /* ケース3: 両方にある → upper が優先されるはず */
    run_test(&ofs, "both layers", "both.txt");

    /* ケース4: 存在しないファイル */
    run_test(&ofs, "not found",   "ghost.txt");

    /* ケース5: whiteout (演習3で手動作成後に確認) */
    run_test(&ofs, "whiteout?",   "test.txt");



    puts("=== copy-up テスト ===\n");

    /* 事前状態を表示 */
    puts("── before copy-up ──");
    system("ls -la /tmp/ovl/lower/");
    system("ls -la /tmp/ovl/upper/");

    /* lower_only.txt を copy-up する */
    int ret = ovl_copy_up(&ofs, "lower_only.txt");
    if (ret != 0) {
        puts("copy-up failed!");
        return 1;
    }

    /* 事後状態を表示 */
    puts("\n── after copy-up ──");
    puts("lower (変わっていないはず):");
    system("ls -la /tmp/ovl/lower/");
    system("cat  /tmp/ovl/lower/lower_only.txt");

    puts("\nupper (コピーが出現したはず):");
    system("ls -la /tmp/ovl/upper/");
    system("cat  /tmp/ovl/upper/lower_only.txt");

    return 0;
}
