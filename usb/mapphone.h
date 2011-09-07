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

#ifndef __RNDIS_MAPPHONE_H__
#define __RNDIS_MAPPHONE_H__

#include "../symsearch/symsearch.h"

/* exported functions */

struct dentry *lookup_hash(struct nameidata *nd);
void usb_interface_enum_cb(int flag);

/* rndis init */

struct android_usb_function* mapphone_f_rndis_init(void);

/* fucntions that had to be implementeded inside this module */

struct usb_ep * mapphone_usb_ep_autoconfig(struct usb_gadget *, struct usb_endpoint_descriptor *);
int mapphone_usb_interface_id(struct usb_configuration *, struct usb_function *);
int mapphone_usb_add_function(struct usb_configuration *, struct usb_function *);
int mapphone_usb_string_id(struct usb_composite_dev *c);
struct usb_endpoint_descriptor * mapphone_usb_find_endpoint(struct usb_descriptor_header **src, struct usb_descriptor_header **copy, struct usb_endpoint_descriptor *match);
struct usb_descriptor_header ** mapphone_usb_copy_descriptors(struct usb_descriptor_header **);												

/* hijacks */

int composite_setup_hijack(struct usb_gadget *gadget, const struct usb_ctrlrequest *ctrl);
void mapphone_init_composite(struct usb_composite_driver *driver);

/* functions mapped via symsearch */

SYMSEARCH_DECLARE_FUNCTION(void,
														mapphone_android_register_function,
														struct android_usb_function *f);
														
SYMSEARCH_DECLARE_FUNCTION(void,
														mapphone_usb_interface_enum_cb,
														int);

SYMSEARCH_DECLARE_FUNCTION(void,
														mapphone_composite_setup_complete,
														struct usb_ep *ep, 
														struct usb_request *req);
														
SYMSEARCH_DECLARE_FUNCTION(int,
														mapphone_usb_descriptor_fillbuf,
														void *,
														unsigned,
 														const struct usb_descriptor_header **);
 														
SYMSEARCH_DECLARE_FUNCTION(int,
														mapphone_usb_gadget_get_string,
														struct usb_gadget_strings *,
														int,
														u8*);

SYMSEARCH_DECLARE_FUNCTION(void,
														mapphone_usb_data_transfer_callback,
														void);


#endif //!__RNDIS_MAPPHONE_H__
