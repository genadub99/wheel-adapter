#ifndef NACON_HOST_H
#define NACON_HOST_H

#include "host/usbh_pvt.h"

#ifdef __cplusplus
extern "C" {
#endif

void nacon_host_init(void);

bool nacon_host_open(
    uint8_t rhport,
    uint8_t dev_addr,
    tusb_desc_interface_t const* itf_desc,
    uint16_t max_len
);

void nacon_host_set_config(
    uint8_t dev_addr,
    uint8_t itf_num
);

bool nacon_host_xfer_cb(
    uint8_t dev_addr,
    uint8_t ep_addr,
    xfer_result_t result,
    uint32_t xferred_bytes
);

void nacon_host_close(uint8_t dev_addr);

#ifdef __cplusplus
}
#endif

#endif
