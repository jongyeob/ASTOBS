'''
Created on 2017. 8. 16.

@author: jongyeob
'''

_tlm0x8D2_image_size = 2048 
tlm0x08D2_struct = [('I','frame_number'),
             ('I','frame_position'),
             ('{}B'.format(_tlm0x8D2_image_size),'data')]
