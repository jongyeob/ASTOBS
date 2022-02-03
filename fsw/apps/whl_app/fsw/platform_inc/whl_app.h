/*******************************************************************************
** File: sample_app.h
**
** Purpose:
**   This file is main hdr file for the SAMPLE application.
**
**
*******************************************************************************/

#ifndef _whl_app_h_
#define _whl_app_h_

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "whl_app_defs.h"
#include "whl_app_msg.h"
#include "whl_app_msgids.h"

/***********************************************************************/

#define WHL_PIPE_DEPTH                     32

#define WHL_DEVICE_WAIT		0
#define WHL_DEVICE_ROTATING 1
#define WHL_DEVICE_STOPPING 2
#define WHL_DEVICE_HOMING	3

#define WHL_HOMING_WAIT				0
#define WHL_HOMING_SENSOR_UP		1
#define WHL_HOMING_SENSOR_ON		2
#define	WHL_HOMING_SENSOR_DOWN		3
#define WHL_HOMING_MOVE_ZERO		4

#define WHL_ROTATE_RELATIVE			0
#define WHL_ROTATE_ABSOLUTE			1

#define WHL_STOP_NORMAL				0
#define WHL_STOP_EMERGENCY			1

#define WHL_SENSOR_OFF			1
#define WHL_SENSOR_ON			0

#define WHL_MOTION_OFF			0
#define WHL_MOTION_ON			1

#define WHL_BANDPASS_AXIS		1
#define WHL_POLARIZER_AXIS      2




/************************************************************************
** Type Definitions
*************************************************************************/
typedef struct
{
	int32 velocity;
	int32 encoder_position;
	int32 encoder_position_angle;

	int8 home;
	int8 moving;


} WHL_DeviceStatus_t;

typedef struct
{
	int32   fd;
	uint8   axis;

	int32	zero;
	int32 home_start_velocity;
	int32 home_final_velocity;
	int32 home_last_velocity;
	int32 target_position_angle;
	int32 target_position;
	int32 target_velocity;
	int32 target_acceleration;
	int32 last_velocity;
	int32 last_acceleration;
	int32 last_home_position;


	uint8 mode;
	uint8 home_mode;

	uint32 pulse_per_rev;

} WHL_Device_t;

typedef WHL_Device_t* WHL_DevicePtr_t;


/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (SAMPLE_AppMain), these
**       functions are not called from any other source module.
*/
void WHL_AppMain(void);
void WHL_AppInit(void);
void WHL_ProcessCommandPacket(void);
void WHL_ProcessGroundCommand(void);
void WHL_ReportHousekeeping(void);
void WHL_ResetCounters(void);

boolean WHL_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

int32 WHL_OpenDevice(char* port_name);
void WHL_CloseDevice(int fd);
void  WHL_InitDevice(int32 fd);
void  WHL_ResetDevice(WHL_Device_t* device);
void WHL_ConfigureDevice(WHL_MotionConfig_t* config);
WHL_Device_t*  WHL_GetDevice(uint8 axis);

WHL_DeviceStatus_t WHL_GetStatus(WHL_Device_t* device);

void WHL_SetZero(WHL_Device_t* device);
void WHL_Rotate(WHL_Device_t* device, int32 angle,uint8 mode);
void WHL_RotateContinous(WHL_Device_t* device, int32 vel);
void WHL_Home(WHL_Device_t* device);
void WHL_Stop(WHL_Device_t* device,uint8 mode);


void WHL_ProcessRotating(WHL_Device_t* device);
void WHL_ProcessHoming(WHL_Device_t* device);
void WHL_ProcessStopping(WHL_Device_t* device);



#endif /* _sample_app_h_ */
