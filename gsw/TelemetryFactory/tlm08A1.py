'''
Created on 2017. 8. 16.

@author: jongyeob
'''
'''
typedef struct 
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];

    int32   fd;

    int32   target_position_angle[WHL_DEVICE_COUNT];
    int32    target_position[WHL_DEVICE_COUNT];
    int32    target_velocity[WHL_DEVICE_COUNT];
    int32   target_acceleration[WHL_DEVICE_COUNT];

    int32   velocity[WHL_DEVICE_COUNT];
    int32   encoder_position[WHL_DEVICE_COUNT];
    int32   encoder_position_angle[WHL_DEVICE_COUNT];

    int8     moving[WHL_DEVICE_COUNT];
    uint8     mode[WHL_DEVICE_COUNT];
    uint8     home_mode[WHL_DEVICE_COUNT];
    uint8   axis_num;

    uint8   command_error_count;
    uint8   command_count;

}   OS_PACK whl_hk_tlm_t  ;
'''

tlm0x08A1_struct = [
    ('i','fd'),
    ('2i','target_position_angle'),
    ('2i','target_position'),
    ('2i','target_velocity'),
    ('2i','target_acceleration'),
    ('2i','velocity'),
    ('2i','encoder_position'),
    ('2i','encoder_position_angle'),
    ('2b','moving'),
    ('2B','mode'),
    ('2B','home_mode'),
    ('B','axis_num'),
    ('B','command_error_count'),
    ('B','command_count')
                    ]    
