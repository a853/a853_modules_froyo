/*=======================================================================================
 * Imported from COMPOSITE.C (__init section)
 *=====================================================================================*/

static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
		char *buf)
{
	struct usb_function *f = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", !f->hidden);
}

static ssize_t enable_store(
		struct device *dev, struct device_attribute *attr,
		const char *buf, size_t size)
{
	struct usb_function *f = dev_get_drvdata(dev);
	struct usb_composite_driver	*driver = f->config->cdev->driver;
	int value;

	sscanf(buf, "%d", &value);
	if (driver->enable_function)
		driver->enable_function(f, value);
	else
		f->hidden = !value;

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);

int __init mapphone_usb_interface_id(struct usb_configuration *config,
		struct usb_function *function)
{
	unsigned id = config->next_interface_id;

	if (id < MAX_CONFIG_INTERFACES) {
		config->interface[id] = function;
		config->next_interface_id = id + 1;
		return id;
	}
	return -ENODEV;
}

int __init mapphone_usb_add_function(struct usb_configuration *config,
		struct usb_function *function)
{
	struct usb_composite_dev	*cdev = config->cdev;
	int	value = -EINVAL;
	int index;

	DBG(cdev, "adding '%s'/%p to config '%s'/%p\n",
			function->name, function,
			config->label, config);

	if (!function->set_alt || !function->disable)
		goto done;

	index = atomic_inc_return(&cdev->driver->function_count);
	function->dev = device_create(cdev->driver->class, NULL,
		MKDEV(0, index), NULL, function->name);
	if (IS_ERR(function->dev))
		return PTR_ERR(function->dev);

	value = device_create_file(function->dev, &dev_attr_enable);
	if (value < 0) {
		device_destroy(cdev->driver->class, MKDEV(0, index));
		return value;
	}
	dev_set_drvdata(function->dev, function);

	function->config = config;
	list_add_tail(&function->list, &config->functions);

	/* REVISIT *require* function->bind? */
	if (function->bind) {
		value = function->bind(config, function);
		if (value < 0) {
			list_del(&function->list);
			function->config = NULL;
		}
	} else
		value = 0;

	/* We allow configurations that don't work at both speeds.
	 * If we run into a lowspeed Linux system, treat it the same
	 * as full speed ... it's the function drivers that will need
	 * to avoid bulk and ISO transfers.
	 */
	if (!config->fullspeed && function->descriptors)
		config->fullspeed = true;
	if (!config->highspeed && function->hs_descriptors)
		config->highspeed = true;

done:
	if (value)
		DBG(cdev, "adding '%s'/%p --> %d\n",
				function->name, function, value);
	return value;
}

int __init mapphone_usb_string_id(struct usb_composite_dev *cdev)
{
	if (cdev->next_string_id < 254) {
		/* string id 0 is reserved */
		cdev->next_string_id++;
		return cdev->next_string_id;
	}
	return -ENODEV;
}
