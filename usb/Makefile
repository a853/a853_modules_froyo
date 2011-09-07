# USB customization module for Motorola Milestone
# Enables RNDIS, optionally IAD and customizes USB settings
#
# Use CONFIG_USB_ANDROID_RNDIS_IAD to enable IAD

EXTRA_CFLAGS += -DCONFIG_USB_ANDROID_RNDIS_IAD

obj-m += mot_usb.o
 
mot_usb-y := rndis.o f_rndis.o u_ether.o hij_composite.o mapphone.o
