#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/audio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/dwc/otg_fs.h>

#include <FreeRTOS.h>
#include <task.h>

#include "usb.h"

uint32_t sampleRate;

static const struct {
	struct usb_audio_header_descriptor_head header_head;
	struct usb_audio_header_descriptor_body header_body;
	struct usb_audio_input_terminal_descriptor in;
	struct usb_audio_feature_unit_descriptor_head feature_head;
	struct usb_audio_feature_unit_descriptor_tail feature_tail;
	struct usb_audio_output_terminal_descriptor out;
} __attribute__((packed)) audio_ctl_function = {
	.header_head = {
		.bLength = sizeof(struct usb_audio_header_descriptor_head) +
		           sizeof(struct usb_audio_header_descriptor_body),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
		.bcdADC = 0x0100,
		.wTotalLength =
			   sizeof(struct usb_audio_header_descriptor_head) +
			   sizeof(struct usb_audio_header_descriptor_body) +
			   sizeof(struct usb_audio_input_terminal_descriptor) +
			   sizeof(struct usb_audio_feature_unit_descriptor_head) +
			   sizeof(struct usb_audio_feature_unit_descriptor_tail) +
			   sizeof(struct usb_audio_output_terminal_descriptor),
		.binCollection = 1,
	},
	.header_body = {
		.baInterfaceNr = 0x01,
	},
	.in = {
		.bLength = sizeof(struct usb_audio_input_terminal_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_AUDIO_TYPE_INPUT_TERMINAL,
		.bTerminalID = 1,
		.wTerminalType = 0x0101, // USB Streaming
		.bAssocTerminal = 0,
		.bNrChannels = 1,
		.wChannelConfig = 0x0004, // Center Front
		.iChannelNames = 0,
		.iTerminal = 0,
	},
	.feature_head = {
		.bLength = sizeof(struct usb_audio_feature_unit_descriptor_head) +
			   sizeof(struct usb_audio_feature_unit_descriptor_tail),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_AUDIO_TYPE_FEATURE_UNIT,
		.bUnitID = 2,
		.bSourceID = 1,
		.bControlSize = 1,
		.bmaControlMaster = 0x0001 // mute
	},
	.feature_tail = {
		.iFeature = 0
	},
	.out = {
		.bLength = sizeof(struct usb_audio_output_terminal_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = USB_AUDIO_TYPE_OUTPUT_TERMINAL,
		.bTerminalID = 2,
		.wTerminalType = 0x0301, // Speaker 
		.bAssocTerminal = 0,
		.bSourceID = 1,
		.iTerminal = 0
	}
};

static const struct usb_interface_descriptor audio_ctl_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.extra = &audio_ctl_function,
	.extralen = sizeof(audio_ctl_function)
}};

static const struct usb_audio_stream_endpoint_descriptor endpoint_iso[] = {{
	.bLength = sizeof(struct usb_audio_stream_endpoint_descriptor),
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x02,
	.bmAttributes = USB_ENDPOINT_ATTR_ISOCHRONOUS | USB_ENDPOINT_ATTR_ADAPTIVE,
	.wMaxPacketSize = 16,
	.bInterval = 1,
	.bRefresh = 0,
	.bSynchAddress = 0
}};

static struct {
	struct usb_audio_stream_interface_descriptor general;
	struct usb_audio_format_type1_descriptor_1freq format;
	struct usb_audio_stream_audio_endpoint_descriptor audio_endpoint;
} __attribute__((packed)) audio_stream_function = {
	.general = {
		.bLength = sizeof(struct usb_audio_stream_interface_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
		.bDescriptorSubtype = 0x01, // General AS Descriptor
		.bTerminalLink = 1, // audio_ctl_function.in
		.bDelay = 1,
		.wFormatTag = 0x0001, // PCM
	},
	.format = {
		.head = {
			.bLength = sizeof(struct usb_audio_format_type1_descriptor_1freq),
			.bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
			.bDescriptorSubtype = 0x02, // Format type descriptor
			.bFormatType = 1, 
			.bNrChannels = 1,
			.bSubFrameSize = 2,
			.bBitResolution = 16,
			.bSamFreqType = 1,
		},
		.freqs = {
			0x001f40 // 8000
		},
	},
	.audio_endpoint = {
		.bLength = sizeof(struct usb_audio_stream_audio_endpoint_descriptor),
		.bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
		.bDescriptorSubtype = 0x1, // General descriptor
		.bmAttributes = 0x01,
		.bLockDelayUnits = 1, // milliseconds
		.wLockDelay = 1
	}
};

static const struct usb_interface_descriptor audio_stream_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_AUDIO,
	.bInterfaceSubClass = USB_AUDIO_SUBCLASS_AUDIOSTREAMING,
	.bInterfaceProtocol = 0,
	.iInterface = 0,
	.endpoint = (struct usb_endpoint_descriptor *)endpoint_iso,
	.extra = &audio_stream_function,
	.extralen = sizeof(audio_stream_function),
}};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = audio_ctl_iface,
}, {
	.num_altsetting = 1,
	.altsetting = audio_stream_iface,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80, // bus powered, no remote wakeup
	.bMaxPower = 250, // 500mA
	.interface = ifaces,
};

static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 16,
	.idVendor = 0x292b,
	.idProduct = 0xf115,
	.bcdDevice = 0x0100,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const char *strings[] = {
	"Ajustage Inc",
	"Ajustage Inc. FXS Ag1171",
	"00000001",
};

static uint8_t control_buffer[128];

static enum usbd_request_return_codes control_cb(
	usbd_device *usbd_dev, struct usb_setup_data *req,
	uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *dev, struct usb_setup_data *req))
{
	if ((req->bmRequestType & USB_REQ_TYPE_IN)) {
		if (req->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_ENDPOINT)) {
			// get sample rate
			if (req->bRequest == 0x81) {
				control_buffer[0] = sampleRate;
				control_buffer[1] = sampleRate >> 8;
				control_buffer[2] = sampleRate >> 16;
				return USBD_REQ_HANDLED;
			}
		}
	} else {
		if (req->bmRequestType & (USB_REQ_TYPE_CLASS | USB_REQ_TYPE_ENDPOINT)) {
                        // set sample rate
			if (req->bRequest == 1) {
				if (*len == 3) {
					sampleRate  = (control_buffer[0] << 0)
					            | (control_buffer[1] << 8)
					            | (control_buffer[2] << 16);
					return USBD_REQ_HANDLED;
				}
			}
		}
	}

	return USBD_REQ_NOTSUPP;
}

static void audio_rx_cb(usbd_device *usbd_dev, uint8_t ep) {
	(void)ep;

	static int16_t *s16_le;
	static int i = 32;
	uint8_t buf[16];

	int len = usbd_ep_read_packet(usbd_dev, 0x02, buf, 16);
	if (OTG_FS_DSTS & (1 << 8)) {
		OTG_FS_DOEPCTL(2) |= OTG_DOEPCTLX_SD0PID;
	} else {
		OTG_FS_DOEPCTL(2) |= (1 << 29);
	}

	s16_le = (int16_t *)buf;

	if (fill_dac1) {
		for (i = 0; i < HSAMPLES; i++) {
			dac_samples[i] = ((uint16_t)(*s16_le++ + 32767)) >> 4;
		}
		fill_dac1 = false;
	} else if (fill_dac2) {
		for (i = HSAMPLES; i < NSAMPLES; i++) {
			dac_samples[i] = ((uint16_t)(*s16_le++ + 32767)) >> 4;
		}
		fill_dac2 = false;
	} else {
		gpio_toggle(GPIOC, GPIO12);
	}
}

static void config_cb(usbd_device *usbd_dev, uint16_t wValue)
{
	usbd_ep_setup(usbd_dev, 0x02, USB_ENDPOINT_ATTR_ISOCHRONOUS, 16, audio_rx_cb);

	usbd_register_control_callback(usbd_dev,
		USB_REQ_TYPE_CLASS | USB_REQ_TYPE_ENDPOINT,
		USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
		control_cb);
}

void task_usb(void *arg)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_OTGFS);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO11 | GPIO12);

	usbd_device *usbd_dev = usbd_init(
		&otgfs_usb_driver, &dev, &config,
		strings, 3,
		control_buffer, sizeof(control_buffer)
	);

	usbd_register_set_config_callback(usbd_dev, config_cb);

	// pull D+ up
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);
	gpio_clear(GPIOC, GPIO11);

	for (;;) {
		usbd_poll(usbd_dev);
	}
}
