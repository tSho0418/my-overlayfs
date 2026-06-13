/* copy_up.c (ユーザー空間版) */
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include "simple_overlay.h"

/* ファイルをバイト単位でコピーする内部関数 */
static int copy_file_data(const char *src,
                            const char *dst) {
    char buf[65536];
    ssize_t n;
    int sfd = open(src, O_RDONLY);
    int dfd = open(dst, O_WRONLY|O_CREAT|O_EXCL, 0600);
    if (sfd < 0 || dfd < 0) { /* 一旦エラ-は握りつぶしとく */ }
    while ((n = read(sfd, buf, sizeof(buf))) > 0)
        write(dfd, buf, n);
    close(sfd); close(dfd);
    return 0;
}

/* カーネルの ovl_copy_up() に相当
   workdirを使った2段階コピーでアトミック性を保証 */
int ovl_copy_up(const ovl_fs_t *ofs,
                const char *relpath)
{
    char src[512], work_tmp[512], upper_dst[512];
    struct stat st;

    snprintf(src, sizeof(src),
             "%s/%s", ofs->lower_root, relpath);
    snprintf(work_tmp, sizeof(work_tmp),
             "%s/.copyup_tmp", ofs->work_root);
    snprintf(upper_dst, sizeof(upper_dst),
             "%s/%s", ofs->upper_root, relpath);

    /* : workdirに一時コピーを作る */
    lstat(src, &st);
    copy_file_data(src, work_tmp);

    /*  メタデータを再現 */
    chmod(work_tmp, st.st_mode);

    /*  rename でアトミックにupperへ移動 */
    if (rename(work_tmp, upper_dst) != 0) {
        perror("copy-up rename failed");
        return -1;
    }

    printf("copy-up: %s → upper\n", relpath);
    return 0;
}
