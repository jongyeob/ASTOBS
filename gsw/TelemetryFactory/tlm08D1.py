'''
Created on 2017. 8. 16.

@author: jongyeob
'''
'''
typedef struct
{
    uint8            TlmHeader[CFE_SB_TLM_HDR_SIZE];
    uint8            quickview_enable;
    uint8            save_enable;
    uint8           image_analysis_enable;

    uint8           command_error_count;
    uint8           command_count;

}   OS_PACK DAP_HkTlm_t  ;
'''
tlm0x08D1_struct = [
                    ('B','quickview_enable'),
                    ('B','save_enable'),
                    ('B','image_analysis_enable'),
                    ('B','command_error_count'),
                    ('B','command_count')
                    ] 