#include <stdlib.h>

#include <stdtype.h>
#include "../EmuStructs.h"
#include "../EmuCores.h"

#include "opnintf.h"
#include "fmopn.h"


static UINT8 device_start_ym2203(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_ym2203(void* param);
static UINT8 device_start_ym2608(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_ym2608(void* param);
static UINT8 device_start_ym2610(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf);
static void device_stop_ym2610(void* param);

static void ssg_set_clock(void* param, UINT32 clock);
static void ssg_write(void* param, UINT8 address, UINT8 data);
static UINT8 ssg_read(void* param);
static void ssg_reset(void* param);


typedef struct _opn_info
{
	void* opn;
	void* ssg;
} OPN_INF;


static DEVDEF_RWFUNC devFunc_MAME_2203[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, ym2203_write},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, ym2203_read},
};
static DEV_DEF devDef_MAME_2203 =
{
	"YM2203", "MAME", FCC_MAME,
	
	device_start_ym2203,
	device_stop_ym2203,
	ym2203_reset_chip,
	ym2203_update_one,
	
	NULL,	// SetOptionBits
	ym2203_set_mutemask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	
	2, devFunc_MAME_2203,	// rwFuncs
};

const DEV_DEF* devDefList_YM2203[] =
{
	&devDef_MAME_2203,
	NULL
};

static DEVDEF_RWFUNC devFunc_MAME_2608[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, ym2608_write},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, ym2608_read},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'B', ym2608_write_pcmromb},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'B', ym2608_alloc_pcmromb},
};
static DEV_DEF devDef_MAME_2608 =
{
	"YM2608", "MAME", FCC_MAME,
	
	device_start_ym2608,
	device_stop_ym2608,
	ym2608_reset_chip,
	ym2608_update_one,
	
	NULL,	// SetOptionBits
	ym2608_set_mutemask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	
	4, devFunc_MAME_2608,	// rwFuncs
};

const DEV_DEF* devDefList_YM2608[] =
{
	&devDef_MAME_2608,
	NULL
};

static DEVDEF_RWFUNC devFunc_MAME_2610[] =
{
	{RWF_REGISTER | RWF_WRITE, DEVRW_A8D8, 0, ym2610_write},
	{RWF_REGISTER | RWF_READ, DEVRW_A8D8, 0, ym2610_read},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'A', ym2610_write_pcmroma},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'A', ym2610_alloc_pcmroma},
	{RWF_MEMORY | RWF_WRITE, DEVRW_BLOCK, 'B', ym2610_write_pcmromb},
	{RWF_MEMORY | RWF_WRITE, DEVRW_MEMSIZE, 'B', ym2610_alloc_pcmromb},
};
static DEV_DEF devDef_MAME_2610 =
{
	"YM2610", "MAME", FCC_MAME,
	
	device_start_ym2610,
	device_stop_ym2610,
	ym2610_reset_chip,
	ym2610_update_one,
	
	NULL,	// SetOptionBits
	ym2610_set_mutemask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	
	6, devFunc_MAME_2610,	// rwFuncs
};
static DEV_DEF devDef_MAME_2610B =
{
	"YM2610B", "MAME", FCC_MAME,
	
	device_start_ym2610,
	device_stop_ym2610,
	ym2610_reset_chip,
	ym2610b_update_one,
	
	NULL,	// SetOptionBits
	ym2610_set_mutemask,
	NULL,	// SetPanning
	NULL,	// SetSampleRateChangeCallback
	
	2, devFunc_MAME_2610,	// rwFuncs
};

const DEV_DEF* devDefList_YM2610[] =
{
	&devDef_MAME_2610,
	NULL
};


static const ssg_callbacks ssgintf =
{
	ssg_set_clock,
	ssg_write,
	ssg_read,
	ssg_reset
};

static UINT8 device_start_ym2203(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	OPN_INF* info;
	DEV_DATA* devData;
	UINT32 clock;
	UINT32 rate;
	
	clock = CHPCLK_CLOCK(cfg->clock);
	rate = clock / 72;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, rate, cfg->smplRate);
	
	info = (OPN_INF*)malloc(sizeof(OPN_INF));
	info->ssg = NULL;
	info->opn = ym2203_init(info, clock, rate, NULL, NULL, &ssgintf);
	
	devData = (DEV_DATA*)info->opn;
	devData->chipInf = info;	// store pointer to OPN_INF into sound chip structure
	retDevInf->dataPtr = devData;
	retDevInf->sampleRate = rate;
	retDevInf->devDef = &devDef_MAME_2203;
	return 0x00;
}

static void device_stop_ym2203(void* param)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	
	ym2203_shutdown(info->opn);
	free(info);
	
	return;
}

void ym2203_update_request(void* param)
{
	devDef_MAME_2203.Update(param, 0, NULL);
}

static UINT8 device_start_ym2608(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	OPN_INF* info;
	DEV_DATA* devData;
	UINT32 clock;
	UINT32 rate;
	
	clock = CHPCLK_CLOCK(cfg->clock);
	rate = clock / 72 / 2;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, rate, cfg->smplRate);
	
	info = (OPN_INF*)malloc(sizeof(OPN_INF));
	info->ssg = NULL;
	info->opn = ym2608_init(info, clock, rate, NULL, NULL, &ssgintf);
	
	devData = (DEV_DATA*)info->opn;
	devData->chipInf = info;	// store pointer to OPN_INF into sound chip structure
	retDevInf->dataPtr = devData;
	retDevInf->sampleRate = rate;
	retDevInf->devDef = &devDef_MAME_2608;
	return 0x00;
}

static void device_stop_ym2608(void* param)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	
	ym2608_shutdown(info->opn);
	free(info);
	
	return;
}

static UINT8 device_start_ym2610(const DEV_GEN_CFG* cfg, DEV_INFO* retDevInf)
{
	OPN_INF* info;
	DEV_DATA* devData;
	UINT32 clock;
	UINT8 mode;
	UINT32 rate;
	
	clock = CHPCLK_CLOCK(cfg->clock);
	mode = CHPCLK_FLAG(cfg->clock);
	rate = clock / 72 / 2;
	SRATE_CUSTOM_HIGHEST(cfg->srMode, rate, cfg->smplRate);
	
	info = (OPN_INF*)malloc(sizeof(OPN_INF));
	info->ssg = NULL;
	info->opn = ym2610_init(info, clock, rate, NULL, NULL, &ssgintf);
	
	devData = (DEV_DATA*)info->opn;
	devData->chipInf = info;	// store pointer to OPN_INF into sound chip structure
	retDevInf->dataPtr = devData;
	retDevInf->sampleRate = rate;
	retDevInf->devDef = mode ? &devDef_MAME_2610B : &devDef_MAME_2610;
	return 0x00;
}

static void device_stop_ym2610(void* param)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	
	ym2610_shutdown(info->opn);
	free(info);
	
	return;
}

void ym2608_update_request(void* param)
{
	devDef_MAME_2608.Update(param, 0, NULL);
}

void ym2610_update_request(void* param)
{
	devDef_MAME_2610B.Update(param, 0, NULL);
}

static void ssg_set_clock(void* param, UINT32 clock)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	if (info->ssg == NULL)
		return;
	
	//ay8910_set_clock_ym(info->ssg, clock);
	return;
}

static void ssg_write(void* param, UINT8 address, UINT8 data)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	if (info->ssg == NULL)
		return;
	
	//ay8910_write_ym(info->ssg, address, data);
	return;
}

static UINT8 ssg_read(void* param)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	if (info->ssg == NULL)
		return 0x00;
	
	//return ay8910_read_ym(info->ssg);
	return 0x00;
}

static void ssg_reset(void* param)
{
	DEV_DATA* devData = (DEV_DATA*)param;
	OPN_INF* info = (OPN_INF*)devData->chipInf;
	if (info->ssg == NULL)
		return;
	
	//ay8910_reset_ym(info->ssg);
	return;
}