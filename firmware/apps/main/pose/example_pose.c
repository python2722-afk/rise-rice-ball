#include "example_pose.h"

#include "qm.h"
#include "yolo11_pose.h"
#include "rise_emotion.h"
#include "agent_app.h"

#define LOG_TAG "example_pose"

extern const uint8_t pose_bus_jpg_start[] asm("_binary_bus_jpg_start");
extern const uint8_t pose_bus_jpg_end[] asm("_binary_bus_jpg_end");

static uint32_t s_person_present_count = 0;
static uint32_t s_no_person_count = 0;

void example_pose_run(void)
{
    yolo11_pose_frame_result_t result;
    size_t len = (size_t)(pose_bus_jpg_end - pose_bus_jpg_start);
    esp_err_t ret;

    /* 先读模型。读 Flash 会关 cache，不要和摄像头一类外设同时进行。 */
    if (yolo11_pose_init() != ESP_OK) {
        QM_LOGE(LOG_TAG, "yolo11_pose_init failed");
        return;
    }

    ret = yolo11_pose_analyze_jpg_named(pose_bus_jpg_start, len, "bus.jpg", &result);
    if (ret != ESP_OK) {
        QM_LOGE(LOG_TAG, "pose bus jpg failed");
        return;
    }
    QM_LOGI(LOG_TAG, "pose test status=%d err=%d", (int)result.status, (int)result.err_type);
}

/* Map person-presence results to normal, tired, or idle feedback states. */
void example_pose_run_camera(const uint8_t *uyvy, size_t uyvy_len,
                             uint16_t src_w, uint16_t src_h)
{
    uint8_t *rgb888 = NULL;
    uint16_t w = 0;
    uint16_t h = 0;
    yolo11_pose_frame_result_t result;
    esp_err_t ret;

    if (uyvy == NULL || uyvy_len == 0) {
        return;
    }

    ret = yolo11_pose_uyvy_to_rgb888_cw90(uyvy, uyvy_len, src_w, src_h,
                                          &rgb888, &w, &h);
    if (ret != ESP_OK) {
        QM_LOGE(LOG_TAG, "uyvy to rgb888 failed");
        return;
    }

    ret = yolo11_pose_analyze_rgb888(rgb888, w, h, "camera", &result);
    if (ret != ESP_OK) {
        QM_LOGE(LOG_TAG, "pose camera failed");
        return;
    }

    // 仅检测是否有人存在 (检测到人体即可，不再进行坐姿不良判定)
    bool person_present = (result.status != YOLO11_POSE_FRAME_NO_PERSON);
    QM_LOGI(LOG_TAG, "presence detection: %s (status=%d)",
            person_present ? "PERSON_PRESENT" : "NO_PERSON", (int)result.status);

    rise_emotion_t cur = rise_emotion_get();
    bool in_voice_dialogue = (cur == RISE_EMO_SAD || cur == RISE_EMO_JOY ||
                              cur == RISE_EMO_ANX || cur == RISE_EMO_WAKE);

    if (person_present) {
        s_person_present_count++;
        s_no_person_count = 0;

        if (!in_voice_dialogue) {
            if (cur == RISE_EMO_NONE) {
                QM_LOGI(LOG_TAG, "person present -> enter RISE_EMO_NORM");
                rise_emotion_set(RISE_EMO_NORM);
            } else if (cur == RISE_EMO_NORM && s_person_present_count >= 15) {
                QM_LOGI(LOG_TAG, "long calm presence -> gentle rest RISE_EMO_TIRED");
                rise_emotion_set(RISE_EMO_TIRED);
            }
        }
    } else {
        s_no_person_count++;
        s_person_present_count = 0;

        if (s_no_person_count >= 5 && !in_voice_dialogue) {
            if (cur == RISE_EMO_TIRED) {
                rise_emotion_set(RISE_EMO_NORM);
            }
        }
    }
}
