/*******************************************************************************
** File: cam_app.h
**
** Purpose:
**   This file is main hdr file for the CAM application.
**
**
*******************************************************************************/

#ifndef _cam_app_h_
#define _cam_app_h_

/*
** Required header files.
*/

#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "cam_app_defs.h"
#include "cam_app_msgids.h"
#include "cam_app_msg.h"




/************************************************************************
** Type Definitions
*************************************************************************/

CAM_APP_DATA_t   CAM_AppData;
CAM_APP_DATA_t*  CAM_AppDataPtr = &CAM_AppData;

CAM_APP_IMAGE_t* CAM_ImagePtr;

CAM_APP_DEVICE_t CAM_Device;
CAM_APP_DEVICE_t* CAM_DevicePtr = &CAM_Device;

/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (CAM_AppMain), these
**       functions are not called from any other source module.
*/
void CAM_AppMain(void);
void CAM_AppInit(void);
void CAM_ProcessCommandPacket(void);
void CAM_ProcessGroundCommand(void);
void CAM_ProcessGrabCommand(CAM_GrabCmd_t* msg);
void CAM_ProcessConfigCommand(CAM_ConfigCmd_t* msg);
void CAM_ReportHousekeeping(void);
void CAM_ResetCounters(void);


int32 CAM_OpenDevice(char* camera_id);
void CAM_CloseDevice(CAM_APP_DEVICE_t* device);
void CAM_GetStatus(CAM_APP_DEVICE_t* device);
void CAM_SetDevice(CAM_APP_DEVICE_t* device);
int8 CAM_DownloadImage(CAM_APP_DEVICE_t* device, CAM_APP_IMAGE_t* image);

CAM_APP_IMAGE_t* CAM_GetNewImage(CAM_APP_DEVICE_t* device);
void CAM_ReleaseImage(CAM_APP_IMAGE_t** image);
CAM_APP_IMAGE_t* CAM_UpdateImage(CAM_APP_DEVICE_t* device, CAM_APP_IMAGE_t* image);


void CAM_GrabFrame(CAM_APP_DEVICE_t* device,CAM_APP_IMAGE_t* image);

#ifdef __EMULATED_DEVICE__
int8 CAM_EmulGrabFrame(CAM_APP_DEVICE_t* device,void* frame_buffer);
#endif

boolean CAM_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

void CAM_DeleteCallBack();

#endif /* _cam_app_h_ */
