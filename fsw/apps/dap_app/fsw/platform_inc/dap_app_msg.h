/*******************************************************************************
** File:
**   dap_app_msg.h
**
** Purpose: 
**  Define DAP App  Messages and info
**
** Notes:
**
**
*******************************************************************************/
#ifndef _dap_app_msg_h_
#define _dap_app_msg_h_

/*
** DAP App command codes
*/
#define DAP_APP_NOOP_CC                 0
#define DAP_APP_RESET_COUNTERS_CC       1
#define DAP_APP_CONFIG_CC			   2

/*************************************************************************/
/*
** Type definition (generic "no arguments" command)
*/
typedef struct
{
   uint8    CmdHeader[CFE_SB_CMD_HDR_SIZE];

} DAP_NoArgsCmd_t;

/*************************************************************************/
/*
** Type definition (DAP App housekeeping)
*/
typedef struct 
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
	uint32             observation_time;
	uint32             exposure_time;
	uint32             frame_number;
	int32              polarization_position_angle;
	int32              bandpass_position_angle;
	uint32 			   optimal_exposure_time;
	uint32 			   frame_pixel_min;
	uint32 			   frame_pixel_max;
	uint32 			   frame_pixel_mean;
	int8               temperature;
	uint8              observation_mode;
} OS_PACK DAP_ImageProcessingTlm_t;

typedef struct
{
	uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
	uint8			quickview_enable;
	uint8			save_enable;
	uint8           image_analysis_enable;

    uint8           command_error_count;
    uint8           command_count;

}   OS_PACK DAP_HkTlm_t  ;

#define DAP_APP_HK_TLM_LNGTH   sizeof ( DAP_HkTlm_t )

typedef struct
{
	uint8			CmdHeader[CFE_SB_CMD_HDR_SIZE];
	uint8			quickview_enable;
	uint8			save_enable;
	uint8           image_analysis_enable;
} DAP_ConfigCmd_t;

#define DAP_APP_QV_BUFFER_SIZE 2048
typedef struct
{
	uint8			TlmHeader[CFE_SB_TLM_HDR_SIZE];
	uint32			frame_number;
	uint32			frame_position;
	uint8			data[DAP_APP_QV_BUFFER_SIZE];

} DAP_QuickviewTlm_t;
#define DAP_APP_QV_TLM_LNGTH sizeof (DAP_QuickviewTlm_t)

typedef struct
{
	uint8			TlmHeader[CFE_SB_TLM_HDR_SIZE];
	uint32			exposure_time;
} DAP_AutoExposureTimeTlm_t;
#define DAP_APP_AE_TLM_LNGTH sizeof (DAP_AutoExposureTimeTlm_t)


#endif /* _dap_app_msg_h_ */

/************************/
/*  End of File Comment */
/************************/
