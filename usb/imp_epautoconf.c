/*=======================================================================================
 * Imported from EPAUTOCONF.c (__init section)
 *=====================================================================================*/

static __initdata unsigned epnum;

/* Initialize epnum to first unclaimed endpoint */
static void __init mapphone_init_epnum(struct usb_gadget *gadget)
{
	struct usb_ep *ep;
	u8	num, max;

	/* Iterate endpoints and check if it's claimed.
	 * First unclaimed is our endpoint. */
	
	max = 0;
	num = 0;
	
	list_for_each_entry (ep, &gadget->ep_list, ep_list) 
	{
		if (ep->name == NULL)
			break;
		
		if (isdigit (ep->name [2]))	
		 num = simple_strtoul (&ep->name [2], NULL, 10);
		 
		if (num > max && ep->driver_data != NULL)
			max = num;
	}
	
	epnum = max + 1;
	printk(KERN_INFO "current epnum: %d", epnum);
	
}

static int __init
ep_matches (
	struct usb_gadget		*gadget,
	struct usb_ep			*ep,
	struct usb_endpoint_descriptor	*desc
)
{
	u8		type;
	const char	*tmp;
	u16		max;

	/* endpoint already claimed? */
	if (NULL != ep->driver_data)
		return 0;

	/* only support ep0 for portable CONTROL traffic */
	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
	if (USB_ENDPOINT_XFER_CONTROL == type)
		return 0;

	/* some other naming convention */
	if ('e' != ep->name[0])
		return 0;

	/* type-restriction:  "-iso", "-bulk", or "-int".
	 * direction-restriction:  "in", "out".
	 */
	if ('-' != ep->name[2]) {
		tmp = strrchr (ep->name, '-');
		if (tmp) {
			switch (type) {
			case USB_ENDPOINT_XFER_INT:
				/* bulk endpoints handle interrupt transfers,
				 * except the toggle-quirky iso-synch kind
				 */
				if ('s' == tmp[2])	// == "-iso"
					return 0;
				/* for now, avoid PXA "interrupt-in";
				 * it's documented as never using DATA1.
				 */
				if (gadget_is_pxa (gadget)
						&& 'i' == tmp [1])
					return 0;
				break;
			case USB_ENDPOINT_XFER_BULK:
				if ('b' != tmp[1])	// != "-bulk"
					return 0;
				break;
			case USB_ENDPOINT_XFER_ISOC:
				if ('s' != tmp[2])	// != "-iso"
					return 0;
			}
		} else {
			tmp = ep->name + strlen (ep->name);
		}

		/* direction-restriction:  "..in-..", "out-.." */
		tmp--;
		if (!isdigit (*tmp)) {
			if (desc->bEndpointAddress & USB_DIR_IN) {
				if ('n' != *tmp)
					return 0;
			} else {
				if ('t' != *tmp)
					return 0;
			}
		}
	}

	/* endpoint maxpacket size is an input parameter, except for bulk
	 * where it's an output parameter representing the full speed limit.
	 * the usb spec fixes high speed bulk maxpacket at 512 bytes.
	 */
	max = 0x7ff & le16_to_cpu(desc->wMaxPacketSize);
	switch (type) {
	case USB_ENDPOINT_XFER_INT:
		/* INT:  limit 64 bytes full speed, 1024 high speed */
		if (!gadget->is_dualspeed && max > 64)
			return 0;
		/* FALLTHROUGH */

	case USB_ENDPOINT_XFER_ISOC:
		/* ISO:  limit 1023 bytes full speed, 1024 high speed */
		if (ep->maxpacket < max)
			return 0;
		if (!gadget->is_dualspeed && max > 1023)
			return 0;

		/* BOTH:  "high bandwidth" works only at high speed */
		if ((desc->wMaxPacketSize & cpu_to_le16(3<<11))) {
			if (!gadget->is_dualspeed)
				return 0;
			/* configure your hardware with enough buffering!! */
		}
		break;
	}

	/* MATCH!! */

	/* report address */
	desc->bEndpointAddress &= USB_DIR_IN;
	if (isdigit (ep->name [2])) {
		u8	num = simple_strtoul (&ep->name [2], NULL, 10);
		desc->bEndpointAddress |= num;
	} else {
		if (++epnum > 15)
			return 0;
		desc->bEndpointAddress |= epnum;
	}

	/* report (variable) full speed bulk maxpacket */
	if (USB_ENDPOINT_XFER_BULK == type) {
		int size = ep->maxpacket;

		/* min() doesn't work on bitfields with gcc-3.5 */
		if (size > 64)
			size = 64;
		desc->wMaxPacketSize = cpu_to_le16(size);
	}
	return 1;
}

static struct usb_ep * __init
find_ep (struct usb_gadget *gadget, const char *name)
{
	struct usb_ep	*ep;

	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		if (0 == strcmp (ep->name, name))
			return ep;
	}
	return NULL;
}

struct usb_ep * __init mapphone_usb_ep_autoconfig (
	struct usb_gadget		*gadget,
	struct usb_endpoint_descriptor	*desc
)
{
	struct usb_ep	*ep;
	u8		type;

	type = desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;

	/* First, apply chip-specific "best usage" knowledge.
	 * This might make a good usb_gadget_ops hook ...
	 */
	if (gadget_is_net2280 (gadget) && type == USB_ENDPOINT_XFER_INT) {
		/* ep-e, ep-f are PIO with only 64 byte fifos */
		ep = find_ep (gadget, "ep-e");
		if (ep && ep_matches (gadget, ep, desc))
			return ep;
		ep = find_ep (gadget, "ep-f");
		if (ep && ep_matches (gadget, ep, desc))
			return ep;

	} else if (gadget_is_goku (gadget)) {
		if (USB_ENDPOINT_XFER_INT == type) {
			/* single buffering is enough */
			ep = find_ep (gadget, "ep3-bulk");
			if (ep && ep_matches (gadget, ep, desc))
				return ep;
		} else if (USB_ENDPOINT_XFER_BULK == type
				&& (USB_DIR_IN & desc->bEndpointAddress)) {
			/* DMA may be available */
			ep = find_ep (gadget, "ep2-bulk");
			if (ep && ep_matches (gadget, ep, desc))
				return ep;
		}

	} else if (gadget_is_sh (gadget) && USB_ENDPOINT_XFER_INT == type) {
		/* single buffering is enough; maybe 8 byte fifo is too */
		ep = find_ep (gadget, "ep3in-bulk");
		if (ep && ep_matches (gadget, ep, desc))
			return ep;

	} else if (gadget_is_mq11xx (gadget) && USB_ENDPOINT_XFER_INT == type) {
		ep = find_ep (gadget, "ep1-bulk");
		if (ep && ep_matches (gadget, ep, desc))
			return ep;
	}

	/* Second, look at endpoints until an unclaimed one looks usable */
	list_for_each_entry (ep, &gadget->ep_list, ep_list) {
		if (ep_matches (gadget, ep, desc))
			return ep;
	}

	/* Fail */
	return NULL;
}
