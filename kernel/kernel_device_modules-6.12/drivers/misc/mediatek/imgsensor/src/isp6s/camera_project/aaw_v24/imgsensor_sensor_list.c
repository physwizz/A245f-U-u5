// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include "kd_imgsensor.h"
#include "imgsensor_sensor_list.h"

/* Add Sensor Init function here
 * Note:
 * 1. Add by the resolution from ""large to small"", due to large sensor
 *    will be possible to be main sensor.
 *    This can avoid I2C error during searching sensor.
 * 2. This file should be the same as
 *    mediatek\custom\common\hal\imgsensor\src\sensorlist.cpp
 */
struct IMGSENSOR_SENSOR_LIST gimgsensor_sensor_list[MAX_NUM_OF_SUPPORT_SENSOR] = {

	{S5KJN1_SENSOR_ID, SENSOR_DRVNAME_S5KJN1_MIPI_RAW, S5KJN1_MIPI_RAW_SensorInit},

	{IMX258_SENSOR_ID, SENSOR_DRVNAME_IMX258_MIPI_RAW, IMX258_MIPI_RAW_SensorInit},
	
	{HI1339_SENSOR_ID, SENSOR_DRVNAME_HI1339_MIPI_RAW, HI1339_MIPI_RAW_SensorInit},

	{GC5035_SENSOR_ID, SENSOR_DRVNAME_GC5035_MIPI_RAW, GC5035_MIPI_RAW_SensorInit},

	{GC02M1_SENSOR_ID, SENSOR_DRVNAME_GC02M1_MIPI_RAW, GC02M1_MIPI_RAW_SensorInit},

	/*  ADD sensor driver before this line */
	{0, {0}, NULL}, /* end of list */
};

/* e_add new sensor driver here */

