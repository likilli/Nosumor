#include <stdint.h>
#include "macros.h"
#include "usb_def.h"

static const unsigned char device[] = {
	18,			// bLength
	DESC_TYPE_DEVICE,	// bDescriptorType
	0x00,			// bcdUSB		USB specification number
	0x02,
	0,			// bDeviceClass
	0,			// bDeviceSubClass
	0,			// bDeviceProtocol
	64,			// bMaxPacketSize
	L8(USB_VID),		// idVendor
	H8(USB_VID),
	L8(USB_PID),		// idProduct
	H8(USB_PID),
	0x00,			// bcdDevice		Device release number
	0x00,
	0,			// iManufacturer	Manufacturer string
	0,			// iProduct		Product string
	0,			// iSerialNumber	Serial number string
	1,			// bNumConfigurations	Number of possible configurations
};

static const unsigned char config[] = {
	// Configuration descriptor 0
	9,			// bLength
	DESC_TYPE_CONFIG,	// bDescriptorType
	9 * 3 + 7 * 1,		// wTotalLength		Total length of data
	0,
	1,			// bNumInterfaces	Number of interfaces
	0,			// bConfigurationValue	Configuration index
	0,			// iConfiguration	Configuration string
	0xa0,			// bmAttributes		(Remote wakeup)
	250,			// bMaxPower		Max power in 2mA units

	//     Interface descriptor 0
	9,			// bLength
	DESC_TYPE_INTERFACE,	// bDescriptorType
	0,			// bInterfaceNumber	Number of interface
	0,			// bAlternateSetting	Alternative setting
	1,			// bNumEndpoints	Number of endpoints used
	3,			// bInterfaceClass	3: HID class
	1,			// bInterfaceSubClass	1: Boot interface
	1,			// bInterfaceProtocol	0: None, 1: Keyboard, 2: Mouse
	0,			// iInterface		Interface string

	//         HID descriptor
	9,			// bLength
	DESC_TYPE_HID,		// bDescriptorType
	0x11,			// bcdHID		HID class specification
	0x01,
	0x00,			// bCountryCode		Hardware target country
	1,			// bNumDescriptors	Number of HID class descriptors
	0x22,			// bDescriptorType	Report descriptor type
	100,			// wDescriptorLength	Total length of Report descriptor
	0,

	//         Endpoint descriptor 1
	7,			// bLength
	DESC_TYPE_ENDPOINT,	// bDescriptorType
	DESC_EP_IN | 1,		// bEndpointAddress
	DESC_EP_INTERRUPT,	// bmAttributes
	64,			// wMaxPacketSize	Maximum packet size
	1,			// bInterval		Polling interval
};

static const struct desc_t desc_device[] = {
	{sizeof(device), device},
};

static const struct desc_t desc_config[] = {
	{sizeof(config), config},
};

const struct descriptor_t descriptors = {
	desc_device, desc_config,
};
