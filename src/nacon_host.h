#ifndef NACON_HOST_H
#define NACON_HOST_H

#include "tusb.h"

#ifdef __cplusplus
extern "C" {
#endif

bool nacon_host_init(void);

uint8_t nacon_host_open(uint8_t dev_addr,
                        tusb_desc_interface_t const* itf_desc,
                        uint16_t max_len);

bool nacon_host_xfer_cb(uint8_t dev_addr,
                        uint8_t ep_addr,
                        xfer_result_t result,
                        uint32_t xferred_bytes);

void nacon_host_close(uint8_t dev_addr);

bool nacon_is_connected(void);

#ifdef __cplusplus
}
#endif

#endif
