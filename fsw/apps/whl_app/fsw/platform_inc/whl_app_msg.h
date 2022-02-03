/*******************************************************************************
** File:
**   sample_app_msg.h 
**
** Purpose: 
**  Define SAMPLE App  Messages and info
**
** Notes:
**
**
*******************************************************************************/
#ifndef _whl_app_msg_h_
#define _whl_app_msg_h_

/*
** SAMPLE App command codes
*/
#define WHL_APP_NOOP_CC                 0
#define WHL_APP_RESET_COUNTERS_CC       1

#define WHL_APP_MOTION_STOP_CC			2
#define WHL_APP_MOTION_ROTATE_CC		3
#define WHL_APP_MOTION_HOME_CC			4
#define WHL_APP_MOTION_RESET_CC			5
#define WHL_APP_MOTION_ZERO_CC			6
#define WHL_APP_MOTION_CONFIG_CC		7
#define WHL_APP_MOTION_DIAGNOSE_CC	8

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} WHL_NoArgsCmd_t;

/*************************************************************************/
/*
** Type definition (SAMPLE App housekeeping)
*/
typedef struct 
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];

	int32   fd;

	int32   target_position_angle[WHL_DEVICE_COUNT];
	int32	target_position[WHL_DEVICE_COUNT];
	int32	target_velocity[WHL_DEVICE_COUNT];
	int32   target_acceleration[WHL_DEVICE_COUNT];

	int32   velocity[WHL_DEVICE_COUNT];
	int32   encoder_position[WHL_DEVICE_COUNT];
	int32   encoder_position_angle[WHL_DEVICE_COUNT];

	int8 	moving[WHL_DEVICE_COUNT];
	uint8 	mode[WHL_DEVICE_COUNT];
	uint8 	home_mode[WHL_DEVICE_COUNT];
	uint8   axis_num;

    uint8   command_error_count;
    uint8   command_count;

}   OS_PACK whl_hk_tlm_t  ;

#define WHL_APP_HK_TLM_LNGTH   sizeof ( whl_hk_tlm_t )

typedef struct
{
    uint8			TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint8			axis;
    int32			target_position;
    int32			position;

}  WHL_RotateFinishTlm_t  ;

/*
 * Type definition
 */

typedef struct
{
	uint8	CmdHeader[CFE_SB_CMD_HDR_SIZE];
	int32	position;
	uint8	axis;
	uint8   mode;
	uint8   spare[2];
} WHL_MotionCmd_t;

typedef WHL_MotionCmd_t* WHL_MotionCmdPtr_t;

typedef struct
{
	uint8 CmdHeader[CFE_SB_CMD_HDR_SIZE];
	uint8 axis;
	uint8 velocity;
	uint8 accereleate;
	uint8 padding;
} WHL_MotionConfig_t;


#endif /* _sample_app_msg_h_ */

/************************/
/*  End of File Comment */
/************************/
