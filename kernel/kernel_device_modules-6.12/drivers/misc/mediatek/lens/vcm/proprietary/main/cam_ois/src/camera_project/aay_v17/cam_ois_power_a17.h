// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Samsung Electronics Inc.
 */

#ifndef _CAM_OIS_POWER_A17_H_
#define _CAM_OIS_POWER_A17_H_

struct mcu_info;

int cam_ois_pinctrl_init(struct mcu_info *mcu_info);
int cam_ois_mcu_power_up(struct mcu_info *mcu_info);
int cam_ois_mcu_power_down(struct mcu_info *mcu_info);
int cam_ois_sysfs_mcu_power_up(struct mcu_info *mcu_info);
int cam_ois_sysfs_mcu_power_down(struct mcu_info *mcu_info);
int cam_ois_af_power_up(struct mcu_info *mcu_info);
int cam_ois_af_power_down(struct mcu_info *mcu_info);

#endif //_CAM_OIS_POWER_A17_H_
