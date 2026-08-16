#include "nacon_host.h"
#include "pico/stdlib.h"
#include <string.h>

#define NACON_VID 0x146B
#define NACON_PID 0x0603

#define NACON_EP_IN  0x81
#define NACON_EP_OUT 0x02

#define NACON_MPS 64

static uint8_t nac_dev_addr = 0;
static uint8_t nac_ep_in = 0;
static uint8_t nac_ep_out = 0;

static uint8_t nac_rx_buf[NACON_MPS];
static uint8_t nac_tx_buf[NACON_MPS];

bool nacon_host_init(void)
{
    nac_dev_addr = 0;
    nac_ep_in = 0;
    nac_ep_out = 0;
    memset(nac_rx_buf, 0, sizeof(nac_rx_buf));
    memset(nac_tx_buf, 0, sizeof(nac_tx_buf));
    return true;
}

uint8_t nacon_host_open(uint8_t dev_addr,
                        tusb_desc_interface_t const* itf_desc,
                        uint16_t max_len)
{
    (void) max_len;

    uint16_t vid = 0;
    uint16_t pid = 0;

    if (!tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        return 0;
    }

    if (vid != NACON_VID || pid != NACON_PID) {
        return 0;
    }

    if (itf_desc->bInterfaceClass    != 0xFF ||
        itf_desc->bInterfaceSubClass != 0x5D ||
        itf_desc->bInterfaceProtocol != 0x01) {
        return 0;
    }

    nac_dev_addr = dev_addr;

    const uint8_t* p_desc = (const uint8_t*) itf_desc;
    uint8_t const* p_end = p_desc + itf_desc->bLength;

    while (p_desc[0] && p_desc < p_end) {
        p_desc += p_desc[0];
    }

    // Descriptor begins immediately after interface descriptor.
    p_desc = (const uint8_t*) itf_desc + itf_desc->bLength;

    for (uint8_t i = 0; i < itf_desc->bNumEndpoints; i++) {
        tusb_desc_endpoint_t const* ep =
            (tusb_desc_endpoint_t const*) p_desc;

        if (ep->bDescriptorType != TUSB_DESC_ENDPOINT) {
            return 0;
        }

        if (ep->bEndpointAddress == NACON_EP_IN) {
            nac_ep_in = NACON_EP_IN;
        } else if (ep->bEndpointAddress == NACON_EP_OUT) {
            nac_ep_out = NACON_EP_OUT;
        }

        p_desc += ep->bLength;
    }

    if (!nac_ep_in || !nac_ep_out) {
        nac_dev_addr = 0;
        return 0;
    }

    gpio_put(DBG_NACON_USB, 1);

    return 0xFF;
}

bool nacon_host_xfer_cb(uint8_t dev_addr,
                        uint8_t ep_addr,
                        xfer_result_t result,
                        uint32_t xferred_bytes)
{
    (void) dev_addr;
    (void) ep_addr;
    (void) result;
    (void) xferred_bytes;
    return true;
}

void nacon_host_close(uint8_t dev_addr)
{
    if (dev_addr == nac_dev_addr) {
        nac_dev_addr = 0;
        nac_ep_in = 0;
        nac_ep_out = 0;

        gpio_put(DBG_NACON_USB, 0);
    }
}

bool nacon_is_connected(void)
{
    return nac_dev_addr != 0;
}
