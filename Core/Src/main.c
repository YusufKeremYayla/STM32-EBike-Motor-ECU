/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "lcd_2x16_drivers.h"
#include "uart_ex.h"
#include "io_driver.h"
#include "adc_drive.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define FILTER_SAMPLES     5
#define PWM_MAX            3360.0f
#define PWM_KICKSTART      300.0f
#define SIGNAL_TIMEOUT_MS  150

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

UART_Ex_t uart3;
Circular_Buffer_t uartCbIn;
Circular_Buffer_t uartCbOut;
IO_Info_t ioInfo;
ADC_Info_t adcInfo;

volatile uint32_t 	manuel_pulse_sayaci = 0;
volatile uint8_t 	veri_hazir = 0;
volatile uint32_t 	log_zaman = 0;
volatile uint32_t 	log_rpm = 0;

volatile uint32_t 	prev_capture = 0;
volatile uint32_t 	curr_capture = 0;
volatile uint32_t 	pulse_interval = 0;
volatile uint8_t 	first_pulse = 1;

// İKİ FARKLI KRONOMETRE
volatile uint32_t last_pulse_tick 		= 0;
volatile uint32_t last_valid_pulse_time = 0;

volatile float raw_rpm 		= 0;
volatile float filtered_rpm = 0.0f;

volatile float 		rpm_buffer[FILTER_SAMPLES] = {0};
volatile uint8_t 	buffer_index = 0;
volatile uint8_t 	buffer_count = 0;

float target_rpm 		= 500.0f;
float target_rpm_hedef 	= 0.0f; // Başlangıçta motor dursun diye 0 yapıldı
float kp = 2.4629, ki 	= 0.07874, kd = 0.0f;
float error = 0, last_error = 0, integral = 0;
float pid_output 		= 0;

LCD_t lcd =
{
	.backLight 	= true,
	.columns 	= 16,
	.rows 	 	= 2,
	.i2c_addr 	= LCD_I2C_DEVICE_ADDRESS,
	.hi2c		= &hi2c1
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM3_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
/* USER CODE BEGIN PFP */

float Update_RPM_Filter(float new_rpm);
void PID_Update(void);

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
  IO_Initalization(&ioInfo);
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */

  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  HAL_TIM_IC_Start_IT(&htim3, TIM_CHANNEL_1);
  HAL_TIM_Base_Start_IT(&htim4);

  ADC_Initialization(&adcInfo, &hadc1);
  if(adcInfo.adcError != ADC_No_Error)
  {
 	  //ADC başlatılamadı, kullanıcıya bilgi ver!
  }

  UARTx_Initalization(&uart3, &huart3, &uartCbIn, &uartCbOut);

  char lineBuffer[128];

  // Motor Yön Pinleri Başlangıç (İleri Vites)
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);

  LCD_Initalization(&lcd);
  LCD_Clear(&lcd);
  LCD_Set_Cursor(&lcd, 0, 0);

  LCD_Send_String(&lcd,"E-BIKE SYSTEM");
  HAL_Delay(1000);

  LCD_Set_Cursor(&lcd, 0, 1);
  LCD_Send_String(&lcd, "YUSUF E-DRIVE");
  LCD_Set_Cursor(&lcd, 1, 2);
  LCD_Send_String(&lcd, "Version 2.0");
  HAL_Delay(2000);

  LCD_Clear(&lcd);
  LCD_Show_Cursor(&lcd);

  uint8_t secilen_mod 				= 1;
  uint32_t son_lcd_guncelleme 		= 0;
  Input_Status_t son_buton_durumu	= Input_Status_Low;

  ioInfo.outputInfo.ledGreen.pinState 	= GPIO_PIN_RESET;
  ioInfo.outputInfo.ledOrange.pinState 	= GPIO_PIN_RESET;
  ioInfo.outputInfo.ledRed.pinState	 	= GPIO_PIN_RESET;
  ioInfo.outputInfo.ledBlue.pinState 	= GPIO_PIN_RESET;

  // KONTROL ÜNİTESİ DURUM DEĞİŞKENLERİ
  static uint8_t sistem_durumu = 0;        // 0-3: Kurulum Menüsü, 4: Aktif Sürüş
  static uint8_t uart_reverse_request = 0; // 0: İleri (F), 1: Geri (R)
  static uint8_t current_direction = 0;    // Motorun fiziksel yönü
  static uint8_t drive_mode_type = 0;      // 0: Otomatik (A), 1: Manuel (M)

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

      // 1. HER ZAMAN POTANSİYOMETREYİ OKU (Arka Plan ADC)
      ADC_DMA_Conversion(&adcInfo);
      uint32_t pot_degeri = adcInfo.adcConvertedData[ADC_Channel_2];

      // =====================================================================
      // 2.(STATE MACHINE)
      // =====================================================================
      if (sistem_durumu == 0) // ADIM 1: Yön Sorma Ekranı
      {
          target_rpm_hedef = 0.0f; // Güvenlik için aracı kilitli tut

          UARTx_Printf(&uart3, "\r\n===============================\n\r");
          UARTx_Printf(&uart3, "   E-DRIVE SISTEM KURULUMU     \n\r");
          UARTx_Printf(&uart3, "===============================\n\r");
          UARTx_Printf(&uart3, "Lutfen Yonu Seciniz:\n\r");
          UARTx_Printf(&uart3, "[F] Ileri (Forward)\n\r");
          UARTx_Printf(&uart3, "[R] Geri (Reverse)\n\r");
          UARTx_Printf(&uart3, "Seciminiz (F/R): ");

          sistem_durumu = 1;
      }
      else if (sistem_durumu == 1) // ADIM 2: Yön Cevabını İşle
      {
          if(UARTx_ReadLine(&uart3, lineBuffer, sizeof(lineBuffer)))
          {
              if (lineBuffer[0] == 'F' || lineBuffer[0] == 'f') {
                  uart_reverse_request = 0;
                  UARTx_Printf(&uart3, "%c\n\r>> ILERI vites secildi.\n\r", lineBuffer[0]);
                  sistem_durumu = 2;
              }
              else if (lineBuffer[0] == 'R' || lineBuffer[0] == 'r') {
                  uart_reverse_request = 1;
                  drive_mode_type = 1;
                  UARTx_Printf(&uart3, "GERI\n\r*** GERI VITES AKTIF (Maks 600 RPM) ***\n\r");
                  UARTx_Printf(&uart3, "\r\n*** KURULUM TAMAMLANDI. MOTOR DEVREDE! ***\n\r");
                  UARTx_Printf(&uart3, "(Durdurmak ve resetlemek icin 'C' yazabilirsiniz)\n\r\n\r");
                  sistem_durumu = 4;
              }
              else {
                  UARTx_Printf(&uart3, "%c\n\r[!] Hatali giris! Lutfen 'F' veya 'R' tuslayiniz: ", lineBuffer[0]);
              }
          }
      }
      else if (sistem_durumu == 2) // ADIM 3: Sürüş Modu Sorma Ekranı
      {
          UARTx_Printf(&uart3, "\r\nLutfen Surus Modunu Seciniz:\n\r");
          UARTx_Printf(&uart3, "[A] Otomatik (Sabit Hiz - Buton)\n\r");
          UARTx_Printf(&uart3, "[M] Manuel (Gaz Kolu Kontrolu)\n\r");
          UARTx_Printf(&uart3, "Seciminiz (A/M): ");

          sistem_durumu = 3;
      }
      else if (sistem_durumu == 3) // ADIM 4: Mod Cevabını İşle
      {
          if(UARTx_ReadLine(&uart3, lineBuffer, sizeof(lineBuffer)))
          {
              if (lineBuffer[0] == 'A' || lineBuffer[0] == 'a') {
                  drive_mode_type = 0;
                  secilen_mod = 1; // Otomatikte ECO'dan başlasın
                  UARTx_Printf(&uart3, "%c\n\r>> OTOMATIK mod secildi.\n\r", lineBuffer[0]);
                  sistem_durumu = 4;
              }
              else if (lineBuffer[0] == 'M' || lineBuffer[0] == 'm') {
                  drive_mode_type = 1;
                  UARTx_Printf(&uart3, "%c\n\r>> MANUEL mod secildi.\n\r", lineBuffer[0]);
                  sistem_durumu = 4;
              }
              else {
                  UARTx_Printf(&uart3, "%c\n\r[!] Hatali giris! Lutfen 'A' veya 'M' tuslayiniz: ", lineBuffer[0]);
              }

              if (sistem_durumu == 4) {
                  UARTx_Printf(&uart3, "\r\n*** KURULUM TAMAMLANDI. MOTOR DEVREDE! ***\n\r");
                  UARTx_Printf(&uart3, "(Durdurmak ve resetlemek icin 'C' yazabilirsiniz)\n\r\n\r");
              }
          }
      }
      else if (sistem_durumu == 4) // ADIM 5: AKTİF SÜRÜŞ (Araç Yolda)
      {
          // --- SÜRÜŞÜ DURDURMA / SIFIRLAMA ---
          if(UARTx_ReadLine(&uart3, lineBuffer, sizeof(lineBuffer)))
          {
               if (lineBuffer[0] == 'C' || lineBuffer[0] == 'c') {
                   UARTx_Printf(&uart3, "\r\n>>> SISTEM DURDURULDU! KURULUM YENIDEN BASLIYOR <<<\n\r");
                   target_rpm_hedef = 0.0f;
                   target_rpm = 0.0f;
                   integral = 0.0f; // PID'yi sıfırla ki motoru zorlamasın
                   __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0); // PWM'i anında kes
                   sistem_durumu = 0; // Başa dön
               }
          }

          // --- H-KÖPRÜSÜ GÜVENLİK FRENİ VE YÖN DEĞİŞİMİ ---
          if (uart_reverse_request != current_direction)
          {
              target_rpm_hedef = 0.0f; // Fren yap

              if (filtered_rpm < 50.0f) // Motor güvenlice durdu
              {
                  current_direction = uart_reverse_request;

                  if (current_direction == 1) // GERİ VİTES
                  {
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_SET);
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
                      ioInfo.outputInfo.ledBlue.pinState = GPIO_PIN_SET;
                  }
                  else // İLERİ VİTES
                  {
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3, GPIO_PIN_RESET);
                      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
                      ioInfo.outputInfo.ledBlue.pinState = GPIO_PIN_RESET;
                  }
              }
          }
          else // --- SEYİR HALİ (GAZ TEPKİLERİ) ---
          {
              if (current_direction == 1)
              {
                  // GERİ VİTES (Sadece Pot, Maks 300 RPM)
                  target_rpm_hedef = ((float)pot_degeri / 4095.0f) * 600.0f;
              }
              else
              {
                  // İLERİ VİTES
                  if (drive_mode_type == 1)
                  {
                      // MANUEL MOD (Potansiyometre kontrolü, Maks 1200 RPM)
                      target_rpm_hedef = ((float)pot_degeri / 4095.0f) * 1200.0f;

                      ioInfo.outputInfo.ledGreen.pinState = GPIO_PIN_SET;
                      ioInfo.outputInfo.ledOrange.pinState = GPIO_PIN_RESET;
                      ioInfo.outputInfo.ledRed.pinState = GPIO_PIN_SET;
                  }
                  else
                  {
                      // OTOMATİK MOD (Mavi Buton ile Sabit Hız Seçimi)
                      IO_Status_Control(&ioInfo);

                      if (ioInfo.inputInfo.userButton.Input_Status == Input_Status_High && son_buton_durumu == Input_Status_Low)
                      {
                          secilen_mod++;
                          if (secilen_mod > 3) secilen_mod = 1;
                          son_lcd_guncelleme = 0;
                      }
                      son_buton_durumu = ioInfo.inputInfo.userButton.Input_Status;

                      switch(secilen_mod)
                      {
                          case 1: // ECO
                              target_rpm_hedef = 500.0f;
                              ioInfo.outputInfo.ledGreen.pinState = GPIO_PIN_SET;
                              ioInfo.outputInfo.ledOrange.pinState = GPIO_PIN_RESET;
                              ioInfo.outputInfo.ledRed.pinState = GPIO_PIN_RESET;
                              break;
                          case 2: // CITY
                              target_rpm_hedef = 900.0f;
                              ioInfo.outputInfo.ledGreen.pinState = GPIO_PIN_RESET;
                              ioInfo.outputInfo.ledOrange.pinState = GPIO_PIN_SET;
                              ioInfo.outputInfo.ledRed.pinState = GPIO_PIN_RESET;
                              break;
                          case 3: // SPORT
                              target_rpm_hedef = 1200.0f;
                              ioInfo.outputInfo.ledGreen.pinState = GPIO_PIN_RESET;
                              ioInfo.outputInfo.ledOrange.pinState = GPIO_PIN_RESET;
                              ioInfo.outputInfo.ledRed.pinState = GPIO_PIN_SET;
                              break;
                      }
                  }
              }
          }
      } // Sürüş Bloğunun Sonu

      if ((HAL_GetTick() - son_lcd_guncelleme) > 300)
      {
          son_lcd_guncelleme = HAL_GetTick();
          LCD_Set_Cursor(&lcd, 0, 0);

          if (sistem_durumu < 4) // Menüdeyken
          {
              LCD_Printf(&lcd, "[ SETUP WIZARD ] ");
          }
          else // Sürüşteyken
          {
              if (current_direction == 1)
              {
                  LCD_Printf(&lcd, "[ REVERSE DRIVE ]");
              }
              else
              {
                  if (drive_mode_type == 1)
                  {
                      LCD_Printf(&lcd, "[ MANUAL DRIVE ] ");
                  }
                  else
                  {
                      switch(secilen_mod)
                      {
                          case 1: LCD_Printf(&lcd, "[ ECO MODE ]     "); break;
                          case 2: LCD_Printf(&lcd, "[ CITY MODE ]    "); break;
                          case 3: LCD_Printf(&lcd, "[ SPORT MODE ]   "); break;
                      }
                  }
              }
          }

          LCD_Set_Cursor(&lcd, 1, 0);
          LCD_Printf(&lcd, "HIZ: %-4lu RPM  ", log_rpm);
      }
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 84;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

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
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 3359;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_IC_InitTypeDef sConfigIC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_IC_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigIC.ICPolarity = TIM_INPUTCHANNELPOLARITY_FALLING;
  sConfigIC.ICSelection = TIM_ICSELECTION_DIRECTTI;
  sConfigIC.ICPrescaler = TIM_ICPSC_DIV1;
  sConfigIC.ICFilter = 15;
  if (HAL_TIM_IC_ConfigChannel(&htim3, &sConfigIC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 8399;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 499;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 3, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_3|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PA3 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_3|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
        PID_Update();
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
    	manuel_pulse_sayaci++;
        uint32_t current_tick = HAL_GetTick();

        if ((current_tick - last_pulse_tick) > 60)
        {
            first_pulse = 1;
        }
        last_pulse_tick = current_tick;

        curr_capture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);

        if (first_pulse)
        {
            prev_capture = curr_capture;
            first_pulse = 0;
        }
        else
        {
        	uint32_t p_interval;
        	if (curr_capture > prev_capture)
        	     p_interval = curr_capture - prev_capture;
        	else
        	{
        	    p_interval = (0xFFFF - prev_capture) + curr_capture + 1;
        	}
        	 prev_capture = curr_capture;

        	if (p_interval > 1500 && p_interval < 60000)
        	  {
        	    pulse_interval = p_interval;
        	    last_valid_pulse_time = current_tick;

        	    float frequency = 1000000.0f / (float)pulse_interval;
        	    raw_rpm = (frequency / 8.0f) * 60.0f;
        	    filtered_rpm = Update_RPM_Filter(raw_rpm);
        	 }
        }
    }
}

void PID_Update(void)
{
    static uint32_t stall_timer = 0;
    static uint8_t in_stall_recovery = 0;

    if ((HAL_GetTick() - last_valid_pulse_time) > SIGNAL_TIMEOUT_MS)
    {
        raw_rpm = 0.0f;
        filtered_rpm = 0.0f;
        first_pulse = 1;

        for(uint8_t i = 0; i < FILTER_SAMPLES; i++) rpm_buffer[i] = 0.0f;
        buffer_index = 0;
        buffer_count = 0;
    }

    if (filtered_rpm == 0.0f && target_rpm > 0.0f)
    {
        if (stall_timer == 0) stall_timer = HAL_GetTick();

        if ((HAL_GetTick() - stall_timer) > 2000)
        {
            in_stall_recovery = 1;
        }
    }
    else
    {
        stall_timer = 0;
        in_stall_recovery = 0;
    }

    if (in_stall_recovery)
    {
        pid_output = 0.0f;
        integral = 0.0f;
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);

        if ((HAL_GetTick() - stall_timer) > 3000)
        {
            in_stall_recovery = 0;
            stall_timer = HAL_GetTick();
        }
        return;
    }

    float rampRate = 15.0f;
    if (target_rpm < target_rpm_hedef)
    {
        target_rpm += rampRate;
        if (target_rpm > target_rpm_hedef) target_rpm = target_rpm_hedef;
    }
    else if (target_rpm > target_rpm_hedef)
    {
        target_rpm -= rampRate;
        if (target_rpm < target_rpm_hedef) target_rpm = target_rpm_hedef;
    }

    error = target_rpm - filtered_rpm;

    integral += error;
    float integral_max = 3360.0f;
    if (ki > 0.0001f) {
        integral_max = PWM_MAX / ki;
    }

    if (integral > integral_max)  integral = integral_max;
    if (integral < -integral_max) integral = -integral_max;

    float derivative = error - last_error;

    pid_output = (kp * error) + (ki * integral) + (kd * derivative);
    last_error = error;

    if (pid_output > PWM_MAX)
    {
        pid_output = PWM_MAX;
    }
    else if (pid_output > 0.5f && pid_output < PWM_KICKSTART)
    {
        pid_output = PWM_KICKSTART;
    }
    else if (pid_output <= 0.5f)
    {
        pid_output = 0.0f;
    }

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, (uint32_t)pid_output);

    log_zaman = HAL_GetTick();
    log_rpm = (uint32_t)filtered_rpm;
    veri_hazir = 1;
}

float Update_RPM_Filter(float new_rpm)
{
    float sum = 0.0f;

    rpm_buffer[buffer_index] = new_rpm;
    buffer_index++;

    if (buffer_count < FILTER_SAMPLES) buffer_count++;
    if (buffer_index >= FILTER_SAMPLES) buffer_index = 0;

    if (buffer_count == 0) return 0.0f;

    for (uint8_t i = 0; i < buffer_count; i++)
    {
        sum += rpm_buffer[i];
    }

    return (sum / (float)buffer_count);
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
