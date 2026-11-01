#include "rise_emotion.h"
#include "bsp_ws2812.h"
#include "bsp_vibrator.h"
#include "qm_log.h"
#include "qm_kernel.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <math.h>

#define LOG_TAG "rise_emo"

static rise_emotion_t s_current_emo = RISE_EMO_NONE;
static uint32_t s_tick_count = 0;
static TaskHandle_t s_task_handle = NULL;

rise_emotion_t rise_emotion_parse_tag(const char *text, const char **out_clean_text)
{
    if (text == NULL) {
        if (out_clean_text) *out_clean_text = NULL;
        return RISE_EMO_NONE;
    }

    while (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n') {
        text++;
    }

    rise_emotion_t detected = RISE_EMO_NONE;
    const char *tag_end = NULL;

    // 1. 标准英文字符标签
    if (strncmp(text, "[EMO:SAD]", 9) == 0) {
        detected = RISE_EMO_SAD;
        tag_end = text + 9;
    } else if (strncmp(text, "[EMO:JOY]", 9) == 0) {
        detected = RISE_EMO_JOY;
        tag_end = text + 9;
    } else if (strncmp(text, "[EMO:ANX]", 9) == 0) {
        detected = RISE_EMO_ANX;
        tag_end = text + 9;
    } else if (strncmp(text, "[EMO:TIRED]", 11) == 0) {
        detected = RISE_EMO_TIRED;
        tag_end = text + 11;
    } else if (strncmp(text, "[EMO:NORM]", 10) == 0 || strncmp(text, "[EMO:BORED]", 11) == 0) {
        detected = RISE_EMO_NORM;
        tag_end = (text[8] == 'M') ? text + 10 : text + 11;
    } else if (strncmp(text, "[EMO:WAKE]", 10) == 0) {
        detected = RISE_EMO_WAKE;
        tag_end = text + 10;
    }
    // 2. 中文与常见括号变体支持
    else if (strncmp(text, "[委屈]", 8) == 0 || strncmp(text, "[难过]", 8) == 0 ||
             strncmp(text, "【委屈】", 10) == 0 || strncmp(text, "【难过】", 10) == 0 ||
             strncmp(text, "[EMO:委屈]", 12) == 0 || strncmp(text, "[EMO:难过]", 12) == 0) {
        detected = RISE_EMO_SAD;
        const char *p = strchr(text, ']');
        if (!p) p = strstr(text, "】");
        tag_end = p ? (p + (p[0] == ']' ? 1 : 3)) : text + 8;
    } else if (strncmp(text, "[狂喜]", 8) == 0 || strncmp(text, "[成就]", 8) == 0 || strncmp(text, "[开心]", 8) == 0 ||
               strncmp(text, "【狂喜】", 10) == 0 || strncmp(text, "【成就】", 10) == 0 || strncmp(text, "【开心】", 10) == 0 ||
               strncmp(text, "[EMO:狂喜]", 12) == 0 || strncmp(text, "[EMO:开心]", 12) == 0) {
        detected = RISE_EMO_JOY;
        const char *p = strchr(text, ']');
        if (!p) p = strstr(text, "】");
        tag_end = p ? (p + (p[0] == ']' ? 1 : 3)) : text + 8;
    } else if (strncmp(text, "[焦虑]", 8) == 0 || strncmp(text, "[烦躁]", 8) == 0 ||
               strncmp(text, "【焦虑】", 10) == 0 || strncmp(text, "【烦躁】", 10) == 0 ||
               strncmp(text, "[EMO:焦虑]", 12) == 0 || strncmp(text, "[EMO:烦躁]", 12) == 0) {
        detected = RISE_EMO_ANX;
        const char *p = strchr(text, ']');
        if (!p) p = strstr(text, "】");
        tag_end = p ? (p + (p[0] == ']' ? 1 : 3)) : text + 8;
    } else if (strncmp(text, "[疲惫]", 8) == 0 || strncmp(text, "[放空]", 8) == 0 || strncmp(text, "[累]", 5) == 0 ||
               strncmp(text, "【疲惫】", 10) == 0 || strncmp(text, "【放空】", 10) == 0 ||
               strncmp(text, "[EMO:疲惫]", 12) == 0 || strncmp(text, "[EMO:放空]", 12) == 0) {
        detected = RISE_EMO_TIRED;
        const char *p = strchr(text, ']');
        if (!p) p = strstr(text, "】");
        tag_end = p ? (p + (p[0] == ']' ? 1 : 3)) : text + 8;
    } else if (strncmp(text, "[无聊]", 8) == 0 || strncmp(text, "[日常]", 8) == 0 || strncmp(text, "[平淡]", 8) == 0 ||
               strncmp(text, "【无聊】", 10) == 0 || strncmp(text, "【日常】", 10) == 0 ||
               strncmp(text, "[EMO:无聊]", 12) == 0 || strncmp(text, "[EMO:日常]", 12) == 0) {
        detected = RISE_EMO_NORM;
        const char *p = strchr(text, ']');
        if (!p) p = strstr(text, "】");
        tag_end = p ? (p + (p[0] == ']' ? 1 : 3)) : text + 8;
    }

    if (tag_end != NULL) {
        while (*tag_end == ' ' || *tag_end == '\t') {
            tag_end++;
        }
        if (out_clean_text) *out_clean_text = tag_end;
        return detected;
    }

    if (out_clean_text) *out_clean_text = text;
    return RISE_EMO_NONE;
}

rise_emotion_t rise_emotion_match_keywords(const char *text)
{
    if (text == NULL || text[0] == '\0') {
        return RISE_EMO_NONE;
    }

    // 委屈或难过
    if (strstr(text, "被老板骂") != NULL || strstr(text, "被骂") != NULL ||
        strstr(text, "委屈") != NULL || strstr(text, "难过") != NULL ||
        strstr(text, "伤心") != NULL || strstr(text, "想哭") != NULL ||
        strstr(text, "哭了") != NULL || strstr(text, "批评") != NULL) {
        return RISE_EMO_SAD;
    }

    // 开心或取得成果
    if (strstr(text, "Offer") != NULL || strstr(text, "offer") != NULL ||
        strstr(text, "拿到") != NULL || strstr(text, "狂喜") != NULL ||
        strstr(text, "开心") != NULL || strstr(text, "太棒了") != NULL ||
        strstr(text, "庆祝") != NULL || strstr(text, "成功") != NULL ||
        strstr(text, "中奖") != NULL || strstr(text, "考上") != NULL) {
        return RISE_EMO_JOY;
    }

    // 焦虑或烦躁
    if (strstr(text, "报错") != NULL || strstr(text, "bug") != NULL ||
        strstr(text, "Bug") != NULL || strstr(text, "烦躁") != NULL ||
        strstr(text, "焦虑") != NULL || strstr(text, "好烦") != NULL ||
        strstr(text, "烦死") != NULL || strstr(text, "抓狂") != NULL ||
        strstr(text, "头大") != NULL || strstr(text, "崩溃") != NULL) {
        return RISE_EMO_ANX;
    }

    // 疲惫或需要休息
    if (strstr(text, "好累") != NULL || strstr(text, "太累") != NULL ||
        strstr(text, "不想动") != NULL || strstr(text, "疲惫") != NULL ||
        strstr(text, "放空") != NULL || strstr(text, "发呆") != NULL ||
        strstr(text, "累死") != NULL || strstr(text, "好困") != NULL ||
        strstr(text, "歇会") != NULL || strstr(text, "瘫") != NULL) {
        return RISE_EMO_TIRED;
    }

    // 平静或日常状态
    if (strstr(text, "什么都没干") != NULL || strstr(text, "无聊") != NULL ||
        strstr(text, "日常") != NULL || strstr(text, "平淡") != NULL ||
        strstr(text, "平常") != NULL || strstr(text, "没事干") != NULL ||
        strstr(text, "无所事事") != NULL) {
        return RISE_EMO_NORM;
    }

    return RISE_EMO_NONE;
}

void rise_emotion_set(rise_emotion_t emo)
{
    if (s_current_emo != emo) {
        QM_LOGI(LOG_TAG, "switch emotion: %d -> %d", s_current_emo, emo);
        s_current_emo = emo;
        s_tick_count = 0;
    }
}

rise_emotion_t rise_emotion_get(void)
{
    return s_current_emo;
}

void rise_emotion_trigger_wake(void)
{
    QM_LOGI(LOG_TAG, "trigger wake animation & haptic feedback");
    rise_emotion_set(RISE_EMO_WAKE);
    bsp_vibrator_pulse(120);
}

static void emotion_worker_task(void *arg)
{
    (void)arg;
    QM_LOGI(LOG_TAG, "emotion feedback task started");

    while (1) {
        s_tick_count++;
        uint32_t t = s_tick_count;

        switch (s_current_emo) {
            case RISE_EMO_WAKE: {
                // White fade-out, then return to the normal state.
                if (t <= 6) {
                    uint8_t bright = 200 - t * 25;
                    bsp_ws2812_fill(bright, bright, bright);
                } else {
                    rise_emotion_set(RISE_EMO_NORM);
                }
                break;
            }

            case RISE_EMO_SAD: {
                // Blue breathing effect with one short pulse per cycle.
                float wave = (sinf((float)t * 0.08f) + 1.0f) * 0.5f;
                uint8_t b = (uint8_t)(90 + wave * 140);
                uint8_t g = (uint8_t)(15 + wave * 35);
                bsp_ws2812_fill(0, g, b);

                if (t % 40 == 10) {
                    bsp_vibrator_pulse(100);
                }
                break;
            }

            case RISE_EMO_JOY: {
                // Pink and orange rotating trail with a two-pulse pattern.
                uint8_t head = (t * 2) % 8;
                for (int i = 0; i < 8; i++) {
                    int dist = (head - i + 8) % 8;
                    if (dist == 0) {
                        bsp_ws2812_set_pixel(i, 255, 60, 140);
                    } else if (dist == 1) {
                        bsp_ws2812_set_pixel(i, 255, 140, 30);
                    } else if (dist == 2) {
                        bsp_ws2812_set_pixel(i, 180, 80, 10);
                    } else {
                        bsp_ws2812_set_pixel(i, 20, 5, 20);
                    }
                }
                bsp_ws2812_refresh();

                if (t % 25 == 2 || t % 25 == 5) {
                    bsp_vibrator_pulse(60);
                }
                break;
            }

            case RISE_EMO_ANX: {
                // Red warning flash with a repeated two-pulse pattern.
                if ((t / 2) % 2 == 0) {
                    bsp_ws2812_fill(255, 0, 0);
                } else {
                    bsp_ws2812_fill(30, 0, 0);
                }

                if (t % 14 == 1 || t % 14 == 4) {
                    bsp_vibrator_pulse(65);
                }
                break;
            }

            case RISE_EMO_TIRED: {
                // Low-brightness warm breathing effect without vibration.
                float breath = (sinf((float)t * 0.04f) + 1.0f) * 0.5f;
                uint8_t r = (uint8_t)(25 + breath * 35);
                uint8_t g = (uint8_t)(15 + breath * 20);
                bsp_ws2812_fill(r, g, 4);
                break;
            }

            case RISE_EMO_POSTURE_BAD: {
                // Red and white warning flash with three short pulses.
                if ((t / 2) % 2 == 0) {
                    bsp_ws2812_fill(255, 0, 0);
                } else {
                    bsp_ws2812_fill(255, 255, 255);
                }

                if (t % 18 == 1 || t % 18 == 5 || t % 18 == 9) {
                    bsp_vibrator_pulse(120);
                }
                break;
            }

            case RISE_EMO_POSTURE_GOOD: {
                // Green-to-orange confirmation effect with one short pulse.
                if (t < 15) {
                    bsp_ws2812_fill(30, 220, 60);
                    if (t == 1) {
                        bsp_vibrator_pulse(80);
                    }
                } else if (t < 30) {
                    bsp_ws2812_fill(255, 160, 50);
                } else {
                    rise_emotion_set(RISE_EMO_NORM);
                }
                break;
            }

            case RISE_EMO_NORM:
            default: {
                // White breathing effect without vibration.
                float pulse = (sinf((float)t * 0.05f) + 1.0f) * 0.5f;
                uint8_t lum = (uint8_t)(20 + pulse * 60);
                bsp_ws2812_fill(lum, lum, lum);
                break;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(40)); // 25 FPS update rate.
    }
}

qm_err_t rise_emotion_init(void)
{
    // Initialize the eight-pixel LED ring and vibration motor.
    bsp_ws2812_init();
    bsp_vibrator_init();

    // Start in the normal breathing state.
    rise_emotion_set(RISE_EMO_NORM);

    BaseType_t ret = xTaskCreatePinnedToCore(
        emotion_worker_task,
        "rise_emo_task",
        3072,
        NULL,
        3,
        &s_task_handle,
        0
    );

    if (ret != pdPASS) {
        QM_LOGE(LOG_TAG, "failed to create emotion worker task");
        return -QM_ENOMEM;
    }

    QM_LOGI(LOG_TAG, "solid-state emotion engine inited ok");
    return QM_EOK;
}
