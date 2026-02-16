#include "LED_Brightness_CTRL.h"

// [중요] main.c에 있는 htim4 핸들을 가져다 쓰겠다고 선언
extern TIM_HandleTypeDef htim4;

/**
  * @brief  75MHz 클럭 기준 10kHz PWM 생성 및 밝기 조절
  * @param  brightness: 0 ~ 100 (%)
  */
void LED_Set_Brightness_10kHz(uint32_t brightness)
{
    // 75MHz Source -> 10kHz Target
    // PSC=74 (1MHz), ARR=100 (10kHz)

    uint32_t psc = 59;
    uint32_t arr = 100; // 100

    // 밝기 범위 제한 (Safety)
    if(brightness > 100) brightness = 100;

    __HAL_TIM_SET_PRESCALER(&htim4, psc);
    __HAL_TIM_SET_AUTORELOAD(&htim4, (arr - 1));// ARR = 99

//    // Duty 계산: (ARR-1) * brightness / 100
//    __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (arr - 1) * brightness / 100);

    /*STM32 타이머의 PWM 동작 원리는 **"비교(Compare)"**입니다.

CNT (현재 카운트) < CCR (설정값) 이면: High

CNT (현재 카운트) >= CCR (설정값) 이면: Low

따라서 CCR 값을 ARR(최대 카운트)보다 더 크게 설정하면, CNT가 CCR보다 커질 일이 없으므로 **영원히 High(DC)**가 나옵니다.*/
    if (brightness == 100)
        {
            // 1. DC High (계속 켜짐)
            // ARR(99)보다 큰 값을 넣으면 무조건 High가 유지됨
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, arr + 1);
        }
        else if (brightness == 0)
        {
            // 2. DC Low (계속 꺼짐)
            // 0을 넣으면 무조건 Low가 유지됨
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, 0);
        }
        else
        {
            // 3. PWM 동작 (1% ~ 99%)
            // 기존 계산식: (99) * brightness / 100
            __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, (arr - 1) * brightness / 100);
        }

    // 설정 적용
    htim4.Instance->EGR = TIM_EGR_UG;
}
