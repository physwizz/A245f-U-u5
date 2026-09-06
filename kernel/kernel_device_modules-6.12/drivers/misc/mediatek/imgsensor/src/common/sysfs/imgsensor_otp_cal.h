// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd
 */

#ifndef __IMGESENSOR_OTP_CAL_H__
#define __IMGESENSOR_OTP_CAL_H__

//read functions of OTP cal
int gc5035_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int s5k3l6_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int sr846d_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int gc5035b_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int gc08a3_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int gc02m1_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int hi2021q_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int hi1339_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int s5k4hayx_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);
int sc501cs_read_otp_cal(unsigned int addr, unsigned char *data, unsigned int size);

#endif //__IMGESENSOR_OTP_CAL_H__
