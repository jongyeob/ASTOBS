'''
Created on 2017. 6. 22.

@author: jongyeob
'''
import socket
import logging
import zmq
import time
from PyQt5.QtCore import QThread, pyqtSignal

class TelemetryReceiver(QThread):
    signalTelemetry = pyqtSignal(object)
    
    def __init__(self, host):
        super(TelemetryReceiver,self).__init__()      
        
        context = zmq.Context()
        subscriber = context.socket(zmq.SUB)
        subscriber.connect("tcp://localhost:1236")
        
        subscriber.setsockopt(zmq.SUBSCRIBE, host)
        
        self.isThreadRunning   = 0
        self.host = host
        self.context = context
        self.subscriber = subscriber

        
    def run(self):
        # Init udp socket
  
        
        self.isThreadRunning = 1
        while self.isThreadRunning == 1:
            try:
                
                [address, datagram] = self.subscriber.recv_multipart()
                # Ignore datagram if it is not long enough (doesnt contain tlm header?)
                
                if len(datagram) < 6:
                    continue

                self.signalTelemetry.emit(datagram)
                
            # Handle errors
            except socket.error, v:
                logging.error("Socket error: {}".format(v))
                self.sleep(1)
                
        self.isThreadRunning = 0
        
        self.subscriber.close()
        self.context.destroy()
                
    def stop(self):
        self.isThreadRunning = 2
        



    
    