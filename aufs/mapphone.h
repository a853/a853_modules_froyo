/*
 * Copyright (C) 2011
 * Motorola Milestone adaptation by Skrilax_CZ
 *
 * This program, aufs is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
 * Support for standalone module for Motorola Milestone
 * Requires symsearch
 */

#ifndef __AUFS_MAPPHONE_H__
#define __AUFS_MAPPHONE_H__

#include "../symsearch/symsearch.h"

extern struct dentry *lookup_hash(struct nameidata *nd);

SYMSEARCH_DECLARE_FUNCTION(int, __lookup_one_len, const char *name, struct qstr *this, struct dentry *base, int len);
SYMSEARCH_DECLARE_FUNCTION(long, do_splice_from, struct pipe_inode_info *pipe, struct file *out, loff_t *ppos, size_t len, unsigned int flags);
SYMSEARCH_DECLARE_FUNCTION(long, do_splice_to, struct file *in, loff_t *ppos, struct pipe_inode_info *pipe, size_t len, unsigned int flags);

SYMSEARCH_DECLARE_FUNCTION(void, mapphone_file_kill, struct file *f);
SYMSEARCH_DECLARE_FUNCTION(int, mapphone_do_truncate, struct dentry *, loff_t start, unsigned int time_attrs, struct file *filp);
SYMSEARCH_DECLARE_FUNCTION(int, mapphone_cap_file_mmap, struct file *file, unsigned long reqprot,
														unsigned long prot, unsigned long flags,
														unsigned long addr, unsigned long addr_only);
														
SYMSEARCH_DECLARE_FUNCTION(int, mapphone_deny_write_access, struct file *h_file);

//symsearch load

extern int load_symbols(void);

#endif //!__AUFS_MAPPHONE_H__
