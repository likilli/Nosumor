#ifndef USB_CLASS_H
#define USB_CLASS_H

#include <stdint.h>

#include "usb_def.h"
#include "usb_desc.h"

#define EP1_SIZE	HID_REPORT_KEYBOARD_SIZE
#define EP2_SIZE	HID_REPORT_VENDOR_IN_SIZE

#ifdef __cplusplus
extern "C" {
#endif

void usbClassInit();
void usbClassReset();
void usbClassHalt(uint16_t epaddr, uint16_t e);
void usbClassSetup(struct setup_t *setup);
void usbClassSetupInterface(struct setup_t *setup);

void usbHIDReport(const void *ptr, uint8_t size);

#ifdef __cplusplus
}
#endif

#endif // USB_CLASS_H