/*
 * Copyright (C) 2005-2011
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
 
#include <linux/module.h>
#include <linux/ima.h>
#include <linux/namei.h>
#include <linux/security.h>
#include <linux/splice.h>
#include <linux/uaccess.h>
#include "aufs.h"
#include "mapphone.h"

SYMSEARCH_DECLARE_FUNCTION_STATIC(struct dentry *, __lookup_hash, struct qstr *name, struct dentry * base, struct nameidata *nd); 

SYMSEARCH_INIT_FUNCTION(__lookup_one_len);
SYMSEARCH_INIT_FUNCTION(do_splice_from);
SYMSEARCH_INIT_FUNCTION(do_splice_to);
SYMSEARCH_INIT_FUNCTION(mapphone_file_kill);
SYMSEARCH_INIT_FUNCTION(mapphone_do_truncate);
SYMSEARCH_INIT_FUNCTION(mapphone_cap_file_mmap);
SYMSEARCH_INIT_FUNCTION(mapphone_deny_write_access);

struct dentry *lookup_hash(struct nameidata *nd)
{
	return __lookup_hash(&nd->last, nd->path.dentry, nd);
}

int load_symbols(void)
{
	SYMSEARCH_BIND_FUNCTION(aufs,__lookup_hash);
	SYMSEARCH_BIND_FUNCTION(aufs,__lookup_one_len);
	SYMSEARCH_BIND_FUNCTION(aufs,do_splice_from);
	SYMSEARCH_BIND_FUNCTION(aufs,do_splice_to);
	
	SYMSEARCH_BIND_FUNCTION_TO(aufs,file_kill,mapphone_file_kill);
	SYMSEARCH_BIND_FUNCTION_TO(aufs,do_truncate,mapphone_do_truncate);
	SYMSEARCH_BIND_FUNCTION_TO(aufs,cap_file_mmap,mapphone_cap_file_mmap);
	SYMSEARCH_BIND_FUNCTION_TO(aufs,deny_write_access,mapphone_deny_write_access);
	
	return 0;
}


