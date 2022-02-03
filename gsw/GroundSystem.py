'''
Created on 2017. 5. 18.

@author: jongyeob
'''
#cFS Ground System Version 2.0.0
#
#!/usr/bin/env python
#
import sys
import os
import socket


from PyQt5.QtCore import pyqtSignal, QObject, QThread, QTimer
from PyQt5.QtWidgets import QApplication,QWidget
from PyQt5.QtGui import QPixmap,QImage

from ui.MainWindow import Ui_MainWindow

import logging
import ConfigParser

from datetime import datetime, timedelta,time
import calendar
import time
import struct
import shlex
import subprocess

import array


from TelemetrySystem import TelemetryReceiver
from TelemetryFactory import *
from PyQt5.Qt import QMessageBox, pyqtSlot
from PyQt5.uic.Compiler.qtproxies import QtCore


logging.basicConfig(level=10)

configFile = 'GroundSystem.ini'
config = ConfigParser.SafeConfigParser()


cmdUtilExe = 'bin/cmdUtil'
if sys.platform.startswith('win'):
    cmdUtilExe += '.exe'

cmdUtilExe = os.path.normpath(cmdUtilExe)

time_format   = "%Y-%m-%d %H:%M:%S"
msg_box       = None
#
# CFS Ground System: Setup and manage the main window
#

# class TimeDisplay(QtCore.QThread):
#     
#     def __init__(self, mainWindow):
#         QtCore.QThread.__init__(self)
# 
#         # Setup signal to communicate with front-end GUI
#         self.signa = QtCore.SIGNAL("TimeRefresh")
# 
#     def run(self):
#         while True:
#             self.emit(self.signal, "")
            


            

class GroundSystem(QWidget):

    bandpassAxis  = 1
    polarizerAxis = 2
    
    bandpassUnitAngle = 90
    polarizerUnitAngle = 60
    
    qv_frame_number = 0
    qv_width  = 256
    qv_height = 256
    
    last_sequence_name = ''
    
    def __init__(self, parent=None):
        super(GroundSystem,self).__init__()
        
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        
        val = config.get('MainWindow','address-sbc')
        self.ui.editAddress.setText(val)
        
        val = config.get('MainWindow','address')
        self.ui.editTelemetryAddress.setText(val)
        
        self.ui.editCameraTemperature.setText('10')   
        self.ui.editObservationNum.setText('0')
        self.ui.editRepeatNum.setText('1')
        self.ui.spinAEAdjust.setValue(0)
        self.ui.spinAEAdjust.setRange(-10,10)
        self.ui.spinAEAdjust.setSingleStep(0.1)
        
        val = config.get("MainWindow","camera-offset-x")
        self.ui.editCameraOffsetX.setText(val)
        val = config.get("MainWindow", "camera-offset-y")
        self.ui.editCameraOffsetY.setText(val)
        
        
        self.timerUpdateDisplay = QTimer()
        self.timerUpdateDisplay.timeout.connect(self.updateDisplay)
        self.timerUpdateDisplay.start(100)      
        
        self.timerUpdateStatus = QTimer()
        self.timerUpdateStatus.timeout.connect(self.updateStatus)
        self.timerUpdateStatus.start(1000)
        
        self.tlmRecvThread = None
        
        self.qv_data = bytearray(self.qv_width*self.qv_height)
        
        self.ui.buttonInitialize.clicked.connect(self.clickInitialize)

        self.ui.buttonSend.clicked.connect(self.clickSend)
        self.ui.buttonClear.clicked.connect(self.clickClear)
        self.ui.buttonConnect.clicked.connect(self.clickConnect)
        self.ui.buttonSetMstAsCurrent.clicked.connect(self.clickSetMstAsCurrent)
        self.ui.buttonConfigTime.clicked.connect(self.clickConfigTime)
        
                
        self.ui.radioBandpass1.clicked.connect(lambda:self.clickWheelRotatePreset(self.bandpassAxis,1))
        self.ui.radioBandpass2.clicked.connect(lambda:self.clickWheelRotatePreset(self.bandpassAxis,2))
        self.ui.radioBandpass3.clicked.connect(lambda:self.clickWheelRotatePreset(self.bandpassAxis,3))
        self.ui.radioBandpass4.clicked.connect(lambda:self.clickWheelRotatePreset(self.bandpassAxis,4))
        
        self.ui.radioPolarizer1.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,1))
        self.ui.radioPolarizer2.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,2))
        self.ui.radioPolarizer3.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,3))
        self.ui.radioPolarizer4.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,4))
        self.ui.radioPolarizer5.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,5))
        self.ui.radioPolarizer6.clicked.connect(lambda:self.clickWheelRotatePreset(self.polarizerAxis,6))
    
        self.ui.buttonBandpassHome.clicked.connect(lambda:self.clickWheelHome(self.bandpassAxis))
        self.ui.buttonBandpassRotate.clicked.connect(lambda:self.clickWheelRotate(self.bandpassAxis))
        self.ui.buttonBandpassStop.clicked.connect(lambda:self.clickWheelStop(self.bandpassAxis))
        self.ui.buttonBandpassConfig.clicked.connect(lambda:self.clickWheelConfig(self.bandpassAxis))
        
        self.ui.buttonPolarizerHome.clicked.connect(lambda:self.clickWheelHome(self.polarizerAxis))
        self.ui.buttonPolarizerRotate.clicked.connect(lambda:self.clickWheelRotate(self.polarizerAxis))
        self.ui.buttonPolarizerStop.clicked.connect(lambda:self.clickWheelStop(self.polarizerAxis))
        self.ui.buttonPolarizerConfig.clicked.connect(lambda:self.clickWheelConfig(self.polarizerAxis))
        
        self.loadSequenceCombo()
        self.ui.comboSequence.setEditable(True)
                
        self.ui.comboSequence.currentIndexChanged[str].connect(self.changeSequenceCombo)
        self.ui.buttonSaveSequence.clicked.connect(lambda:self.saveSequenceCombo(''))
                
        self.ui.buttonObservationStart.clicked.connect(self.clickObservationStart)
        self.ui.buttonObservationStop.clicked.connect(self.clickObservationStop)
        
        self.ui.buttonCapture.clicked.connect(self.clickCapture)
        self.ui.buttonCaptureStop.clicked.connect(self.clickCaptureStop)
        self.ui.buttonConfigCamera.clicked.connect(self.clickConfigCamera)

        
        self.ui.buttonPerfStart.clicked.connect(self.clickPerfStart)
        self.ui.buttonPerfStop.clicked.connect(self.clickPerfStop)
                
        self.ui.buttonConfigDAP.clicked.connect(self.clickConfigDAP)
        
        #self.ui.labelImage.setScaledContents(True)
        
               
        self.telemetry = {}
        self.pktCount  = {}
        
        self.initTelemetry("0x08A1")
        self.initTelemetry("0x08B1")
        self.initTelemetry("0x08C1")
        self.initTelemetry("0x08D1")
        self.initTelemetry("0x08D2")
        self.initTelemetry("0x08D4")
        
        self.start_time = 0
        self.end_time = 0
         
        
    def initTelemetry(self,id):
        self.pktCount.setdefault(id,0)
        self.telemetry.setdefault(id,{})
        
    def updateQuickViewStatus(self):
        
        mission_epoch = config.getfloat('MainWindow','mission-epoch')
        time_epoch = config.getfloat('MainWindow','time-epoch')
        
        qv_text = ''
        
        if self.telemetry['0x08D4']:
            tlm = self.telemetry['0x08D4']
            obs_time = tlm['observation_time']
            obs_time_string = datetime.fromtimestamp(obs_time + time_epoch).strftime("%Y%m%d_%H%M%S")

            qv_text += 'Observation Time: {} [{}]\n'.format(
                obs_time_string,
                obs_time)            
            qv_text += 'Frame Number: {}\n'.format(tlm['frame_number'])
            qv_text += 'Observation Mode: {}\n'.format(tlm['observation_mode'])
            qv_text += 'Exposure Time: {}ms\n'.format(tlm['exposure_time'])

            qv_text += 'Polarization Angle: {}\n'.format(tlm['polarization_position_angle']%3600)
            qv_text += 'Bandpass Angle: {}\n'.format(tlm['bandpass_position_angle']%3600)
            
            qv_text += 'CCD Temperature: {}\n'.format(tlm['temperature'])
            
            qv_text += 'Minimum Pixel Value: {}\n'.format(tlm['frame_pixel_min'])
            qv_text += 'Maximum Pixel Value: {}\n'.format(tlm['frame_pixel_max'])
            qv_text += 'Median Pixel Value: {}\n'.format(tlm['frame_pixel_mean'])
            qv_text += 'Optimal Exposure Time: {}\n'.format(tlm['optimal_exposure_time'])
            
            self.ui.qvOutput.setText(qv_text)

        
    def updateStatus(self):
        
        status_text = ''
        if self.telemetry['0x08C1']:
            tlm = self.telemetry['0x08C1']
            status_text += 'OBS[{command_count}/{command_error_count}]: '.format(**tlm)
            status_text += 'ID[{sequence_id}] '.format(**tlm)
            status_text += 'T[{sequence_start_time}-{sequence_end_time}] '.format(**tlm)
            status_text += '({sequence_repeat_status}/{sequence_repeat})'.format(**tlm)
            
            if self.telemetry['0x08D1']:
                tlm = self.telemetry['0x08D1']
    
                status_text += '  DAP[{command_count}/{command_error_count}]: '.format(**tlm)
                status_text += 'S{save_enable} I{image_analysis_enable} Q{quickview_enable}'.format(**tlm)
                
            status_text += '\n'
            
        
        if self.telemetry['0x08A1']:
            tlm = self.telemetry['0x08A1']
            
            status_text += 'WHL[{command_count}/{command_error_count}]: h:{fd} '.format(**tlm)
            status_text += 'F[V:{target_velocity[1]},A:{target_acceleration[1]}] '.format(**tlm)
            status_text += 'P[V:{target_velocity[0]},A:{target_acceleration[0]}]\n'.format(**tlm)
            
            status_text += '  FIL[{moving[1]}:'.format(**tlm) 
            status_text += '{target_position_angle[1]}<{encoder_position_angle[1]},{encoder_position[1]}]'.format(**tlm)
            status_text += '  POL[{moving[0]}: '.format(**tlm) 
            status_text += '{target_position_angle[0]}<{encoder_position_angle[0]},{encoder_position[0]}]'.format(**tlm)
            status_text += '\n'
            
        if self.telemetry['0x08B1']:
            tlm = self.telemetry['0x08B1']
            status_text += 'CAM[{command_count}/{command_error_count}]: '.format(**tlm)
            status_text += 'h:{handle} {frame_width}:{camera_bin_x}x{frame_height}:{camera_bin_y}x{frame_bitpix} '.format(**tlm)
            status_text += '({camera_offset_x},{camera_offset_y}) '.format(**tlm)
            status_text += 'T:{temperature}/{target_temperature} AE[{auto_exposure_enable}:{auto_exposure_adjust}]'.format(**tlm)
            status_text += '\n'
            
            status_text += '  IMG[{observation_mode}]: {frame_number}({current_repeat}/{target_repeat})'.format(**tlm)
            status_text += 'T{exposure_time}/{auto_exposure_time} '.format(**tlm)
            status_text += 'P{polarization_angle} F{bandpass_angle} '.format(**tlm)
            
            status_text += '\n'
            
        
            
            
            
        self.ui.hkOutput.setText(status_text)
        
        _,elapsed_time_sec,_ = self.getElapsedTime()
        
        if self.start_time and self.start_time > elapsed_time_sec :
            remain_time_from_start = timedelta(seconds=(self.start_time -elapsed_time_sec ))
            self.ui.buttonObservationStart.setText("Start({})".format(remain_time_from_start))
        else:
            self.ui.buttonObservationStart.setText("Start")
        
        if self.end_time and self.end_time > elapsed_time_sec:
            remain_time_from_end = timedelta(seconds=(self.end_time - elapsed_time_sec))
            self.ui.buttonObservationStop.setText("Stop({})".format(remain_time_from_end))
        else:
            self.ui.buttonObservationStop.setText("Stop")
        
    def processTelemetry(self,datagram):
        # Packet Header
        #   uint16  StreamId;   0   
        #   uint16  sequence;   2
        #   uint16  length;     4 
        
        # Read the telemetry header
        
        
        stream_id, sequence, length = struct.unpack(">HHH",datagram[:6])
        pktId = '0x' + '{:04x}'.format(stream_id).upper()
        

        if pktId == '0x0808': # Event
            app_name = "".join(struct.unpack("<20s",datagram[12:32]))
            event_text = "".join(struct.unpack("<122sxx",datagram[44:]))
            app_name = app_name.split("\0")[0]
            event_text = event_text.split("\0")[0]
            event_string = app_name + " : " + event_text
            self.ui.eventOutput.append(event_string)

        if pktId == '0x08C1': # for app    
            self.pktCount['0x08C1'] += 1
            
            unpack_tlm = tlm0x08C1.unpack(datagram)
            self.telemetry["0x08C1"].update(dict(unpack_tlm))
            
        if pktId == '0x08A1': # for whl
            self.pktCount['0x08A1'] += 1
            
            unpack_tlm = tlm0x08A1.unpack(datagram)
            self.telemetry['0x08A1'].update(dict(unpack_tlm))
            
            
        if pktId == '0x08B1': # for cam    
            self.pktCount['0x08B1'] += 1
            
            unpack_tlm = tlm0x08B1.unpack(datagram)
            self.telemetry['0x08B1'].update(dict(unpack_tlm))
        
            
        if pktId == '0x08D2': # for cam quick view    
            self.pktCount['0x08D2'] += 1
            
            unpack_tlm = tlm0x08D2.unpack(datagram)
            self.telemetry["0x08D2"].update(dict(unpack_tlm))
            
            frame_number    = self.telemetry['0x08D2']['frame_number']
            frame_position  = self.telemetry['0x08D2']['frame_position']
            data            = self.telemetry['0x08D2']['data']
            data_size = len(data)
            
            if self.qv_frame_number < frame_number:
                self.qv_frame_number = frame_number
            
            if frame_number == self.qv_frame_number:
                self.qv_data[frame_position:frame_position+data_size] = data
            
                
            self.refreshQuickView()
            
        if pktId == '0x08D1':
            self.pktCount['0x08D1'] += 1
            
            unpack_tlm = tlm0x08D1.unpack(datagram)
            self.telemetry["0x08D1"].update(dict(unpack_tlm))
            
            
        if pktId == '0x08D4':
            self.pktCount['0x08D4'] += 1
            
            unpack_tlm = tlm0x08D4.unpack(datagram)
            self.telemetry["0x08D4"].update(dict(unpack_tlm))
            
            self.updateQuickViewStatus()
            
            
            
    def updateDisplay(self):
        
        self.utcnow = datetime.utcnow()
        self.ltnow  = datetime.now()
        
        
        mission_epoch = config.getfloat('MainWindow','mission-epoch')
        time_epoch = config.getfloat('MainWindow','time-epoch')
        
        
        mission_elapsed_time = timedelta(seconds=(time.time() - mission_epoch - time_epoch))

        self.ui.editLocalTime.setText(self.utcnow.strftime(time_format))
        self.ui.editUniversalTime.setText(self.ltnow.strftime(time_format))
        self.ui.editMST.setText(datetime.fromtimestamp(mission_epoch + time_epoch).strftime(time_format))
        self.ui.editMET.setText(str(mission_elapsed_time))
        
    def clickInitialize(self):
        
        if self.tlmRecvThread:
            
            self.tlmRecvThread.stop()
            self.tlmRecvThread = None
            self.ui.editAddress.setEnabled(True)
            self.ui.editTelemetryAddress.setEnabled(True)
            self.ui.buttonInitialize.setText("Initialize")
        
        else:
            
            addr = str(self.ui.editAddress.text())
            
            self.tlmRecvThread = TelemetryReceiver(addr)
            self.tlmRecvThread.signalTelemetry.connect(self.processTelemetry)
            
            self.tlmRecvThread.start()
            
            self.ui.editAddress.setEnabled(False)
            self.ui.editTelemetryAddress.setEnabled(False)          
            self.ui.buttonInitialize.setText('Change')

        
        
    def clickClear(self):
        self.ui.editCmdCode.setText('')
        self.ui.editMsgID.setText('')
        self.ui.editParameter.setText('')
        
    def clickSend(self):
        
        msgId = '0x'+str(self.ui.editMsgID.text())
        cmdCode = str(self.ui.editCmdCode.text())
        endian = 'LE'
        parameter_string = str(self.ui.editParameter.text())
        
        self.sendCommand(msgId, cmdCode, endian, parameter_string)
                
    def clickConnect(self):   
            
        address = 's:{}'.format(self.ui.editTelemetryAddress.text())
        msgId = '0x1880'
        cmdCode = 6
        endian = 'LE'
            
        self.sendCommand(msgId, cmdCode, endian,address)
        
            
    
    def getElapsedTime(self):
        
        elapse = time.time() - config.getfloat('MainWindow','time-epoch')
                  
        elapse_second = int(elapse)
        elapse_msecond = int((elapse - elapse_second) * 10**6)
        
        return elapse,elapse_second,elapse_msecond
        
    def clickSetMstAsCurrent(self):
        
        elapse,elapse_second,elapse_subsecond = self.getElapsedTime()
        
        parameter = 'l:{} l:{}'.format(elapse_second,elapse_subsecond)
        
        self.sendCommand('0x1805', 8, 'LE', parameter)
                    
        config.set('MainWindow','mission-epoch',str(elapse))
        
    
    def clickConfigTime(self):
        
        text = str(self.ui.editObsStart.text())
        config.set('MainWindow','observation-start',text)
        
        text = str(self.ui.editObsEnd.text())
        config.set('MainWindow','observation-end',text)
                
        elapse,elapse_second,elapse_subsecond = self.getElapsedTime()
                                    
        parameter = 'l:{} l:{}'.format(elapse_second,elapse_subsecond)
        self.sendCommand('0x1805', 7, 'LE', parameter) # Time Set SCT
        
        time_format   = "%Y-%m-%d %H:%M:%S"
        
            
        WriteConfigure(configFile)
                
    def clickWheelRotatePreset(self,axis,position):
        unit_angle = [0,self.bandpassUnitAngle, self.polarizerUnitAngle][axis]
        
        angle = (position -1)*unit_angle
        
        parameter = "l:{} b:{} b:1".format(angle,axis)
        self.sendCommand('0x18A1', 3, 'LE', parameter)
        
    def clickWheelRotate(self,axis):
    
        angle = [0,self.ui.editBandpassAngle, self.ui.editPolarizerAngle]    
        angle_text = str(angle[axis].text())
        
        parameter = "l:{} b:{} b:0".format(angle_text,axis)
        self.sendCommand('0x18A1', 3, 'LE', parameter )
        
    def clickWheelStop(self,axis):
        
        parameter = "l:0 b:{} b:0".format(axis)
        self.sendCommand('0x18A1', 2, 'LE', parameter)
        
    def clickWheelHome(self,axis):
        
        parameter = "l:0 b:{} b:0".format(axis)
        self.sendCommand('0x18A1', 4, 'LE', parameter)
        
    def clickWheelConfig(self,axis):
        
        editVelocity = [0, self.ui.editBandpassVelocity, self.ui.editPolarizerVelocity]
        editAcceleration = [0, self.ui.editBandpassAcceleration, self.ui.editPolarizerAcceleration]

 
        parameter = "b:{} b:{} b:{}".format(
            axis,
            str(editVelocity[axis].text()),
            str(editAcceleration[axis].text()))
        
        self.sendCommand('0x18A1', 7, 'LE',parameter)
        
    def clickObservationStart(self):
 
        time_epoch = config.getfloat('MainWindow','time-epoch')

        sequence_id       = self.ui.editSequenceId.text()
        start_time_string = self.ui.editObsStart.text()
        end_time_string   = self.ui.editObsEnd.text()
        sequence_repeat   = self.ui.editSequenceRepeat.text()
        
        self.start_time = 0
        self.end_time = 0
        
        if self.ui.checkObservationEnable.isChecked():
        
            try:
                if start_time_string:
                    self.start_time  = time.mktime(time.strptime(start_time_string,time_format))
                    self.start_time  = int(self.start_time - time_epoch)
                
                if end_time_string:
                    self.end_time    = time.mktime(time.strptime(end_time_string,time_format))
                    self.end_time    = int(self.end_time - time_epoch)
            except:
                msg_box.setText("Check time format like {}".format(time_format))
                msg_box.exec_()
                return
              
    
        parameter = 'l:{} l:{} h:{} b:{}'.format(self.start_time,self.end_time,sequence_repeat,sequence_id)        
        self.sendCommand('0x18C1', 2, 'LE', parameter)
            

    def clickObservationStop(self):
        self.sendCommand('0x18C1',3,'LE')
        
        self.start_time = 0
        self.end_time = 0
        
        self.ui.buttonObservationStart.setText("Start")
        self.ui.buttonObservationStop.setText("Stop")
                      
        
    def sendCommand(self,pktId,cmdCode,endian,parameter_string=''):
        address = str(self.ui.editAddress.text())
        port = '1234'
        
        parameters = shlex.split(parameter_string)
        
        argument_string = ''
        for parameter in parameters:
            i = parameter.find(':')
            
            dtype = ''
            if i > 0 :
                dtype = ['byte','half','long','string','double'][['b','h','l','s','d'].index(parameter[i-1])]
                
            if dtype == 'string':
                length = parameter[:i-1]
                if not length:
                    length = len(parameter[i+1:])
                    
                argument_string += ' --{}="{}:{}"'.format(dtype,length,parameter[i+1:]) 
            else:
                argument_string += ' --{}={}'.format(dtype,parameter[i+1:])
        
        cmdString = cmdUtilExe + ' --host=\"{}\" --port={} --pktid={} --endian={} --cmdcode={}{}'.format(address,port,pktId,endian,cmdCode,argument_string)
        

        logging.debug("Send Command: {}".format(cmdString))
                
        error = os.system(cmdString)
    

    
    def closeEvent(self, *args, **kwargs):
        config.set('MainWindow','address-sbc',str(self.ui.editAddress.text()))
        config.set('MainWindow','address',str(self.ui.editTelemetryAddress.text()))            
        
        return QWidget.closeEvent(self, *args, **kwargs)
      
    def clickCapture(self):
        '''
        typedef struct
        {
            uint8              CmdHeader[CFE_SB_CMD_HDR_SIZE];
            double             auto_exposure_adjust;
            uint32             exposure_time;
            uint8              observation_mode;
            uint8              auto_exposure_enable;
            uint8              repeat;
        
        }CAM_GrabCmd_t;
        '''
        exposure_time = 0
        try:
            exposure_time = int(self.ui.editExposureTime.text())
        except:
            msg_box.setText("exposure time not assign")
            msg_box.exec_()
            return
        
        auto_exposure_enable = 0
        if self.ui.checkAE.isChecked():
            auto_exposure_enable = 1
            
        observation_mode = 0
        try:
            observation_mode = int(self.ui.editObservationNum.text())
        except:
            pass
        
        repeat = 1
        try:
            repeat = int(self.ui.editRepeatNum.text())
        except:
            pass
        
        auto_exposure_adjust = 0
        try:
            auto_exposure_adjust = float(self.ui.spinAEAdjust.text())
        except:
            pass
                    
        parameter = "d:{} l:{} b:{} b:{} b:{}".format(
            auto_exposure_adjust,
            exposure_time,
            observation_mode,
            auto_exposure_enable,
            repeat)
        
        self.sendCommand('0x18B1', 2, 'LE', parameter)
        
    def clickCaptureStop(self):
        self.sendCommand('0x18B1', 4, 'LE')
        
        
    def refreshQuickView(self):
        image = QImage(self.qv_data,self.qv_width,self.qv_height,QImage.Format_Grayscale8)
        pixmap = QPixmap.fromImage(image)
        self.ui.labelImage.setPixmap(pixmap)
        
    def clickPerfStart(self):
        
        parameter = "l:0"
        self.sendCommand("0x1806", 14,"LE",parameter)

        filename = self.ltnow.strftime("/cf/log/%Y%m%d_%H%M%S.dat")
        self.ui.editPerfLogFile.setText(filename) 
        

    
    def clickPerfStop(self):
        
        filename = str(self.ui.editPerfLogFile.text())
        parameter = '64s:{}'.format(filename)
        
        self.sendCommand("0x1806",15, "LE", parameter)
        
    def clickConfigCamera(self):
        '''
        typedef struct
        {
            uint8              CmdHeader[CFE_SB_CMD_HDR_SIZE];
            uint16              width;
            uint16              height;
            uint16               offset_x;
            uint16              offset_y;
            uint8               bin_x;
            uint8               bin_y;
            int8                  temperature;
        
        }CAM_ConfigCmd_t;
        '''
        width = 700
        height = 700
        
        try:
            offset_x = int(self.ui.editCameraOffsetX.text())
            offset_y = int(self.ui.editCameraOffsetY.text())
            
            if ( offset_x + width > 2048 or
                 offset_y + height > 2048 or
                 offset_x < 0 or offset_y < 0 ):
                raise ValueError
            
        except:
            msg_box.setText("Check Camera Offset!")
            msg_box.exec_()
            return
        
        temperature = int(self.ui.editCameraTemperature.text())
        parameter = 'h:{width} h:{height} h:{offset_x} h:{offset_y} b:{bin_x} b:{bin_y} b:{temperature}'.format(
            width=width,
            height=height,
            offset_x=offset_x,
            offset_y=offset_y,
            bin_x=1,
            bin_y=1,
            temperature=temperature)
        
        self.sendCommand("0x18B1",3, "LE", parameter)
        
    def clickConfigDAP(self):
        '''
        typedef struct
        {
            uint8            CmdHeader[CFE_SB_CMD_HDR_SIZE];
            uint8            quickview_enable;
            uint8            save_enable;
            uint8            image_analysis_enable;
        } DAP_ConfigCmd_t;
        '''
        
        quickview_enable = 0
        save_enable = 0
        image_analysis_enable = 0
        
        
        if self.ui.checkDataSave.isChecked():
            save_enable = 1
            
        if self.ui.checkImageAnalysis.isChecked():
            image_analysis_enable = 1
        
        if self.ui.checkQuickView.isChecked():
            quickview_enable = 1
        
        parameter = 'b:{} b:{} b:{}'.format(
            quickview_enable,
            save_enable,
            image_analysis_enable)
                    
        self.sendCommand("0x18D1",2, "LE", parameter)
        
    def loadSequenceCombo(self):
        sequence_name_text = config.get("MainWindow","sequence-name")
        combo_list = sequence_name_text.split()
        
        self.ui.comboSequence.clear()
        
        self.ui.comboSequence.addItem('')
        for elem in combo_list:
            self.ui.comboSequence.addItem(elem)
    
    
    def changeSequenceCombo(self,name):
        
        if not name:
            return
                
        if self.last_sequence_name:
            self.saveSequenceCombo(self.last_sequence_name)
        
        self.ui.editSequenceId.setText(config.get('MainWindow','{}-sequence-id'.format(name)))
        self.ui.editSequenceRepeat.setText(config.get('MainWindow','{}-sequence-repeat'.format(name)))
        self.ui.editObsStart.setText(config.get('MainWindow','{}-sequence-start'.format(name)))
        self.ui.editObsEnd.setText(config.get('MainWindow','{}-sequence-end'.format(name)))
        
        self.last_sequence_name = name
            
    def saveSequenceCombo(self,input_name):
        
        name = input_name
        if not input_name:
            name = self.ui.comboSequence.currentText()
            
        if not name:
            return
            
        sequence_name_text = config.get("MainWindow","sequence-name")
        combo_list = sequence_name_text.split()
        
        if name not in combo_list:
            combo_list.append(name)
            self.ui.comboSequence.addItem(name)
            
            sequence_name_text = " ".join(combo_list)
            config.set("MainWindow","sequence-name",sequence_name_text)
        
        text = self.ui.editSequenceId.text()
        config.set('MainWindow','{}-sequence-id'.format(name),text)
        text = self.ui.editSequenceRepeat.text()
        config.set('MainWindow','{}-sequence-repeat'.format(name),text)
        text = self.ui.editObsStart.text()
        config.set('MainWindow','{}-sequence-start'.format(name),text)
        text = self.ui.editObsEnd.text()
        config.set('MainWindow','{}-sequence-end'.format(name),text)
            
        
        
        
    
def ReadConfigure(config_file):
        
    if os.path.exists(config_file):
        config.read(configFile)
        
    current_utc_string =  datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')
    epoch = str(time.mktime(time.strptime("19800101", "%Y%m%d")))
    section = 'MainWindow'
    
    
    default_options = (
        ('address','192.168.0.50'),
        ('address-sbc','192.168.0.100'),
        ('sequence-id','0'),
        ('time-epoch',epoch),
         ('mission-epoch',epoch),
         ('sequence-name','eclipse'),
         ('eclipse-sequence-id','0'),
         ('eclipse-sequence-repeat','1'),
         ('eclipse-sequence-start',current_utc_string),
         ('eclipse-sequence-end',current_utc_string),
         ('camera-offset-x','674'),
         ('camera-offset-y','674')
        )
    
    if not config.has_section(section):
        config.add_section(section)
    
    for option,value in default_options:
        if not config.has_option(section, option):
            config.set(section,option,value)
        
    WriteConfigure(config_file)

    
    
def WriteConfigure(config_file):
    fp = open(config_file,'w')
    config.write(fp)
    fp.close()
        
if __name__ == "__main__":
    
    # Start Telemetry Routing
    proc_tlm_router = subprocess.Popen(['python','TelemetryRouter.py'])
    
    # Init app
    app = QApplication(sys.argv)
    msg_box = QMessageBox()
    
    ReadConfigure(configFile)
    
    MainWindow = GroundSystem()
    MainWindow.show()
    MainWindow.raise_()

    
    error = app.exec_()
    
    WriteConfigure(configFile)
    
    proc_tlm_router.terminate()
    
    sys.exit(error)
    
    
    
    
