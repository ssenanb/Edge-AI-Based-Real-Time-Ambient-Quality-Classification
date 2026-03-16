/*
 * BME280.h
 *
 *  Created on: Oct 4, 2025
 *      Author: sssen
 */

#ifndef INC_BME280_H_
#define INC_BME280_H_

#include "main.h"
//#include "cmsis_os.h"

// register definitions
#define DEVICE_ADDRESS 			0x76 << 1 // 0xEC

#define CHIP_ID 				0xD0
#define CHIP_ID_VALUE			0x60
#define CTRL_HUM_REG			0xF2
#define STATUS_REG				0xF3
#define CTRL_MEAS_REG			0xF4
#define CONFIG_REG 				0xF5
#define CHIP_RESET				0xE0
#define CHIP_RESET_VALUE		0x00
#define CHIP_RESET_WRITE_VALUE 	0xB6
#define PRESS_MSB				0xF7
#define PRESS_LSB				0xF8
#define PRESS_XLSB				0xF9
#define TEMP_MSB				0xFA
#define TEMP_LSB				0xFB
#define TEMP_XLSB				0xFC
#define HUM_MSB					0xFD
#define HUM_LSB					0xFE
#define CALIB00					0x88
#define CALIB26					0xE1

// device mode
#define SLEEP_MODE			0x00
#define FORCED_MODE			0x01
#define NORMAL_MODE			0x03

// humidity oversampling
#define OSRS_H_OFF			0x00
#define OSRS_H_1			0x01
#define OSRS_H_2			0x02
#define OSRS_H_4			0x03
#define OSRS_H_8			0x04
#define OSRS_H_16			0x05

// pressure oversampling
#define OSRS_P_OFF			0x00
#define OSRS_P_1			0x01
#define OSRS_P_2			0x02
#define OSRS_P_4			0x03
#define OSRS_P_8			0x04
#define OSRS_P_16			0x05

// temperature oversampling
#define OSRS_T_OFF			0x00
#define OSRS_T_1			0x01
#define OSRS_T_2			0x02
#define OSRS_T_4			0x03
#define OSRS_T_8			0x04
#define OSRS_T_16			0x05

// standby time
#define T_SB_0_5			0x00
#define T_SB_62_5			0x01
#define T_SB_125			0x02
#define T_SB_250			0x03
#define T_SB_500			0x04
#define T_SB_1000			0x05
#define T_SB_10				0x06
#define T_SB_20				0x07

// IIR filter
#define IIR_OFF				0x00
#define IIR_2				0x01
#define IIR_4				0x02
#define IIR_8				0x03
#define IIR_16				0x04


typedef struct{
	uint16_t dig_T1, dig_P1, h4, h5;
	int16_t dig_T2, dig_T3, dig_P2, dig_P3, dig_P4,
			dig_P5, dig_P6, dig_P7, dig_P8, dig_P9,
			dig_H2, dig_H4, dig_H5;
	uint8_t dig_H1, dig_H3;
	int8_t dig_H6;
}BME280_Calibration_Parameters_t;

static BME280_Calibration_Parameters_t data;

typedef enum{
	READ_SUCCESS = 1,
	READ_FAIL = 0,
}BME280_ReadStatus_t;

typedef enum{
	WRITE_SUCCESS = 1,
	WRITE_FAIL = 0,
}BME280_WriteStatus_t;

typedef enum{
	INIT_SUCCESS = 1,
	INIT_FAIL = 0,
}BME280_InitStatus_t;

typedef struct{
	uint8_t OSRS_H: 3; // humidty oversampling
	uint8_t RESERVED: 5;
}CTRL_HUMRegister_t;

typedef struct{
	uint8_t MODE: 2;   // device mode
	uint8_t OSRS_P: 3; // pressure oversampling
	uint8_t OSRS_T: 3; // temperature oversampling
}CTRL_MEASRegister_t;

typedef struct{
	uint8_t SPI_EN: 1;     // SPI wire enabled/disabled
	uint8_t RESERVED: 1;
	uint8_t IIR_FILTER: 3; // iir filter
	uint8_t STANDBY_T: 3;    // standby time
}ConfigRegister_t;


BME280_InitStatus_t BME280_Init(void);
void BME280_ReadSensorData(float *temperature, float *humidity, float *pressure);

#endif /* INC_BME280_H_ */
