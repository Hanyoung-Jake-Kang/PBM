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
 *First appropriate communication with pc software made by hanyoung kang
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
// 1. USB 통신용
volatile uint8_t usb_data_received = 0;
uint8_t rx_buffer[64]; // 수신 버퍼 (Raw Data)

// 2. Heartbeat & Safety
uint32_t last_hb_received_tick = 0; // 마지막으로 Host 연락 받은 시간

// 3. Photic Stimulator 구조체 (확장됨)
typedef struct {
	uint8_t is_running;       // 동작 중? (1:ON, 0:OFF)
	uint32_t freq_hz;         // 주파수
	uint32_t intensity;       // [NEW] 빛 세기 (1~10)
	uint32_t flash_duration;  // [NEW] 점등 시간 (ms)

	uint32_t start_tick;
	uint32_t duration_ms;
	uint32_t last_toggle_tick;
	uint32_t period_ms;
	uint8_t led_state;

	uint8_t error_code;       // [NEW] 에러 코드 (0:정상, 1:타임아웃 등)
	uint8_t ext_trig_state;   // [NEW] PA8 핀 상태
} PhoticStimulator_t;

PhoticStimulator_t photic = { 0 };

// 상수 정의 (프로토콜 규격)
#define SOP 0x1C
#define EOP 0x0D
#define CMD_HEARTBEAT 0x68
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
int main(void) {

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
	last_hb_received_tick = HAL_GetTick();
	while (1) {
		uint32_t current_time = HAL_GetTick();

		// [Task 1] USB 패킷 처리
		if (usb_data_received == 1) {
			Parse_Heartbeat_Packet(rx_buffer); // 해석하고 -> 세팅하고 -> 응답 보냄
			usb_data_received = 0; // 깃발 내리기
		}

		// [Task 2] Fail-safe (Heartbeat Timeout 감시)
		// 규격: 2초 이상 수신 없으면 정지
		if (current_time - last_hb_received_tick > 2000) {
			if (photic.error_code != 1) { // 에러가 아닐 때만 진입 (반복 실행 방지)
				photic.is_running = 0;       // 동작 정지
				HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4); // PWM 끄기
				photic.error_code = 1;       // 에러 1: Timeout 설정
			}
		}

		// [Task 3] LED 깜빡임 제어
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
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief TIM4 Initialization Function
 * @param None
 * @retval None
 */
static void MX_TIM4_Init(void) {

	/* USER CODE BEGIN TIM4_Init 0 */

	/* USER CODE END TIM4_Init 0 */

	TIM_ClockConfigTypeDef sClockSourceConfig = { 0 };
	TIM_MasterConfigTypeDef sMasterConfig = { 0 };
	TIM_OC_InitTypeDef sConfigOC = { 0 };

	/* USER CODE BEGIN TIM4_Init 1 */

	/* USER CODE END TIM4_Init 1 */
	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 0;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 749;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
	if (HAL_TIM_Base_Init(&htim4) != HAL_OK) {
		Error_Handler();
	}
	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK) {
		Error_Handler();
	}
	if (HAL_TIM_PWM_Init(&htim4) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig)
			!= HAL_OK) {
		Error_Handler();
	}
	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 0;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4)
			!= HAL_OK) {
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
static void MX_TIM7_Init(void) {

	/* USER CODE BEGIN TIM7_Init 0 */

	/* USER CODE END TIM7_Init 0 */

	TIM_MasterConfigTypeDef sMasterConfig = { 0 };

	/* USER CODE BEGIN TIM7_Init 1 */

	/* USER CODE END TIM7_Init 1 */
	htim7.Instance = TIM7;
	htim7.Init.Prescaler = 74;
	htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim7.Init.Period = 65535;
	htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
	if (HAL_TIM_Base_Init(&htim7) != HAL_OK) {
		Error_Handler();
	}
	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig)
			!= HAL_OK) {
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
static void MX_USART1_UART_Init(void) {

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
	if (HAL_UART_Init(&huart1) != HAL_OK) {
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
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
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

// [기능 1] Device -> Host 상태 응답 전송 (14 Bytes)
void Send_Status_Response() {
	uint8_t packet[14];

	// [0] SOP
	packet[0] = SOP;
	// [1] ID ('h')
	packet[1] = CMD_HEARTBEAT;
	// [2] Freq (+0x20 인코딩)
	packet[2] = photic.freq_hz + 0x20;
	// [3] Duration (고정 0x20)
	packet[3] = 0x20;
	// [4] Intensity (+0x20)
	packet[4] = photic.intensity + 0x20;
	// [5] Flash Duration (+0x20)
	packet[5] = photic.flash_duration + 0x20;

	// [6] Status Flags
	// Bit 0: Active, Bit 1: Standby, Bit 2: Force Stop, Bit 3: Error
	uint8_t status = 0;
	if (photic.is_running)
		status |= (1 << 0); // Active
	else
		status |= (1 << 1);                   // Standby

	if (photic.error_code != 0)
		status |= (1 << 3); // Error Present
	packet[6] = status;

	// [7] External Trigger (PA8 상태 읽기)
	// PA8이 High면 1, Low면 0
	photic.ext_trig_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);
	packet[7] = photic.ext_trig_state;

	// [8] Error Code
	packet[8] = photic.error_code;

	// [9] SW Version (예: 1.0 -> 0x10)
	packet[9] = 0x10;

	// [10~12] Spare
	packet[10] = 0;
	packet[11] = 0;
	packet[12] = 0;

	// [13] EOP
	packet[13] = EOP;

	// 전송!
	CDC_Transmit_FS(packet, 14);
}

// [기능 2] Host 패킷 해석 (Parsing) 및 제어
void Parse_Heartbeat_Packet(uint8_t *data) {
	// 1. 유효성 검사 (SOP, EOP 확인)
	if (data[0] != SOP || data[1] != CMD_HEARTBEAT || data[11] != EOP)
		return;

	// 2. Heartbeat 수신 인정 (타임아웃 리셋)
	last_hb_received_tick = HAL_GetTick();
	if (photic.error_code == 1)
		photic.error_code = 0; // 타임아웃 에러 해제
	// 3. 제어 플래그 확인
	uint8_t ctrl_flag = data[6];
	uint8_t force_stop = (ctrl_flag >> 1) & 0x01;
	uint8_t enable = (ctrl_flag >> 0) & 0x01;

	if (force_stop) {
		photic.is_running = 0;
		HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
	} else if (enable) {
		// [수정된 부분] 동작 설정 업데이트
		uint32_t new_freq = data[2] - 0x20;
		uint32_t new_intensity = data[4] - 0x20; // 1 ~ 10 값
		uint32_t new_flash = data[5] - 0x20;

		// 값 업데이트
		photic.freq_hz = new_freq;
		photic.flash_duration = new_flash;

		// ★★★ 여기서 실제 밝기 적용! ★★★
		// 값이 바뀌었을 때만 함수를 호출해서 CPU 낭비를 줄입니다.
		if (photic.intensity != new_intensity) {
			photic.intensity = new_intensity;

			// Protocol(1~10) -> PWM(10~100%) 변환
			// 예: 5 -> 50%
			LED_Set_Brightness_10kHz(photic.intensity * 10);
		}

		// 주기 재계산
		if (photic.freq_hz > 0)
			photic.period_ms = 1000 / photic.freq_hz;
		else
			photic.period_ms = 1000;

		// 시작 플래그가 꺼져 있었다면 시작
		if (photic.is_running == 0) {
			photic.is_running = 1;
			photic.start_tick = HAL_GetTick();
			photic.last_toggle_tick = HAL_GetTick();

			// 시작할 때도 현재 밝기로 확실하게 세팅하고 켬
			LED_Set_Brightness_10kHz(photic.intensity * 10);
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
			photic.led_state = 1;
		}
	} else {
		// 일반 정지
		photic.is_running = 0;
		HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
	}

	// 4. 응답 전송 (그대로)
	Send_Status_Response();

}

// [기능 3] LED 제어 (Non-blocking)
void Update_Photic_Sequence() {
	if (photic.is_running == 0)
		return;

	uint32_t current_tick = HAL_GetTick();

	// 깜빡임 로직
	if (photic.led_state == 1) { // 켜짐 상태
		// 설정된 Flash Duration 만큼 켜짐
		if (current_tick - photic.last_toggle_tick >= photic.flash_duration) {
			HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_4);
			photic.led_state = 0;
			// last_toggle_tick은 갱신하지 않음 (주기 유지를 위해)
		}
	} else { // 꺼짐 상태
		if (current_tick - photic.last_toggle_tick >= photic.period_ms) {
			HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
			photic.led_state = 1;
			photic.last_toggle_tick = current_tick;
		}
	}
}

// [기능 4] USB 수신 핸들러
void USB_CDC_RxHandler(uint8_t *Buf, uint32_t Len) {
	// 12바이트 패킷 복사
	if (Len >= 12) {
		memcpy(rx_buffer, Buf, 12);
		usb_data_received = 1; // 깃발 들기
	}
}

int _write(int file, char *ptr, int len) // printf 함수가 호출되면 _write 함수가 호출되고 _write 함수가 다시 __io_putchar 함수를 호출하는 구조
{
//	USART1 디버거로 보내는 코드
//    // 만약 USART2를 쓴다면 &huart2 로 변경하세요!
//    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 10);
//    return len;

//	USB CDC 로 송신하는 코드
	CDC_Transmit_FS((uint8_t*) ptr, len);
	return len;
}

// [TIM7 기반 정밀 딜레이] 절대 안 멈춤
void Delay_us(uint16_t us) {
	// 1. 카운터를 0으로 리셋
	__HAL_TIM_SET_COUNTER(&htim7, 0);

	// 2. 시간이 될 때까지 기다림 (TIM7은 1us마다 1씩 증가함)
	while (__HAL_TIM_GET_COUNTER(&htim7) < us)
		;
}

//
//void Start_Photic_Sequence(uint32_t freq_hz, uint32_t duration_sec) {
//
//    // [수정됨] 1. 주파수 유효성 검사 (Safety Filter)
//    // 허용된 주파수가 아니면 무조건 1Hz로 고정합니다.
//    switch (freq_hz) {
//        case 1: case 3: case 5: case 10:
//        case 13: case 15: case 20: case 25: case 30:
//            // 유효한 주파수임. 통과!
//            break;
//        default:
//            // 목록에 없는 값이거나 0이 들어오면 기본값 1Hz로 변경
//            freq_hz = 1;
//            break;
//    }
//
//    // 2. 설정값 저장
//    photic.freq_hz = freq_hz;
//    photic.duration_ms = duration_sec * 1000;
//
//    // 3. 주기 계산 (정수 나눗셈)
//    // 예: 13Hz -> 1000 / 13 = 76ms (약간의 오차는 있지만 허용 범위)
//    photic.period_ms = 1000 / freq_hz;
//    photic.on_time_ms = 10; // 광자극 표준 10ms
//
//    // 방어 코드: 주기가 너무 짧아서 꺼질 틈이 없는 경우 보정
//    if (photic.period_ms <= photic.on_time_ms) {
//        photic.period_ms = photic.on_time_ms + 1;
//    }
//
//    // 4. 타이밍 초기화 및 시작
//    photic.start_tick = HAL_GetTick();       // 전체 동작 시작 시간
//    photic.last_toggle_tick = HAL_GetTick(); // 깜빡임 기준 시간
//    photic.is_running = 1;                   // 작동 플래그 ON
//
//    // 5. 즉시 시작 (첫 번째 펄스)
//    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
//    photic.led_state = 1; // 켜짐 상태로 시작
//}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {

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
