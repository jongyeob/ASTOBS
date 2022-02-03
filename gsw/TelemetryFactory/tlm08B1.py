'''
Created on 2017. 8. 16.

@author: jongyeob
'''
'''
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
'''

tlm0x08B1_struct = [
    ('i', 'handle'),
    ('I', 'frame_width'),
    ('I', 'frame_height'),
    ('I', 'exposure_time'),
    ('i',  'polarization_angle'),
    ('i',  'bandpass_angle'),

    ('I', 'frame_number'),
    ('I', 'auto_exposure_time'),

    ('I', 'target_repeat'),
    ('I', 'current_repeat'),

    ('f',   'target_temperature'),
    ('f',   'temperature'),
    ('d',  'auto_exposure_adjust'),

    ('H', 'camera_offset_x'),
    ('H', 'camera_offset_y'),

    ('B',  'mode'),
    ('B',  'observation_mode'),
    ('B',  'auto_exposure_enable'),
    ('b',   'frame_bitpix'),

    ('B',  'camera_bin_x'),
    ('B',  'camera_bin_y'),
    ('B',  'command_error_count'),
    ('B',  'command_count')]    
