#ifndef USB_EP0_SETUP_H
#define USB_EP0_SETUP_H

#include <stdint.h>
#include <macros.h>

typedef union PACKED {
	struct {
		uint8_t bmRequestType;
		uint8_t bRequest;
		union {
			uint16_t wValue;
			struct {
				uint8_t bIndex;
				uint8_t bType;
			};
		};
		union {
			uint16_t wIndex;
			struct {
				uint8_t bID;
				uint8_t bEntityID;
			};
		};
		uint16_t wLength;
	};
	uint32_t raw[2];
} usb_setup_t;

void usb_ep0_setup(void *p, uint32_t size);

#endif // USB_EP0_SETUP_H