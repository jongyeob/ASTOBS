/*******************************************************************************
** File:
**   cam_app_msg.h
**
** Purpose: 
**  Define CAM App  Messages and info
**
** Notes:
**
**
*******************************************************************************/
#ifndef _cam_app_msg_h_
#define _cam_app_msg_h_

/*
** CAM App command codes
*/
#define CAM_APP_NOOP_CC                 0
#define CAM_APP_RESET_COUNTERS_CC       1
#define CAM_APP_GRAB_CC				2
#define CAM_APP_CONFIG_CC			3
#define CAM_APP_GRAB_STOP_CC			4

struct CAM_APP_IMAGE_HEADER_t;
/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} CAM_NoArgsCmd_t;

/*************************************************************************/
/*
** Type definition (CAM App housekeeping)
*/
typedef struct 
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];

	int32   handle;
	uint32 frame_width;
	uint32 frame_height;
	uint32 exposure_time;
	int32  polarization_angle;
	int32  bandpass_angle;

	uint32 frame_number;
	uint32 auto_exposure_time;

	uint32 target_repeat;
	uint32 current_repeat;

	float   target_temperature;
	float   temperature;
	double  auto_exposure_adjust;

	uint16 camera_offset_x;
	uint16 camera_offset_y;

	uint8  mode;
	uint8  observation_mode;
	uint8  auto_exposure_enable;
	int8   frame_bitpix;

	uint8  camera_bin_x;
	uint8  camera_bin_y;
    uint8  command_error_count;
    uint8  command_count;


}   OS_PACK CAM_HkTlm_t  ;

#define CAM_APP_HK_TLM_LNGTH   sizeof ( CAM_HkTlm_t )

typedef struct
{
	uint8			  TlmHeader[CFE_SB_TLM_HDR_SIZE];
}CAM_RepeatFinishTlm_t;

typedef struct
{
	uint8			  CmdHeader[CFE_SB_CMD_HDR_SIZE];
	double			  auto_exposure_adjust;
	uint32 			  exposure_time;
	uint8			  observation_mode;
	uint8			  auto_exposure_enable;
	uint8			  repeat;
	uint8			  spare;

}CAM_GrabCmd_t;

typedef struct
{
	uint8			  CmdHeader[CFE_SB_CMD_HDR_SIZE];
	uint16		      width;
	uint16			  height;
	uint16 			  offset_x;
	uint16            offset_y;
	uint8 			  bin_x;
	uint8 			  bin_y;
	int8				  temperature;
	uint8			  spare;

}CAM_ConfigCmd_t;

typedef struct
{
	uint8			  TlmHeader[CFE_SB_TLM_HDR_SIZE];
	CAM_APP_IMAGE_t   image;
}CAM_ImageTlm_t;

#endif /* _cam_app_msg_h_ */

/************************/
/*  End of File Comment */
/************************/
