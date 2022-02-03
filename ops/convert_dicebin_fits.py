# -*- coding: utf-8 -*-
"""
Created on Sat Aug 05 13:29:11 2017

@author: leeji
"""

'''
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

'''
import os
import glob
import struct
import numpy
from astropy.io import fits



bin_file_list = glob.glob("*.bin")
header_key      = ['width','height','bitpix','temperature','observation_mode',
                   'observation_time','observation_time2','exposure_time','frame_number','polarization_angle','bandpass_angle']
fits_header_key = ['','','','TEMP','OBSMODE',
                   'OBSTIME','OBSTIME2','EXPTIME','FRN','POLANGLE','BPANGLE']
header_format = 'IIbbB1xIIIIii'
header_size   = struct.calcsize(header_format)
  
for bin_file in bin_file_list:
    fits_file = os.path.splitext(bin_file)[0] + '.fits'
    if os.path.exists(fits_file):
        print( "File already exists! {}".format(fits_file) )
        continue
    
    fp = open(bin_file)
    
    header_value = struct.unpack(header_format, fp.read(header_size))
    header = dict( zip(header_key,header_value))
    
    image_size = header['width'] * header['height']
    
    image = numpy.fromfile(fp,numpy.dtype('u2'),image_size)
    image.shape = (header['width'],header['height'])
    
    read_size = fp.tell()
    fp.seek(0,2)
    file_size = fp.tell()
    fp.close()
    
    
    if  read_size != file_size :
		print "Read {} bytes, but File is {} bytes".format(read_size,file_size)
	
       
    hdr = fits.Header()
    for fits_key, key in zip(fits_header_key, header_key):
        if not fits_key:
            continue
        
        hdr[fits_key] = int(header[key])

    prihdu = fits.PrimaryHDU(image,hdr)   
    prihdu.writeto(fits_file)
    
  
        
    print "Convert {} -> {}".format(bin_file,fits_file)

