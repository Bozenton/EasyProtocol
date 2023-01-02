/**
 * **************************************************************
 * @file arduino_code.ino
 * @author xiaobaige (buaaxiaobaige@163.com)
 * @brief This example use Arduino UNO as a sender and receiver
 * @version 0.1
 * @date 2022-12-20
 * 
 * @copyright Copyright (c) 2022
 * 
 * **************************************************************
 */

#include "EasyProtocol.h"

#define TX_BUF_SIZE   128
#define RX_BUF_SIZE   128


ToParsePacket_t rx_packet;
uint8_t rx_buf[RX_BUF_SIZE];
ToMakePacket_t tx_packet;
uint8_t tx_buf[TX_BUF_SIZE];

const Feedback_t rxPacket(ToParsePacket_t * p_rx_packet, 
                          uint8_t * p_rx_buf, 
                          uint16_t rx_buf_capacity, 
                          uint32_t timeout_ms);

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
}

void loop() { // run over and over
  const uint16_t tx_data_len = 7;
  uint8_t tx_data[tx_data_len] = {99, 0xFF, 0xFF, 0xFD, 12, 147, 25};

  // *************************************************************
  // Receive data
  // *************************************************************
  Feedback_t fb = rxPacket(&rx_packet, rx_buf, RX_BUF_SIZE, 5000);
  
  // *************************************************************
  // Write data
  // *************************************************************
  tx_data[0] = uint8_t(fb);
  fb = begin_make_packet(&tx_packet, 1, 1, tx_buf, TX_BUF_SIZE);
  if(fb == FEEDBACK_OK){
    fb = add_data_to_packet(&tx_packet, tx_data, tx_data_len);
    fb = add_data_to_packet(&tx_packet, rx_buf, rx_packet.recv_data_len);
    if(fb == FEEDBACK_OK){
      fb = end_make_packet(&tx_packet);
    }
  }
  if(fb == FEEDBACK_OK){
    Serial.write(tx_buf, tx_packet.generated_packet_length);
  }

}

/**
 * @brief try to receive a packet from serial. This function 
 *        contains a loop inside.
 * 
 * @param p_rx_packet pointer to the packet which is for storing information.
 * @param p_rx_buf pointer to the buffer array which is for storing parsed data
 * @param rx_buf_capacity capacity of rx buffer
 * @param timeout_ms timeout in [ms]
 * @return const Feedback_t 
 */
const Feedback_t rxPacket(ToParsePacket_t * p_rx_packet, 
                          uint8_t * p_rx_buf, 
                          uint16_t rx_buf_capacity, 
                          uint32_t timeout_ms){
  Feedback_t fb;
  uint32_t pre_time_ms;
  
  // Receive status packet
  fb = begin_parse_packet(p_rx_packet, p_rx_buf, rx_buf_capacity);  // initialize
  if(fb == FEEDBACK_OK){
    pre_time_ms = millis();   // record the time for calculating timeout
    while(1){
      if(Serial.available()>0){
        fb = parse_packet(p_rx_packet, Serial.read());  // parse byte one by one
        if(fb != FEEDBACK_PROCEEDING){
          break;
        }
      }

      if(millis()-pre_time_ms >= timeout_ms){
        fb = FEEDBACK_ERROR_TIMEOUT;
        break;
      }
    } // end while
  }

  return fb;
}

