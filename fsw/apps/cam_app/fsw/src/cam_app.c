/*******************************************************************************
 ** File: cam_app.c
 **
 ** Purpose:
 **   This file contains the source code for the Cam App.
 **
 *******************************************************************************/

/*
 **   Include Files:
 */

#include "network_includes.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "cam_app.h"
#include "cam_app_events.h"
#include "cam_app_version.h"
#include "cam_app_perfids.h"

#include "whl_app.h"
#include "dap_app.h"

#define CAM_SERVER_ADDRESS "127.0.0.1"
#define CAM_SERVER_PORT 1500
#define CAM_COMMAND_FORMAT "%dx%dx%d,%d:%d,%dx%d,%f"

/*
 ** global data
 */
#define CAM_SOCKET_BUFFER_SIZE 256
char SockBuffer[CAM_SOCKET_BUFFER_SIZE];

/*
 * Command and Telemetry
 */
CAM_HkTlm_t CAM_HkTelemetryPkt;
CAM_RepeatFinishTlm_t CAM_RepeatFinishPkt;
CAM_ImageTlm_t* CAM_ImageTlmPtr;

CFE_SB_PipeId_t CAM_CommandPipe;
CFE_SB_MsgPtr_t CAM_MsgPtr;

CFE_SB_ZeroCopyHandle_t CAM_ImageZeroCopyHandle;

int32 device_fd;

static CFE_EVS_BinFilter_t CAM_EventFilters[] =
    { /* Event ID    mask */
        { CAM_RESERVED_EID, 0x0000 },

        { CAM_STARTUP_INF_EID, 0x0000 },
        { CAM_COMMAND_ERR_EID, 0x0000 },
        { CAM_COMMANDNOP_INF_EID, 0x0000 },
        { CAM_COMMANDRST_INF_EID, 0x0000 }, };

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* CAM_AppMain() -- Application entry point and main process loop          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void CAM_AppMain( void )
{
    int32 status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry( CAM_APP_PERF_ID );

    CAM_AppInit();
    /*
     ** CAM Runloop
     */
    while( CFE_ES_RunLoop( &RunStatus ) == TRUE )
    {
        CFE_ES_PerfLogExit( CAM_APP_PERF_ID );

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg( &CAM_MsgPtr, CAM_CommandPipe, 10 );

        CFE_ES_PerfLogEntry( CAM_APP_PERF_ID );

        if ( status == CFE_SUCCESS )
        {
            CAM_ProcessCommandPacket();
        }

        switch( CAM_AppData.mode )
        {
            case CAM_MODE_WAIT:
                break;
            case CAM_MODE_GRAB:
                if ( CAM_AppData.auto_exposure_enable )
                {
                    CAM_DevicePtr->exposure_time =
                        CAM_AppData.auto_exposure_time;

                }

                CAM_GrabFrame( CAM_DevicePtr, CAM_ImagePtr );
                CAM_AppData.current_repeat++;
                CAM_AppData.mode = CAM_MODE_GRABING;
                break;
            case CAM_MODE_GRABING:

                if ( CAM_AppData.target_repeat == 0
                    || CAM_AppData.current_repeat < CAM_AppData.target_repeat )
                {
                    CAM_AppData.mode = CAM_MODE_GRAB;
                    break;
                }

                CAM_AppData.mode = CAM_MODE_STOPPING;
                break;

            case CAM_MODE_STOPPING:

                CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &CAM_RepeatFinishPkt );
                CFE_SB_SendMsg( (CFE_SB_Msg_t *) &CAM_RepeatFinishPkt );

                CAM_AppData.mode = CAM_MODE_WAIT;
                break;

        }

    }

    CFE_ES_ExitApp( RunStatus );

} /* End of CAM_AppMain() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* CAM_AppInit() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void CAM_AppInit( void )
{
    /*
     ** Register the app with Executive services
     */
    CFE_ES_RegisterApp();

    /*
     ** Register the events
     */
    CFE_EVS_Register( CAM_EventFilters,
        sizeof( CAM_EventFilters ) / sizeof(CFE_EVS_BinFilter_t),
        CFE_EVS_BINARY_FILTER );

    /*
     ** Create the Software Bus command pipe and subscribe to housekeeping
     **  messages
     */
    CFE_SB_CreatePipe( &CAM_CommandPipe, CAM_PIPE_DEPTH, "CAM_CMD_PIPE" );
    CFE_SB_Subscribe( CAM_APP_CMD_MID, CAM_CommandPipe );
    CFE_SB_Subscribe( CAM_APP_SEND_HK_MID, CAM_CommandPipe );
    CFE_SB_Subscribe( WHL_APP_ROTATE_FINISH_MID, CAM_CommandPipe );
    CFE_SB_Subscribe( DAP_APP_AE_TLM_MID, CAM_CommandPipe );

    CAM_ResetCounters();

    CFE_SB_InitMsg( &CAM_HkTelemetryPkt,
    CAM_APP_HK_TLM_MID,
    CAM_APP_HK_TLM_LNGTH, TRUE );
    CFE_SB_InitMsg( &CAM_RepeatFinishPkt,
    CAM_APP_REPEAT_FINISH_MID, sizeof(CAM_RepeatFinishTlm_t), TRUE );

    device_fd = CAM_OpenDevice( 0 );

    CAM_Device.handle = device_fd;

    CAM_Device.frame_width = CAM_IMAGE_WIDTH;
    CAM_Device.frame_height = CAM_IMAGE_HEIGHT;
    CAM_Device.frame_bitpix = CAM_IMAGE_BITPIX;
    CAM_Device.frame_size = CAM_Device.frame_width * CAM_Device.frame_height;
    CAM_Device.frame_bytesize = CAM_Device.frame_size * CAM_Device.frame_bitpix
        / 8;

    CAM_Device.camera_bin_x = CAM_IMAGE_BIN_X;
    CAM_Device.camera_bin_y = CAM_IMAGE_BIN_Y;

    CAM_Device.camera_offset_x = CAM_IMAGE_OFFSET_X;
    CAM_Device.camera_offset_y = CAM_IMAGE_OFFSET_Y;

    CAM_Device.target_temperature = CAM_DEFAULT_TEMPERATURE;

    CAM_SetDevice( CAM_DevicePtr );
    CAM_ImagePtr = CAM_UpdateImage( CAM_DevicePtr, CAM_ImagePtr );

    OS_TaskInstallDeleteHandler( &CAM_DeleteCallBack );

    CFE_EVS_SendEvent( CAM_STARTUP_INF_EID, CFE_EVS_INFORMATION,
        "CAM App Initialized. Version %d.%d.%d.%d",
        CAM_APP_MAJOR_VERSION,
        CAM_APP_MINOR_VERSION,
        CAM_APP_REVISION,
        CAM_APP_MISSION_REV );

} /* End of CAM_AppInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CAM_ProcessCommandPacket                                        */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the CAM    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void CAM_ProcessCommandPacket( void )
{
    CFE_SB_MsgId_t MsgId;

    MsgId = CFE_SB_GetMsgId( CAM_MsgPtr );

    switch( MsgId )
    {
        case CAM_APP_CMD_MID:
            CAM_ProcessGroundCommand();
            break;

        case CAM_APP_SEND_HK_MID:
            CAM_ReportHousekeeping();
            break;

        case WHL_APP_ROTATE_FINISH_MID:
        {

            WHL_RotateFinishTlm_t* tlm = (WHL_RotateFinishTlm_t*) CAM_MsgPtr;

            switch( tlm->axis )
            {
                case WHL_BANDPASS_AXIS:
                    CAM_DevicePtr->bandpass_angle = tlm->position;
                    break;
                case WHL_POLARIZER_AXIS:
                    CAM_DevicePtr->polarization_angle = tlm->position;
                    break;
            }

        }
            break;
        case DAP_APP_AE_TLM_MID:
        {

            DAP_AutoExposureTimeTlm_t* tlm =
                (DAP_AutoExposureTimeTlm_t*) CAM_MsgPtr;

            CAM_AppData.auto_exposure_time = tlm->exposure_time
                * pow( 2, CAM_AppData.auto_exposure_adjust );

            if ( CAM_AppData.auto_exposure_time > CAM_AUTO_EXPOSURE_TIME_MAX )
            {
                CAM_AppData.auto_exposure_time = CAM_AUTO_EXPOSURE_TIME_MAX;
            }
            if ( CAM_AppData.auto_exposure_time < CAM_AUTO_EXPOSURE_TIME_MIN )
            {
                CAM_AppData.auto_exposure_time = CAM_AUTO_EXPOSURE_TIME_MIN;
            }

        }
            break;
        default:
            CAM_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent( CAM_COMMAND_ERR_EID, CFE_EVS_ERROR,
                "CAM: invalid command packet,MID = 0x%x", MsgId );
            break;
    }

    return;

} /* End CAM_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* CAM_ProcessGroundCommand() -- CAM ground commands                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void CAM_ProcessGroundCommand( void )
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode( CAM_MsgPtr );

    /* Process "known" CAM app ground commands */
    switch( CommandCode )
    {
        case CAM_APP_NOOP_CC:
            CAM_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( CAM_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                "CAM: NOOP command" );
            break;

        case CAM_APP_RESET_COUNTERS_CC:
            CAM_ResetCounters();
            break;

        case CAM_APP_GRAB_CC:
            CAM_HkTelemetryPkt.command_count++;

            CAM_ProcessGrabCommand( (CAM_GrabCmd_t*) CAM_MsgPtr );
            CAM_AppData.mode = CAM_MODE_GRAB;

            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "CAM: CONTROL GRAB command AEadj=%1.1f, AE=%d, Exp=%d, OBSMODE=%d, Repeat=%d",
                ( (CAM_GrabCmd_t*) CAM_MsgPtr )->auto_exposure_adjust,
                ( (CAM_GrabCmd_t*) CAM_MsgPtr )->auto_exposure_enable,
                ( (CAM_GrabCmd_t*) CAM_MsgPtr )->exposure_time,
                ( (CAM_GrabCmd_t*) CAM_MsgPtr )->observation_mode,
                ( (CAM_GrabCmd_t*) CAM_MsgPtr )->repeat );

            break;
        case CAM_APP_CONFIG_CC:
            CAM_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "CAM: CONTROL CONFIG command" );

            CAM_ProcessConfigCommand( (CAM_GrabCmd_t*) CAM_MsgPtr );
            CAM_SetDevice( &CAM_Device );
            break;
        case CAM_APP_GRAB_STOP_CC:
            CAM_HkTelemetryPkt.command_count++;

            CAM_AppData.mode = CAM_MODE_STOPPING;
            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "CAM: CONTROL STOP command" );

            break;

            /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of CAM_ProcessGroundCommand() */

void CAM_ProcessGrabCommand( CAM_GrabCmd_t* msg )
{

    CAM_AppData.observation_mode = msg->observation_mode;
    CAM_AppData.auto_exposure_adjust = msg->auto_exposure_adjust;
    CAM_AppData.auto_exposure_enable = msg->auto_exposure_enable;
    CAM_AppData.target_repeat = msg->repeat;
    CAM_AppData.current_repeat = 0;

    CAM_DevicePtr->exposure_time = msg->exposure_time;
    CAM_AppData.auto_exposure_time = msg->exposure_time;

    return;

}

void CAM_ProcessConfigCommand( CAM_ConfigCmd_t* msg )
{
    CAM_Device.target_temperature = msg->temperature;
    CAM_Device.camera_offset_x = msg->offset_x;
    CAM_Device.camera_offset_y = msg->offset_y;

    return;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CAM_ReportHousekeeping                                              */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void CAM_ReportHousekeeping( void )
{

    CAM_HkTelemetryPkt.handle = CAM_Device.handle;
    CAM_HkTelemetryPkt.frame_width = CAM_Device.frame_width;
    CAM_HkTelemetryPkt.frame_height = CAM_Device.frame_height;
    CAM_HkTelemetryPkt.exposure_time = CAM_Device.exposure_time;
    CAM_HkTelemetryPkt.polarization_angle = CAM_Device.polarization_angle;
    CAM_HkTelemetryPkt.bandpass_angle = CAM_Device.bandpass_angle;

    CAM_HkTelemetryPkt.frame_number = CAM_AppData.frame_number;
    CAM_HkTelemetryPkt.auto_exposure_time = CAM_AppData.auto_exposure_time;

    CAM_HkTelemetryPkt.target_repeat = CAM_AppData.target_repeat;
    CAM_HkTelemetryPkt.current_repeat = CAM_AppData.current_repeat;

    CAM_HkTelemetryPkt.target_temperature = CAM_Device.target_temperature;
    CAM_HkTelemetryPkt.temperature = CAM_Device.temperature;
    CAM_HkTelemetryPkt.auto_exposure_adjust = CAM_AppData.auto_exposure_adjust;

    CAM_HkTelemetryPkt.camera_offset_x = CAM_Device.camera_offset_x;
    CAM_HkTelemetryPkt.camera_offset_y = CAM_Device.camera_offset_y;

    CAM_HkTelemetryPkt.mode = CAM_AppData.mode;
    CAM_HkTelemetryPkt.observation_mode = CAM_AppData.observation_mode;
    CAM_HkTelemetryPkt.auto_exposure_enable = CAM_AppData.auto_exposure_enable;
    CAM_HkTelemetryPkt.frame_bitpix = CAM_Device.frame_bitpix;

    CAM_HkTelemetryPkt.camera_bin_x = CAM_Device.camera_bin_x;
    CAM_HkTelemetryPkt.camera_bin_y = CAM_Device.camera_bin_y;

    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &CAM_HkTelemetryPkt );
    CFE_SB_SendMsg( (CFE_SB_Msg_t *) &CAM_HkTelemetryPkt );
    return;

} /* End of CAM_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  CAM_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
/* End of CAM_ResetCounters() */
void CAM_ResetCounters( void )
{
    /* Status of commands processed by the CAM App */
    CAM_HkTelemetryPkt.command_count = 0;
    CAM_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent( CAM_COMMANDRST_INF_EID, CFE_EVS_INFORMATION,
        "CAM: RESET command" );
    return;

}
int32 CAM_OpenDevice( char* camera_id )
{

    OS_printf( "OpenDevice()\n" );

    int sockfd;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket( AF_INET, SOCK_STREAM, 0 );
    if ( sockfd < 0 )
    {
        OS_printf( "ERROR opening socket" );
        return -1;
    }

    memset( &serv_addr, 0, sizeof( serv_addr ) );
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr( CAM_SERVER_ADDRESS );
    serv_addr.sin_port = htons( CAM_SERVER_PORT );

    int opt = 1;
    setsockopt( sockfd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof( opt ) );

    if ( connect( sockfd, (struct sockaddr *) &serv_addr, sizeof( serv_addr ) )
        < 0 )
    {
        OS_printf( "Connection failed!\n" );
        return -1;
    }
    else
    {
        int32 value = fcntl( sockfd, F_GETFL, 0 );
        value &= ~O_NONBLOCK;
        fcntl( sockfd, F_SETFL, value );
    }

    return sockfd;

}
void CAM_CloseDevice( CAM_APP_DEVICE_t* device )
{
    if ( device->handle != -1 )
    {
        close( device->handle );
    }

    device->handle = -1;

}
int8 CAM_DownloadImage( CAM_APP_DEVICE_t* device, CAM_APP_IMAGE_t* image )
{
    int n;
    long read_size = 0;
    long download_size = image->header.width * image->header.height
        * image->header.bitpix / 8;

    if ( download_size < device->frame_bytesize )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR,
            "Image Buffer Smaller than Device Need: %d < %d", download_size,
            device->frame_bytesize );

        return -1;
    }

    memset( image->data, 0, download_size );

    char* buffer = image->data;
    while( read_size < download_size )
    {
        n = read( device->handle, (void*) buffer, download_size - read_size );

        if ( n < 0 )
        {
            CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Socket Error" );
            return -1;
        }

        read_size += n;
        OS_printf( "0x%08X: %d/%d received\n", buffer, n, read_size );
        buffer += n;

    }

    return 0;

}

void CAM_GetStatus( CAM_APP_DEVICE_t* device )
{
    int32 n;

    sprintf( SockBuffer, "STATUS" );
    n = write( device->handle, SockBuffer, strlen( SockBuffer ) );
    if ( n <= 0 )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Can NOT Write Message %s",
            SockBuffer );

        return;
    }

    n = read( device->handle, SockBuffer, CAM_SOCKET_BUFFER_SIZE - 1 );
    if ( n <= 0 )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Can NOT Read Message" );
        return;
    }

    sscanf( SockBuffer, CAM_COMMAND_FORMAT, &( device->frame_width ),
        &( device->frame_height ), &( device->frame_bitpix ),
        &( device->camera_bin_x ), &( device->camera_bin_y ),
        &( device->camera_offset_x ), &( device->camera_offset_y ),
        &( device->temperature ) );

    sprintf( SockBuffer, CAM_COMMAND_FORMAT, ( device->frame_width ),
        ( device->frame_height ), ( device->frame_bitpix ),
        ( device->camera_bin_x ), ( device->camera_bin_y ),
        ( device->camera_offset_x ), ( device->camera_offset_y ),
        ( device->temperature ) );

}
void CAM_SetDevice( CAM_APP_DEVICE_t* device )
{
    int n;
    char command[128];

    sprintf( command, "CONFIG" );
    sprintf( command + 6, CAM_COMMAND_FORMAT, device->frame_width,
        device->frame_height, device->frame_bitpix, device->camera_bin_x,
        device->camera_bin_y, device->camera_offset_x, device->camera_offset_y,
        device->target_temperature );

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Set Device: %s", command );

    n = write( device->handle, command, strlen( command ) );
    if ( n < 0 )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "ERROR writing to socket" );
    }

}

CAM_APP_IMAGE_t* CAM_GetNewImage( CAM_APP_DEVICE_t* device )
{
    uint32 new_image_size = sizeof(CAM_APP_IMAGE_t) + device->frame_bytesize;

    OS_printf( "New Image Allocation Size: %d\n", new_image_size );
    CAM_APP_IMAGE_t* new_image = (CAM_APP_IMAGE_t*) malloc( new_image_size );
    if ( new_image == NULL )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Memmory allocation error" );
        return 0;
    }

    memset( new_image, 0, new_image_size );

    new_image->header.width = device->frame_width;
    new_image->header.height = device->frame_height;
    new_image->header.bitpix = device->frame_bitpix;

    new_image->data = &new_image[1];

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
        "Image Buffer Allocation: 0x%08X %d bytes", new_image, new_image_size );

    return new_image;

}

void CAM_ReleaseImage( CAM_APP_IMAGE_t** image )
{
    if ( *image != NULL )
    {
        free( *image );
    }

    *image = NULL;

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Image Buffer Released: 0x%08X",
        image );
}

CAM_APP_IMAGE_t* CAM_UpdateImage( CAM_APP_DEVICE_t* device,
    CAM_APP_IMAGE_t* image )
{
    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Update Image Buffer: 0x%08X",
        image );

    uint32 total_image_size = 0;

    if ( image != NULL )
    {
        total_image_size = image->header.width * image->header.height
            * image->header.bitpix / 8;
    }

    if ( total_image_size < device->frame_bytesize )
    {
        if ( image != NULL )
        {
            CAM_ReleaseImage( &image );
        }
        image = CAM_GetNewImage( device );
        OS_printf( "New Image: 0x%08X\n", image );
    }

    return image;

}

void CAM_GrabFrame( CAM_APP_DEVICE_t* device, CAM_APP_IMAGE_t* image )
{
    OS_printf( "Grab Frame() -> Device: %dx%dx%d\n", device->frame_width,
        device->frame_height, device->frame_bitpix );

    CAM_GetStatus( CAM_DevicePtr );

    image->header.exposure_time = device->exposure_time;
    image->header.observation_mode = CAM_AppData.observation_mode;

    CFE_TIME_SysTime_t observation_time = CFE_TIME_GetTime();
    image->header.observation_time = observation_time.Seconds;
    image->header.observation_time2 = CFE_TIME_Sub2MicroSecs(
        observation_time.Subseconds );
    image->header.temperature = device->temperature;
    image->header.bandpass_angle = device->bandpass_angle;
    image->header.polarization_angle = device->polarization_angle;

    int32 error;

#ifdef __EMULATED_DEVICE__

    error = CAM_EmulGrabFrame(device, image->data);

    if ( error != 0 )
    {
        CFE_EVS_SendEvent(0,CFE_EVS_ERROR,
            "Frame grab error! %d",error);
        return;

    }
#else
    //Start to exposure

    int32 n;

    sprintf( SockBuffer, "GRAB%d", image->header.exposure_time );
    n = write( device->handle, SockBuffer, strlen( SockBuffer ) );
    if ( n < 0 )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Can NOT Write Message %s",
            SockBuffer );

        return;
    }

    error = CAM_DownloadImage( device, image );
    if ( error )
    {
        return;
    }

#endif

    image->header.frame_number = ++CAM_AppData.frame_number;

    uint32 pkt_size = sizeof(CAM_ImageTlm_t) + device->frame_bytesize;

    CAM_ImageTlmPtr = (CAM_ImageTlm_t*) CFE_SB_ZeroCopyGetPtr( pkt_size,
        &CAM_ImageZeroCopyHandle );
    if ( CAM_ImageTlmPtr == NULL )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR,
            "Can Not Get ZeroCopy Buffer! size: %d", pkt_size );
        return;
    }

    CFE_SB_InitMsg( (CFE_SB_Msg_t*) CAM_ImageTlmPtr,
        CAM_APP_GRAB_FINISH_MID, pkt_size,
        TRUE );

    CAM_ImageTlmPtr->image.header = image->header;
    CAM_ImageTlmPtr->image.data = &CAM_ImageTlmPtr[1];

    memcpy( CAM_ImageTlmPtr->image.data, image->data, device->frame_bytesize );

    OS_printf(
        "Image Tlm: 0x%08X, Header: 0x%08X, Size: %d, Data: 0x%08X(0x%08X), Size: %d\n",
        CAM_ImageTlmPtr, &( CAM_ImageTlmPtr->image.header ),
        sizeof(CAM_ImageTlm_t), CAM_ImageTlmPtr->image.data,
        &CAM_ImageTlmPtr[1], device->frame_bytesize );

    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t*) CAM_ImageTlmPtr );
    CFE_SB_ZeroCopySend( (CFE_SB_Msg_t*) CAM_ImageTlmPtr,
        CAM_ImageZeroCopyHandle );

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Grab Finish" );

}

#ifdef __EMULATED_DEVICE__
int8 CAM_EmulGrabFrame(CAM_APP_DEVICE_t* device, void* frame_buffer)
{

    uint32 pix;
    uint32 i;
    uint8 pixel_byte = device->frame_bitpix/8;
    uint32 image_size = device->frame_width*device->frame_height;
    uint32 exposure_time = device->exposure_time;

    OS_printf("EmulGrabFrame(): %dx%dx%d Buffer: 0x%08X\n",
        device->frame_width,
        device->frame_height,
        device->frame_bitpix,
        frame_buffer);

    if ( device->frame_bitpix%2 != 0)
    {
        CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
            "Impossible to Bitpix:%d",device->frame_bitpix);
        return -1;
    }

    for ( i = 0; i < image_size; i++)
    {
        pix = (uint32)((i/(float)image_size)*(1<<device->frame_bitpix));
        memcpy(frame_buffer+i*pixel_byte,&pix,pixel_byte);

    }
    OS_printf("Image Buffer Write: [-1]: %d\n",pix);

    OS_TaskDelay(exposure_time);

    CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
        "Emulate Camera Grab within %d",exposure_time);

    return 0;

}
#endif
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* CAM_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
boolean CAM_VerifyCmdLength( CFE_SB_MsgPtr_t msg, uint16 ExpectedLength )
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

        CFE_EVS_SendEvent( CAM_LEN_ERR_EID, CFE_EVS_ERROR,
            "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
            MessageID, CommandCode, ActualLength, ExpectedLength );
        result = FALSE;
        CAM_HkTelemetryPkt.command_error_count++;
    }

    return ( result );

} /* End of CAM_VerifyCmdLength() */

void CAM_DeleteCallBack()
{

    CAM_ReleaseImage( &CAM_ImagePtr );

    close( device_fd );

}

