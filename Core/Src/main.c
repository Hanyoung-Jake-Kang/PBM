/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *This code is meant to test the USB CDC connection!!
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include "LED_Brightness_CTRL.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
// 'volatile'은 "이 변수는 인터럽트에서 갑자기 바뀔 수 있으니 최적화하지 마!"라는 뜻입니다.
// 1. USB 통신용 변수
volatile uint8_t usb_data_received = 0; // 데이터 도착 알림 깃발
char rx_cmd_buffer[64];                 // [누락된 변수 1] 명령어 저장소

// 2. Heartbeat용 변수
uint32_t last_heartbeat_tick = 0;       // [누락된 변수 2] 마지막 심장박동 시간

// 3. Photic Stimulator (LED 제어) 구조체 정의
typedef struct {
    uint8_t is_running;       // 작동 중인가?
    uint32_t freq_hz;         // 주파수
    uint32_t start_tick;      // 시작 시간
    uint32_t duration_ms;     // 총 동작 시간
    uint32_t last_toggle_tick;// 마지막 깜빡임 시간
    uint32_t period_ms;       // 주기
    uint32_t on_time_ms;      // 켜짐 시간 (10ms)
    uint8_t led_state;        // LED 상태 (ON/OFF)
} PhoticStimulator_t;

PhoticStimulator_t photic = {0}; // 구조체 변수 생성
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM4_Init(void);
static void MX_TIM7_Init(void);
/* USER CODE BEGIN PFP */


void Run_Photic_Sequence(uint32_t freq_hz, uint32_t duration_sec);
void Update_Photic_Sequence();

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */


  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  MX_TIM7_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */
  // 1. 스톱워치용 타이머(TIM7) 켜기
  HAL_TIM_Base_Start(&htim7);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // LED 5V ON
  LED_Set_Brightness_10kHz(BRIGHTNESS_30); // 밝기 설정


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */

  while (1)
  {
//	  if (led_flag == 1)
//	        {
//	            // 1. PWM 시작! (불 켜기)
//	            HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
//
//	            // 2. "켰습니다!" 라고 PC에 답장 보내기 (선택사항)
//	            char msg[] = "OK! PWM ON for 1 sec\n\r";
//	            CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
//
//	            // 3. 1초 대기 (이제 여기서는 안전함)
//	            HAL_Delay(1000);
//
//	            // 4. PWM 정지! (불 끄기)
//	            HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
//
//	            // 5. 깃발 내리기 (중요! 안 내리면 무한 반복됨)
//	            led_flag = 0;
//	        }
//	  // [Task 1] USB 데이터 처리
//	      if (usb_data_received == 1) { // 깃발이 들려있으면?
//
//	          // 1. 할 일을 합니다 (예: 데이터 해석, 모드 변경)
//	          // Process_Heartbeat_Packet();
//
//	          // 2. 중요! 일을 다 했으니 깃발을 내립니다.
//	          usb_data_received = 0;
//	      }
//
//	      // [Task 2] LED 제어 (논블로킹 함수 호출)
//	      // 아까 만든 그 함수입니다. 계속 호출해줘야 시간이 되면 켜지고 꺼집니다.
//	      Update_Photic_Sequence();
	  uint32_t current_time = HAL_GetTick();

	        // ==========================================
	        // [Task 1] USB 명령어 처리 (깃발 확인)
	        // ==========================================
	        if (usb_data_received == 1)
	        {
	            // 1. "LED_ON" 명령어가 왔는지 확인 (6글자 비교)
	            if (strncmp(rx_cmd_buffer, "LED_ON", 6) == 0)
	            {
	                // 2. LED 시퀀스 시작! (예: 5Hz로 10초간)
	                // (나중에는 "LED_ON:5" 처럼 뒤에 숫자를 파싱해서 넣을 수도 있음)
	                Start_Photic_Sequence(5, 10);

	                // 3. "알겠다"고 응답 보냄
	                char ack[] = "CMD_OK: Started 5Hz\r\n";
	                CDC_Transmit_FS((uint8_t*)ack, strlen(ack));
	            }
	            else if (strncmp(rx_cmd_buffer, "LED_OFF", 7) == 0)
	            {
	                // 강제 종료 기능
	                photic.is_running = 0;
	                HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
	            }

	            // 4. 처리 끝났으니 깃발 내림
	            usb_data_received = 0;
	        }

	        // ==========================================
	        // [Task 2] Heartbeat 전송 (250ms 주기)
	        // ==========================================
	        if (current_time - last_heartbeat_tick >= 250)
	        {
	            Send_Heartbeat_Packet();      // 상태 보고
	            last_heartbeat_tick = current_time; // 시간 갱신
	        }

	        // ==========================================
	        // [Task 3] LED 제어 (논블로킹)
	        // ==========================================
	        // 얘는 멈추지 않고 계속 불러줘야 시간이 되면 알아서 켜고 끕니다.
	        Update_Photic_Sequence();


  }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 15;
  RCC_OscInitStruct.PLL.PLLN = 144;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 5;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 0;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 749;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */
  HAL_TIM_MspPostInit(&htim4);

}

/**
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 74;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 65535;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */
  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_CTRL_GPIO_Port, LED_CTRL_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_CTRL_Pin */
  GPIO_InitStruct.Pin = LED_CTRL_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_CTRL_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TRIG_IN_Pin */
  GPIO_InitStruct.Pin = TRIG_IN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TRIG_IN_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// [TIM7 기반 정밀 딜레이] 절대 안 멈춤
void Delay_us(uint16_t us)
{
    // 1. 카운터를 0으로 리셋
    __HAL_TIM_SET_COUNTER(&htim7, 0);

    // 2. 시간이 될 때까지 기다림 (TIM7은 1us마다 1씩 증가함)
    while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}

//void Run_Photic_Sequence(uint32_t freq_hz, uint32_t duration_sec)
//{
//    // 1. 0으로 나누기 방지 (Safety)
//    if (freq_hz == 0) return;
//
//    // 2. 주기 및 꺼짐 시간 계산 (단위: ms)
//    // 예: 5Hz -> period = 200ms, off_time = 190ms
//    uint32_t period_ms = 1000 / freq_hz;
//    uint32_t on_time_ms = 10; // 광자극 표준 (Flash Duration)
//
//    // 방어 코드: 주기가 10ms보다 짧으면 켜져만 있게 됨
//    if (period_ms <= on_time_ms) period_ms = on_time_ms + 1;
//
//    uint32_t off_time_ms = period_ms - on_time_ms;
//
//    // 3. 반복 횟수 계산
//    // 예: 5Hz로 10초 동안 하려면? -> 5 * 10 = 50번 깜빡여야 함
//    uint32_t total_counts = freq_hz * duration_sec;
//
//    // 4. 실행 루프
//    for (uint32_t i = 0; i < total_counts; i++)
//    {
//        // [ON] 10ms
//        HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
//        Delay_us(on_time_ms * 1000); // us 단위 함수 사용 시 *1000
//
//        // [OFF] 계산된 시간만큼
//        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
//        HAL_Delay(off_time_ms);
//    }
//}
// [기능 1] LED 시퀀스 시작 설정 (설정만 하고 바로 끝남)
void Start_Photic_Sequence(uint32_t freq_hz, uint32_t duration_sec) {

    // [수정됨] 1. 주파수 유효성 검사 (Safety Filter)
    // 허용된 주파수가 아니면 무조건 1Hz로 고정합니다.
    switch (freq_hz) {
        case 1: case 3: case 5: case 10:
        case 13: case 15: case 20: case 25: case 30:
            // 유효한 주파수임. 통과!
            break;
        default:
            // 목록에 없는 값이거나 0이 들어오면 기본값 1Hz로 변경
            freq_hz = 1;
            break;
    }

    // 2. 설정값 저장
    photic.freq_hz = freq_hz;
    photic.duration_ms = duration_sec * 1000;

    // 3. 주기 계산 (정수 나눗셈)
    // 예: 13Hz -> 1000 / 13 = 76ms (약간의 오차는 있지만 허용 범위)
    photic.period_ms = 1000 / freq_hz;
    photic.on_time_ms = 10; // 광자극 표준 10ms

    // 방어 코드: 주기가 너무 짧아서 꺼질 틈이 없는 경우 보정
    if (photic.period_ms <= photic.on_time_ms) {
        photic.period_ms = photic.on_time_ms + 1;
    }

    // 4. 타이밍 초기화 및 시작
    photic.start_tick = HAL_GetTick();       // 전체 동작 시작 시간
    photic.last_toggle_tick = HAL_GetTick(); // 깜빡임 기준 시간
    photic.is_running = 1;                   // 작동 플래그 ON

    // 5. 즉시 시작 (첫 번째 펄스)
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
    photic.led_state = 1; // 켜짐 상태로 시작
}

// [기능 2] LED 상태 업데이트 (while문에서 계속 호출됨)
void Update_Photic_Sequence() {
    // 1. 작동 중이 아니면 바로 리턴 (CPU 낭비 없음)
    if (photic.is_running == 0) return;

    uint32_t current_tick = HAL_GetTick();

    // 2. 전체 시간(10초) 종료 확인
    if (current_tick - photic.start_tick >= photic.duration_ms) {
        HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // 끄기
        photic.is_running = 0; // 종료
        photic.led_state = 0;
        return;
    }

    // 3. LED 켜고 끄기 타이밍 계산 (비동기 처리)
    if (photic.led_state == 1) {
        // [켜진 상태] -> 10ms 지났는지 확인
        if (current_tick - photic.last_toggle_tick >= photic.on_time_ms) {
            HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // 끄기
            photic.led_state = 0;
            // 주의: last_toggle_tick을 여기서 갱신하지 않음 (주기 유지를 위해)
        }
    }
    else {
        // [꺼진 상태] -> (주기 - 10ms) 지났는지 확인 (즉, 다음 주기가 되었는지)
        if (current_tick - photic.last_toggle_tick >= photic.period_ms) {
            HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4); // 켜기
            photic.led_state = 1;
            photic.last_toggle_tick = current_tick; // 주기 리셋
        }
    }
}

// [기능 3] Heartbeat 전송 (250ms마다 호출됨)
void Send_Heartbeat_Packet() {
    char msg[64];
    // 현재 상태(동작 중인지, 주파수는 몇인지)를 담아서 보냄
    sprintf(msg, "HB: Status=%s, Freq=%luHz\r\n",
            photic.is_running ? "RUN" : "IDLE",
            photic.freq_hz);

    // 전송 (바쁘면 무시하거나 재시도 로직 추가 가능)
    CDC_Transmit_FS((uint8_t*)msg, strlen(msg));
}


// [기능 4] USB 수신 핸들러 (인터럽트에서 호출)
void USB_CDC_RxHandler(uint8_t* Buf, uint32_t Len)
{
//    // 1. "LED_ON" 6글자가 맞는지 확인
//    if (Len >= 6 && strncmp((char*)Buf, "LED_ON", 6) == 0)
//    {
//        // 2. 맞으면 깃발을 1로 세팅하고 끝!
//    	usb_data_received = 1;
//    }
	// 데이터를 안전한 곳(rx_cmd_buffer)에 복사
	    memset(rx_cmd_buffer, 0, sizeof(rx_cmd_buffer));
	    if (Len < 64) memcpy(rx_cmd_buffer, Buf, Len);
	    else memcpy(rx_cmd_buffer, Buf, 63); // 버퍼 오버플로우 방지

	    usb_data_received = 1; // "메인아, 편지 왔다!" 깃발 들기

}

int _write(int file, char *ptr, int len) // printf 함수가 호출되면 _write 함수가 호출되고 _write 함수가 다시 __io_putchar 함수를 호출하는 구조
{
//	USART1 디버거로 보내는 코드
//    // 만약 USART2를 쓴다면 &huart2 로 변경하세요!
//    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 10);
//    return len;

//	USB CDC 로 송신하는 코드
	CDC_Transmit_FS((uint8_t*) ptr, len); return len;
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {

  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
