#include "nacon_host.h"
#include "host/usbh_pvt.h"
#include "pico/stdlib.h"

#include <string.h>

#define NACON_VID 0x146B
#define NACON_PID 0x0603

#define NACON_EP_IN  0x81
#define NACON_EP_OUT 0x02

#define NACON_MPS 64

// GP2 = Nacon mounted
#define DBG_NACON_USB 2

static uint8_t nac_dev_addr = 0;
static uint8_t nac_itf_num = 0;
static uint8_t nac_ep_in = 0;
static uint8_t nac_ep_out = 0;

static uint8_t nac_rx_buf[NACON_MPS];

bool nacon_host_init(void)
{
    nac_dev_addr = 0;
    nac_itf_num = 0;
    nac_ep_in = 0;
    nac_ep_out = 0;

    memset(nac_rx_buf, 0, sizeof(nac_rx_buf));

    return true;
}

bool nacon_host_open(uint8_t rhport,
                     uint8_t dev_addr,
                     tusb_desc_interface_t const* itf_desc,
                     uint16_t max_len)
{
    (void) rhport;

    uint16_t vid = 0;
    uint16_t pid = 0;

    if (!tuh_vid_pid_get(dev_addr, &vid, &pid)) {
        return false;
    }

    if (vid != NACON_VID || pid != NACON_PID) {
        return false;
    }

    if (itf_desc->bInterfaceClass != 0xFF ||
        itf_desc->bInterfaceSubClass != 0x5D ||
        itf_desc->bInterfaceProtocol != 0x01) {
        return false;
    }

    printf("NACON DRIVER OPEN: %04X:%04X\n", vid, pid);

    uint8_t const* desc =
        (uint8_t const*) itf_desc + itf_desc->bLength;

    uint16_t remaining =
        max_len > itf_desc->bLength ?
        (uint16_t)(max_len - itf_desc->bLength) : 0;

    while (remaining >= 2) {
        uint8_t len = desc[0];
        uint8_t type = desc[1];

        if (len < 2 || len > remaining) {
            break;
        }

        if (type == TUSB_DESC_ENDPOINT) {
            tusb_desc_endpoint_t const* ep =
                (tusb_desc_endpoint_t const*) desc;

            if (ep->bEndpointAddress == NACON_EP_IN) {
                if (!tuh_edpt_open(dev_addr, ep)) {
                    return false;
                }

                nac_ep_in = NACON_EP_IN;

                printf("NACON IN opened: 0x%02X\n", NACON_EP_IN);
            }
            else if (ep->bEndpointAddress == NACON_EP_OUT) {
                if (!tuh_edpt_open(dev_addr, ep)) {
                    return false;
                }

                nac_ep_out = NACON_EP_OUT;

                printf("NACON OUT opened: 0x%02X\n", NACON_EP_OUT);
            }
        }

        desc += len;
        remaining = (uint16_t)(remaining - len);
    }

    if (!nac_ep_in || !nac_ep_out) {
        printf("NACON: endpoints missing\n");
        return false;
    }

    nac_dev_addr = dev_addr;
    nac_itf_num = itf_desc->bInterfaceNumber;

    return true;
}

bool nacon_host_set_config(uint8_t dev_addr, uint8_t itf_num)
{
    if (dev_addr != nac_dev_addr || itf_num != nac_itf_num) {
        return true;
    }

    printf("NACON CONFIGURED\n");

    gpio_put(DBG_NACON_USB, 1);

    // Tell TinyUSB that our interface configuration is complete.
    usbh_driver_set_config_complete(dev_addr, itf_num);

    // Start receiving packets from Nacon.
    if (nac_ep_in) {
        if (!usbh_edpt_xfer(dev_addr,
                            nac_ep_in,
                            nac_rx_buf,
                            sizeof(nac_rx_buf))) {
            printf("NACON RX start failed\n");
        }
    }

    return true;
}

bool nacon_host_xfer_cb(uint8_t dev_addr,
                        uint8_t ep_addr,
                        xfer_result_t result,
                        uint32_t xferred_bytes)
{
    if (dev_addr != nac_dev_addr) {
        return true;
    }

    if (ep_addr == nac_ep_in &&
        result == XFER_RESULT_SUCCESS) {

        printf("NACON RX: %lu bytes\n",
               (unsigned long)xferred_bytes);

        if (nac_ep_in) {
            usbh_edpt_xfer(dev_addr,
                           nac_ep_in,
                           nac_rx_buf,
                           sizeof(nac_rx_buf));
        }
    }

    return true;
}

void nacon_host_close(uint8_t dev_addr)
{
    if (dev_addr == nac_dev_addr) {
        nac_dev_addr = 0;
        nac_itf_num = 0;
        nac_ep_in = 0;
        nac_ep_out = 0;

        gpio_put(DBG_NACON_USB, 0);

        printf("NACON REMOVED\n");
    }
}

bool nacon_is_connected(void)
{
    return nac_dev_addr != 0;
}
