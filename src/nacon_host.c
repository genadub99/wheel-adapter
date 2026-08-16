#include "nacon_host.h"
#include "pico/stdlib.h"

#include <stdio.h>
#include <string.h>

#define NACON_VID 0x146B
#define NACON_PID 0x0603

#define NACON_EP_IN  0x81
#define NACON_EP_OUT 0x02
#define NACON_PACKET_SIZE 64

#define DBG_NACON_USB 2

static uint8_t nac_dev_addr = 0;
static uint8_t nac_itf_num = 0;
static uint8_t nac_ep_in = 0;
static uint8_t nac_ep_out = 0;

static uint8_t nac_rx_buf[NACON_PACKET_SIZE];

void nacon_host_init(void)
{
    nac_dev_addr = 0;
    nac_itf_num = 0;
    nac_ep_in = 0;
    nac_ep_out = 0;

    memset(nac_rx_buf, 0, sizeof(nac_rx_buf));
}

bool nacon_host_open(
    uint8_t rhport,
    uint8_t dev_addr,
    tusb_desc_interface_t const* itf_desc,
    uint16_t max_len
)
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

    printf(
        "NACON OPEN %04X:%04X IF=%u\n",
        vid,
        pid,
        itf_desc->bInterfaceNumber
    );

    uint8_t const* desc =
        (uint8_t const*) itf_desc + itf_desc->bLength;

    uint16_t remaining =
        (max_len > itf_desc->bLength)
            ? (uint16_t)(max_len - itf_desc->bLength)
            : 0;

    for (uint8_t i = 0; i < itf_desc->bNumEndpoints; i++)
    {
        if (remaining < sizeof(tusb_desc_endpoint_t)) {
            return false;
        }

        tusb_desc_endpoint_t const* ep =
            (tusb_desc_endpoint_t const*) desc;

        if (ep->bDescriptorType != TUSB_DESC_ENDPOINT) {
            return false;
        }

        if (ep->bEndpointAddress == NACON_EP_IN)
        {
            if (!tuh_edpt_open(dev_addr, ep)) {
                printf("NACON IN OPEN FAIL\n");
                return false;
            }

            nac_ep_in = NACON_EP_IN;

            printf(
                "NACON IN  EP=0x%02X MPS=%u\n",
                ep->bEndpointAddress,
                tu_edpt_packet_size(ep)
            );
        }
        else if (ep->bEndpointAddress == NACON_EP_OUT)
        {
            if (!tuh_edpt_open(dev_addr, ep)) {
                printf("NACON OUT OPEN FAIL\n");
                return false;
            }

            nac_ep_out = NACON_EP_OUT;

            printf(
                "NACON OUT EP=0x%02X MPS=%u\n",
                ep->bEndpointAddress,
                tu_edpt_packet_size(ep)
            );
        }

        desc += ep->bLength;
        remaining = (uint16_t)(remaining - ep->bLength);
    }

    if (nac_ep_in != NACON_EP_IN ||
        nac_ep_out != NACON_EP_OUT)
    {
        printf("NACON ENDPOINTS MISSING\n");
        return false;
    }

    nac_dev_addr = dev_addr;
    nac_itf_num = itf_desc->bInterfaceNumber;

    return true;
}

void nacon_host_set_config(
    uint8_t dev_addr,
    uint8_t itf_num
)
{
    if (dev_addr != nac_dev_addr ||
        itf_num != nac_itf_num) {
        return;
    }

    printf("NACON CONFIGURED\n");

    gpio_put(DBG_NACON_USB, 1);

    // Enumeration must be explicitly released to the next interface.
    usbh_driver_set_config_complete(
        dev_addr,
        itf_num
    );

    // Start listening to Nacon's interrupt IN endpoint.
    if (!usbh_edpt_xfer(
            dev_addr,
            nac_ep_in,
            nac_rx_buf,
            sizeof(nac_rx_buf)))
    {
        printf("NACON RX START FAIL\n");
    }
}

bool nacon_host_xfer_cb(
    uint8_t dev_addr,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
)
{
    if (dev_addr != nac_dev_addr) {
        return true;
    }

    if (ep_addr == nac_ep_in)
    {
        if (result == XFER_RESULT_SUCCESS)
        {
            printf(
                "NACON RX %lu bytes\n",
                (unsigned long)xferred_bytes
            );

            // Keep listening continuously.
            usbh_edpt_xfer(
                dev_addr,
                nac_ep_in,
                nac_rx_buf,
                sizeof(nac_rx_buf)
            );
        }
        else
        {
            printf(
                "NACON RX ERROR %d\n",
                result
            );
        }
    }

    return true;
}

void nacon_host_close(uint8_t dev_addr)
{
    if (dev_addr != nac_dev_addr) {
        return;
    }

    printf("NACON REMOVED\n");

    nac_dev_addr = 0;
    nac_itf_num = 0;
    nac_ep_in = 0;
    nac_ep_out = 0;

    gpio_put(DBG_NACON_USB, 0);
}
