'''
Created on 2017. 8. 16.

@author: jongyeob
'''
'''
typedef struct 
{
    uint8            TlmHeader[CFE_SB_TLM_HDR_SIZE];

    uint32            sequence_start_time;
    uint32            sequence_end_time;
    uint32            frame_start_sec;
    uint32            frame_start_msec;
    int32              frame_remain_time;
    int32              frame_elapse_msec;

    uint16            sequence_repeat;
    uint16            sequence_repeat_status;

    uint8            sequence_id;
    uint8              sequence_mode;
    uint8              wheel_motion_status;
    uint8            command_error_count;
    uint8            command_count;

}   OS_PACK OBS_HkTlm_t  ;
'''

tlm0x08C1_struct = [
    ('I','sequence_start_time'),
    ('I','sequence_end_time'),
    ('I','frame_start_sec'),
    ('I','frame_start_msec'),
    ('i','frame_remain_time'),
    ('i','frame_elapse_msec'),
    ('H','sequence_repeat'),
    ('H','sequence_repeat_status'),
    ('B','sequence_id'),
    ('B','sequence_mode'),
    ('B','wheel_motion_status'),
    ('B','command_error_count'),
    ('B','command_count'),
    ]    
