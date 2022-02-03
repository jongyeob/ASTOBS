/*******************************************************************************
** File: sample_app.c
**
** Purpose:
**   This file contains the source code for the Sample App.
**
*******************************************************************************/

/*************************************************************************
** Includes
*************************************************************************/
#include <stdio.h>   /* Standard input/output definitions */
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */

#include "whl_app.h"

#include "whl_app_perfids.h"
#include "whl_app_events.h"
#include "whl_app_version.h"

/*************************************************************************
** Macro Definitions
*************************************************************************/
#define SEND_SIZE 64
#define RECV_SIZE 64
#define BAUDRATE 9600
#define DELAY_MSEC(l) (l*2)


#ifndef __GEAR_RATIO__
#define __GEAR_RATIO__ 1
#endif

#define MOTOR_VELOCITY_WEIGHT   (64424*__GEAR_RATIO__)
#define MOTOR_ACCELATION_WEIGHT (15.82*__GEAR_RATIO__)


#ifndef __DEVICE_PATH__
#define __DEVICE_PATH__ "/dev/ttyUSB0"
#endif

/*************************************************************************
** Private Function Prototypes
*************************************************************************/

int8 WHL_WriteString(int32 fd,char*);
int8 WHL_ReadString(int32 fd,char*,int);
char* WHL_SendCommand(int32 fd, uint8 axis, char* command);

void WHL_DeleteCallBack();

/*
** global data
*/

whl_hk_tlm_t       WHL_HkTelemetryPkt;
WHL_RotateFinishTlm_t WHL_RotateFinishCmd;

CFE_SB_PipeId_t    WHL_CommandPipe;
CFE_SB_MsgPtr_t    WHL_MsgPtr;
WHL_MotionCmdPtr_t WHL_MotionCmdPtr;
WHL_DevicePtr_t    WHL_DevicePtr;

int32 WHL_DeviceFD = -1;


WHL_Device_t       WHL_DeviceList[WHL_DEVICE_COUNT];
WHL_Device_t*      WHL_Polarizer = &WHL_DeviceList[1];
WHL_Device_t*      WHL_Bandpass  = &WHL_DeviceList[0];


WHL_DeviceStatus_t WHL_DeviceStatus[WHL_DEVICE_COUNT];
WHL_DeviceStatus_t*      WHL_PolarizerStatus = &WHL_DeviceStatus[1];
WHL_DeviceStatus_t*      WHL_BandpassStatus  = &WHL_DeviceStatus[0];




char command_buffer[SEND_SIZE];
char send_buffer[SEND_SIZE];
char recv_buffer[RECV_SIZE];
char report_buffer[RECV_SIZE];


static CFE_EVS_BinFilter_t  WHL_EventFilters[] =
       {  /* Event ID    mask */
    	  {WHL_RESERVED_EID,       0x0000},

          {WHL_STARTUP_INF_EID,       0x0000},
          {WHL_COMMAND_ERR_EID,       0x0000},
          {WHL_COMMANDNOP_INF_EID,    0x0000},
          {WHL_COMMANDRST_INF_EID,    0x0000},
       };

/** * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* WHL_AppMain() -- Application entry point and main process loop          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * *  * * * * **/
void WHL_AppMain( void )
{
	int i;
    int32  status;
    uint32 RunStatus = CFE_ES_APP_RUN;

    CFE_ES_PerfLogEntry(WHL_APP_PERF_ID);

    WHL_AppInit();
    /*
    ** WHL Runloop
    */

    while (CFE_ES_RunLoop(&RunStatus) == TRUE)
    {
        CFE_ES_PerfLogExit(WHL_APP_PERF_ID);

        /* Pend on receipt of command packet -- timeout set to 500 millisecs */
        status = CFE_SB_RcvMsg(&WHL_MsgPtr, WHL_CommandPipe, 50);
        
        CFE_ES_PerfLogEntry(WHL_APP_PERF_ID);

        if (status == CFE_SUCCESS)
        {
            WHL_ProcessCommandPacket();
        }


        for ( i = 0; i < WHL_DEVICE_COUNT; i++ )
        {
			WHL_DevicePtr = &WHL_DeviceList[i];

			switch (WHL_DevicePtr->mode)
			{
			case WHL_DEVICE_ROTATING:

				WHL_ProcessRotating(WHL_DevicePtr);
				break;
			case WHL_DEVICE_STOPPING:

				WHL_ProcessStopping(WHL_DevicePtr);
				break;
			case WHL_DEVICE_HOMING:
				WHL_ProcessHoming(WHL_DevicePtr);
				break;

			}

        }


    }

    CFE_ES_ExitApp(RunStatus);

} /* End of WHL_AppMain() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* WHL_ProcessGroundCommand() -- WHL ground commands                    */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/

void WHL_ProcessGroundCommand(void)
{
    uint16 cmd_code;

    cmd_code = CFE_SB_GetCmdCode(WHL_MsgPtr);
    WHL_MotionCmdPtr  = (WHL_MotionCmdPtr_t)WHL_MsgPtr;


    /* Process "known" WHL app ground commands */
    switch (cmd_code)
    {
        case WHL_APP_NOOP_CC:
            WHL_HkTelemetryPkt.command_count++;
            CFE_EVS_SendEvent(WHL_COMMANDNOP_INF_EID,CFE_EVS_INFORMATION,
			"NOOP command");
            break;

        case WHL_APP_RESET_COUNTERS_CC:
            WHL_ResetCounters();
            break;

		case WHL_APP_MOTION_STOP_CC:
			WHL_HkTelemetryPkt.command_count++;
			WHL_DevicePtr = WHL_GetDevice(WHL_MotionCmdPtr->axis);

			if ( WHL_DevicePtr == 0 )
			{
				WHL_HkTelemetryPkt.command_error_count++;
				break;
			}

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Motion STOP command at AXIS %d on Mode %d",
						WHL_MotionCmdPtr->axis,
						WHL_MotionCmdPtr->mode);

			WHL_Stop(WHL_DevicePtr,
					 WHL_MotionCmdPtr->mode);

			WHL_DevicePtr->mode = WHL_DEVICE_STOPPING;

			break;

		case WHL_APP_MOTION_ROTATE_CC:
			WHL_HkTelemetryPkt.command_count++;
			WHL_DevicePtr = WHL_GetDevice(WHL_MotionCmdPtr->axis);

			if ( WHL_DevicePtr == 0 )
			{
				WHL_HkTelemetryPkt.command_error_count++;
				break;
			}

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
					"Motion ROTATE command for position %d at AXIS %d on Mode %d",
					WHL_MotionCmdPtr->position,
					WHL_MotionCmdPtr->axis,
					WHL_MotionCmdPtr->mode);

			WHL_Rotate(WHL_DevicePtr,
					   WHL_MotionCmdPtr->position*10,
					   WHL_MotionCmdPtr->mode);

			WHL_DevicePtr->mode = WHL_DEVICE_ROTATING;

			break;

		case WHL_APP_MOTION_HOME_CC:
			WHL_HkTelemetryPkt.command_count++;
			WHL_DevicePtr = WHL_GetDevice(WHL_MotionCmdPtr->axis);
			if ( WHL_DevicePtr == 0 )
			{
				WHL_HkTelemetryPkt.command_error_count++;
				break;
			}

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Motion HOME command at AXIS %d",
						WHL_MotionCmdPtr->axis);

			WHL_Home(WHL_DevicePtr);

			WHL_DevicePtr->mode = WHL_DEVICE_HOMING;

			break;

		case WHL_APP_MOTION_RESET_CC:
			WHL_HkTelemetryPkt.command_count++;
			WHL_DevicePtr = WHL_GetDevice(WHL_MotionCmdPtr->axis);
			if ( WHL_DevicePtr == 0 )
			{
				WHL_HkTelemetryPkt.command_error_count++;
				break;
			}

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Motion RESET command at AXIS %d",
						WHL_MotionCmdPtr->axis);

			WHL_ResetDevice(WHL_DevicePtr);

			break;

		case WHL_APP_MOTION_ZERO_CC:
			WHL_HkTelemetryPkt.command_count++;
			WHL_DevicePtr = WHL_GetDevice(WHL_MotionCmdPtr->axis);
			if ( WHL_DevicePtr == 0 )
			{
				WHL_HkTelemetryPkt.command_error_count++;
				break;
			}

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Motion ZERO command at AXIS %d",
						WHL_MotionCmdPtr->axis);

			WHL_SetZero(WHL_DevicePtr);

			break;

		case WHL_APP_MOTION_CONFIG_CC:
			WHL_HkTelemetryPkt.command_count++;
			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Motion CONFIG command at AXIS %d",
						WHL_MotionCmdPtr->axis);

			WHL_ConfigureDevice((WHL_MotionConfig_t*)WHL_MsgPtr);

			break;

        /* default case already found during FC vs length test */
        default:
            break;
    }
    return;

} /* End of WHL_ProcessGroundCommand() */


/*
 *
 * WHL_Process*()
 *
 */
void WHL_ProcessRotating(WHL_Device_t* device)
{

	WHL_DeviceStatus_t status = WHL_GetStatus(device);
	if ( status.home == WHL_SENSOR_ON)
	{
		CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
				"Home sensor on at %d, difference %d from the last home sensor",
				status.encoder_position,
				status.encoder_position - device->last_home_position);

		device->last_home_position = status.encoder_position;


	}

	if ( status.moving == WHL_MOTION_OFF)
	{
		device->mode = WHL_DEVICE_STOPPING;
	}


}
void WHL_ProcessHoming(WHL_Device_t* device)
{
	WHL_DeviceStatus_t status = WHL_GetStatus(device);

	switch ( device->home_mode )
	{
	case WHL_HOMING_WAIT:
		if ( status.moving == WHL_MOTION_ON )
		{
			WHL_Stop(device,WHL_STOP_NORMAL);
			break;
		}

		WHL_RotateContinous(device,device->home_start_velocity); // Initial velocity
		device->home_last_velocity = device->home_start_velocity;

		device->home_mode = WHL_HOMING_SENSOR_UP;

		CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
				"Home Search Start");

		break;
	case WHL_HOMING_SENSOR_UP:
		if (status.home == WHL_SENSOR_ON)
		{
			WHL_Stop(device,WHL_STOP_NORMAL);

			device->home_mode = WHL_HOMING_SENSOR_ON;

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
					"Home Sensor Up Detected");
		}
		break;
	case WHL_HOMING_SENSOR_ON:
		if ( status.moving == WHL_MOTION_ON )
		{
			WHL_Stop(device,WHL_STOP_NORMAL);
			break;
		}
		int32 new_velocity;

		new_velocity =  -0.5 * device->home_last_velocity;

		device->home_mode = WHL_HOMING_SENSOR_UP;

		if ( status.home == WHL_SENSOR_ON)
		{
			new_velocity = device->home_final_velocity;

			device->home_mode = WHL_HOMING_SENSOR_DOWN;
		}

		if ( abs(new_velocity) < device->home_final_velocity)
		{
			device->mode = WHL_DEVICE_STOPPING;

			device->home_mode = WHL_HOMING_WAIT;

			CFE_EVS_SendEvent(0,CFE_EVS_ERROR,
					"Home search error!");
			break;

		}

		WHL_RotateContinous(device,new_velocity);

		device->home_last_velocity = new_velocity;

		CFE_EVS_SendEvent(0, CFE_EVS_INFORMATION,
				"Home Sensor On Detected: [%d] %d",
				device->home_mode,
				new_velocity);


		break;

	case WHL_HOMING_SENSOR_DOWN:
		if (status.home == WHL_SENSOR_OFF)
		{
			WHL_Stop(device,WHL_STOP_EMERGENCY);

			device->home_mode = WHL_HOMING_MOVE_ZERO;

			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
					"Home Sensor Down Detected");
		}
		break;
	case WHL_HOMING_MOVE_ZERO:
		if ( status.moving == WHL_MOTION_OFF)
		{
			device->target_velocity = device->last_velocity;
			device->target_acceleration = device->last_acceleration;

			WHL_ResetDevice(device);
			WHL_SetZero(device);
			WHL_GetStatus(device);

			device->mode = WHL_DEVICE_WAIT;
			device->home_mode = WHL_HOMING_WAIT;


			CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
						"Home Search Finished");
		}

		break;

	}

}
void WHL_ProcessStopping(WHL_Device_t* device)
{
	WHL_DeviceStatus_t status = WHL_GetStatus(device);

	if ( status.moving == WHL_MOTION_OFF )
	{

		WHL_RotateFinishCmd.axis = device->axis;
		WHL_RotateFinishCmd.position = status.encoder_position_angle;
		WHL_RotateFinishCmd.target_position = device->target_position_angle;

		CFE_SB_TimeStampMsg(&WHL_RotateFinishCmd);
		CFE_SB_SendMsg(&WHL_RotateFinishCmd);

		device->mode = WHL_DEVICE_WAIT;
		WHL_ResetDevice(device);
	}

}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  */
/*                                                                            */
/* WHL_AppInit() --  initialization                                       */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
void WHL_AppInit(void)
{
    /*
    ** Register the app with Executive services
    */
    CFE_ES_RegisterApp() ;

    /*
    ** Register the events
    */ 
    CFE_EVS_Register(WHL_EventFilters,
                     sizeof(WHL_EventFilters)/sizeof(CFE_EVS_BinFilter_t),
                     CFE_EVS_BINARY_FILTER);


    /*
    ** Create the Software Bus command pipe and subscribe to housekeeping
    **  messages
    */
    CFE_SB_CreatePipe(&WHL_CommandPipe, WHL_PIPE_DEPTH,"WHL_CMD_PIPE");

    CFE_SB_Subscribe(WHL_APP_CMD_MID, WHL_CommandPipe);
    CFE_SB_Subscribe(WHL_APP_SEND_HK_MID, WHL_CommandPipe);

    WHL_ResetCounters();

    CFE_SB_InitMsg(&WHL_HkTelemetryPkt,
    		WHL_APP_HK_TLM_MID,
		WHL_APP_HK_TLM_LNGTH, TRUE);

    CFE_SB_InitMsg(&WHL_RotateFinishCmd,
    		WHL_APP_ROTATE_FINISH_MID,
		sizeof(WHL_RotateFinishTlm_t),TRUE);

    /*
     * Motor Initialize
     *
     */

    WHL_DeviceFD = WHL_OpenDevice(__DEVICE_PATH__);

    if ( WHL_DeviceFD != -1)
    {
		WHL_InitDevice(WHL_DeviceFD);

		WHL_Polarizer->fd    = WHL_DeviceFD;
		WHL_Polarizer->axis = 1;
		WHL_Polarizer->home_final_velocity = 1000;
		WHL_Polarizer->home_mode = WHL_HOMING_WAIT;
		WHL_Polarizer->home_start_velocity = 15000;
		WHL_Polarizer->home_last_velocity = 0;
		WHL_Polarizer->mode = WHL_DEVICE_WAIT;
		WHL_Polarizer->pulse_per_rev = 4000*__GEAR_RATIO__;
		WHL_Polarizer->target_position = 0;
		WHL_Polarizer->target_velocity 	  = 500000;
		WHL_Polarizer->target_acceleration = 500000;
		WHL_Polarizer->zero = 0;

		WHL_ResetDevice(WHL_Polarizer);

		WHL_Bandpass->fd	   = WHL_DeviceFD;
		WHL_Bandpass->axis    = 2;
		WHL_Bandpass->home_final_velocity = 1000;
		WHL_Bandpass->home_mode = WHL_HOMING_WAIT;
		WHL_Bandpass->home_start_velocity = 15000;
		WHL_Bandpass->home_last_velocity = 0;
		WHL_Bandpass->mode = WHL_DEVICE_WAIT;
		WHL_Bandpass->pulse_per_rev = 4000*__GEAR_RATIO__;
		WHL_Bandpass->target_position = 0;
		WHL_Bandpass->target_velocity = 500000;
		WHL_Bandpass->target_acceleration = 500000;
		WHL_Bandpass->zero = 0;

		WHL_ResetDevice(WHL_Bandpass);
    }

    OS_TaskInstallDeleteHandler(&WHL_DeleteCallBack);

    CFE_EVS_SendEvent (WHL_STARTUP_INF_EID, CFE_EVS_INFORMATION,
               "WHL App Initialized. Version %d.%d.%d.%d",
                WHL_APP_MAJOR_VERSION,
                WHL_APP_MINOR_VERSION, 
                WHL_APP_REVISION, 
                WHL_APP_MISSION_REV);
				
} /* End of WHL_AppInit() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  WHL_ProcessCommandPacket                                        */
/*                                                                            */
/*  Purpose:                                                                  */
/*     This routine will process any packet that is received on the WHL    */
/*     command pipe.                                                          */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void WHL_ProcessCommandPacket(void)
{
    CFE_SB_MsgId_t  MsgId;

    MsgId = CFE_SB_GetMsgId(WHL_MsgPtr);

    switch (MsgId)
    {
        case WHL_APP_CMD_MID:
            WHL_ProcessGroundCommand();
            break;

        case WHL_APP_SEND_HK_MID:
            WHL_ReportHousekeeping();
            break;

        default:
            WHL_HkTelemetryPkt.command_error_count++;
            CFE_EVS_SendEvent(WHL_COMMAND_ERR_EID,CFE_EVS_ERROR,
			"WHL: invalid command packet,MID = 0x%x", MsgId);
            break;
    }

    return;

} /* End WHL_ProcessCommandPacket */


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  WHL_ReportHousekeeping                                              */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function is triggered in response to a task telemetry request */
/*         from the housekeeping task. This function will gather the Apps     */
/*         telemetry, packetize it and send it to the housekeeping task via   */
/*         the software bus                                                   */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void WHL_ReportHousekeeping(void)
{
	WHL_HkTelemetryPkt.fd = WHL_DeviceFD;
	WHL_HkTelemetryPkt.axis_num = WHL_DEVICE_COUNT;

	uint8 axis;
	for ( axis = 0; axis < WHL_DEVICE_COUNT; axis++ )
	{
		WHL_HkTelemetryPkt.target_position_angle[axis] = WHL_DeviceList[axis].target_position_angle;
		WHL_HkTelemetryPkt.target_position[axis] = WHL_DeviceList[axis].target_position;
		WHL_HkTelemetryPkt.target_velocity[axis] = WHL_DeviceList[axis].target_velocity;
		WHL_HkTelemetryPkt.target_acceleration[axis] = WHL_DeviceList[axis].target_acceleration;

		WHL_HkTelemetryPkt.velocity[axis] = WHL_DeviceStatus[axis].velocity;
		WHL_HkTelemetryPkt.encoder_position[axis] = WHL_DeviceStatus[axis].encoder_position;
		WHL_HkTelemetryPkt.encoder_position_angle[axis] = WHL_DeviceStatus[axis].encoder_position_angle;

		WHL_HkTelemetryPkt.moving[axis] = WHL_DeviceStatus[axis].moving;

		WHL_HkTelemetryPkt.mode[axis] = WHL_DeviceList[axis].mode;
		WHL_HkTelemetryPkt.home_mode[axis] = WHL_DeviceList[axis].mode;
	}

	WHL_HkTelemetryPkt.command_error_count;
	WHL_HkTelemetryPkt.command_count;


    CFE_SB_TimeStampMsg((CFE_SB_Msg_t *) &WHL_HkTelemetryPkt);
    CFE_SB_SendMsg((CFE_SB_Msg_t *) &WHL_HkTelemetryPkt);
    return;

} /* End of WHL_ReportHousekeeping() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*  Name:  WHL_ResetCounters                                               */
/*                                                                            */
/*  Purpose:                                                                  */
/*         This function resets all the global counter variables that are     */
/*         part of the task telemetry.                                        */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * *  * *  * * * * */
void WHL_ResetCounters(void)
{
    /* Status of commands processed by the WHL App */
    WHL_HkTelemetryPkt.command_count       = 0;
    WHL_HkTelemetryPkt.command_error_count = 0;

    CFE_EVS_SendEvent(WHL_COMMANDRST_INF_EID, CFE_EVS_INFORMATION,
		"WHL: RESET command");
    return;

} /* End of WHL_ResetCounters() */

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
/*                                                                            */
/* WHL_VerifyCmdLength() -- Verify command packet length                   */
/*                                                                            */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * **/
boolean WHL_VerifyCmdLength(CFE_SB_MsgPtr_t msg, uint16 ExpectedLength)
{     
    boolean result = TRUE;

    uint16 ActualLength = CFE_SB_GetTotalMsgLength(msg);

    /*
    ** Verify the command packet length.
    */
    if (ExpectedLength != ActualLength)
    {
        CFE_SB_MsgId_t MessageID   = CFE_SB_GetMsgId(msg);
        uint16         CommandCode = CFE_SB_GetCmdCode(msg);

        CFE_EVS_SendEvent(WHL_LEN_ERR_EID, CFE_EVS_ERROR,
           "Invalid msg length: ID = 0x%X,  CC = %d, Len = %d, Expected = %d",
              MessageID, CommandCode, ActualLength, ExpectedLength);
        result = FALSE;
        WHL_HkTelemetryPkt.command_error_count++;
    }

    return(result);

} /* End of WHL_VerifyCmdLength() */



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Sample Lib function                                             */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int32 WHL_OpenDevice(char* port_name)
{

	int32 fd;

	fd = open(port_name, O_RDWR | O_NOCTTY | O_NDELAY);
	if (fd == -1)
	{
		/*
		* Could not open the port.
		*/
		CFE_EVS_SendEvent(0,CFE_EVS_ERROR,
				"Unable to open %s",port_name);
		return fd;

	}
	else
	{
		fcntl(fd, F_SETFL, 0);
	}

	CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
			"Port open %s",port_name);


	struct termios options;
	bzero(&options, sizeof(options)); /* clear struct for new port settings */

	/*
	 * Get the current options for the port...
	 */

	options.c_cflag = B9600 | CS8 | CLOCAL | CREAD;
	options.c_oflag = 0;
	options.c_iflag = IGNPAR | ICRNL;
	//options.c_lflag = 0;
	options.c_lflag = ICANON;

	options.c_cc[VINTR]    = 0;     /* Ctrl-c */
	options.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	options.c_cc[VERASE]   = 0;     /* del */
	options.c_cc[VKILL]    = 0;     /* @ */
	options.c_cc[VEOF]     = 4;     /* Ctrl-d */
	options.c_cc[VTIME]    = 1;     /* inter-character timer unused [tenth of seconds] */
	options.c_cc[VMIN]     = 0;     /* blocking read until 1 character arrives */
	options.c_cc[VSWTC]    = 0;     /* '\0' */
	options.c_cc[VSTART]   = 0;     /* Ctrl-q */
	options.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	options.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	options.c_cc[VEOL]     = 0;     /* '\0' */
	options.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	options.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	options.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	options.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	options.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &options); // Set the new options for the port...

	return fd;

}
void WHL_CloseDevice(int fd)
{
	close(fd);
}

int8 WHL_WriteString(int32 fd,char* msg)
{
	int length = strlen(msg);
	int written_length;
	int tries;


	for (tries=0;tries < 3; tries++)
	{
		written_length = write(fd, msg, length);
		OS_TaskDelay(DELAY_MSEC(written_length));

		if ( written_length == length)
			break;
	}

	if (written_length != length)
	{
		CFE_EVS_SendEvent(0,CFE_EVS_ERROR,
				"Write to serial failed! %s %d bytes written",msg,written_length);
		return -1;
	}

	OS_printf("Write to serial: %s",msg);


	return 0;

}
int8 WHL_ReadString(int32 fd,char* buf, int length)
{

	char* bufptr = buf;
	int  nbytes;       /* Number of bytes read */
	char test_buf[64];

	/* read characters into our string buffer until we get a CR or NL */


	while ((nbytes = read(fd, bufptr, buf + length - bufptr - 1)) > 0)
	{

		snprintf(test_buf,nbytes,"%s",bufptr);
		test_buf[nbytes] = 0;

		bufptr += nbytes;
		if (bufptr[-1] == '\n' || bufptr[-1] == '\r')
			break;

		if ((buf-bufptr + 1) > length)
		{
			CFE_EVS_SendEvent(0,CFE_EVS_ERROR,
					"Read buffer overflow!");
			return -1;
		}
	}

	*bufptr = '\0';

	OS_printf("Read from serial: %s",buf);


	return 0;

}

WHL_DeviceStatus_t WHL_GetStatus(WHL_Device_t* device)
{
	int32 fd = device->fd;
	uint8 axis = device->axis;

	char* recv;

	recv = WHL_SendCommand(fd,axis,"RP"); // Read Position
	if ( recv != 0)
		WHL_DeviceStatus[axis-1].encoder_position = atoi(recv);


	recv = WHL_SendCommand(fd,axis,"RV"); // Read Velocity
	if ( recv != 0)
		WHL_DeviceStatus[axis-1].velocity = atoi(recv);

	recv = WHL_SendCommand(fd,axis,"RUA"); // Read input A
	if ( recv != 0 )
		WHL_DeviceStatus[axis-1].home = atoi(recv);

	recv = WHL_SendCommand(fd,axis,"RBt"); // Read trajectory
	if ( recv != 0 )
		WHL_DeviceStatus[axis-1].moving = atoi(recv);

	WHL_DeviceStatus[axis-1].encoder_position_angle =
			( (WHL_DeviceStatus[axis-1].encoder_position - device->zero) / (float)device->pulse_per_rev ) * 3600;

	return WHL_DeviceStatus[axis-1];


}
void WHL_SetZero(WHL_Device_t* device)
{
	int32 fd = device->fd;
	uint8 axis = device->axis;

	WHL_SendCommand(fd,axis,"O=0");

}


void WHL_Rotate(WHL_Device_t* device, int32 angle, uint8 mode)
{
	int32 fd = device->fd;
	uint8 axis = device->axis;

	WHL_DeviceStatus_t status = WHL_GetStatus(device);

	int32 absolute_position = status.encoder_position%device->pulse_per_rev + device->zero;
	int32 position = (angle * device->pulse_per_rev) / 3600 + device->zero;


	WHL_SendCommand(fd,axis,"MP");

	switch ( mode )
	{
	case WHL_ROTATE_RELATIVE:

		sprintf(command_buffer,"D=%d",position);
		WHL_SendCommand(fd,axis,command_buffer);
		WHL_SendCommand(fd,axis,"G");

		device->target_position_angle = status.encoder_position_angle + angle;

		break;

	case WHL_ROTATE_ABSOLUTE:
		if ( device->target_position_angle != angle )
		{
			position = position - absolute_position;
			if ( position <  0 )
			{
				position += device->pulse_per_rev;
			}

			sprintf(command_buffer,"D=%d",position);
			WHL_SendCommand(fd,axis,command_buffer);
			WHL_SendCommand(fd,axis,"G");

			device->target_position_angle = angle;

		}

		break;

	}

	device->target_position = position;

	CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
			"Rotate mode %d, %d[P] AXIS %d with %d[P] %d[V] %d[A]",
			mode,
			absolute_position,
			axis,
			position,
			device->target_velocity,
			device->target_acceleration);



}
void WHL_RotateContinous(WHL_Device_t* device, int32 vel)
{
	int32 fd = device->fd;
	uint8 axis = device->axis;

	int32 velocity = (vel * MOTOR_VELOCITY_WEIGHT) / device->pulse_per_rev;


	WHL_SendCommand(fd,axis,"MV");

	sprintf(command_buffer,"V=%d",vel);
	WHL_SendCommand(fd,axis,command_buffer);

	WHL_SendCommand(fd,axis,"G");


}
void WHL_Home(WHL_Device_t* device)
{
	int32 fd = device->fd;
	uint8 axis = device->axis;

	device->last_velocity = device->target_velocity;
	device->last_acceleration = device->target_acceleration;
	device->home_mode = WHL_HOMING_WAIT;

}
void WHL_Stop(WHL_Device_t* device,uint8 mode)
{
	WHL_DeviceStatus_t status = WHL_GetStatus(device);

	int32 fd = device->fd;
	uint8 axis = device->axis;

	if ( mode == WHL_STOP_EMERGENCY )
	{
		WHL_SendCommand(fd,axis,"S");
	}
	else if ( mode == WHL_STOP_NORMAL )
	{
		WHL_SendCommand(fd,axis,"X");
	}


}

void WHL_InitDevice(int32 fd)
{

	uint8 i;

	sprintf(send_buffer,"%cECHO\n",128);
	WHL_WriteString(fd,send_buffer);

	sprintf(send_buffer,"%cBAUD9600\n",128);
	WHL_WriteString(fd,send_buffer);

	sprintf(send_buffer,"%cOCHN(RS2,0,N,9600,1,8,C)\n",128);
	WHL_WriteString(fd,send_buffer);

	sprintf(send_buffer,"%cECHO\n",128);
	WHL_WriteString(fd,send_buffer);

	tcflush(fd,TCIOFLUSH);

	for ( i = 0; i <= WHL_DEVICE_COUNT; i++)
	{
		sprintf(send_buffer,"%cSADDR%d\n",128, i );
		WHL_WriteString(fd,send_buffer);
		if ( i == 0)
		{
			sprintf(send_buffer,"%cECHO_OFF\n",128);
			WHL_WriteString(fd,send_buffer);
			continue;
		}

		sprintf(send_buffer,"%cECHO\n",128 + i );
		WHL_WriteString(fd,send_buffer);
		sprintf(send_buffer,"%cSLEEP\n",128 + i );
		WHL_WriteString(fd,send_buffer);

	}
	sprintf(send_buffer,"%cWAKE\n",128 );
	WHL_WriteString(fd,send_buffer);

	tcflush(fd,TCIOFLUSH);


}
void WHL_ResetDevice(WHL_Device_t* device)
{
	int32 fd   = device->fd;
	uint8 axis = device->axis;

	OS_printf("Reset Device: fd=%d, axis=%d\n",fd,axis);

	WHL_SendCommand(fd, axis,"SLE");
	WHL_SendCommand(fd, axis,"UDM");
	WHL_SendCommand(fd, axis,"UCP");
	WHL_SendCommand(fd, axis,"UDI");
	WHL_SendCommand(fd, axis,"UCI");
	WHL_SendCommand(fd, axis,"SLD");
	WHL_SendCommand(fd, axis,"ZS");

	sprintf(command_buffer,"V=%d", device->target_velocity);
	WHL_SendCommand(fd,axis,command_buffer);

	sprintf(command_buffer,"A=%d", device->target_acceleration);
	WHL_SendCommand(fd,axis,command_buffer);

}

void WHL_ConfigureDevice(WHL_MotionConfig_t* config)
{
	WHL_Device_t* device = WHL_GetDevice(config->axis);

	int32 val;

	val = (config->velocity * MOTOR_VELOCITY_WEIGHT)/60; // convert unit RPM to pulses
	device->target_velocity = val;
	sprintf(command_buffer,"V=%d",val);
	WHL_SendCommand(device->fd,device->axis,command_buffer);

	val = config->accereleate * MOTOR_ACCELATION_WEIGHT;
	device->target_acceleration = val;
	sprintf(command_buffer,"A=%d",val);
	WHL_SendCommand(device->fd,device->axis,command_buffer);

	CFE_EVS_SendEvent(0,CFE_EVS_INFORMATION,
			"WHL: Configuration V=%d A=%d",
			device->target_velocity,
			device->target_acceleration);

}
WHL_Device_t*  WHL_GetDevice(uint8 axis)
{
	if ( axis < 1 || axis > WHL_DEVICE_COUNT)
		return 0;

	return &WHL_DeviceList[axis-1];

}
void WHL_DeleteCallBack()
{
    OS_printf("WHL delete callback -- Closing File Descripters.\n");

    if ( WHL_DeviceFD > 0 )
    {
			close(WHL_DeviceFD);
			WHL_DeviceFD = -1;
    }


}
char* WHL_SendCommand(int32 fd, uint8 axis, char* command)
{
	OS_printf("SendCommand: %d, %d, %s\n",fd,axis,command);

	int length = strlen(command);

	if ( length + 2  > SEND_SIZE - 1)
	{
		CFE_EVS_SendEvent(0, CFE_EVS_ERROR,
				"Buffer Overflow: Buffer Size: %d, Text(%d): %s",
				SEND_SIZE,
				length,
				command);
		return 0;
	}

	sprintf(send_buffer,"%c%s\n", axis + 128, command); // length + 2

	uint8 new_length = strlen(send_buffer);
	uint8 tries, cmp_send_recv;
	for (tries = 0 ; tries < 3; tries++)
	{
		tcflush(fd,TCIOFLUSH);
		WHL_WriteString(fd,send_buffer);
		WHL_ReadString(fd,recv_buffer,sizeof(recv_buffer));

		cmp_send_recv = strcmp(recv_buffer,send_buffer);
		if ( cmp_send_recv == 0)
			break;

	}

	if ( cmp_send_recv != 0)
	{
		OS_printf("Error: Command NOT sent\n");
		return 0;
	}


	sprintf(send_buffer,"%cECHO\n", axis + 128);
	WHL_WriteString(fd,send_buffer);
	WHL_ReadString(fd,recv_buffer,sizeof(recv_buffer));
	if ( strcmp(recv_buffer,send_buffer) == 0)
	{
		OS_printf("Info: Non return value\n");
		return 0;
	}

	strcpy(report_buffer,recv_buffer);

	WHL_ReadString(fd,recv_buffer,sizeof(recv_buffer));
	if ( strcmp(recv_buffer,send_buffer) != 0 )
	{
		OS_printf("Warning: ECHO NOT received\n");
	}

	return report_buffer;

}
