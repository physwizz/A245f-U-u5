// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2024 Samsung Electronics Inc.
 */

#include "imgsensor_eeprom_rear_s5kjn1.h"

#define REAR_S5KJN1_MAX_CAL_SIZE               (0x3E59 + 0x1)

#define REAR_S5KJN1_HEADER_CHECKSUM_LEN        (0x00FB - 0x0000 + 0x1)
#define REAR_S5KJN1_OEM_CHECKSUM_LEN           (0x2FEB - 0x2FA0 + 0x1)
#define REAR_S5KJN1_AWB_CHECKSUM_LEN           (0x381B - 0x3800 + 0x1)
#define REAR_S5KJN1_SHADING_CHECKSUM_LEN       (0x3C1B - 0x3820 + 0x1)
#define REAR_S5KJN1_SENSOR_CAL_CHECKSUM_LEN    (0x224B - 0x01A0 + 0x1)
#define REAR_S5KJN1_DUAL_CHECKSUM_LEN          -1
#define REAR_S5KJN1_PDAF_CHECKSUM_LEN          (0x37FB - 0x2FF0 + 0x1)
#define REAR_S5KJN1_OIS_CHECKSUM_LEN           -1

#define REAR_S5KJN1_CONVERTED_MAX_CAL_SIZE     (0x41A9 + 0x1)
#define REAR_S5KJN1_CONVERTED_AWB_CHECKSUM_LEN (0x380B - 0x3800 + 0x1)
#define REAR_S5KJN1_CONVERTED_LSC_CHECKSUM_LEN (0x3F6B - 0x3810 + 0x1)

const struct rom_converted_cal_addr rear_s5kjn1_converted_cal_addr = {
	.rom_awb_cal_data_start_addr            = 0x3800,
	.rom_awb_checksum_addr                  = 0x380C,
	.rom_awb_checksum_len                   = (REAR_S5KJN1_CONVERTED_AWB_CHECKSUM_LEN),
	.rom_shading_cal_data_start_addr        = 0x3810,
	.rom_shading_checksum_addr              = 0x3F6C,
	.rom_shading_checksum_len               = (REAR_S5KJN1_CONVERTED_LSC_CHECKSUM_LEN),
};



const struct rom_cal_addr rear_s5kjn1_hwggc_addr = {
	.name                            = CROSSTALK_CAL_HW_GGC,
	.addr                            = 0x13B2,
	.size                            = (0x150B - 0x13B2 + 1),
	.next                            = NULL,
};

const struct rom_cal_addr rear_s5kjn1_swggc_addr = {
	.name                            = CROSSTALK_CAL_SW_GGC,
	.addr                            = 0x1140,
	.size                            = (0x13B1 - 0x1140 + 1),
	.next                            = &rear_s5kjn1_hwggc_addr,
};

const struct rom_cal_addr rear_s5kjn1_pdxtc_addr = {
	.name                            = CROSSTALK_CAL_PD_XTC,
	.addr                            = 0x01A0,
	.size                            = (0x113F - 0x01A0 + 1),
	.next                            = &rear_s5kjn1_swggc_addr,
};

const struct rom_cal_addr rear_s5kjn1_sensorxtc_addr = {
	.name                            = CROSSTALK_CAL_SENSOR_XTC,
	.addr                            = 0x1F40,
	.size                            = (0x223F - 0x1F40 + 1),
	.next                            = &rear_s5kjn1_pdxtc_addr,
};

const struct rom_cal_addr rear_s5kjn1_tetraxtc_addr = {
	.name                            = CROSSTALK_CAL_TETRA_XTC,
	.addr                            = 0x150C,
	.size                            = (0x1F3F - 0x150C + 1),
	.next                            = &rear_s5kjn1_sensorxtc_addr,
};

const struct rom_sac_cal_addr rear_s5kjn1_sac_addr = {
	.rom_mode_addr = 0x0130,
	.rom_time_addr = 0x0131,
};

const struct rom_cal_addr rear_s5kjn1_ois_addr = {
	.addr                                   = 0x0140,
	.size                                   = (0x0197 - 0x0140 + 0x1),
	.next                                   = NULL,
};

const struct imgsensor_vendor_rom_addr rear_s5kjn1_cal_addr = {
	/* Set '-1' if not used */
	.camera_module_es_version               = 'A',
	.cal_map_es_version                     = '1',
	.rom_max_cal_size                       = REAR_S5KJN1_MAX_CAL_SIZE,
	.rom_header_cal_data_start_addr         = 0x00,
	.rom_header_main_module_info_start_addr = 0x6E,
	.rom_header_cal_map_ver_start_addr      = 0x90,
	.rom_header_project_name_start_addr     = 0x98,
	.rom_header_module_id_addr              = 0xB0 + 0x06,
	.rom_header_main_sensor_id_addr         = 0xC0,

	.rom_header_sub_module_info_start_addr  = -1,
	.rom_header_sub_sensor_id_addr          = -1,

	.rom_header_main_header_start_addr      = 0x00,
	.rom_header_main_header_end_addr        = 0x04,
	.rom_header_main_oem_start_addr         = 0x40,
	.rom_header_main_oem_end_addr           = 0x44,
	.rom_header_main_awb_start_addr         = 0x50,
	.rom_header_main_awb_end_addr           = 0x54,
	.rom_header_main_shading_start_addr     = 0x58,
	.rom_header_main_shading_end_addr       = 0x5C,
	.rom_header_main_sensor_cal_start_addr  = 0x18,
	.rom_header_main_sensor_cal_end_addr    = 0x1C,
	.rom_header_dual_cal_start_addr         = -1,
	.rom_header_dual_cal_end_addr           = -1,
	.rom_header_pdaf_cal_start_addr         = 0x48,
	.rom_header_pdaf_cal_end_addr           = 0x4C,
	.rom_header_ois_cal_start_addr          = -1,
	.rom_header_ois_cal_end_addr            = -1,

	.rom_header_sub_oem_start_addr          = -1,
	.rom_header_sub_oem_end_addr            = -1,
	.rom_header_sub_awb_start_addr          = -1,
	.rom_header_sub_awb_end_addr            = -1,
	.rom_header_sub_shading_start_addr      = -1,
	.rom_header_sub_shading_end_addr        = -1,

	.rom_header_main_mtf_data_addr          = -1,
	.rom_header_sub_mtf_data_addr           = -1,

	.rom_header_checksum_addr               = 0x00FC,
	.rom_header_checksum_len                = REAR_S5KJN1_HEADER_CHECKSUM_LEN,

	.rom_oem_af_inf_position_addr           = 0x2FAC,
	.rom_oem_af_macro_position_addr         = 0x2FB8,
	.rom_oem_module_info_start_addr         = -1,
	.rom_oem_checksum_addr                  = 0x2FEC,
	.rom_oem_checksum_len                   = REAR_S5KJN1_OEM_CHECKSUM_LEN,

	.rom_module_cal_data_start_addr         = -1,
	.rom_module_module_info_start_addr      = -1,
	.rom_module_checksum_addr               = -1,
	.rom_module_checksum_len                = -1,

	.rom_awb_cal_data_start_addr            = 0x3800,
	.rom_awb_module_info_start_addr         = -1,
	.rom_awb_checksum_addr                  = 0x381C,
	.rom_awb_checksum_len                   = REAR_S5KJN1_AWB_CHECKSUM_LEN,

	.rom_shading_cal_data_start_addr        = 0x3820,
	.rom_shading_module_info_start_addr     = -1,
	.rom_shading_checksum_addr              = 0x3C1C,
	.rom_shading_checksum_len               = REAR_S5KJN1_SHADING_CHECKSUM_LEN,

	.rom_sensor_cal_module_info_start_addr  = -1,
	.rom_sensor_cal_checksum_addr           = 0x224C,
	.rom_sensor_cal_checksum_len            = REAR_S5KJN1_SENSOR_CAL_CHECKSUM_LEN,

	.rom_dual_module_info_start_addr        = -1,
	.rom_dual_checksum_addr                 = -1,
	.rom_dual_checksum_len                  = REAR_S5KJN1_DUAL_CHECKSUM_LEN,

	.rom_pdaf_module_info_start_addr        = 0x2FF0,
	.rom_pdaf_checksum_addr                 = 0x37FC,
	.rom_pdaf_checksum_len                  = REAR_S5KJN1_PDAF_CHECKSUM_LEN,
	.rom_ois_checksum_addr                  = -1,
	.rom_ois_checksum_len                   = REAR_S5KJN1_OIS_CHECKSUM_LEN,

	.rom_sub_oem_af_inf_position_addr       = -1,
	.rom_sub_oem_af_macro_position_addr     = -1,
	.rom_sub_oem_module_info_start_addr     = -1,
	.rom_sub_oem_checksum_addr              = -1,
	.rom_sub_oem_checksum_len               = -1,

	.rom_sub_awb_module_info_start_addr     = -1,
	.rom_sub_awb_checksum_addr              = -1,
	.rom_sub_awb_checksum_len               = -1,

	.rom_sub_shading_module_info_start_addr = -1,
	.rom_sub_shading_checksum_addr          = -1,
	.rom_sub_shading_checksum_len           = -1,

	.rom_dual_cal_data2_start_addr          = -1,
	.rom_dual_cal_data2_size                = -1,
	.rom_dual_tilt_x_addr                   = -1,
	.rom_dual_tilt_y_addr                   = -1,
	.rom_dual_tilt_z_addr                   = -1,
	.rom_dual_tilt_sx_addr                  = -1,
	.rom_dual_tilt_sy_addr                  = -1,
	.rom_dual_tilt_range_addr               = -1,
	.rom_dual_tilt_max_err_addr             = -1,
	.rom_dual_tilt_avg_err_addr             = -1,
	.rom_dual_tilt_dll_version_addr         = -1,
	.rom_dual_tilt_dll_modelID_addr         = -1,
	.rom_dual_tilt_dll_modelID_size         = -1,
	.rom_dual_shift_x_addr                  = -1,
	.rom_dual_shift_y_addr                  = -1,


	.crosstalk_cal_addr                     = &rear_s5kjn1_tetraxtc_addr,
	.sac_cal_addr                           = &rear_s5kjn1_sac_addr,
	.ois_cal_addr                           = &rear_s5kjn1_ois_addr,
	.extend_cal_addr                        = NULL,

	.converted_cal_addr                     = &rear_s5kjn1_converted_cal_addr,
	.rom_converted_max_cal_size             = REAR_S5KJN1_CONVERTED_MAX_CAL_SIZE,

	.sensor_maker                           = "SLSI",
	.sensor_name                            = "S5KJN1",
	.sub_sensor_maker                       = NULL,
	.sub_sensor_name                        = NULL,

	.bayerformat                            = SENSOR_OUTPUT_FORMAT_RAW_4CELL_Gr,
};
