/* fs/overlayfs/ovl_entry.h (抜粋・簡略版) */

// どのファイルがどのレイヤーに属しているかを保持する
struct ovl_entry {
    unsigned numlower;         /* lowerレイヤーの数 */
    struct ovl_path lowerstack[]; /* lowerのdentry+mnt配列 */
};

struct ovl_inode {
    struct dentry *upper_dentry; /* upperのdentry(書込済みならnon-NULL)*/
    struct inode  *lower_inode;  /* lower側のinode */
    struct inode  vfs_inode;     /* VFSに見せるinode(末尾に必須) */
};
