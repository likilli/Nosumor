#include <malloc.h>
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_macros.h"
#include "../usb_ep.h"
#include "../usb_irq.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_ep_data.h"

#define DATA_MAX_PKT	1u
#define DATA_MAX_SIZE	(EP_MAX_SIZE * DATA_MAX_PKT)

typedef struct {
	void *data[2];
	int swap, enabled;
} epdata_t;

static void epout_recv(usb_t *usb, uint32_t n)
{
#if DATA_MAX_PKT == 1u
	// Check frame parity
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	uint32_t fn = FIELD(dev->DSTS, USB_OTG_DSTS_FNSOF) & 1u;
	fn = fn ? USB_OTG_DOEPCTL_SD0PID_SEVNFRM_Msk : USB_OTG_DOEPCTL_SODDFRM_Msk;
#else
	uint32_t fn = 0;
#endif
	// Configure endpoint DMA
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	ep->DOEPDMA = (uint32_t)epdata->data[epdata->swap];
	// Reset packet counter
	ep->DOEPTSIZ = (DATA_MAX_PKT << USB_OTG_DOEPTSIZ_PKTCNT_Pos) | DATA_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_OTEPDIS_Msk | USB_OTG_DOEPINT_XFRC_Msk;
	// Enable endpoint
	ep->DOEPCTL |= fn | USB_OTG_DOEPCTL_CNAK | USB_OTG_DOEPCTL_EPENA;
	epdata->swap = !epdata->swap;
}

static void epout_init(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	// Set endpoint type
	ep->DOEPCTL = USB_OTG_DOEPCTL_USBAEP_Msk | EP_TYP_ISOCHRONOUS | DATA_MAX_SIZE;
	// Clear interrupts
	ep->DOEPINT = USB_OTG_DOEPINT_OTEPDIS_Msk | USB_OTG_DOEPINT_XFRC_Msk;
	// Unmask interrupts
	USB_OTG_DeviceTypeDef *dev = DEV(usb->base);
	if (n == 1u) {
		dev->DEACHINT = USB_OTG_DEACHINT_OEP1INT_Msk;
		dev->DOUTEP1MSK = USB_OTG_DOEPINT_XFRC_Msk;
		dev->DEACHMSK |= USB_OTG_DEACHINTMSK_OEP1INTM_Msk;
	} else {
		dev->DAINTMSK |= DAINTMSK_OUT(n);
	}
}

static void epout_halt(usb_t *usb, uint32_t n, int halt)
{
	epdata_t *epdata = usb->epout[n].data;
	audio_out_enable(!halt);
	if (!epdata->enabled != !halt)
		return;
	epdata->enabled = !halt;
	if (!halt) {
		usb_isoc_check(usb, n | EP_DIR_OUT, 1);
		// Clear interrupts
		USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
		ep->DOEPINT = USB_OTG_DOEPINT_OTEPDIS_Msk | USB_OTG_DOEPINT_XFRC_Msk;
		// Receive data
		epout_recv(usb, n);
	}
}

static void epout_xfr_cplt(usb_t *usb, uint32_t n)
{
	USB_OTG_OUTEndpointTypeDef *ep = EP_OUT(usb->base, n);
	epdata_t *epdata = usb->epout[n].data;
	uint32_t siz = ep->DOEPTSIZ;
	if (epdata->enabled)
		epout_recv(usb, n);
	uint32_t pktcnt = DATA_MAX_PKT - FIELD(siz, USB_OTG_DOEPTSIZ_PKTCNT);
	uint32_t size = DATA_MAX_SIZE - FIELD(siz, USB_OTG_DOEPTSIZ_XFRSIZ);
	if (pktcnt)
		audio_play(epdata->data[epdata->swap], size);
	// No more isochronous incomplete checks needed
	usb_isoc_check(usb, n | EP_DIR_OUT, 0);
}

int usb_audio2_ep_data_register(usb_t *usb)
{
	epdata_t *epdata = calloc(1u, sizeof(epdata_t));
	if (!epdata)
		panic();
	epdata->data[0] = malloc(DATA_MAX_SIZE);
	epdata->data[1] = malloc(DATA_MAX_SIZE);
	if (!epdata->data[0] || !epdata->data[1])
		panic();

	const epout_t epout = {
		.data = epdata,
		.init = &epout_init,
		.halt = &epout_halt,
		.xfr_cplt = &epout_xfr_cplt,
	};
	int ep;
	usb_ep_register(usb, 0, 0, &epout, &ep);
	return ep;
}

void usb_audio2_ep_data_halt(usb_t *usb, int ep, int halt)
{
	epout_halt(usb, ep, halt);
}
