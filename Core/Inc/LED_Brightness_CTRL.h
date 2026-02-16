/*
 * LED_Brightness_CTRL.h
 *
 *  Created on: Feb 5, 2026
 *      Author: ybrain
 */

#ifndef LED_BRIGHTNESS_CTRL_H
#define LED_BRIGHTNESS_CTRL_H

#include "main.h" // HAL 라이브러리 타입(uint32_t 등)을 알기 위해 필수!

// [설정] 밝기 레벨 정의 (변수 대신 매크로 사용 권장)
#define BRIGHTNESS_10   10 // 안켜짐
#define BRIGHTNESS_20   20 // 9,678
#define BRIGHTNESS_30   30 // 14,800
#define BRIGHTNESS_40   40 // 19,800
#define BRIGHTNESS_50   50 // 24,600
#define BRIGHTNESS_60   60 // 29,200
#define BRIGHTNESS_70   70 // 33,600
#define BRIGHTNESS_80   80 // 38,000
#define BRIGHTNESS_90   90 // 42,000
#define BRIGHTNESS_100   99 // 42,000
#define BRIGHTNESS_DC_MAX  100 // 48,000


// [함수 선언] 외부에서 이 함수를 쓸 수 있게 명찰을 답니다.
void LED_Set_Brightness_10kHz(uint32_t brightness);

#endif /* LED_BRIGHTNESS_CTRL_H */
