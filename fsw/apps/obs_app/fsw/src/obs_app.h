/*******************************************************************************
** File: sample_app.h
**
** Purpose:
**   This file is main hdr file for the SAMPLE application.
**
**
*******************************************************************************/

#ifndef _obs_app_h_
#define _obs_app_h_

/*
** Required header files.
*/

#include "cfe.h"
#include "cfe_error.h"
#include "cfe_evs.h"
#include "cfe_sb.h"
#include "cfe_es.h"

#include "obs_app_msg.h"
#include "obs_app_msgids.h"


/***********************************************************************/

#define OBS_PIPE_DEPTH                     32

#define OBS_WAIT 			0
#define OBS_ONTIME			1

#define OBS_MODE_WAIT		 0
#define OBS_MODE_READY		 1
#define OBS_MODE_ROTATE		 2
#define OBS_MODE_ROTATING	 3
#define OBS_MODE_GRAB		 4
#define OBS_MODE_GRABING	      5
#define OBS_MODE_GRAB_STOPPING 6
#define OBS_MODE_POST_PROCESS  7
#define OBS_MODE_STOP		  8

#define OBS_OPTION_AE_ENABLE 1

#define OBS_SEQUENCE_ONCE    0
#define OBS_SEQUENCE_REPEAT  1

#define OBS_SEQUENCE_MAX		10

#define OBS_SEQUENCE_ID_ECLIPSE 0

#define OBS_SEQUENCE_ID_BASE 100

#define OBS_DEFAULT_SEQUENCE_FILEPATH "/cf/apps/default.seq"

/************************************************************************
** Type Definitions
*************************************************************************/


typedef struct OBS_Sequence_t OBS_Sequence_t;

struct OBS_Sequence_t
{

	uint16 bandpass_position;
	uint16 polarizer_position;
	uint16 repeat_number;
	uint16 option;
	uint32 duration_time; // milliseconds
	uint32 exposure_time; // milliseconds

	OBS_Sequence_t* next;
};

typedef struct
{
	uint8  sequence_id;
	OBS_Sequence_t* sequence;
	uint8  sequence_mode;
	uint8  wheel_motion_status;
	uint32 frame_start_sec;
	uint32 frame_start_msec;
	int32  frame_remain_time;
	int32  frame_elapse_msec;
	uint32 sequence_start_time;
	uint32 sequence_end_time;
	uint16 sequence_repeat;
	uint16 sequence_repeat_status;


} OBS_AppData_t;

OBS_Sequence_t  sequence_list[OBS_SEQUENCE_MAX];


/****************************************************************************/
/*
** Local function prototypes.
**
** Note: Except for the entry point (SAMPLE_AppMain), these
**       functions are not called from any other source module.
*/
void OBS_AppMain(void);
void OBS_AppInit(void);
void OBS_ProcessCommandPacket(void);
void OBS_ProcessObservationSequence(void);
void OBS_ProcessGroundCommand(void);
void OBS_ReportHousekeeping(void);
void OBS_ResetCounters(void);

boolean OBS_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength);

void OBS_StartSequence(OBS_SequenceConfig_t* msg);
void OBS_StopSequence();
uint8 OBS_CheckObservationTime();

OBS_Sequence_t*  OBS_GetSequence(uint8 id);
OBS_Sequence_t* OBS_LoadSequenceFile(char* filepath);
void OBS_ReleaseSequence(OBS_Sequence_t* sequence);

void OBS_RotateWheel(uint8 axis, int32 position);

void CAM_DeleteCallBack();

#endif /* _sample_app_h_ */
