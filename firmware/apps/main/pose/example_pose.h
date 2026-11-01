#ifndef EXAMPLE_POSE_H
#define EXAMPLE_POSE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void example_pose_run(void);
void example_pose_run_camera(const uint8_t *uyvy, size_t uyvy_len,
                             uint16_t src_w, uint16_t src_h);

#ifdef __cplusplus
}
#endif

#endif
