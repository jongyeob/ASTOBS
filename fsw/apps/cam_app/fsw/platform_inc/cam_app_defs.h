/*
 * cam_app_defs.h
 *
 *  Created on: 2017. 7. 4.
 *      Author: jongyeob
 */

#ifndef APPS_CAM_APP_FSW_SRC_CAM_APP_DEFS_H_
#define APPS_CAM_APP_FSW_SRC_CAM_APP_DEFS_H_

#define CAM_PIPE_DEPTH                     32


#define CAM_IMAGE_WIDTH				   700
#define CAM_IMAGE_HEIGHT			   	   700
#define CAM_IMAGE_BITPIX			       16
#define CAM_IMAGE_BIN_X						1
#define CAM_IMAGE_BIN_Y						1
#define CAM_IMAGE_OFFSET_X					674
#define CAM_IMAGE_OFFSET_Y					674
#define CAM_DEFAULT_TEMPERATURE				20

#define CAM_TEST_MODE_INCRESED				1
#define CAM_TEST_MODE_RANDOM					2

#define CAM_AUTO_EXPOSURE_OFF				0
#define CAM_AUTO_EXPOSURE_ON					1

#define CAM_MODE_WAIT						0
#define CAM_MODE_GRAB						1
#define CAM_MODE_GRABING						2
#define CAM_MODE_STOPPING					3

#define CAM_AUTO_EXPOSURE_TIME_MAX          1000
#define CAM_AUTO_EXPOSURE_TIME_MIN          100

typedef struct
{
	int32  handle;
	float   target_temperature;
	float   temperature;

	uint32 frame_width;
	uint32 frame_height;
	int8   frame_bitpix;
	uint32 frame_size;
	uint32 frame_bytesize;

	uint8 camera_bin_x;
	uint8 camera_bin_y;
	uint16 camera_offset_x;
	uint16 camera_offset_y;

	uint32 exposure_time;
	int32  polarization_angle;
	int32  bandpass_angle;


}CAM_APP_DEVICE_t;

typedef struct
{
	uint32 width;
	uint32 height;
	int8   bitpix;
	int8   temperature;
	uint8  observation_mode;
	uint8  spare;
	uint32 observation_time;
	uint32 observation_time2;
	uint32 exposure_time;
	uint32 frame_number;
	int32  polarization_angle;
	int32  bandpass_angle;

}CAM_APP_IMAGE_HEADER_t;

typedef struct
{
	CAM_APP_IMAGE_HEADER_t header;
	uint8*	data;
}CAM_APP_IMAGE_t;

typedef struct
{
	uint8 mode;
	uint8  observation_mode;
	uint32 frame_number;
	double auto_exposure_adjust;
	uint8 auto_exposure_enable;
	uint32 auto_exposure_time;
	uint32 target_repeat;
	uint32 current_repeat;
}CAM_APP_DATA_t;



#endif /* APPS_CAM_APP_FSW_SRC_CAM_APP_DEFS_H_ */
