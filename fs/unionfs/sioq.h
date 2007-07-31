#ifndef _SIOQ_H
#define _SIOQ_H

struct deletewh_args {
	struct unionfs_dir_state *namelist;
	struct dentry *dentry;
	int bindex;
};

struct is_opaque_args {
	struct dentry *dentry;
};

struct create_args {
	struct inode *parent;
	struct dentry *dentry;
	umode_t mode;
	struct nameidata *nd;
};

struct mkdir_args {
	struct inode *parent;
	struct dentry *dentry;
	umode_t mode;
};

struct mknod_args {
	struct inode *parent;
	struct dentry *dentry;
	umode_t mode;
	dev_t dev;
};

struct symlink_args {
	struct inode *parent;
	struct dentry *dentry;
	char *symbuf;
	umode_t mode;
};

struct unlink_args {
	struct inode *parent;
	struct dentry *dentry;
};


struct sioq_args {
	struct completion comp;
	struct work_struct work;
	int err;
	void *ret;

	union {
		struct deletewh_args deletewh;
		struct is_opaque_args is_opaque;
		struct create_args create;
		struct mkdir_args mkdir;
		struct mknod_args mknod;
		struct symlink_args symlink;
		struct unlink_args unlink;
	};
};

extern int __init init_sioq(void);
extern __exit void stop_sioq(void);
extern void run_sioq(work_func_t func, struct sioq_args *args);

/* Extern definitions for our privlege escalation helpers */
extern void __unionfs_create(struct work_struct *work);
extern void __unionfs_mkdir(struct work_struct *work);
extern void __unionfs_mknod(struct work_struct *work);
extern void __unionfs_symlink(struct work_struct *work);
extern void __unionfs_unlink(struct work_struct *work);
extern void __delete_whiteouts(struct work_struct *work);
extern void __is_opaque_dir(struct work_struct *work);

#endif /* _SIOQ_H */
