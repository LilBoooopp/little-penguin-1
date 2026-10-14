// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/rbtree.h>
#include <linux/nsproxy.h>
#include <linux/sched.h>
#include <linux/dcache.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/err.h>
#include "mount.h"

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("List mount points in /proc/mymounts");
MODULE_LICENSE("GPL");

static struct proc_dir_entry *mymounts_entry;

static void mymounts_show_one(struct seq_file *m, struct mount *mnt, char *buf)
{
	struct path path = {
		.mnt = &mnt->mnt,
		.dentry = mnt->mnt.mnt_root,
	};
	const char *full;
	const char *name;

	full = d_path(&path, buf, PATH_MAX);
	if (IS_ERR(full))
		return;

	if (strcmp(full, "/") == 0)
		name = "root";
	else
		name = strrchr(full, '/') + 1;

	seq_printf(m, "%-10s %s\n", name, full);
}

static int mymounts_show(struct seq_file *m, void *v)
{
	struct mnt_namespace *ns = current->nsproxy->mnt_ns;
	struct rb_node *node;
	char *buf;

	buf = kmalloc(PATH_MAX, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;
	/* 
	* namespace_sem is static in fs/namespace.c,
	* so this walk is not protected against concurrent mount or unmount.
	*/
	for (node = rb_first(&ns->mounts); node; node = rb_next(node)) {
		struct mount *mnt = rb_entry(node, struct mount, mnt_node);

		if (mnt == ns->root)
			continue;
		mymounts_show_one(m, mnt, buf);
	}

	kfree(buf);
	return 0;
}

static int mymounts_open(struct inode *inode, struct file *file)
{
	return single_open(file, mymounts_show, NULL);
}

static const struct proc_ops mymounts_ops = {
	.proc_open = mymounts_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

static int __init mymounts_init(void)
{
	mymounts_entry = proc_create("mymounts", 0444, NULL, &mymounts_ops);
	if (!mymounts_entry)
		return -ENOMEM;
	return 0;
}

static void __exit mymounts_exit(void)
{
	proc_remove(mymounts_entry);
}

module_init(mymounts_init);
module_exit(mymounts_exit);
