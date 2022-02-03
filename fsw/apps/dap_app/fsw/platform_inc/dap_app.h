/*******************************************************************************
** File: dap_app.h
**
** Purpose:
**   This file is main hdr file for the SAMPLE application.
**
**
*******************************************************************************/

#ifndef _dap_app_h_
#define _dap_app_h_

/*
** Required header files.
*/
#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "dap_app_msgids.h"
#include "dap_app_msg.h"

#include "cam_app_defs.h"
#include "cam_app_msgids.h"
#include "cam_app_msg.h"


/***********************************************************************/

#define DAP_PIPE_DEPTH                     32

#define DAP_QV_IMAGE_WIDTH			   	   256
#define DAP_QV_IMAGE_HEIGHT	     	   	   256
#define DAP_QV_IMAGE_BITPIX		           8
#define DAP_QV_IMAGE_SIZE			  (DAP_QV_IMAGE_WIDTH*DAP_QV_IMAGE_HEIGHT)
#define DAP_QV_IMAGE_BYTESIZE      	  (DAP_QV_IMAGE_SIZE*DAP_QV_IMAGE_BITPIX/8)




/************************************************************************
** Type Definitions
*************************************************************************/
typedef struct
{
	uint32 width;
	uint32 height;
	int8   bitpix;
	uint32 exposure_time;


}DAP_ImageHeader_t;

typedef struct
{
	DAP_ImageHeader_t header;
	uint8*	data;
}DAP_Image_t;


typedef struct
{
	uint8			quickview_enable;
	uint8			save_enable;
	uint8           image_analysis_enable;
	CAM_APP_IMAGE_t*    image;
	uint32 			frame_number;
	uint32 			optimal_exposure_time;
	uint32 			frame_pixel_min;
	uint32 			frame_pixel_max;
	uint32 			frame_pixel_mean;

}DAP_AppData_t;


/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (SAMPLE_AppMain), these
**       functions are not called from any other source module.
*/
void DAP_AppMain(void);
void DAP_AppInit(void);
void DAP_ProcessCommandPacket(void);
void DAP_ProcessGroundCommand(void);
void DAP_ProcessImageTelemetry(CAM_ImageTlm_t* tlm);
void DAP_ReportHousekeeping(void);
void DAP_ReportImageProcessingTelemetry(void);
void DAP_ResetCounters(void);

boolean DAP_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

void DAP_Config(DAP_ConfigCmd_t* command);

void DAP_SaveImage(CAM_APP_IMAGE_t* image);
void DAP_AnalyzeImage(DAP_Image_t* image);
void DAP_SendQvImage();
int8 DAP_CopyImage(DAP_Image_t* src, DAP_Image_t* dst, double min, double max);

int8 DAP_GetImageBuffer(DAP_Image_t** image_buffer, DAP_ImageHeader_t header);

void DAP_DeleteCallBack();


#endif /* _dap_app_h_ */
