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
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/smp_lock.h>

/* we need to read the serial again */
#include <plat/io.h>

#include "u_ether.h"
#include "rndis.h"
#include "f_mot_android.h"
#include "mapphone.h"

MODULE_AUTHOR("Skrilax_CZ");
MODULE_DESCRIPTION("Motorola Milestone RDNIS driver enabler");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

struct device_pid_vid 
{
	char *name;
	u32 type;
	int vid;
	int pid;
	char *config_name;
	int class;
	int subclass;
	int protocol;
};

struct android_dev 
{
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int product_id;
	int version;
	int factory_enabled;
};

struct device_pid_vid* mot_android_vid_pid;
struct android_dev* _android_dev;
struct usb_configuration* android_config_driver;
struct usb_device_descriptor* device_desc;

/* Motorola usb function structure patch */
static char *usb_functions_ums[] = 
{
	"usb_mass_storage",
};

static char *usb_functions_ums_adb[] = 
{
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_rndis[] = 
{
	"rndis",
};

static char *usb_functions_rndis_adb[] = 
{
	"rndis",
	"adb",
};

static char *usb_functions_phone_portal_lite[] = 
{
	"acm",
	"usbnet",
};

static char *usb_functions_phone_portal_lite_adb[] = 
{
	"acm",
	"usbnet",
	"adb",
};

static char *usb_functions_phone_portal[] = 
{
	"acm",
	"usbnet",
	"mtp",
};

static char *usb_functions_phone_portal_adb[] = 
{
	"acm",
	"usbnet",
	"mtp",
	"adb",
};

static char *usb_functions_mtp[] = 
{
	"mtp",
};

static char *usb_functions_mtp_adb[] = 
{
	"mtp",
	"adb",
};

static char *usb_functions_all[] = 
{
	"acm",
	"usbnet",
	"mtp",
	"acm",
	"rndis",
	"usb_mass_storage",
	"adb",
};

/* VID and PID definitions */
#define MAPPHONE_VENDOR_ID														 0x22B8
#define MAPPHONE_PRODUCT_ID														 0x41D9
#define MAPPHONE_ADB_PRODUCT_ID												 0x41DB
#define MAPPHONE_RNDIS_PRODUCT_ID											 0x41E4
#define MAPPHONE_RNDIS_ADB_PRODUCT_ID									 0x41E5
#define MAPPHONE_PHONE_PORTAL_PRODUCT_ID               0x41D8
#define MAPPHONE_PHONE_PORTAL_ADB_PRODUCT_ID           0x41DA
#define MAPPHONE_PHONE_PORTAL_LITE_PRODUCT_ID          0x41D5
#define MAPPHONE_PHONE_PORTAL_LITE_ADB_PRODUCT_ID      0x41ED
#define MAPPHONE_MTP_PRODUCT_ID                        0x41D6
#define MAPPHONE_MTP_ADB_PRODUCT_ID                    0x41DC

/* Motorola usb products structure patch */
static struct android_usb_product usb_products[] = 
{
	{
		.product_id     = MAPPHONE_PHONE_PORTAL_LITE_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_phone_portal_lite),
		.functions      = usb_functions_phone_portal_lite,
	},
	{
		.product_id     = MAPPHONE_PHONE_PORTAL_LITE_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_phone_portal_lite_adb),
		.functions      = usb_functions_phone_portal_lite_adb,
	},
	{
		.product_id     = MAPPHONE_PHONE_PORTAL_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_phone_portal),
		.functions      = usb_functions_phone_portal,
	},
	{
		.product_id     = MAPPHONE_PHONE_PORTAL_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_phone_portal_adb),
		.functions      = usb_functions_phone_portal_adb,
	},
	{
		.product_id     = MAPPHONE_MTP_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp),
		.functions      = usb_functions_mtp,
	},
	{
		.product_id     = MAPPHONE_MTP_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_mtp_adb),
		.functions      = usb_functions_mtp_adb,
	},
	{
		.product_id			= MAPPHONE_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions			= usb_functions_ums,
	},
	{
		.product_id			= MAPPHONE_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions			= usb_functions_ums_adb,
	},
	{
		.product_id			= MAPPHONE_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions			= usb_functions_rndis,
	},
	{
		.product_id			= MAPPHONE_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions			= usb_functions_rndis_adb,
	},
};

/* Motorola vid pid structure patch */
static struct device_pid_vid mot_vid_pid_patch[] = 
{
	{"rndis", RNDIS_TYPE_FLAG, MAPPHONE_VENDOR_ID, MAPPHONE_RNDIS_PRODUCT_ID, 
		"Motorola RNDIS Device", USB_CLASS_WIRELESS_CONTROLLER, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},
		
	{"rndis_adb", RNDIS_TYPE_FLAG | ADB_TYPE_FLAG, MAPPHONE_VENDOR_ID, MAPPHONE_RNDIS_ADB_PRODUCT_ID,
		"Motorola RNDIS ADB Device", USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE, USB_CLASS_PER_INTERFACE},
		
	{"acm_eth", ACM_TYPE_FLAG | ETH_TYPE_FLAG, MAPPHONE_VENDOR_ID, MAPPHONE_PHONE_PORTAL_LITE_PRODUCT_ID, 
	"Motorola Config 30l", USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC},
	
};

static struct device_pid_vid mot_vid_pid_edit[] = 
{
	{"acm_eth_adb", ACM_TYPE_FLAG | ETH_TYPE_FLAG | ADB_TYPE_FLAG, MAPPHONE_VENDOR_ID, MAPPHONE_PHONE_PORTAL_LITE_ADB_PRODUCT_ID, 
	"Motorola Config 31l", USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC, USB_CLASS_VENDOR_SPEC},
};

/* rdnis device (from board-mapphone.c) */
#define MAX_DEVICE_TYPE_NUM   20
#define MAX_USB_SERIAL_NUM		17
#define DIE_ID_REG_BASE				(L4_WK_34XX_PHYS + 0xA000)
#define DIE_ID_REG_OFFSET			0x218
static char device_serial[MAX_USB_SERIAL_NUM];

static struct usb_ether_platform_data rndis_pdata = 
{
	/* ethaddr is filled by board_serialno_setup */
	.vendorID	= MAPPHONE_VENDOR_ID,
	.vendorDescr	= "Motorola",
};

static struct platform_device rndis_device = 
{
	.name	= "rndis",
	.id	= -1,
	.dev	= 
	{
		.platform_data = &rndis_pdata,
	},
};

/* symsearch */
SYMSEARCH_DECLARE_ADDRESS_STATIC(android_enable_function);
SYMSEARCH_DECLARE_ADDRESS_STATIC(enable_android_usb_product_function);
SYMSEARCH_DECLARE_ADDRESS_STATIC(composite_setup);

SYMSEARCH_DECLARE_ADDRESS_STATIC(adb_mode_change_cb);
SYMSEARCH_DECLARE_ADDRESS_STATIC(android_setup_config);
SYMSEARCH_DECLARE_ADDRESS_STATIC(mot_android_vid_pid);

SYMSEARCH_DECLARE_FUNCTION_STATIC(int, get_product_id, struct android_dev *dev);

SYMSEARCH_INIT_FUNCTION(mapphone_android_register_function);
SYMSEARCH_INIT_FUNCTION(mapphone_usb_interface_enum_cb);
SYMSEARCH_INIT_FUNCTION(mapphone_composite_setup_complete);
SYMSEARCH_INIT_FUNCTION(mapphone_usb_descriptor_fillbuf);
SYMSEARCH_INIT_FUNCTION(mapphone_usb_gadget_get_string);
SYMSEARCH_INIT_FUNCTION(mapphone_usb_data_transfer_callback);

/* include the implementations that are in __init section */
#include "imp_epautoconf.c"
#include "imp_composite.c"
#include "imp_config.c"

/* TODO: Only use usbd to switch mode, otherwise you may get undefined result,
 * because of the way Moto wrote the driver. The enable function will get disabled
 * once custom usbd is working.
 */

/* When directly switching a function */
static void android_enable_function_hijack(struct usb_function *f, int enable)
{	
	struct usb_function *func; 
	struct android_dev *dev = _android_dev;
	int disable = !enable;
	int product_id;

	if (!!f->hidden != disable) 
	{
		f->hidden = disable;

		if (!strcmp(f->name, "rndis")) 
		{
			/* We need to specify the COMM class in the
			 * device descriptor if we are using RNDIS.
			 */
			if (enable)
				dev->cdev->desc.bDeviceClass = USB_CLASS_WIRELESS_CONTROLLER;
			else
				dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
		
			/* Windows does not support other interfaces
			 * when RNDIS is enabled, so we disable UMS when
			 * RNDIS is on.  */
			list_for_each_entry(func, &android_config_driver->functions, list)
			{
				if (!strcmp(func->name, "usb_mass_storage"))
				{
					func->hidden = enable;
					break;
				}
			}
		}

		product_id = get_product_id(dev);
		device_desc->idProduct = __constant_cpu_to_le16(product_id);
		
		if (dev->cdev)
			dev->cdev->desc.idProduct = device_desc->idProduct;

		/* force reenumeration */
		if (dev->cdev && dev->cdev->gadget) 
		{
			usb_gadget_disconnect(dev->cdev->gadget);
			msleep(50);
			usb_gadget_connect(dev->cdev->gadget);
			msleep(50);
		}
	}
}

/* When switching a function using usbd */
static int enable_android_usb_product_function_hijack(char *device_name, int cnt)
{
	struct usb_function *f;
	int enable = 1;
	int disable = 0;
	
	/* Motorola wrote the code so "Charge Only" mode is seen as USB Mass Storage,
	 * and based on that the driver chooses vid and pid. That's why "charge_only"
	 * acts as if Mass Storage is enabled.
	 */

	if (!strncmp(device_name, "eth", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "usbnet"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "eth_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "usbnet") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "mtp", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "mtp"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "mtp_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "mtp") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "acm", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "acm_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "acm_eth", cnt - 1)) 
	{	
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm") || !strcmp(f->name, "usbnet"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "acm_eth_adb", cnt - 1)) 
	{	
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm") || !strcmp(f->name, "usbnet") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	
	if (!strncmp(device_name, "acm_eth_mtp", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm") || !strcmp(f->name, "usbnet")
			    || !strcmp(f->name, "mtp"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "acm_eth_mtp_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "acm") || !strcmp(f->name, "usbnet")
			    || !strcmp(f->name, "mtp") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
		
	if (!strncmp(device_name, "msc", cnt - 1) || !strncmp(device_name, "cdrom", cnt - 1)
			|| !strncmp(device_name, "charge_only", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "usb_mass_storage"))				
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "msc_adb", cnt - 1)  || !strncmp(device_name, "cdrom_adb", cnt - 1)
			|| !strncmp(device_name, "charge_only_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "usb_mass_storage") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	
	if (!strncmp(device_name, "rndis", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "rndis"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}
	if (!strncmp(device_name, "rndis_adb", cnt - 1)) 
	{
		list_for_each_entry(f, &(android_config_driver->functions), list) 
		{
			if (!strcmp(f->name, "rndis") || !strcmp(f->name, "adb"))
				f->hidden = disable;
			else
				f->hidden = enable;
		}
		return 0;
	}

	printk(KERN_ERR "unknown mode: %s\n", device_name);
	return -1;
}

static unsigned long __init get_function_end_address(unsigned long address, int max_advance_in_instr)
{
	unsigned int i;
	unsigned long data;

	/*it ends with:
	 *LDMFD   SP!, ...
	 *
	 *mask:
	 *E8 B0 00 00
	 */

	for (i = 0; i < max_advance_in_instr; i++)
	{
		data = *((unsigned long*)(address + i*4));
		
		if ((data & 0xFFF00000) == 0xE8B00000)
			return address + i*4;
	}
	
	return 0xFFFFFFFF;
}

static void __init init_board_rndis(void)
{
	unsigned int val[2];
	unsigned int reg;
	int i;
	char *src;

	reg = DIE_ID_REG_BASE + DIE_ID_REG_OFFSET;
	val[0] = omap_readl(reg);
	val[1] = omap_readl(reg + 4);

	snprintf(device_serial, MAX_USB_SERIAL_NUM, "%08X%08X", val[1], val[0]);

	rndis_pdata.ethaddr[0] = 0x02;
	src = device_serial;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	platform_device_register(&rndis_device);
}

static void __init update_usb_config(struct android_usb_function* rndis_function)
{
	struct list_head* adb_list_head = NULL;
	struct list_head* last_list_head = NULL;
	struct usb_function* f;

	/* fist add it, then call bind manually (so it doesn't call bind for the rest) */
	mapphone_android_register_function(rndis_function);
	printk(KERN_INFO "Registered rndis function.\n");

	/* update usb data */
	_android_dev->products = usb_products;
	_android_dev->num_products = ARRAY_SIZE(usb_products);
	_android_dev->functions = usb_functions_all;
	_android_dev->num_functions = ARRAY_SIZE(usb_functions_all);
	printk(KERN_INFO "Patched _android_dev products and functions.\n");

	/* manually bind config, as we're calling this after the original config is bound */
	rndis_function->bind_config(android_config_driver);
	printk(KERN_INFO "Bound rndis config.\n");
	
	/* rearrange the functions in the config driver list (put adb as the last one) */
	list_for_each_entry(f, &(android_config_driver->functions), list)
	{
		last_list_head = &f->list;
		if (!strcmp(f->name, "adb"))
			adb_list_head = &f->list;
	}
	
	if (last_list_head != NULL && adb_list_head != last_list_head)
		list_move(adb_list_head, last_list_head);
}

static int __init mapphone_rndis_init(void)
{
	struct android_usb_function* rndis_function;
	int i, j, patch_index;
	unsigned long addr;
	
	SYMSEARCH_BIND_ADDRESS(rndis, android_enable_function);
	SYMSEARCH_BIND_ADDRESS(rndis, enable_android_usb_product_function);
	SYMSEARCH_BIND_ADDRESS(rndis, composite_setup);
	
	SYMSEARCH_BIND_ADDRESS(rndis, adb_mode_change_cb);
	SYMSEARCH_BIND_ADDRESS(rndis, android_setup_config);
	SYMSEARCH_BIND_ADDRESS(rndis, mot_android_vid_pid);
	
	SYMSEARCH_BIND_FUNCTION(rndis, get_product_id);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, usb_interface_enum_cb, mapphone_usb_interface_enum_cb);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, android_register_function, mapphone_android_register_function);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, composite_setup_complete, mapphone_composite_setup_complete);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, usb_descriptor_fillbuf, mapphone_usb_descriptor_fillbuf);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, usb_gadget_get_string, mapphone_usb_gadget_get_string);
	SYMSEARCH_BIND_FUNCTION_TO(rndis, usb_data_transfer_callback, mapphone_usb_data_transfer_callback);
	
	mot_android_vid_pid = (void *)SYMSEARCH_GET_ADDRESS(mot_android_vid_pid);
		
	/* fetch the structures from adb_mode_change_cb fucntion	*/
	addr = SYMSEARCH_GET_ADDRESS(adb_mode_change_cb);
	addr = get_function_end_address(addr, 0x800); //2kB
	
	if (addr == 0xFFFFFFFF)
	{
		printk(KERN_ERR "Could not fetch _android_dev");
		return -EBUSY;
	}
		
	_android_dev = *((struct android_dev**) (*((unsigned long*)(addr + 4))));
	printk(KERN_INFO "_android_dev is on 0x%08lx.\n", (unsigned long)_android_dev);
	
	android_config_driver = _android_dev->config;
	device_desc = &(_android_dev->cdev->desc);
			
	patch_index = -1;
	
	/* Check Motorola VID & PID structure */
	for (i = 0; i < MAX_DEVICE_TYPE_NUM - ARRAY_SIZE(mot_vid_pid_patch); i++)
	{
		if (mot_android_vid_pid[i].name == NULL)
		{
			patch_index = i;
			break;
		}
	}
	
	if (patch_index == -1)
	{
		printk(KERN_ERR "Cannot patch mot_android_vid_pid for rndis support.\n");
		return -EBUSY;
	}
	
	/* do edits and append new configurations */
	for (i = 0; i < patch_index; i++)
	{	
		for (j = 0; j < ARRAY_SIZE(mot_vid_pid_edit); j++)
		{
			if (!strcmp(mot_android_vid_pid[i].name, mot_vid_pid_edit[j].name))
				memcpy(&mot_android_vid_pid[i], &mot_vid_pid_edit[j], sizeof(struct device_pid_vid));
		}
	}
	
	memcpy(&mot_android_vid_pid[patch_index], &mot_vid_pid_patch[0], sizeof(struct device_pid_vid) * ARRAY_SIZE(mot_vid_pid_patch));
	printk(KERN_INFO "Patched mot_android_vid_pid for rndis support.\n");
	
	/* reinitalize board */
	init_board_rndis();

	/* initialize endpoint autoconfig */
	mapphone_init_epnum(android_config_driver->cdev->gadget);
	
	/* initialize composite driver in hijack */
	mapphone_init_composite(android_config_driver->cdev->driver);
	
	/* hijack enable functions (both directly and via usbd) and composite setup */
	
	lock_kernel();
	
	hijack_function(SYMSEARCH_GET_ADDRESS(android_enable_function), (unsigned long)&android_enable_function_hijack);
	printk(KERN_INFO "Hijacked android_enable_function.\n");
	
	hijack_function(SYMSEARCH_GET_ADDRESS(enable_android_usb_product_function), (unsigned long)&enable_android_usb_product_function_hijack);
	printk(KERN_INFO "Hijacked enable_android_usb_product_function.\n");
	
	hijack_function(SYMSEARCH_GET_ADDRESS(composite_setup), (unsigned long)&composite_setup_hijack);
	printk(KERN_INFO "Hijacked composite_setup.\n");
	
	unlock_kernel();
		

	/* get rndis_function */
	rndis_function = mapphone_f_rndis_init();
	
	if (rndis_function == NULL)
	{
		printk(KERN_ERR "f_rndis_init didn't return valid rndis_function.\n");
		return -EBUSY;
	}

	/* update usb configuration */
	update_usb_config(rndis_function);

	return 0;
}

module_init(mapphone_rndis_init);

