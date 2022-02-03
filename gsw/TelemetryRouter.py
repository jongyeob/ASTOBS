'''
Created on 2017. 6. 22.

@author: jongyeob
'''
import sys
import socket
import time
import zmq
import signal

udp_recv_port = 1235
tcp_send_port = 1236

context = None
publisher = None
sock = None


def main():
    global context,publisher,sock
    
    context = zmq.Context()
    publisher = context.socket(zmq.PUB)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)


    publisher.bind("tcp://*:{}".format(tcp_send_port))
    sock.bind(('', udp_recv_port))
        
    while True:
        try:
            # Receive message
            datagram, host = sock.recvfrom(65536) # buffer size is 1024 bytes
            
            addr,_ = host
            publisher.send_multipart([addr,datagram])
                        
            # Ignore datagram if it is not long enough (doesnt contain tlm header?)
            if len(datagram) < 6:
                continue

        # Handle errors
        except socket.error, v:
            print("Socket error: {}".format(v))
            time.sleep(1)     

def closeall():
    print("Close All")
    if sock: sock.close()
    if publisher: publisher.close()
    if context: context.destroy()
    
def sigterm_handler(signum, frame):
    closeall()
    sys.exit(0)

if __name__ == '__main__':
    signal.signal(signal.SIGTERM, sigterm_handler)

    try: 
        main()
    except KeyboardInterrupt:
        print("Keyboard Interrupt")
        closeall()
