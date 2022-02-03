/*******************************************************************************
 ** File: sample_app.c
 **
 ** Purpose:
 **   This file contains the source code for the Sample App.
 **
 *******************************************************************************/

/*
 **   Include Files:
 */
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "cam_app.h"
#include "whl_app.h"
#include "obs_app.h"

#include "obs_app_perfids.h"
#include "obs_app_events.h"
#include "obs_app_version.h"

/*
 ** global data
 */

OBS_HkTlm_t OBS_HkTelemetryPkt;
OBS_AppData_t OBS_AppData;
CFE_SB_PipeId_t OBS_CommandPipe;
CFE_SB_MsgPtr_t OBS_MsgPtr;

CAM_GrabCmd_t OBS_CamCmd;
WHL_MotionCmd_t OBS_WhlCmd;

OBS_Sequence_t* current_sequence;

static CFE_EVS_BinFilter_t OBS_EventFilters[] =
    { /* Event ID    mask */
        { OBS_RESERVED_EID, 0x0000 },
        { OBS_STARTUP_INF_EID, 0x0000 },
        { OBS_COMMAND_ERR_EID, 0x0000 },
        { OBS_COMMANDNOP_INF_EID, 0x0000 },
        { OBS_COMMANDRST_INF_EID, 0x0000 },

        { OBS_CONFIG_INF_EID, 0x0000 },
        { OBS_CHECK_OBSTIME_INF_EID, 0x0000 }, };

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* OBS_AppMain() -- Application entry point and main process loop          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void OBS_AppMain( void )
{
    int32 status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry( OBS_APP_PERF_ID );

    OBS_AppInit();
    /*
     ** OBS Runloop
     */
    CFE_TIME_SysTime_t current_time;

    //DAP_ControlCmd_t dataproc_command;

    while( CFE_ES_RunLoop( &RunStatus ) == TRUE )
    {
        CFE_ES_PerfLogExit( OBS_APP_PERF_ID );

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg( &OBS_MsgPtr, OBS_CommandPipe, 10 );

        if ( status == CFE_SUCCESS )
        {
            OBS_ProcessCommandPacket();
        }

        current_time = CFE_TIME_GetTime();

        if ( OBS_CheckObservationTime( current_time ) == OBS_ONTIME )
        {
            if ( OBS_AppData.sequence_mode == OBS_MODE_WAIT )
            {
                OBS_AppData.sequence_mode = OBS_MODE_READY;
                CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                    "Start Sequence at %d", current_time.Seconds );
            }

            if ( OBS_AppData.sequence_mode != OBS_MODE_STOP )
            {
                OBS_ProcessObservationSequence();
            }

        }
        else
        {

            if ( OBS_AppData.sequence_mode != OBS_MODE_WAIT )
            {

                CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                    "Finish Sequence mode=%d at %d", OBS_AppData.sequence_mode,
                    current_time.Seconds );

                OBS_AppData.sequence_mode = OBS_MODE_WAIT;

            }

        }

        CFE_ES_PerfLogEntry( OBS_APP_PERF_ID );

    }

    CFE_ES_ExitApp( RunStatus );

} /* End of OBS_AppMain() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* OBS_AppInit() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void OBS_AppInit( void )
{
    /*
     ** Register the app with Executive services
     */
    CFE_ES_RegisterApp();

    /*
     ** Register the events
     */
    CFE_EVS_Register( OBS_EventFilters,
        sizeof( OBS_EventFilters ) / sizeof(CFE_EVS_BinFilter_t),
        CFE_EVS_BINARY_FILTER );

    /*
     ** Create the Software Bus command pipe and subscribe to housekeeping
     **  messages
     */
    CFE_SB_CreatePipe( &OBS_CommandPipe, OBS_PIPE_DEPTH, "OBS_CMD_PIPE" );

    CFE_SB_Subscribe( OBS_APP_CMD_MID, OBS_CommandPipe );
    CFE_SB_Subscribe( OBS_APP_SEND_HK_MID, OBS_CommandPipe );
    CFE_SB_Subscribe( WHL_APP_ROTATE_FINISH_MID, OBS_CommandPipe );
    CFE_SB_Subscribe( CAM_APP_REPEAT_FINISH_MID, OBS_CommandPipe );

    OBS_ResetCounters();

    CFE_SB_InitMsg( &OBS_HkTelemetryPkt,
    OBS_APP_HK_TLM_MID,
    OBS_APP_HK_TLM_LNGTH, TRUE );
    CFE_SB_InitMsg( &OBS_CamCmd,
    CAM_APP_CMD_MID, sizeof( OBS_CamCmd ), TRUE );
    CFE_SB_InitMsg( &OBS_WhlCmd,
    WHL_APP_CMD_MID, sizeof( OBS_WhlCmd ), TRUE );

    OBS_AppData.sequence_id = 0;
    OBS_AppData.sequence = OBS_GetSequence( OBS_AppData.sequence_id );
    OS_printf( "sequence 0x%08X\n", OBS_AppData.sequence );

    CFE_EVS_SendEvent( OBS_STARTUP_INF_EID, CFE_EVS_INFORMATION,
        "OBS App Initialized. Version %d.%d.%d.%d",
        OBS_APP_MAJOR_VERSION,
        OBS_APP_MINOR_VERSION,
        OBS_APP_REVISION,
        OBS_APP_MISSION_REV );

} /* End of OBS_AppInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  OBS_ProcessCommandPacket                                        */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the OBS    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void OBS_ProcessCommandPacket( void )
{

    CFE_SB_MsgId_t MsgId;

    MsgId = CFE_SB_GetMsgId( OBS_MsgPtr );

    switch( MsgId )
    {
        case OBS_APP_CMD_MID:
            OBS_ProcessGroundCommand();
            break;

        case OBS_APP_SEND_HK_MID:
            OBS_ReportHousekeeping();
            break;

        case WHL_APP_ROTATE_FINISH_MID:
            if ( OBS_AppData.sequence_mode == OBS_MODE_ROTATING )
            {
                WHL_RotateFinishTlm_t* tlm = (WHL_RotateFinishTlm_t*) OBS_MsgPtr;

                OBS_AppData.wheel_motion_status &= ~( 1 << tlm->axis );

                CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                    "Rotate Finish Wheel Axis:%d, Status:0x%d", tlm->axis,
                    OBS_AppData.wheel_motion_status );

                if ( OBS_AppData.wheel_motion_status <= 0 )
                {
                    OBS_AppData.sequence_mode = OBS_MODE_GRAB;
                    CFE_ES_PerfLogExit( OBS_APP_ROTATE_WHEEL_PERF_ID );

                }
            }
            break;

        case CAM_APP_REPEAT_FINISH_MID:

            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Repeat Finish" );

            if ( OBS_AppData.sequence_mode == OBS_MODE_GRABING
                || OBS_AppData.sequence_mode == OBS_MODE_GRAB_STOPPING )
            {
                OBS_AppData.sequence_mode = OBS_MODE_POST_PROCESS;
            }

            break;

        default:
            OBS_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent( OBS_COMMAND_ERR_EID, CFE_EVS_ERROR,
                "OBS: invalid command packet,MID = 0x%x", MsgId );
            break;
    }

    return;

} /* End OBS_ProcessCommandPacket */

void OBS_ProcessObservationSequence()
{
    CFE_TIME_SysTime_t current_time = CFE_TIME_GetTime();
    uint32 current_msec = CFE_TIME_Sub2MicroSecs( current_time.Subseconds )
        / 1000;

    OBS_AppData.frame_elapse_msec = ( current_time.Seconds
        - OBS_AppData.frame_start_sec ) * 1000 + current_msec
        - OBS_AppData.frame_start_msec;

    OBS_AppData.frame_remain_time = current_sequence->duration_time
        - OBS_AppData.frame_elapse_msec;

    switch( OBS_AppData.sequence_mode )
    {
        case OBS_MODE_READY:
            // Prepare Next Sequence
            CFE_ES_PerfLogEntry( OBS_APP_OBSERVATION_PERF_ID );
            OBS_AppData.frame_start_sec = current_time.Seconds;
            OBS_AppData.frame_start_msec = current_msec;

            OBS_AppData.sequence_mode = OBS_MODE_ROTATE;

            OS_printf( "Frame [0x%08X] Start secs=%u, msecs=%u, duration=%u\n",
                current_sequence, OBS_AppData.frame_start_sec,
                OBS_AppData.frame_start_msec, current_sequence->duration_time );

            break;
        case OBS_MODE_ROTATE:
            //Rotate Wheel to next position
            OBS_RotateWheel( WHL_BANDPASS_AXIS,
                current_sequence->bandpass_position );
            OBS_RotateWheel( WHL_POLARIZER_AXIS,
                current_sequence->polarizer_position );

            OBS_AppData.sequence_mode = OBS_MODE_ROTATING;

            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "Rotate Bandpass:%d, Polarizer:%d",
                current_sequence->bandpass_position,
                current_sequence->polarizer_position );

            CFE_ES_PerfLogEntry( OBS_APP_ROTATE_WHEEL_PERF_ID );

            break;

#ifdef __EMULATE_WHL__
            case OBS_MODE_ROTATING:
            usleep(150000);
            OBS_AppData.sequence_mode = OBS_MODE_GRAB;
            OBS_AppData.wheel_motion_status = 0;
            OS_printf("Wheel rotate finish!\n");
            CFE_ES_PerfLogExit(OBS_APP_ROTATE_WHEEL_PERF_ID);
            break;
#endif

        case OBS_MODE_GRAB:

            //Grab Frame Message

            OS_printf( "Frame_elapse_msec: %d, remain_msec: %d\n",
                OBS_AppData.frame_elapse_msec, OBS_AppData.frame_remain_time );

            OBS_CamCmd.repeat = current_sequence->repeat_number;
            OBS_CamCmd.exposure_time = current_sequence->exposure_time;
            OBS_CamCmd.observation_mode = OBS_SEQUENCE_ID_BASE
                + OBS_AppData.sequence_id;
            OBS_CamCmd.auto_exposure_enable = current_sequence->option
                & OBS_OPTION_AE_ENABLE;

            CFE_SB_SetCmdCode( (CFE_SB_Msg_t *) &OBS_CamCmd, CAM_APP_GRAB_CC );
            CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &OBS_CamCmd );

#ifndef __EMULATE_CAM__
            CFE_SB_SendMsg( (CFE_SB_Msg_t *) &OBS_CamCmd );
#endif

            OBS_AppData.sequence_mode = OBS_MODE_GRABING;

            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "Grab frame in obsmode:%d, exptime:%d, remain:%d",
                OBS_CamCmd.observation_mode, OBS_CamCmd.exposure_time,
                OBS_AppData.frame_remain_time );

            break;

        case OBS_MODE_GRABING:

            if ( current_sequence->duration_time > 0
                && OBS_AppData.frame_remain_time
                    < current_sequence->exposure_time )
            {
                CFE_SB_SetCmdCode( (CFE_SB_Msg_t *) &OBS_CamCmd,
                    CAM_APP_GRAB_STOP_CC );
                CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &OBS_CamCmd );
                CFE_SB_SendMsg( (CFE_SB_Msg_t *) &OBS_CamCmd );

                OBS_AppData.sequence_mode = OBS_MODE_GRAB_STOPPING;

            }

#ifdef __EMULATE_CAM__
            CFE_ES_PerfLogEntry(OBS_APP_GRAB_FRAME_PERF_ID);
            usleep(OBS_CamCmd.exposure_time*1000);
            OS_printf("Frame grab finish! exposure time: %d\n",OBS_CamCmd.exposure_time);
            OBS_AppData.sequence_mode = OBS_MODE_POST_PROCESS;
            CFE_ES_PerfLogExit(OBS_APP_GRAB_FRAME_PERF_ID);
#endif

            break;

        case OBS_MODE_POST_PROCESS:

            // Data Analyze and Save

            if ( OBS_AppData.frame_remain_time <= 0 )
            {
                current_sequence = current_sequence->next;
                OBS_AppData.sequence_mode = OBS_MODE_READY;

                if ( current_sequence == NULL )
                {
                    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                        "Sequence Observation Finished (%d/%d)",
                        OBS_AppData.sequence_repeat_status,
                        OBS_AppData.sequence_repeat );

                    current_sequence = OBS_AppData.sequence;

                    if ( OBS_AppData.sequence_repeat == 0
                        || OBS_AppData.sequence_repeat_status
                            < OBS_AppData.sequence_repeat )
                    {
                        current_sequence = OBS_AppData.sequence;
                        OBS_AppData.sequence_repeat_status++;
                        break;

                    }

                    OBS_AppData.sequence_mode = OBS_MODE_STOP;
                    CFE_ES_PerfLogExit( OBS_APP_OBSERVATION_PERF_ID );
                    return;
                }
            }

            break;

    }

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* OBS_ProcessGroundCommand() -- OBS ground commands                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void OBS_ProcessGroundCommand( void )
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode( OBS_MsgPtr );

    /* Process "known" OBS app ground commands */
    switch( CommandCode )
    {
        case OBS_APP_NOOP_CC:
            OBS_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( OBS_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                "OBS: NOOP command" );
            break;

        case OBS_APP_RESET_COUNTERS_CC:
            OBS_ResetCounters();
            break;

        case OBS_APP_START_SEQUENCE_CC:
            OBS_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( OBS_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                "Start Sequence Command" );
            OBS_StartSequence( (OBS_SequenceConfig_t*) OBS_MsgPtr );

            break;
        case OBS_APP_STOP_SEQUENCE_CC:
            OBS_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( OBS_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                "Stop Sequence Command" );
            OBS_StopSequence();

            break;

            /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of OBS_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  OBS_ReportHousekeeping                                              */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void OBS_ReportHousekeeping( void )
{

    OBS_HkTelemetryPkt.sequence_end_time = OBS_AppData.sequence_end_time;
    OBS_HkTelemetryPkt.sequence_start_time = OBS_AppData.sequence_start_time;
    OBS_HkTelemetryPkt.frame_start_sec = OBS_AppData.frame_start_sec;
    OBS_HkTelemetryPkt.frame_start_msec = OBS_AppData.frame_start_msec;
    OBS_HkTelemetryPkt.frame_remain_time = OBS_AppData.frame_remain_time;
    OBS_HkTelemetryPkt.frame_elapse_msec = OBS_AppData.frame_elapse_msec;
    OBS_HkTelemetryPkt.sequence_repeat = OBS_AppData.sequence_repeat;
    OBS_HkTelemetryPkt.sequence_repeat_status =
        OBS_AppData.sequence_repeat_status;
    OBS_HkTelemetryPkt.sequence_id = OBS_AppData.sequence_id;
    OBS_HkTelemetryPkt.sequence_mode = OBS_AppData.sequence_mode;
    OBS_HkTelemetryPkt.wheel_motion_status = OBS_AppData.wheel_motion_status;

    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &OBS_HkTelemetryPkt );
    CFE_SB_SendMsg( (CFE_SB_Msg_t *) &OBS_HkTelemetryPkt );
    return;

} /* End of OBS_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  OBS_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void OBS_ResetCounters( void )
{
    /* Status of commands processed by the OBS App */
    OBS_HkTelemetryPkt.command_count = 0;
    OBS_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent( OBS_COMMANDRST_INF_EID, CFE_EVS_INFORMATION,
        "OBS: RESET command" );
    return;

} /* End of OBS_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* OBS_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
boolean OBS_VerifyCmdLength( CFE_SB_MsgPtr_t msg, uint16 ExpectedLength )
{
    boolean result = TRUE;

    uint16 ActualLength = CFE_SB_GetTotalMsgLength( msg );

    /*
     ** Verify the command packet length.
     */
    if ( ExpectedLength != ActualLength )
    {
        CFE_SB_MsgId_t MessageID = CFE_SB_GetMsgId( msg );
        uint16 CommandCode = CFE_SB_GetCmdCode( msg );

        CFE_EVS_SendEvent( OBS_LEN_ERR_EID, CFE_EVS_ERROR,
            "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
            MessageID, CommandCode, ActualLength, ExpectedLength );
        result = FALSE;
        OBS_HkTelemetryPkt.command_error_count++;
    }

    return ( result );

} /* End of OBS_VerifyCmdLength() */

void OBS_StartSequence( OBS_SequenceConfig_t* msg )
{

    boolean result = TRUE;

    if ( OBS_AppData.sequence_id != msg->sequence_id )
    {
        if ( OBS_AppData.sequence != NULL )
        {
            OBS_ReleaseSequence( OBS_AppData.sequence );
            OBS_AppData.sequence = NULL;
        }

        OBS_AppData.sequence = OBS_GetSequence( msg->sequence_id );
        OBS_AppData.sequence_id = msg->sequence_id;

    }

    if ( OBS_AppData.sequence == NULL )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
            "Sequence id[%d] not loaded!", msg->sequence_id );
        return;
    }

    OBS_AppData.sequence_start_time = msg->sequence_start_time;
    OBS_AppData.sequence_end_time = msg->sequence_end_time;
    OBS_AppData.sequence_repeat = msg->sequence_repeat;
    OBS_AppData.sequence_repeat_status = 1;

    if ( msg->sequence_end_time == 0 )
    {
        OBS_AppData.sequence_end_time = -1;
    }

    current_sequence = OBS_AppData.sequence;
    OBS_AppData.sequence_mode = OBS_MODE_WAIT;
    OBS_AppData.wheel_motion_status = 0;

    CFE_EVS_SendEvent( OBS_CONFIG_INF_EID, CFE_EVS_INFORMATION,
        "Sequence ID:%d Addr:0x%08X Current:%u, Duration: [%u, %u]",
        OBS_AppData.sequence_id, OBS_AppData.sequence,
        CFE_TIME_GetTime().Seconds, OBS_AppData.sequence_start_time,
        OBS_AppData.sequence_end_time );

    return;

} /* End of OBS_ProcessConfig() */

void OBS_StopSequence()
{
    OBS_AppData.sequence_mode = OBS_MODE_STOP;

}

uint8 OBS_CheckObservationTime( CFE_TIME_SysTime_t time )
{

    uint8 check_time = OBS_WAIT;

    if ( time.Seconds > OBS_AppData.sequence_start_time
        && time.Seconds <= OBS_AppData.sequence_end_time )
    {
        check_time = OBS_ONTIME;

    }

    return check_time;

} /* End of OBS_ProcessConfig() */
OBS_Sequence_t* OBS_GetSequence( uint8 id )
{
    char filepath[256];

    sprintf( filepath, "/cf/apps/sequence%d.seq", id );

    return OBS_LoadSequenceFile( filepath );

}
OBS_Sequence_t* OBS_LoadSequenceFile( char* filepath )
{
    FILE* fr = fopen( filepath, "rt" );
    if ( fr == NULL )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "File Opening Failed!: %s",
            filepath );
        return 0;
    }
    OBS_Sequence_t new;
    OBS_Sequence_t *parent = NULL, *child, *current;
    uint32 total_sequence = 0;
    int32 read_vars = 0;

    while( !feof( fr ) )
    {
        read_vars = fscanf( fr, "%hu %hu %u %hu %u %hu", &new.bandpass_position,
            &new.polarizer_position, &new.exposure_time, &new.repeat_number,
            &new.duration_time, &new.option );

        if ( read_vars != 6 )
            continue;

        child = (OBS_Sequence_t*) malloc( sizeof(OBS_Sequence_t) );
        *child = new;
        child->next = NULL;
        total_sequence++;

        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
            "Frame [%u]: %u %u %u %u %u %u", total_sequence,
            child->bandpass_position, child->polarizer_position,
            child->exposure_time, child->repeat_number, child->duration_time,
            child->option );

        if ( parent == NULL )
        {
            parent = child;
            current = parent;
            continue;
        }

        current->next = child;
        current = child;

    }

    fclose( fr );

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
        "Sequence File Load Successfully!: %s - %d frames", filepath,
        total_sequence );

    return parent;

}

void OBS_ReleaseSequence( OBS_Sequence_t* sequence )
{
    OBS_Sequence_t* cur = sequence;
    OBS_Sequence_t* next = sequence;

    while( cur != NULL )
    {
        next = cur->next;
        free( cur );
        cur = next;
    }

}

void OBS_RotateWheel( uint8 axis, int32 position )
{
    OBS_WhlCmd.axis = axis;
    OBS_WhlCmd.position = position;
    OBS_WhlCmd.mode = WHL_ROTATE_ABSOLUTE;

    CFE_SB_SetCmdCode( (CFE_SB_Msg_t *) &OBS_WhlCmd, WHL_APP_MOTION_ROTATE_CC );
    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &OBS_WhlCmd );

    OBS_AppData.wheel_motion_status |= ( 1 << axis );

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "RotateWheel Axis:%d, Status:%d",
        axis, OBS_AppData.wheel_motion_status );

#ifndef __EMULATE_WHL__
    CFE_SB_SendMsg( (CFE_SB_Msg_t *) &OBS_WhlCmd );
#endif

}

void OBS_DeleteCallBack()
{
    OBS_ReleaseSequence( OBS_AppData.sequence );

}
