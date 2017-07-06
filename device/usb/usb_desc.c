#include <malloc.h>
#include <string.h>
#include "usb_debug.h"
#include "usb_desc.h"
#include "usb.h"
#include "usb_ep0.h"
#include "usb_setup.h"
#include "usb_macros.h"

#define USB_VID		0x0483
#define USB_PID		0x5750
#define USB_RELEASE	0x0003

void usb_desc_init(usb_t *usb)
{
	usb_desc_device(usb);
	usb_desc_config(usb);
}

/* Device descriptor */

typedef struct PACKED desc_dev_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t bcdUSB;
	uint8_t bDeviceClass;
	uint8_t bDeviceSubClass;
	uint8_t bDeviceProtocol;
	uint8_t bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t iManufacturer;
	uint8_t iProduct;
	uint8_t iSerialNumber;
	uint8_t bNumConfigurations;
} desc_dev_t;

static const desc_dev_t desc_dev = {
	18u, SETUP_DESC_TYPE_DEVICE, 0x0200u, 0u, 0u, 0u, 64u,
	USB_VID, USB_PID, USB_RELEASE, 0u, 0u, 0u, 1u
};

desc_t usb_desc_device(usb_t *usb)
{
	if (usb->desc.dev.size)
		return usb->desc.dev;
	desc_dev_t *pd = (desc_dev_t *)malloc(desc_dev.bLength);
	memcpy(pd, &desc_dev, desc_dev.bLength);
	pd->bMaxPacketSize0 = usb_ep0_max_size(usb->base);
	pd->iManufacturer = usb_desc_add_string(usb, 0, LANG_EN_US, "zhiyb");
	pd->iProduct = usb_desc_add_string(usb, 0, LANG_EN_US, "Nosumor remake");
	pd->iSerialNumber = usb_desc_add_string(usb, 0, LANG_EN_US, "V0.3");
	usb->desc.dev.p = pd;
	usb->desc.dev.size = pd->bLength;
	return usb->desc.dev;
}

/* Configration descriptor */

typedef struct PACKED desc_config_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wTotalLength;
	uint8_t bNumInterfaces;
	uint8_t bConfigurationValue;
	uint8_t iConfiguration;
	uint8_t bmAttributes;
	uint8_t bMaxPower;
} desc_config_t;

enum {ConfigAttributeSelfPowered = 1u << 6u,
	ConfigAttributeRemoteWakeup = 1u << 5u};

static const desc_config_t desc_config = {
	9u, SETUP_DESC_TYPE_CONFIGURATION, 9u,
	0u, 1u, 0u, 0x80 | ConfigAttributeRemoteWakeup, 250,
};

desc_t usb_desc_config(usb_t *usb)
{
	if (usb->desc.config.size)
		return usb->desc.config;
	desc_config_t *cp = (desc_config_t *)malloc(desc_config.bLength);
	memcpy(cp, &desc_config, desc_config.bLength);
	usb->desc.config.p = cp;
	usb->desc.config.size = cp->bLength;
	for (usb_if_t **ip = &usb->usbif; *ip != 0; ip = &(*ip)->next) {
		(*ip)->id = cp->bNumInterfaces;
		(*ip)->config(usb, (*ip)->data);
		cp = (desc_config_t *)usb->desc.config.p;
	}
	cp->wTotalLength = usb->desc.config.size;
	return usb->desc.config;
}

/* String descriptor */

typedef struct PACKED desc_string_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wPayload[];
} desc_string_t;

static const desc_string_t desc_string = {2u, SETUP_DESC_TYPE_STRING};

desc_t usb_desc_string(usb_t *usb, uint8_t index, uint16_t lang)
{
	// Default empty descriptor
	desc_t desc = {0, 0};
	// Language list or out-of-range
	if (index == 0) {
		if (usb->desc.lang.size)
			return usb->desc.lang;
		return desc;
	} else if (index > usb->desc.nstring)
		return desc;

	desc_string_list_t **ps = &usb->desc.string[index - 1u];
	// Using the first string as default language
	if (*ps != 0)
		desc = (*ps)->desc;
	// Search for requested language
	for (; *ps != 0; ps = &(*ps)->next)
		if ((*ps)->lang == lang) {
			desc = (*ps)->desc;
			break;
		}
	return desc;
}

uint32_t usb_desc_add_string(usb_t *usb, uint16_t id, uint16_t lang, const char *str)
{
	// Update language list
	if (!usb->desc.lang.size) {
		// New language list
		desc_string_t *pl = (desc_string_t *)malloc(desc_string.bLength + 2u);
		memcpy(pl, &desc_string, desc_string.bLength);
		pl->wPayload[0] = lang;
		pl->bLength += 2;
		usb->desc.lang.p = pl;
		usb->desc.lang.size = pl->bLength;
	} else {
		// Append only if not exists
		desc_string_t *pl = (desc_string_t *)usb->desc.lang.p;
		uint16_t *plang = pl->wPayload;
		for (uint8_t len = pl->bLength; len != desc_string.bLength; len -= 2u)
			if (*plang++ == lang)
				goto add;
		pl = (desc_string_t *)realloc(pl, pl->bLength + 2u);
		pl->wPayload[(pl->bLength - desc_string.bLength) >> 1u] = lang;
		pl->bLength += 2;
		usb->desc.lang.p = pl;
		usb->desc.lang.size = pl->bLength;
	}

add:	// Add string to string lists
	if (id > usb->desc.nstring)
		id = 0;

	// Find string list entry
	desc_string_list_t **psl;
	if (id == 0) {	// Append new string entry
		usb->desc.string =
				(desc_string_list_t **)realloc(usb->desc.string,
				sizeof(desc_string_list_t *) * (usb->desc.nstring + 1u));
		psl = &usb->desc.string[usb->desc.nstring++];
		id = usb->desc.nstring;
		*psl = 0;
	} else
		psl = &usb->desc.string[id - 1u];

	// Iterate to end of string list
	// TODO: Duplication check
	for (; *psl != 0; psl = &(*psl)->next);
	// Add new string entry
	*psl = (desc_string_list_t *)malloc(sizeof(desc_string_list_t));
	(*psl)->next = 0;
	(*psl)->lang = lang;
	// Allocate string descriptor
	uint32_t len = strlen(str);
	desc_string_t *ps = (desc_string_t *)malloc(desc_string.bLength + (len << 1u));
	memcpy(ps, &desc_string, desc_string.bLength);
	ps->bLength += (len << 1u);
	uint16_t *ppl = ps->wPayload;
	while (len--)	// Unicode conversion?
		*ppl++ = *str++;
	(*psl)->desc.p = ps;
	(*psl)->desc.size = ps->bLength;
	return id;
}

/* Interface descriptor */

typedef struct PACKED desc_interface_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bInterfaceNumber;
	uint8_t bAlternateSetting;
	uint8_t bNumEndpoints;
	uint8_t bInterfaceClass;
	uint8_t bInterfaceSubClass;
	uint8_t bInterfaceProtocol;
	uint8_t iInterface;
} desc_interface_t;

static const desc_interface_t desc_interface = {
	9u, SETUP_DESC_TYPE_INTERFACE, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
};

void usb_desc_add_interface(usb_t *usb, uint8_t bNumEndpoints,
			    uint8_t bInterfaceClass, uint8_t bInterfaceSubClass,
			    uint8_t bInterfaceProtocol, uint8_t iInterface)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + desc_interface.bLength);
	desc_interface_t *ip = (desc_interface_t *)(usb->desc.config.p + usb->desc.config.size);
	memcpy(ip, &desc_interface, desc_interface.bLength);
	ip->bNumEndpoints = bNumEndpoints;
	ip->bInterfaceClass = bInterfaceClass;
	ip->bInterfaceSubClass = bInterfaceSubClass;
	ip->bInterfaceProtocol = bInterfaceProtocol;
	ip->iInterface = iInterface;
	usb->desc.config.size += ip->bLength;
	desc_config_t *pc = (desc_config_t *)usb->desc.config.p;
	ip->bInterfaceNumber = pc->bNumInterfaces;
	pc->bNumInterfaces++;
}

/* Endpoint descriptor */

typedef struct PACKED desc_ep_t {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint8_t bEndpointAddress;
	uint8_t bmAttributes;
	uint16_t wMaxPacketSize;
	uint8_t bInterval;
} desc_ep_t;

static const desc_ep_t desc_ep = {
	7u, SETUP_DESC_TYPE_ENDPOINT, 0u, 0u, 0u, 0u,
};

void usb_desc_add_endpoint(usb_t *usb, uint8_t bEndpointAddress, uint8_t bmAttributes,
			   uint16_t wMaxPacketSize, uint8_t bInterval)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + desc_ep.bLength);
	desc_ep_t *ep = (desc_ep_t *)(usb->desc.config.p + usb->desc.config.size);
	memcpy(ep, &desc_ep, desc_ep.bLength);
	ep->bEndpointAddress = bEndpointAddress;
	ep->bmAttributes = bmAttributes;
	ep->wMaxPacketSize = wMaxPacketSize;
	ep->bInterval = bInterval;
	usb->desc.config.size += ep->bLength;
}

/* Arbitrary descriptor */

void usb_desc_add(usb_t *usb, const void *ptr, uint8_t size)
{
	usb->desc.config.p = realloc(usb->desc.config.p,
				     usb->desc.config.size + size);
	void *p = usb->desc.config.p + usb->desc.config.size;
	memcpy(p, ptr, size);
	usb->desc.config.size += size;
}
