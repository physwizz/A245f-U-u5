/* SPDX-License-Identifier: BSD-2-Clause */
/*
 * Copyright (c) 2023 MediaTek Inc.
 */

/*! \file   "gl_sys.h"
 *    \brief  Functions for sysfs driver and others
 *
 *    Functions for sysfs driver and others
 */

#ifndef _GL_SYS_H
#define _GL_SYS_H


/*******************************************************************************
 *                              F U N C T I O N S
 *******************************************************************************
 */
#if CFG_EXT_FEATURE
u_int8_t glIsWiFi7CfgFile(void);
#endif

#ifndef CFG_HDM_WIFI_SUPPORT
#define CFG_HDM_WIFI_SUPPORT 0
#endif

#if CFG_HDM_WIFI_SUPPORT
void HdmWifi_SysfsInit(void);
void HdmWifi_SysfsUninit(void);
#endif

struct INI_PARSE_STATE_S {
	int8_t *ptr;
	int8_t *text;
	uint32_t textsize;
	int32_t nexttoken;
	uint32_t maxSize;
};

extern char * const apucRstReason[RST_REASON_MAX];
#if IS_ENABLED(CFG_MTK_WIFI_CONNV3_SUPPORT)
extern void connv3_coredump_set_memdump_mode(unsigned int mode);
#endif

void iniFileErrorCheck(struct ADAPTER *prAdapter,
	uint8_t **ppucIniBuf, uint32_t *pu4ReadSize);
void iniTableParsing(uint32_t *);
uint32_t iniFileParsing(uint8_t *, uint32_t);
int32_t iniFindNextToken(struct INI_PARSE_STATE_S *state);

#endif /* _GL_SYS_H */