'''
Created on 2017. 5. 24.

@author: parkj
'''
import struct

class Telemetry():
    
    def __init__(self,type_list):
        self.type_list = type_list
        
    def unpack(self,datagram):
        data_list = []
        
        i = 12
        for data_type,name in self.type_list:
            
            data_size = struct.calcsize(data_type)
            
            if not data_type.endswith('x'):
                data_type = '<' + data_type       
                unpack_data = struct.unpack(data_type,datagram[i:i+data_size])
                if len(unpack_data) == 1:
                    unpack_data = unpack_data[0]
                data_list.append((name,unpack_data))
                
            i += data_size
        
        return data_list



def test_telemetry():
    data_name   = 'test'
    data_format = 'h'
    raw_data = 255
    
    test_tlm = Telemetry([(data_name,data_format)])
    
    test_datagram = struct.pack(data_format,raw_data)
    
    unpack_tlm = dict(test_tlm.unpack(test_datagram))
    
    assert unpack_tlm[data_name] == raw_data
    