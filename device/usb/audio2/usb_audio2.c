#include <malloc.h>
#include <string.h>
#include "../../debug.h"
#include "../usb.h"
#include "../usb_structs.h"
#include "../usb_setup.h"
#include "../usb_ram.h"
#include "../usb_ep.h"
#include "usb_audio2.h"
#include "usb_audio2_desc.h"
#include "usb_audio2_structs.h"
#include "usb_audio2_entities.h"
#include "usb_audio2_ep_data.h"
#include "usb_audio2_ep_feedback.h"

static void usbif_ac_config(usb_t *usb, void *pdata)
{
	usb_audio_t *audio = (usb_audio_t *)pdata;
	if (!audio->entities)
		return;
	if (!desc_ac.wTotalLength) {
		desc_ac.wTotalLength = desc_ac.bLength;
		for (usb_audio_entity_t **p = &audio->entities; *p; p = &(*p)->next)
			desc_ac.wTotalLength += (*p)->desc->bLength;
	}
	// Audio interface association
	uint32_t s = usb_desc_add_string(usb, 0, LANG_EN_US, "USB Audio");
	usb_desc_add_interface_association(usb, 2u, AUDIO_FUNCTION,
					   FUNCTION_SUBCLASS_UNDEFINED, AF_VERSION_02_00, s);
	// Audio control interface
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOCONTROL, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_ac, desc_ac.bLength);
	// Entity descriptors
	for (usb_audio_entity_t **p = &audio->entities; *p; p = &(*p)->next) {
		usb_desc_add(usb, (*p)->desc, (*p)->desc->bLength);
		free((*p)->desc);
		(*p)->desc = 0;
	}
}

static void usbif_ac_setup_class(usb_t *usb, void *data, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_Msk) {
	case DIR_D2H:
		switch (pkt.bRequest) {
		case CUR:
		case RANGE:
			usb_audio2_get(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	case DIR_H2D:
		switch (pkt.bRequest) {
		case CUR:
		case RANGE:
			usb_audio2_set(usb, data, ep, pkt);
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
		}
		break;
	default:
		usb_ep_in_stall(usb->base, ep);
		dbgbkpt();
	}
}

static void usbif_as_config(usb_t *usb, void *pdata)
{
	usb_audio_t *data = (usb_audio_t *)pdata;
	if (!data->entities)
		return;

	// Register endpoints
	data->ep_data = usb_audio2_ep_data_register(usb);
	// TODO: Feedback endpoint need to have the same endpoint number
	data->ep_feedback = usb_audio2_ep_feedback_register(usb);

	// Alternate setting 0, zero-bandwidth
	usb_desc_add_interface(usb, 0u, 0u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);

	// Alternate setting 1, operational
	usb_desc_add_interface(usb, 1u, 2u, AUDIO, AUDIOSTREAMING, IP_VERSION_02_00, 0u);
	usb_desc_add(usb, &desc_as[0], desc_as[0].bLength);
	for (unsigned int i = 0; i != ASIZE(desc_pcm); i++)
		usb_desc_add(usb, &desc_pcm[i], desc_pcm[i].bLength);

	// Data stream endpoint descriptor
	usb_desc_add_endpoint(usb, EP_DIR_OUT | data->ep_data,
			      EP_ISOCHRONOUS | EP_ISO_ASYNC | EP_ISO_DATA,
			      EP_MAX_SIZE, 1u);
	usb_desc_add(usb, &desc_ep[0], desc_ep[0].bLength);
	// Feedback endpoint descriptor
	usb_desc_add_endpoint_sync(usb, EP_DIR_IN | data->ep_feedback,
				   EP_ISOCHRONOUS | EP_ISO_NONE | EP_ISO_FEEDBACK,
				   4u, 1u, 9u, 0u);
}

static void usbif_as_enable(usb_t *usb, void *pdata)
{
	usb_audio_t *data = pdata;
	usb_audio2_ep_data_halt(usb, data->ep_data, 0);
	usb_audio2_ep_feedback_halt(usb, data->ep_feedback, 0);
	dbgprintf(ESC_MSG "[UAC2] Audio streaming " ESC_ENABLE "enabled\n");
}

static void usbif_as_disable(usb_t *usb, void *pdata)
{
	usb_audio_t *data = pdata;
	usb_audio2_ep_data_halt(usb, data->ep_data, 1);
	usb_audio2_ep_feedback_halt(usb, data->ep_feedback, 1);
	dbgprintf(ESC_MSG "[UAC2] Audio streaming " ESC_DISABLE "disabled\n");
}

static void usbif_as_setup_std(usb_t *usb, void *pdata, uint32_t ep, setup_t pkt)
{
	switch (pkt.bmRequestType & DIR_H2D) {
	case DIR_H2D:
		switch (pkt.bRequest) {
		case SET_INTERFACE:
			switch (pkt.wValue) {
			case 0:
				usbif_as_disable(usb, pdata);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			case 1:
				usbif_as_enable(usb, pdata);
				usb_ep_in_transfer(usb->base, ep, 0, 0);
				break;
			default:
				usb_ep_in_stall(usb->base, ep);
				dbgbkpt();
			}
			break;
		default:
			usb_ep_in_stall(usb->base, ep);
			dbgbkpt();
			break;
		}
		break;
	default:
		dbgbkpt();
		break;
	}
}

usb_audio_t *usb_audio2_init(usb_t *usb)
{
	usb_audio_t *data = calloc(1u, sizeof(usb_audio_t));
	if (!data)
		panic();
	// Audio control interface
	const usb_if_t usbif_ac = {
		.data = data,
		.config = &usbif_ac_config,
		.setup_class = &usbif_ac_setup_class,
	};
	usb_interface_register(usb, &usbif_ac);
	// Audio streaming interface
	const usb_if_t usbif_as = {
		.data = data,
		.config = &usbif_as_config,
		.enable = &usbif_as_enable,
		.disable = &usbif_as_disable,
		.setup_std = &usbif_as_setup_std,
	};
	usb_interface_register(usb, &usbif_as);
	return data;
}
