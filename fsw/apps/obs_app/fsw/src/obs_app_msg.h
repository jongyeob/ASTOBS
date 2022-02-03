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
#ifndef _obs_app_msg_h_
#define _obs_app_msg_h_

/*
** SAMPLE App command codes
*/
#define OBS_APP_NOOP_CC                      0
#define OBS_APP_RESET_COUNTERS_CC      		1
#define OBS_APP_START_SEQUENCE_CC			2
#define OBS_APP_STOP_SEQUENCE_CC				3


/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} OBS_NoArgsCmd_t;
/*************************************************************************/
/*
** Type definition (OBS App Configuration)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

   uint32 	sequence_start_time;
   uint32 	sequence_end_time;
   uint16	sequence_repeat;


   uint8	sequence_id;


} OBS_SequenceConfig_t;
/*************************************************************************/
/*
** Type definition (SAMPLE App housekeeping)
*/
typedef struct 
{
    uint8			TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint32			sequence_start_time;
    uint32			sequence_end_time;
	uint32			frame_start_sec;
	uint32			frame_start_msec;
	int32  			frame_remain_time;
	int32  			frame_elapse_msec;

	uint16			sequence_repeat;
	uint16			sequence_repeat_status;

	uint8			sequence_id;
	uint8  			sequence_mode;
	uint8  			wheel_motion_status;
    uint8			command_error_count;
    uint8			command_count;

}   OS_PACK OBS_HkTlm_t  ;

#define OBS_APP_HK_TLM_LNGTH   sizeof ( OBS_HkTlm_t )


#endif /* _sample_app_msg_h_ */

/************************/
/*  End of File Comment */
/************************/
