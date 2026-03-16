/*
 * BME280.c
 *
 *  Created on: Oct 4, 2025
 *      Author: sssen
 */


#include "BME280.h"

extern I2C_HandleTypeDef hi2c1;
//extern osMutexId_t i2cMutexHandle;

static BME280_ReadStatus_t BME280_ReadRegisterData(uint8_t registerAddress, uint8_t sizeofData, uint8_t *dataBuffer){
	//if(osMutexAcquire(i2cMutexHandle, 1000) == osOK){
		if (HAL_I2C_Mem_Read(&hi2c1, DEVICE_ADDRESS, registerAddress, 1, dataBuffer, sizeofData, 1000) == HAL_OK) {
		//	osMutexRelease(i2cMutexHandle);
			return READ_SUCCESS;
		}
	//}
	return READ_FAIL;
}

static BME280_WriteStatus_t BME280_WriteRegisterData(uint8_t registerAddress, uint8_t value){
	//if(osMutexAcquire(i2cMutexHandle, 1000) == osOK){
		if (HAL_I2C_Mem_Write(&hi2c1, DEVICE_ADDRESS, registerAddress, I2C_MEMADD_SIZE_8BIT, &value, 1, 1000) == HAL_OK) {
			//osMutexRelease(i2cMutexHandle);
			return WRITE_SUCCESS;
		}
	//}
	return WRITE_FAIL;
}

static BME280_ReadStatus_t BME280_ReadRegister16Data(uint8_t registerAddress, uint8_t sizeofData, uint16_t *dataBuffer){
	uint8_t tempData[2] = {0};

	//if(osMutexAcquire(i2cMutexHandle, 1000) == HAL_OK){ //osOK
		if (HAL_I2C_Mem_Read(&hi2c1, DEVICE_ADDRESS, registerAddress, 1, tempData, sizeofData, 1000) == HAL_OK) {
			/* data is stored in little-endian format.
			 * tempData[0] -> LSB
			 * tempData[1] -> MSB
			 */
			*dataBuffer = (uint16_t) ((tempData[1] << 8) | tempData[0]); // 16 bit -> MSB + LSB
			//osMutexRelease(i2cMutexHandle);
			return READ_SUCCESS;
		//}
	}
	return READ_FAIL;
}

static void BME280_ReadCalibrationData(){

	BME280_ReadRegister16Data(CALIB00, 2, &data.dig_T1);

	BME280_ReadRegister16Data(CALIB00 + 0x02, 2, (uint16_t*)&data.dig_T2);

	BME280_ReadRegister16Data(CALIB00 + 0x04, 2, (uint16_t*)&data.dig_T3);

	BME280_ReadRegister16Data(CALIB00 + 0x06, 2, &data.dig_P1);

	BME280_ReadRegister16Data(CALIB00 + 0x08, 2, (uint16_t*)&data.dig_P2);

	BME280_ReadRegister16Data(CALIB00 + 0x0A, 2, (uint16_t*)&data.dig_P3);

	BME280_ReadRegister16Data(CALIB00 + 0x0C, 2, (uint16_t*)&data.dig_P4);

	BME280_ReadRegister16Data(CALIB00 + 0x0E, 2, (uint16_t*)&data.dig_P5);

	BME280_ReadRegister16Data(CALIB00 + 0x10, 2, (uint16_t*)&data.dig_P6);

	BME280_ReadRegister16Data(CALIB00 + 0x12, 2, (uint16_t*)&data.dig_P7);

	BME280_ReadRegister16Data(CALIB00 + 0x14, 2, (uint16_t*)&data.dig_P8);

	BME280_ReadRegister16Data(CALIB00 + 0x16, 2, (uint16_t*)&data.dig_P9);

	BME280_ReadRegisterData(CALIB00 + 0x19, 1, &data.dig_H1);

	BME280_ReadRegisterData(CALIB26, 2, &data.dig_H2);

	BME280_ReadRegisterData(CALIB26 + 0x02, 1, &data.dig_H3);

	BME280_ReadRegister16Data(CALIB26 + 0x03, 2, &data.h4);

	BME280_ReadRegister16Data(CALIB26 + 0x04, 2, &data.h5);

	BME280_ReadRegisterData(CALIB26 + 0x06, 1, &data.dig_H6);

    data.dig_H4 = (data.h4 & 0x00ff) << 4 | (data.h4 & 0x0f00) >> 8;

    data.dig_H5 = data.h5 >> 4;

}

BME280_InitStatus_t BME280_Init(){

	uint8_t idRegVerify = 0;
	uint8_t tempReg = 0;

	CTRL_HUMRegister_t humidity = {0};
	CTRL_MEASRegister_t measurement = {0};
	ConfigRegister_t config = {0};

	if(BME280_WriteRegisterData(CHIP_RESET, CHIP_RESET_WRITE_VALUE) != WRITE_SUCCESS) return INIT_FAIL;

	//osDelay(2);
	HAL_Delay(2);
	// humidity init
	humidity.OSRS_H = OSRS_H_4;
	tempReg = *((uint8_t*)&humidity);

	BME280_WriteRegisterData(CTRL_HUM_REG, tempReg);

	//osDelay(10);
	HAL_Delay(10);
	// pressure and temperature init
	measurement.MODE = NORMAL_MODE;
	measurement.OSRS_P = OSRS_P_4;
	measurement.OSRS_T = OSRS_T_4;

	tempReg = *((uint8_t*)&measurement);

	BME280_WriteRegisterData(CTRL_MEAS_REG, tempReg);

	//osDelay(10);
	HAL_Delay(10);
	// IIR filter, SPI EN/DIS and standby init
	config.IIR_FILTER = IIR_4;
	config.SPI_EN = 0x00; // SPI is disabled
	config.STANDBY_T = T_SB_125;

	tempReg = *((uint8_t*)&config);

	BME280_WriteRegisterData(CONFIG_REG, tempReg);

	//osDelay(10);
	HAL_Delay(10);
	BME280_ReadCalibrationData();

	if(BME280_ReadRegisterData(CHIP_ID, 1, &idRegVerify) != READ_SUCCESS) return INIT_FAIL;

	if(idRegVerify != CHIP_ID_VALUE) return INIT_FAIL;

	return INIT_SUCCESS;
}

/************* COMPENSATION CALCULATION AS PER DATASHEET (page 25) **************************/

int32_t t_fine;

static int32_t BME280_Compensate_T_int32(int32_t adc_T)
{
	int32_t var1, var2, T;

	var1 = ((((adc_T>>3) - ((int32_t)data.dig_T1<<1))) * ((int32_t)data.dig_T2)) >> 11;
	var2 = (((((adc_T>>4) - ((int32_t)data.dig_T1)) * ((adc_T>>4) - ((int32_t)data.dig_T1)))>> 12) *((int32_t)data.dig_T3)) >> 14;
	t_fine = var1 + var2;
	T = (t_fine * 5 + 128) >> 8;

	return T;
}

static uint32_t BME280_Compensate_P_int32(int32_t adc_P)
{
	int32_t var1, var2;
	uint32_t p;

	var1 = (((int32_t)t_fine)>>1) - (int32_t)64000;
	var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)data.dig_P6);
	var2 = var2 + ((var1*((int32_t)data.dig_P5))<<1);
	var2 = (var2>>2)+(((int32_t)data.dig_P4)<<16);
	var1 = (((data.dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int32_t)data.dig_P2) *var1)>>1))>>18;
	var1 =((((32768+var1))*((int32_t)data.dig_P1))>>15);
	if (var1 == 0)
	{
		return 0;
	}
	p = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125;
	if (p < 0x80000000)
	{
		p = (p << 1) / ((uint32_t)var1);
	}
	else
	{
		p = (p / (uint32_t)var1) * 2;
	}
	var1 = (((int32_t)data.dig_P9) * ((int32_t)(((p>>3) * (p>>3))>>13)))>>12;
	var2 = (((int32_t)(p>>2)) * ((int32_t)data.dig_P8))>>13;
	p = (uint32_t)((int32_t)p + ((var1 + var2 + data.dig_P7) >> 4));

	return p;
}

static uint32_t BME280_Compensate_H_int32(int32_t adc_H)
{
	int32_t v_x1_u32r;

	v_x1_u32r = (t_fine - ((int32_t)76800));
	v_x1_u32r = (((((adc_H << 14) - (((int32_t)data.dig_H4) << 20) - (((int32_t)data.dig_H5) *\
				v_x1_u32r)) + ((int32_t)16384)) >> 15) * (((((((v_x1_u32r *\
                ((int32_t)data.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)data.dig_H3)) >> 11) +\
                ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)data.dig_H2) +\
			    8192) >> 14));
	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *\
			    ((int32_t)data.dig_H1)) >> 4));
	v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
	v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

	return (uint32_t)(v_x1_u32r>>12);
}

/******************************************************************************************************/

static void BME280_ReadRegister(int32_t *temperature, uint32_t *humidity, uint32_t* pressure){

	int32_t adcTemperature = 0;
	int32_t adcHumidity = 0;
	int32_t adcPressure = 0;

	uint8_t data[8] = {0};

	BME280_ReadRegisterData(PRESS_MSB, 8, data);

	adcPressure = data[0] << 12 | data[1] << 4 | data[2] >> 4;
	adcTemperature = data[3] << 12 | data[4] << 4 | data[5] >> 4;
	adcHumidity = data[6] << 8 | data[7];

	*temperature = BME280_Compensate_T_int32(adcTemperature);
	*humidity = BME280_Compensate_H_int32(adcHumidity);
	*pressure = BME280_Compensate_P_int32(adcPressure);
}

void BME280_ReadSensorData(float *temperature, float *humidity, float *pressure){

	int32_t fixed_temperature;
	uint32_t fixed_humidty, fixed_pressure;

	BME280_ReadRegister(&fixed_temperature, &fixed_humidty, &fixed_pressure);

	*temperature = (float) fixed_temperature / 100.0;
	*humidity = (float) fixed_humidty / 1024.0;
	*pressure = (float) fixed_pressure / 256.0;
}

