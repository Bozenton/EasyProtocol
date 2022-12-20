"""
This is an example where sending functions and receiving functions
are tested in python itself to make sure there is no logic error.
"""

from EasyProtocol import EasyProtocol
from numpy import uint8, uint16

if __name__ == "__main__":
    ep = EasyProtocol()
    ep.begin_make_packet(uint8(1), uint16(6))       # id=1, instruction=6
    data1 = uint8([38, 12, 0XDA])                   # data 1
    data2 = uint8([0xFF, 0xFF, 0xFD])               # data 2
    data3 = uint8([0xA4, 1, 0xFF])                  # data 3
    ep.add_data_to_packet(data1)
    ep.add_data_to_packet(data2)
    ep.add_data_to_packet(data3)
    ep.end_make_packet()                            # Make packet
    packet = ep.get_tx_packet()
    print(packet)                                   # Display tx packet

    ep.begin_parse_packet()                         # Prepare for parsing
    for i in range(len(packet)):
        ep.parse_packet(packet[i])                  # Parse one by one
    print(ep.get_rx_data())                         # Display data
    print(ep.get_rx_inst())                         # Display instruction

