/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "lwip.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "httpd.h"
#include <stdio.h>
#include <string.h>

#include <bum_player.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define PLAYER_NAME "Jojo"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */

event e;

#define TIM2_MS 10
#define BUMPER_DT_MS 100

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_UART5_Init(void);
static void MX_USART3_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Serial constants
USART_TypeDef* SERIAL = USART6;
UART_HandleTypeDef* UART_SERIAL = &huart6;
uint8_t serial_char = 0;

// Accelerometer constants
USART_TypeDef* ACCELEROMETER = UART5;
UART_HandleTypeDef* UART_ACCELEROMETER = &huart5;
#define ACCELEROMETER_FRAMES_LEN 3
int8_t acceloremeter_in[ACCELEROMETER_FRAMES_LEN];

void HAL_UART_RxCpltCallback(UART_HandleTypeDef* huart) {
    // SERIAL message
    if (huart->Instance == SERIAL) {
        // TODO: do something

        // Restart receiving data on USART6
        HAL_UART_Receive_IT(UART_SERIAL, &serial_char, 1);
    } else if (huart->Instance == ACCELEROMETER) {
        // Accelerometer message

        // Decode accelerations
        int8_t x = acceloremeter_in[0];
        int8_t y = acceloremeter_in[1];
        int8_t z = acceloremeter_in[2];

        bum_game_acceleration(x, y, z);

        // Restart receiving data on accelerometer
        HAL_UART_Receive_IT(UART_ACCELEROMETER, acceloremeter_in,
            ACCELEROMETER_FRAMES_LEN);
    }
}

int _write(int file, char* ptr, int len) {
    HAL_UART_Transmit(UART_SERIAL, (uint8_t*)ptr, len, 1000);
    return len;
}

void bumper_signal_error(int x) {
    HAL_GPIO_WritePin(LD3_GPIO_Port, LD5_Pin, 1); // RED

    char buff[20];
    sprintf(buff, "ERROR %d<br/>", x);

    bum_log(buff);
}

void bumper_signal_debug(int x) {
    HAL_GPIO_TogglePin(LD6_GPIO_Port, LD6_Pin); // BLUE
}

// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// **********************************************************************************************
// ----------------------------------------------------------------------------------------------
// PLAYER
BumperProtocolPlayer bum_player;

#define JSON_ORDERS_SIZE 500
char json_orders[JSON_ORDERS_SIZE + 1];

int bumper_game_step(uint8_t step, uint8_t param) {
    int len = strlen(json_orders);
    if (len > JSON_ORDERS_SIZE - 50)
        return 0;

    char msg[40];
    msg[0] = 0;
    switch (step) {
    case BUM_STEP_REGISTERED:
        switch (param) {
        case 0: // internal error
            strcpy(msg, "Internal Error");
            break;
        case 1: // OK
            strcpy(msg, "OK registered as ");
            strcat(msg, bum_player.name);
            break;
        case 2: // OK but already registered
            strcpy(msg, "OK already as ");
            strcat(msg, bum_player.name);
            break;
        case 3: // NO, too many players
            strcpy(msg, "Too many players");
            break;
        case 4: // NO, game already started
            strcpy(msg, "Too late");
            break;
        default:
            strcpy(msg, "param ???");
            break;
        }
        break;

    case BUM_STEP_START:
        strcpy(msg, "Playing as ");
        strcat(msg, bum_player.name);
        break;
        break;

    case BUM_STEP_RESULT:
        break;

    case BUM_STEP_END:
        sprintf(msg, "Game Over");
        break;

    default:
        strcpy(msg, "step ???");
        break;
    }

    if (msg[0]) {
        sprintf(json_orders + len,
            "{\"display\":[{\"id\":\"step\",\"x\":200,\"y\":-5,\"content\":\"%"
            "s\"}]},",
            msg);
    }

    return 1;
}

int bumper_game_new_player(uint8_t id, const char* name, uint32_t color) {
    int len = strlen(json_orders);
    if (len > JSON_ORDERS_SIZE - 50)
        return 0;
    sprintf(json_orders + len,
        "{\"new_player\":[{\"i\":%d,\"name\":\"%s\",\"color\": \"#%06X\"}]},",
        (unsigned int)id, name, (unsigned int)color);
    return 1;
}

int bumper_game_player_move(uint8_t id, uint16_t x, uint16_t y, uint16_t s) {
    int len = strlen(json_orders);
    if (len > JSON_ORDERS_SIZE - 50)
        return 0;
    sprintf(json_orders + len,
        "{\"move\":[{\"i\":%d,\"x\":%d,\"y\":%d,\"a\":%d,\"s\":%d}]},",
        (unsigned int)id, (unsigned int)x, (unsigned int)y, 0,
        (unsigned int)s);
    return 1;
}

int bumper_game_print(const char* msg) {
    int len = strlen(json_orders);
    if (len > JSON_ORDERS_SIZE - 50)
        return 0;
    sprintf(json_orders + len,
        "{\"display\":[{\"id\":\"score\",\"x\":10,\"y\":-5,\"content\":\"%"
        "s\"}]},",
        msg);
    return 1;
}

void bumper_init_player() {
    bum_player.game_step = bumper_game_step;
    bum_player.game_new_player = bumper_game_new_player;
    bum_player.game_player_move = bumper_game_player_move;
    bum_player.game_print = bumper_game_print;

    bum_player.error = bumper_signal_error;
    bum_player.debug = bumper_signal_debug;

    bum_init_player(&bum_player);
}

uint8_t user_button_pushed = 0;

int check_board_id() {
    uint32_t UID[3];
    UID[0] = HAL_GetUIDw0();
    UID[1] = HAL_GetUIDw1();
    UID[2] = HAL_GetUIDw2();

    uint32_t UID_COORDINATOR[3];
    UID_COORDINATOR[0] = 0x002d0025;
    UID_COORDINATOR[1] = 0x3137470f;
    UID_COORDINATOR[2] = 0x30373234;

    uint32_t UID_PLAYER_1[3];
    UID_PLAYER_1[0] = 0x470058;
    UID_PLAYER_1[1] = 0x58485018;
    UID_PLAYER_1[2] = 0x20383852;

    if ((UID[0] == UID_COORDINATOR[0]) && (UID[1] == UID_COORDINATOR[1]) && (UID[2] == UID_COORDINATOR[2]))
        return 0;

    if ((UID[0] == UID_PLAYER_1[0]) && (UID[1] == UID_PLAYER_1[1]) && (UID[2] == UID_PLAYER_1[2]))
        return 1;

    return 2;
}

WebInterface wi;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
    /* USER CODE BEGIN 1 */
    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    // UART3 : XBee
    // UART5 : MEMS board
    // UART6 : debug / 232

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_I2C1_Init();
    MX_USB_HOST_Init();
    MX_USART6_UART_Init();
    MX_UART5_Init();
    MX_USART3_UART_Init();
    MX_LWIP_Init();
    /* USER CODE BEGIN 2 */

    web_interface_init(&wi);

    event_init(&e);

    HAL_UART_Receive_IT(UART_SERIAL, &serial_char, 1);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */

    httpd_init();

    bumper_init_player();

    strcpy(json_orders, "{\"L\":[");

    // v = xbee_configure_API1();
    // start receiving data on accelerometer
    HAL_UART_Receive_IT(UART_ACCELEROMETER, acceloremeter_in,
        ACCELEROMETER_FRAMES_LEN);

    while (1) {
        /* USER CODE END WHILE */
        MX_USB_HOST_Process();

        /* USER CODE BEGIN 3 */

        // Make ETH work
        MX_LWIP_Process();

        // LD3 (orange), LD4 (green), LD5 (red) and LD6 (blue)

        bum_process_player();

        // Execute buttons pushed in web interface
        if (event_check(&wi.evt)) {
            if (wi.button_register_player) {
                wi.button_register_player = 0;
                bum_game_register(bum_player.name);
            }

            if (wi.button_acc) {
                wi.button_acc = 0;
                bum_game_acceleration(wi.acc_x, wi.acc_y, 0);
            }
        }

        if (event_check(&e)) // If the event has been triggered
        {
            bum_process(TIM2_MS);

            static int N_BUMPER_DT_MS = 0;
            N_BUMPER_DT_MS += TIM2_MS;
            if (N_BUMPER_DT_MS >= BUMPER_DT_MS) {
                N_BUMPER_DT_MS = 0;

                // We remove this line because it is blocking
                // printf( "%u mV\r\n", ( unsigned int )v );
            }

            static int N500 = 0;
            N500 += TIM2_MS;
            if (N500 >= 500) {
                N500 = 0;
                HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin); // ORANGE
            }
        }
    }
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
    /** Initializes the CPU, AHB and APB busses clocks
   */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }
    /** Initializes the CPU, AHB and APB busses clocks
   */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

    /* USER CODE BEGIN I2C1_Init 0 */

    /* USER CODE END I2C1_Init 0 */

    /* USER CODE BEGIN I2C1_Init 1 */

    /* USER CODE END I2C1_Init 1 */
    hi2c1.Instance = I2C1;
    hi2c1.Init.ClockSpeed = 100000;
    hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN I2C1_Init 2 */

    /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief UART5 Initialization Function
 * @param None
 * @retval None
 */
static void MX_UART5_Init(void) {

    /* USER CODE BEGIN UART5_Init 0 */

    /* USER CODE END UART5_Init 0 */

    /* USER CODE BEGIN UART5_Init 1 */

    /* USER CODE END UART5_Init 1 */
    huart5.Instance = UART5;
    huart5.Init.BaudRate = 115200;
    huart5.Init.WordLength = UART_WORDLENGTH_8B;
    huart5.Init.StopBits = UART_STOPBITS_1;
    huart5.Init.Parity = UART_PARITY_NONE;
    huart5.Init.Mode = UART_MODE_TX_RX;
    huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart5.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart5) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN UART5_Init 2 */

    /* USER CODE END UART5_Init 2 */
}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void) {

    /* USER CODE BEGIN USART3_Init 0 */

    /* USER CODE END USART3_Init 0 */

    /* USER CODE BEGIN USART3_Init 1 */

    /* USER CODE END USART3_Init 1 */
    huart3.Instance = USART3;
    huart3.Init.BaudRate = 9600;
    huart3.Init.WordLength = UART_WORDLENGTH_8B;
    huart3.Init.StopBits = UART_STOPBITS_1;
    huart3.Init.Parity = UART_PARITY_NONE;
    huart3.Init.Mode = UART_MODE_TX_RX;
    huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart3.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart3) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART3_Init 2 */

    __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);

    /* USER CODE END USART3_Init 2 */
}

/**
 * @brief USART6 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART6_UART_Init(void) {

    /* USER CODE BEGIN USART6_Init 0 */

    /* USER CODE END USART6_Init 0 */

    /* USER CODE BEGIN USART6_Init 1 */

    /* USER CODE END USART6_Init 1 */
    huart6.Instance = USART6;
    huart6.Init.BaudRate = 115200;
    huart6.Init.WordLength = UART_WORDLENGTH_8B;
    huart6.Init.StopBits = UART_STOPBITS_1;
    huart6.Init.Parity = UART_PARITY_NONE;
    huart6.Init.Mode = UART_MODE_TX_RX;
    huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart6.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart6) != HAL_OK) {
        Error_Handler();
    }
    /* USER CODE BEGIN USART6_Init 2 */

    /* USER CODE END USART6_Init 2 */
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(CS_I2C_SPI_GPIO_Port, CS_I2C_SPI_Pin, GPIO_PIN_RESET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(OTG_FS_PowerSwitchOn_GPIO_Port, OTG_FS_PowerSwitchOn_Pin,
        GPIO_PIN_SET);

    /*Configure GPIO pin Output Level */
    HAL_GPIO_WritePin(GPIOD,
        LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin,
        GPIO_PIN_RESET);

    /*Configure GPIO pin : CS_I2C_SPI_Pin */
    GPIO_InitStruct.Pin = CS_I2C_SPI_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(CS_I2C_SPI_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : OTG_FS_PowerSwitchOn_Pin */
    GPIO_InitStruct.Pin = OTG_FS_PowerSwitchOn_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(OTG_FS_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : PDM_OUT_Pin */
    GPIO_InitStruct.Pin = PDM_OUT_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(PDM_OUT_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : BOOT1_Pin */
    GPIO_InitStruct.Pin = BOOT1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : CLK_IN_Pin */
    GPIO_InitStruct.Pin = CLK_IN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(CLK_IN_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pins : LD4_Pin LD3_Pin LD5_Pin LD6_Pin
                           Audio_RST_Pin */
    GPIO_InitStruct.Pin = LD4_Pin | LD3_Pin | LD5_Pin | LD6_Pin | Audio_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /*Configure GPIO pin : I2S3_SCK_Pin */
    GPIO_InitStruct.Pin = I2S3_SCK_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(I2S3_SCK_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : OTG_FS_OverCurrent_Pin */
    GPIO_InitStruct.Pin = OTG_FS_OverCurrent_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(OTG_FS_OverCurrent_GPIO_Port, &GPIO_InitStruct);

    /*Configure GPIO pin : MEMS_INT2_Pin */
    GPIO_InitStruct.Pin = MEMS_INT2_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(MEMS_INT2_GPIO_Port, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */

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
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
   number, tex: printf("Wrong parameters value: file %s on line %d\r\n", file,
   line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
