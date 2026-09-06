// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Samsung Electronics Inc.
 */

#ifndef _CAM_OIS_DEFINE_H_
#define _CAM_OIS_DEFINE_H_

#if defined(CONFIG_CAMERA_AAY_V17)
#include "camera_project/aay_v17/cam_ois_define_a17.h"
#elif defined(CONFIG_CAMERA_AAW_V24)
#include "camera_project/aaw_v24/cam_ois_define_a24.h"
#else
//default
#include "camera_project/default/cam_ois_define_default.h"
#endif

#endif //_CAM_OIS_DEFINE_H_
