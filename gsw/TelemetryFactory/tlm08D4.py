'''
Created on 2017. 8. 16.

@author: jongyeob
'''
'''
typedef struct 
{
    uint8              TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint32             observation_time;
    uint32             exposure_time;
    uint32             frame_number;
    int32              polarization_position_angle;
    int32              bandpass_position_angle;
    uint32                optimal_exposure_time;
    uint32                frame_pixel_min;
    uint32                frame_pixel_max;
    uint32                frame_pixel_mean;
    int8               temperature;
    uint8              observation_mode;
} OS_PACK DAP_ImageProcessingTlm_t;


'''
tlm0x08D4_struct = [('I','observation_time'),
                    ('I','exposure_time'),
                    ('I','frame_number'),
                    ('i','polarization_position_angle'),
                    ('i','bandpass_position_angle'),
                    ('I','optimal_exposure_time'),
                    ('I','frame_pixel_min'),
                    ('I','frame_pixel_max'),
                    ('I','frame_pixel_mean'),
                    ('b','temperature'),
                    ('B','observation_mode')
                    ] 