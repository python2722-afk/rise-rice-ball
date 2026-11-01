#ifndef RISE_EMOTION_H
#define RISE_EMOTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "qm.h"
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief 大米饭团 (Rise) 灯效与震动反馈状态
 */
typedef enum {
    RISE_EMO_NONE = 0,
    RISE_EMO_SAD,           // 蓝色慢呼吸灯效，周期性短震
    RISE_EMO_JOY,           // 粉橙色旋转灯效，双短震
    RISE_EMO_ANX,           // 红色快速闪烁，双短震
    RISE_EMO_TIRED,         // 低亮暖色慢呼吸，无震动
    RISE_EMO_NORM,          // 白色慢呼吸，无震动
    RISE_EMO_WAKE,          // 高亮渐隐，120 ms 短震
    RISE_EMO_POSTURE_BAD,   // 红白闪烁，三次短震
    RISE_EMO_POSTURE_GOOD   // 绿色转橙色，单次短震
} rise_emotion_t;

/**
 * @brief 初始化灯效与震动反馈任务
 */
qm_err_t rise_emotion_init(void);

/**
 * @brief 设置当前情绪/行为状态
 */
void rise_emotion_set(rise_emotion_t emo);

/**
 * @brief 获取当前情绪状态
 */
rise_emotion_t rise_emotion_get(void);

/**
 * @brief 触发唤醒灯效与短震反馈
 */
void rise_emotion_trigger_wake(void);

/**
 * @brief 解析大模型文本开头的 [EMO:xxx] 协议标签（支持英中文标签）
 * @param text 输入文本流
 * @param out_clean_text 输出剥离标签后的干净文本起始指针（供TTS朗读）
 * @return 检测到的情绪枚举；若无标签则返回 RISE_EMO_NONE
 */
rise_emotion_t rise_emotion_parse_tag(const char *text, const char **out_clean_text);

/**
 * @brief 从用户输入语句(ASR)或无标签回复文本中提取情绪语义关键词（兜底触发）
 * @param text 输入文本
 * @return 匹配到的情绪枚举；若未命中则返回 RISE_EMO_NONE
 */
rise_emotion_t rise_emotion_match_keywords(const char *text);

#ifdef __cplusplus
}
#endif

#endif // RISE_EMOTION_H
