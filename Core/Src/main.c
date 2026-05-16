/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx_hal.h"
#include "lvgl.h"
#include "ili9341.h"
#include "fonts.h"
#include "ili9341_touch.h"
#include <stdio.h>
#include <string.h>

#include "screens/screen_home.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

#define XPSR_INITIAL 0x01000000
#define STACK_SIZE 256  // Erhöht auf 1024 Bytes für printf-Sicherheit

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);

extern void Start_OS(void); 

TCB_t tasks[4];
TCB_t *currentTCB = NULL; 
volatile uint8_t lvgl_is_initialized = 0;

/* Stacks mit 8-Byte Alignment */
uint32_t stackA[STACK_SIZE] __attribute__((aligned(8)));
uint32_t stackB[STACK_SIZE] __attribute__((aligned(8)));
static uint32_t stackIdle[128] __attribute__((aligned(8)));
static uint32_t stackDisplay[1024] __attribute__((aligned(8)));

/* --- LVGL Display Variablen --- */
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[320 * 10]; // Puffer für 10 Zeilen des Displays (ca. 6.4 KB RAM)


/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

void OS_Delay(uint32_t ticks)
{
  __disable_irq();
  currentTCB->sleep_ticks = ticks;
  __enable_irq();

  SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

void my_flush_cb(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_p) {
    // 1. Berechne Breite und Höhe des Bereichs
    uint16_t width = (area->x2 - area->x1 + 1);
    uint16_t height = (area->y2 - area->y1 + 1);

    // 2. Nutze die vorhandene Treiber-Funktion
    // Diese setzt das Fenster UND schickt die Daten via SPI
    ILI9341_DrawImage(area->x1, area->y1, width, height, (const uint16_t *)color_p);

    // 3. LVGL mitteilen, dass das Zeichnen fertig ist
    lv_disp_flush_ready(disp_drv);
}

static void SPI1_SetSpeed(uint32_t prescaler)
{
    /* SPI kurz deaktivieren, Prescaler ändern, wieder aktivieren */
    __HAL_SPI_DISABLE(&hspi1);
    hspi1.Instance->CR1 = (hspi1.Instance->CR1 & ~SPI_CR1_BR_Msk) | prescaler;
    __HAL_SPI_ENABLE(&hspi1);
}

void touch_lvgl_read(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    uint16_t x, y;

    /* Auf ~650 Kbit/s runterschalten (Prescaler 128 → 84MHz/128 = 656 Kbit/s) */
    SPI1_SetSpeed(SPI_CR1_BR_2 | SPI_CR1_BR_0); /* 0b111 = /256 → ~328 Kbit */

    if (ILI9341_TouchGetCoordinates(&x, &y)) {
        data->point.x = x;
        data->point.y = y;
        data->state   = LV_INDEV_STATE_PRESSED;

    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }

    /* Zurück auf Display-Geschwindigkeit (Prescaler 4 → 21 Mbit/s) */
    SPI1_SetSpeed(SPI_CR1_BR_0); /* 0b001 = /4 */
}

void TaskA(void);
void TaskB(void);
void Task_Idle(void) {
  while(1){
    __asm("WFI"); //go to power save mode
  }
}
void Task_Display(void){
  
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 320 * 10);

  // 3. Display-Treiber konfigurieren
    static lv_disp_drv_t disp_drv;          /* Statisch, da LVGL den Zeiger behält */
    lv_disp_drv_init(&disp_drv);            /* Basis-Werte setzen */
    disp_drv.draw_buf = &draw_buf;          /* Den Puffer zuweisen */
    disp_drv.flush_cb = my_flush_cb;        /* Deine SPI-Sende-Funktion */
    disp_drv.hor_res = 240;                 /* Breite deines ILI9341 */
    disp_drv.ver_res = 320;                 /* Höhe deines ILI9341 */
    
    lv_disp_drv_register(&disp_drv);        /* Den Treiber im System anmelden */

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touch_lvgl_read;
    lv_indev_drv_register(&indev_drv);

    screen_home_init();

  while(1){
    lv_timer_handler();
    OS_Delay(5);
  }
}

/* --- Die saubere Prepare-Methode --- */
uint32_t* Prepare_Task_Stack(uint32_t *stack_top, void (*task_func)(void)) {
    // 1. Hardware Frame (wird vom PendSV-Handler automatisch gepoppt)
    stack_top[-1] = XPSR_INITIAL;                          // xPSR (Thumb-Bit)
    stack_top[-2] = ((uint32_t)task_func) | 0x01;        // PC (Task-Einstieg)
    stack_top[-3] = 0xFFFFFFFD;                          // LR (Wichtig für PendSV)
    stack_top[-4] = 0;                                   // R12
    stack_top[-5] = 0;                                   // R3
    stack_top[-6] = 0;                                   // R2
    stack_top[-7] = 0;                                   // R1
    stack_top[-8] = 0;                                   // R0
    
    // 2. Software Frame (wird von unserem Assembler-Code gepoppt)
    for(int i = -9; i >= -16; i--) {
        stack_top[i] = 0;                                // R11 bis R4
    }
    
    return &stack_top[-16]; // Zeiger auf R4 zurückgeben
}

void TaskSheduler(void) {
    int next_task = -1;
    int current_index = (currentTCB - &tasks[0]);

    for(int i = 1; i <=4; i++){
      int index = (current_index + i) % 4;
      if(tasks[index].sleep_ticks == 0) {
        next_task = index;
        break;
      }
    }

    if(next_task != -1) {
      currentTCB = &tasks[next_task];
    } 
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

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
  /* FPU HARDWARE ABSCHALTEN */
  //SCB->CPACR &= ~((3UL << 10*2)|(3UL << 11*2));
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
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
  ILI9341_Init(); 

  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_6, GPIO_PIN_SET); // Display CS inaktiv
HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET); // Touch CS inaktiv
   
  HAL_SYSTICK_Config(SystemCoreClock / 1000);

  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0); 
  HAL_NVIC_SetPriority(PendSV_IRQn, 15, 0);
 
  tasks[0].sp = Prepare_Task_Stack(&stackA[STACK_SIZE], TaskA);
  tasks[1].sp = Prepare_Task_Stack(&stackB[STACK_SIZE], TaskB);
  tasks[2].sp = Prepare_Task_Stack(&stackIdle[128], Task_Idle);
  tasks[2].sleep_ticks = 0;

  tasks[3].sp = Prepare_Task_Stack(&stackDisplay[1024], Task_Display);
  tasks[3].sleep_ticks = 0;

  currentTCB = &tasks[0];
 
  lv_init();
  lvgl_is_initialized = 1; 
  Start_OS();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
     HAL_Delay(500);
  }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LCD_DC_Pin|LCD_RES_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LCD_DC_Pin LCD_RES_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin|LCD_RES_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  // T_CS → PB7 als Output
  GPIO_InitStruct.Pin = TOUCH_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(TOUCH_CS_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET); // CS idle HIGH

  // T_IRQ → PC7 als Input
  GPIO_InitStruct.Pin = TOUCH_IRQ_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(TOUCH_IRQ_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void TaskA(void)
{
    while(1) { 
      OS_Delay(1000);
    }
}

void TaskB(void)
{
    while(1) { 
      OS_Delay(500);
    }
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
