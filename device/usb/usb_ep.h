#ifndef USB_EP_H
#define USB_EP_H

#include <stm32f7xx.h>
#include <stdint.h>
#include "usb.h"

void usb_ep_register(usb_t *usb, const epin_t *epin, int *in, const epout_t *epout, int *out);

uint32_t usb_ep_in_max_size(USB_OTG_GlobalTypeDef *usb, int ep);
void usb_ep_in_transfer(USB_OTG_GlobalTypeDef *usb, int n, const void *p, uint32_t size);
void usb_ep_in_stall(USB_OTG_GlobalTypeDef *usb, int ep);
int usb_ep_in_wait(USB_OTG_GlobalTypeDef *usb, int n);

void usb_ep_out_transfer(USB_OTG_GlobalTypeDef *usb, int n, void *p,
			 uint8_t scnt, uint8_t pcnt, uint32_t size);
void usb_ep_out_ack(USB_OTG_GlobalTypeDef *usb, int n);
void usb_ep_out_stall(USB_OTG_GlobalTypeDef *usb, int n);
void usb_ep_out_wait(USB_OTG_GlobalTypeDef *usb, int n);

#endif // USB_EP_H
