// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Samsung Electronics Inc.
 */

#ifndef _CAM_OIS_DEFINE_A17_H_
#define _CAM_OIS_DEFINE_A17_H_

#define ENABLE_AOIS                      1
#define OIS_AF_MAIN_NAME                 AFDRV_DW9818AF

#define MCU_I2C_SLAVE_W_ADDR             0xC4
#define OIS_GYRO_SCALE_FACTOR            114 // LSM6DSO
#define OIS_GYRO_OFFSET_SPEC             15000 // LSM6DSO

#define OIS_GYRO_WX_POLE                 0x04
#define OIS_GYRO_WY_POLE                 0
#define OIS_GYRO_ORIENTATION             0

#endif //_CAM_OIS_DEFINE_A17_H_

