"""
This example use python as a sender and receiver. You can run this
script on your PC and communicate with:
    * Arduino
    * ...
"""

import serial
import serial.tools.list_ports
import threading
import time
from numpy import uint8, uint16
from EasyProtocol import EasyProtocol, Feedback


class PortHandler:
    def __init__(self, baud):
        self.baud = baud    # baud rate
        self.start_time = time.time()
        ports_list = list(serial.tools.list_ports.comports())
        self.ep = EasyProtocol()

        while True:     # Find available port
            if len(ports_list) >= 1:
                idx = 0
                for item in ports_list:
                    print(idx, '-', item.device)
                    idx += 1
                self.portx = ports_list[int(input('Please enter the number of the port: '))].device
                break
            else:
                input('Did not find the com, please check')

        self.time_out = 2
        self.serial = serial.Serial(self.portx, self.baud, timeout=self.time_out)
        print(self.portx, "Open!")

        # Create a thread for receiving data
        self.recv_thread = threading.Thread(target=self.recv_thread_func, args=())
        self.recv_thread.daemon = True
        self.recv_thread.start()

    def recv_thread_func(self):
        rx_timeout = 5
        while True:
            self.ep.begin_parse_packet()    # initialize
            pre_time = time.time()          # record the time for calculating timeout
            while True:
                if self.serial.in_waiting > 0:
                    recv_byte = self.serial.read()      # get one byte from serial
                    recv_byte = uint8(recv_byte[0])     # convert type
                    feedback = self.ep.parse_packet(recv_byte)      # parse one by one
                    if feedback is not Feedback.FEEDBACK_PROCEEDING:
                        break
                if time.time() - pre_time > rx_timeout:
                    feedback = Feedback.FEEDBACK_ERROR_TIMEOUT

            if feedback is Feedback.FEEDBACK_OK:
                print('Receive data: ', self.ep.get_rx_data())      # Display data
                print('Instruction id: ', self.ep.get_rx_inst())    # Display instruction


if __name__ == "__main__":
    ph = PortHandler(115200)
    data1 = uint8([38, 12, 0XDA])                   # data 1
    data2 = uint8([0xFF, 0xFF, 0xFD])               # data 2
    data3 = uint8([0xA4, 1, 0xFF])                  # data 3

    while True:
        ph.ep.begin_make_packet(uint8(1), uint16(1))
        ph.ep.add_data_to_packet(data1)
        ph.ep.add_data_to_packet(data2)
        ph.ep.add_data_to_packet(data3)
        ph.ep.end_make_packet()
        tx_buf = bytes(ph.ep.get_tx_packet())
        ph.serial.write(tx_buf)
        print('Send data: ', tx_buf)

        time.sleep(2)


