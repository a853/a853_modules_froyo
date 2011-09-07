
include ${src}/magic.mk

AUFS_DEF_CONFIG = -DCONFIG_AUFS_MODULE -DCONFIG_AUFS_BRANCH_MAX_127 -DCONFIG_AUFS_SBILIST -UCONFIG_AUFS
EXTRA_CFLAGS += ${AUFS_DEF_CONFIG}

# sparse doesn't allow spaces
ccflags-y += -D'pr_fmt(fmt)=AUFS_NAME"\040%s:%d:%s[%d]:\040"fmt,__func__,__LINE__,current->comm,current->pid'

obj-m += aufs.o
aufs-y := module.o sbinfo.o super.o branch.o xino.o sysaufs.o opts.o \
	wkq.o vfsub.o dcsub.o \
	cpup.o whout.o wbr_policy.o \
	dinfo.o dentry.o \
	dynop.o \
	finfo.o file.o f_op.o \
	dir.o vdir.o \
	iinfo.o inode.o i_op.o i_op_add.o i_op_del.o i_op_ren.o \
	ioctl.o \
	mapphone.o

# all are boolean
aufs-$(CONFIG_PROC_FS) += procfs.o plink.o
aufs-$(CONFIG_SYSFS) += sysfs.o
aufs-$(CONFIG_DEBUG_FS) += dbgaufs.o
aufs-$(CONFIG_AUFS_BDEV_LOOP) += loop.o
aufs-$(CONFIG_AUFS_HNOTIFY) += hnotify.o
aufs-$(CONFIG_AUFS_HFSNOTIFY) += hfsnotify.o
aufs-$(CONFIG_AUFS_HINOTIFY) += hinotify.o
aufs-$(CONFIG_AUFS_EXPORT) += export.o
aufs-$(CONFIG_AUFS_POLL) += poll.o
aufs-$(CONFIG_AUFS_RDU) += rdu.o
aufs-$(CONFIG_AUFS_SP_IATTR) += f_op_sp.o
aufs-$(CONFIG_AUFS_BR_HFSPLUS) += hfsplus.o
aufs-$(CONFIG_AUFS_DEBUG) += debug.o
aufs-$(CONFIG_AUFS_MAGIC_SYSRQ) += sysrq.o
