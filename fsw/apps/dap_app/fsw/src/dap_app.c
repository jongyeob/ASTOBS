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

#include "dap_app.h"

#include "dap_app_perfids.h"
#include "dap_app_events.h"
#include "dap_app_version.h"

/*
 ** global data
 */
DAP_QuickviewTlm_t DAP_QvTelemetryPkt;
DAP_HkTlm_t DAP_HkTelemetryPkt;
DAP_ImageProcessingTlm_t DAP_ImgProcTlmPkt;
DAP_AutoExposureTimeTlm_t DAP_AETelemetryPkt;

CFE_SB_PipeId_t DAP_CommandPipe;
CFE_SB_MsgPtr_t DAP_MsgPtr;

DAP_AppData_t DAP_AppData;
DAP_AppData_t* DAP_AppDataPtr = &DAP_AppData;

DAP_Image_t DAP_QuickView;
DAP_Image_t* DAP_QuickViewPtr = &DAP_QuickView;

DAP_Image_t* DAP_ImageBufferPtr;

CFE_SB_ZeroCopyHandle_t DAP_QvImageTlmHandle;

static CFE_EVS_BinFilter_t DAP_EventFilters[] =
    { /* Event ID    mask */
        { DAP_STARTUP_INF_EID, 0x0000 },
        { DAP_COMMAND_ERR_EID, 0x0000 },
        { DAP_COMMANDNOP_INF_EID, 0x0000 },
        { DAP_COMMANDRST_INF_EID, 0x0000 }, };

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* DAP_AppMain() -- Application entry point and main process loop          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void DAP_AppMain( void )
{
    int32 status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry( DAP_APP_PERF_ID );

    DAP_AppInit();

    /*
     ** DAP Runloop
     */
    while( CFE_ES_RunLoop( &RunStatus ) == TRUE )
    {
        CFE_ES_PerfLogExit( DAP_APP_PERF_ID );

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg( &DAP_MsgPtr, DAP_CommandPipe, 500 );

        CFE_ES_PerfLogEntry( DAP_APP_PERF_ID );

        if ( status == CFE_SUCCESS )
        {
            DAP_ProcessCommandPacket();
        }

    }

    CFE_ES_ExitApp( RunStatus );

} /* End of DAP_AppMain() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* DAP_AppInit() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void DAP_AppInit( void )
{
    /*
     ** Register the app with Executive services
     */
    CFE_ES_RegisterApp();

    /*
     ** Register the events
     */
    CFE_EVS_Register( DAP_EventFilters,
            sizeof( DAP_EventFilters ) / sizeof(CFE_EVS_BinFilter_t),
            CFE_EVS_BINARY_FILTER );

    /*
     ** Create the Software Bus command pipe and subscribe to housekeeping
     **  messages
     */
    CFE_SB_CreatePipe( &DAP_CommandPipe, DAP_PIPE_DEPTH, "DAP_CMD_PIPE" );
    CFE_SB_Subscribe( DAP_APP_CMD_MID, DAP_CommandPipe );
    CFE_SB_Subscribe( DAP_APP_SEND_HK_MID, DAP_CommandPipe );
    CFE_SB_Subscribe( CAM_APP_GRAB_FINISH_MID, DAP_CommandPipe );

    DAP_ResetCounters();

    CFE_SB_InitMsg( &DAP_HkTelemetryPkt,
    DAP_APP_HK_TLM_MID,
    DAP_APP_HK_TLM_LNGTH, TRUE );
    CFE_SB_InitMsg( &DAP_QvTelemetryPkt,
    DAP_APP_QV_TLM_MID,
    DAP_APP_QV_TLM_LNGTH, TRUE );
    CFE_SB_InitMsg( &DAP_AETelemetryPkt,
    DAP_APP_AE_TLM_MID,
    DAP_APP_AE_TLM_LNGTH, TRUE );
    CFE_SB_InitMsg( &DAP_ImgProcTlmPkt,
    DAP_APP_IP_TLM_MID, sizeof(DAP_ImageProcessingTlm_t), TRUE );

    DAP_QuickView.header.width = DAP_QV_IMAGE_WIDTH;
    DAP_QuickView.header.height = DAP_QV_IMAGE_HEIGHT;
    DAP_QuickView.header.bitpix = DAP_QV_IMAGE_BITPIX;

    DAP_QuickView.data = (uint8*) malloc( DAP_QV_IMAGE_BYTESIZE );

    DAP_GetImageBuffer( &DAP_ImageBufferPtr, DAP_QuickView.header );
    OS_printf( "ImageBuffer: 0x%08X\n", DAP_ImageBufferPtr );

    DAP_AppData.save_enable = TRUE;
    DAP_AppData.quickview_enable = TRUE;
    DAP_AppData.image_analysis_enable = TRUE;

    OS_TaskInstallDeleteHandler( &DAP_DeleteCallBack );

    CFE_EVS_SendEvent( DAP_STARTUP_INF_EID, CFE_EVS_INFORMATION,
            "DAP App Initialized. Version %d.%d.%d.%d",
            DAP_APP_MAJOR_VERSION,
            DAP_APP_MINOR_VERSION,
            DAP_APP_REVISION,
            DAP_APP_MISSION_REV );

} /* End of DAP_AppInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DAP_ProcessCommandPacket                                        */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the DAP    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void DAP_ProcessCommandPacket( void )
{
    CFE_SB_MsgId_t MsgId;

    MsgId = CFE_SB_GetMsgId( DAP_MsgPtr );

    switch( MsgId )
    {
        case DAP_APP_CMD_MID:
            DAP_ProcessGroundCommand();
            break;

        case DAP_APP_SEND_HK_MID:
            DAP_ReportHousekeeping();
            break;

        case CAM_APP_GRAB_FINISH_MID:
            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "CAM_APP_GRAB_FINISH" );
            DAP_ProcessImageTelemetry( (CAM_ImageTlm_t*) DAP_MsgPtr );

            break;

        default:
            DAP_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent( DAP_COMMAND_ERR_EID, CFE_EVS_ERROR,
                    "DAP: invalid command packet,MID = 0x%x", MsgId );
            break;
    }

    return;

}
void DAP_ProcessImageTelemetry( CAM_ImageTlm_t* tlm )
{
    OS_printf( "tlm: 0x%08X, header: 0x%08X, data: 0x%08X\n", tlm, &tlm->image,
            tlm->image.data );

    DAP_Image_t tlm_image;
    tlm_image.header.width = tlm->image.header.width;
    tlm_image.header.height = tlm->image.header.height;
    tlm_image.header.bitpix = tlm->image.header.bitpix;
    tlm_image.header.exposure_time = tlm->image.header.exposure_time;
    tlm_image.data = tlm->image.data;

    DAP_AppData.image = &tlm->image;
    DAP_AppData.frame_number = tlm->image.header.frame_number;

    double qv_min_pix = 0;
    double qv_max_pix = 1 << tlm_image.header.bitpix;

    if ( DAP_AppData.save_enable )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Save Image" );
        DAP_SaveImage( &tlm->image );
    }

    if ( DAP_AppData.image_analysis_enable )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Analyze Image" );

        uint32 byte_size = tlm_image.header.width * tlm_image.header.height
                * tlm_image.header.bitpix / 8;

        int8 error = DAP_GetImageBuffer( &DAP_ImageBufferPtr,
                tlm_image.header );
        OS_printf( "ImageBuffer: 0x%08X\n", DAP_ImageBufferPtr );
        if ( error == 0 )
        {
            memcpy( DAP_ImageBufferPtr->data, tlm_image.data, byte_size );

            DAP_AnalyzeImage( DAP_ImageBufferPtr );

        }
        else
        {
            CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                    "Image Analysis Buffer NOT Created!" );
        }

        DAP_ReportImageProcessingTelemetry();

    }
    if ( DAP_AppData.quickview_enable )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "Quickview Image Buffer update" );

        DAP_CopyImage( &tlm_image, &DAP_QuickView, qv_min_pix, qv_max_pix );

        OS_printf( "quickview: %dx%dx%d, data[0]=%d, data[-1]=%d\n",
                DAP_QuickView.header.width, DAP_QuickView.header.height,
                DAP_QuickView.header.bitpix, DAP_QuickView.data[0],
                DAP_QuickView.data[DAP_QV_IMAGE_BYTESIZE] );

        DAP_SendQvImage();

        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "CAM: QvImage created" );

    }

}
/* End DAP_ProcessCommandPacket */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* DAP_ProcessGroundCommand() -- DAP ground commands                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void DAP_ProcessGroundCommand( void )
{
    uint16 CommandCode;

    CommandCode = CFE_SB_GetCmdCode( DAP_MsgPtr );

    /* Process "known" DAP app ground commands */
    switch( CommandCode )
    {
        case DAP_APP_NOOP_CC:
            DAP_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( DAP_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                    "DAP: NOOP command" );

            break;

        case DAP_APP_RESET_COUNTERS_CC:
            DAP_ResetCounters();
            break;

        case DAP_APP_CONFIG_CC:
            DAP_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent( DAP_COMMANDNOP_INF_EID, CFE_EVS_INFORMATION,
                    "DAP: CONFIG command" );

            DAP_Config( (DAP_ConfigCmd_t*) DAP_MsgPtr );

            break;

            /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of DAP_ProcessGroundCommand() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DAP_ReportHousekeeping                                              */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void DAP_ReportHousekeeping( void )
{

    DAP_HkTelemetryPkt.image_analysis_enable =
            DAP_AppData.image_analysis_enable;
    DAP_HkTelemetryPkt.quickview_enable = DAP_AppData.quickview_enable;
    DAP_HkTelemetryPkt.save_enable = DAP_AppData.save_enable;

    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &DAP_HkTelemetryPkt );
    CFE_SB_SendMsg( (CFE_SB_Msg_t *) &DAP_HkTelemetryPkt );

    return;

} /* End of DAP_ReportHousekeeping() */

void DAP_ReportImageProcessingTelemetry( void )
{

    DAP_ImgProcTlmPkt.frame_number = DAP_AppData.frame_number;
    DAP_ImgProcTlmPkt.frame_pixel_min = DAP_AppData.frame_pixel_min;
    DAP_ImgProcTlmPkt.frame_pixel_max = DAP_AppData.frame_pixel_max;
    DAP_ImgProcTlmPkt.frame_pixel_mean = DAP_AppData.frame_pixel_mean;
    DAP_ImgProcTlmPkt.optimal_exposure_time = DAP_AppData.optimal_exposure_time;

    DAP_ImgProcTlmPkt.exposure_time = DAP_AppData.image->header.exposure_time;
    DAP_ImgProcTlmPkt.observation_mode =
            DAP_AppData.image->header.observation_mode;
    DAP_ImgProcTlmPkt.observation_time =
            DAP_AppData.image->header.observation_time;
    DAP_ImgProcTlmPkt.temperature = DAP_AppData.image->header.temperature;
    DAP_ImgProcTlmPkt.bandpass_position_angle =
            DAP_AppData.image->header.bandpass_angle;
    DAP_ImgProcTlmPkt.polarization_position_angle =
            DAP_AppData.image->header.polarization_angle;

    CFE_SB_TimeStampMsg( (CFE_SB_Msg_t *) &DAP_ImgProcTlmPkt );
    CFE_SB_SendMsg( (CFE_SB_Msg_t *) &DAP_ImgProcTlmPkt );
    return;

} /* End of DAP_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  DAP_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void DAP_ResetCounters( void )
{
    /* Status of commands processed by the DAP App */
    DAP_HkTelemetryPkt.command_count = 0;
    DAP_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent( DAP_COMMANDRST_INF_EID, CFE_EVS_INFORMATION,
            "DAP: RESET command" );
    return;

} /* End of DAP_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* DAP_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
boolean DAP_VerifyCmdLength( CFE_SB_MsgPtr_t msg, uint16 ExpectedLength )
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

        CFE_EVS_SendEvent( DAP_LEN_ERR_EID, CFE_EVS_ERROR,
                "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
                MessageID, CommandCode, ActualLength, ExpectedLength );
        result = FALSE;
        DAP_HkTelemetryPkt.command_error_count++;
    }

    return ( result );

} /* End of DAP_VerifyCmdLength() */

int8 DAP_CopyImage( DAP_Image_t* src, DAP_Image_t* dst, double min, double max )
{

    uint32 width_step = 1;
    if ( src->header.width > dst->header.width )
        width_step = src->header.width / dst->header.width;

    uint32 height_step = 1;
    if ( src->header.width > dst->header.width )
        height_step = src->header.height / dst->header.height;

    uint32 src_byte_size = src->header.bitpix / 8;
    uint32 dst_byte_size = dst->header.bitpix / 8;

    uint32 x, y;
    uint8* src_pix_ptr;
    uint8* dst_pix_ptr;

    double pix_val;

    OS_printf( "Copy: src 0x%08X dst 0x%08X ws:%d hs:%d\n", src->data,
            dst->data, width_step, height_step );

    for ( y = 0; y < dst->header.height; y++ )
    {
        if ( y >= src->header.height )
            break;

        src_pix_ptr = src->data
                + y * src->header.width * src_byte_size * height_step;
        dst_pix_ptr = dst->data + y * dst->header.width * dst_byte_size;

        for ( x = 0; x < dst->header.width; x++ )
        {

            if ( x >= src->header.width )
                break;

            switch( src->header.bitpix )
            {
                case 8:
                    pix_val = *(uint8*) ( src_pix_ptr );
                    break;
                case 16:
                    pix_val = *(uint16*) ( src_pix_ptr );
                    break;
                case 32:
                    pix_val = *(uint32*) ( src_pix_ptr );
                    break;

            }

            pix_val = ( pix_val - min ) / ( max - min );

            switch( dst->header.bitpix )
            {
                case 8:

                    *(uint8*) dst_pix_ptr = (uint32) ( pix_val * ( 1 << 8 ) );
                    break;

                case 16:
                    *(uint16*) dst_pix_ptr = (uint32) ( pix_val * ( 1 << 16 ) );
                    break;
                case 32:
                    *(uint32*) dst_pix_ptr = (uint32) ( pix_val * ( 1 << 32 ) );
                    break;

            }

            src_pix_ptr += src_byte_size * width_step;
            dst_pix_ptr += dst_byte_size;

        }
    }

    OS_printf( "END Copy\n" );
    return 0;

}

int8 DAP_GetImageBuffer( DAP_Image_t** image, DAP_ImageHeader_t header )
{
    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION, "Update Image Buffer: 0x%08X",
            *image );

    uint32 old_image_size = 0;
    uint32 new_image_size = header.width * header.height * header.bitpix / 8;

    DAP_Image_t* image_buffer = *image;

    if ( image_buffer != NULL )
    {
        old_image_size = image_buffer->header.width
                * image_buffer->header.height * image_buffer->header.bitpix / 8;
    }

    if ( old_image_size < new_image_size )
    {
        if ( image_buffer != NULL )
        {
            free( image_buffer );
            *image = 0;
        }

        uint32 total_size = sizeof(DAP_Image_t) + new_image_size;

        OS_printf( "New Image Allocation Size: %d\n", new_image_size );
        image_buffer = (DAP_Image_t*) malloc( total_size );
        if ( image_buffer == NULL )
        {
            CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Memmory allocation error" );
            return -1;
        }

        memset( image_buffer, 0, total_size );

        image_buffer->header = header;
        image_buffer->data = &image_buffer[1];

        *image = image_buffer;

        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "Image Buffer Allocation: 0x%08X %d bytes", image_buffer,
                total_size );
    }

    return 0;

}

void DAP_SaveImage( CAM_APP_IMAGE_t* image )
{
    char filepath[256];
    sprintf( filepath, "/cf/download/%d_%06d.bin",
            image->header.observation_time, image->header.observation_time2 );

    FILE* fp = fopen( filepath, "wb" );
    if ( fp == 0 )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR, "Frame NOT saved: %s", filepath );
        return;
    }
    uint32 image_byte_size = image->header.width * image->header.height
            * image->header.bitpix / 8;
    uint32 expected_file_size = sizeof(CAM_APP_IMAGE_HEADER_t)
            + image_byte_size;
    OS_printf( "%dx%dx%d: header %d + data %d bytes\n", image->header.width,
            image->header.height, image->header.bitpix,
            sizeof(CAM_APP_IMAGE_HEADER_t), image_byte_size );

    uint32 file_size = 0;

    OS_printf( "filesize: 0x%08X %d bytes\n", image,
            sizeof(CAM_APP_IMAGE_HEADER_t) );
    file_size += fwrite( image, 1, sizeof(CAM_APP_IMAGE_HEADER_t), fp );

    OS_printf( "filesize: 0x%08X %d bytes\n", image->data, image_byte_size );
    file_size += fwrite( image->data, 1, image_byte_size, fp );

    if ( file_size != expected_file_size )
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_ERROR,
                "FILE Written with different size! expected: %d, but %d bytes",
                file_size, expected_file_size );
    }
    else
    {
        CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
                "FILE Written successfully at %s of %d bytes", filepath,
                file_size );
    }

    fclose( fp );
}

int compare_pixel_8bit( void* first, void* second )
{
    if ( *(unsigned char*) first > *(unsigned char*) second )
        return 1;
    else if ( *(unsigned char*) first < *(unsigned char*) second )
        return -1;
    else
        return 0;
}
int compare_pixel_16bit( void* first, void* second )
{
    if ( *(unsigned short*) first > *(unsigned short*) second )
        return 1;
    else if ( *(unsigned short*) first < *(unsigned short*) second )
        return -1;
    else
        return 0;
}

void DAP_AnalyzeImage( DAP_Image_t* image )
{

    DAP_AppData_t* app_data = DAP_AppDataPtr;

    uint32 pixel_max, pixel_min, pixel_mean;
    uint32 optimal_exposure_time;

    uint32 x, y;
    uint8* pix_ptr;

    uint32 image_size = image->header.width * image->header.height;
    uint8 byte_size = image->header.bitpix / 8;
    double margin = 0.01;

    OS_printf( "DAP_AnalyzeImage: 0x%08X %dx%dx%d\n", image,
            image->header.width, image->header.height, image->header.bitpix );

    int (*compare_func)( void*, void* );
    switch( image->header.bitpix )
    {
        case 8:
            compare_func = &compare_pixel_8bit;
            break;
        case 16:
            compare_func = &compare_pixel_16bit;
            break;
        default:
            OS_printf( "Not Supported BITPIX %d\n", image->header.bitpix );
            return;
    }

    qsort( image->data, image_size, byte_size, compare_func );

    app_data->frame_pixel_min = ( (uint16*) image->data )[(int) ( image_size
            * margin )];
    app_data->frame_pixel_max = ( (uint16*) image->data )[(int) ( image_size
            * ( 1 - margin ) )];
    app_data->frame_pixel_mean = ( (uint16*) image->data )[(int) ( image_size
            / 2 )];

    OS_printf( "Optimal Exposure: target mean=%d, exptime=%d, mean:%d\n",
            ( 1 << image->header.bitpix - 1 ), image->header.exposure_time,
            app_data->frame_pixel_mean );

    app_data->optimal_exposure_time = ( ( 1 << image->header.bitpix - 1 )
            * image->header.exposure_time )
            / ( app_data->frame_pixel_mean + 0.1 );

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
            "Image Analysis: min=%d, max=%d, mean=%d opt. exp. time=%d",
            app_data->frame_pixel_min, app_data->frame_pixel_max,
            app_data->frame_pixel_mean, app_data->optimal_exposure_time );

    DAP_AETelemetryPkt.exposure_time = app_data->optimal_exposure_time;

    CFE_SB_TimeStampMsg( &DAP_AETelemetryPkt );
    CFE_SB_SendMsg( &DAP_AETelemetryPkt );

}
void DAP_SendQvImage()
{
    DAP_AppData_t* app_data = DAP_AppDataPtr;
    DAP_Image_t* image = DAP_QuickViewPtr;

    DAP_QvTelemetryPkt.frame_number = app_data->frame_number;

    uint32 data_size = image->header.width * image->header.height
            * image->header.bitpix / 8;

    uint32 send_size = 0;
    uint8* buffer_ptr = image->data;
    OS_printf( "Quickview Buffer: 0x%08X\n", buffer_ptr );

    while( send_size < data_size )
    {
        DAP_QvTelemetryPkt.frame_position = send_size;
        memcpy( &DAP_QvTelemetryPkt.data, buffer_ptr + send_size,
                DAP_APP_QV_BUFFER_SIZE );

        CFE_SB_TimeStampMsg( (CFE_SB_Msg_t*) &DAP_QvTelemetryPkt );
        CFE_SB_SendMsg( (CFE_SB_Msg_t *) &DAP_QvTelemetryPkt );

        send_size += DAP_APP_QV_BUFFER_SIZE;

    }

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
            "Quickview Image Sent! %d/%d bytes val %d", send_size, data_size,
            DAP_QvTelemetryPkt.data[DAP_APP_QV_BUFFER_SIZE - 1] );

}

void DAP_Config( DAP_ConfigCmd_t* command )
{

    DAP_AppData.save_enable = command->save_enable;
    DAP_AppData.image_analysis_enable = command->image_analysis_enable;
    DAP_AppData.quickview_enable = command->quickview_enable;

    CFE_EVS_SendEvent( 0, CFE_EVS_INFORMATION,
            "DAP Config: Save=%d, Img Anal=%d, Qv=%d", DAP_AppData.save_enable,
            DAP_AppData.image_analysis_enable, DAP_AppData.quickview_enable );

}

void DAP_DeleteCallBack()
{
    if ( DAP_QuickView.data != 0 )
    {
        free( DAP_QuickView.data );
        DAP_QuickView.data = 0;
    }

    if ( DAP_ImageBufferPtr != 0 )
    {
        free( DAP_ImageBufferPtr );
        DAP_ImageBufferPtr = 0;
    }

}
