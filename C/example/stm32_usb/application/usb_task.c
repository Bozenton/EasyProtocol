#include "usb_task.h"
#include "EasyProtocol.h"
#include "bsp_usb_handler.h"
#include "cmsis_os.h"

extern usb_handler_t * usb;

#define TX_BUF_SIZE   128
#define RX_BUF_SIZE   128

ToParsePacket_t rx_packet;
uint8_t rx_buf[RX_BUF_SIZE];
ToMakePacket_t tx_packet;
uint8_t tx_buf[TX_BUF_SIZE];


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
Feedback_t rxPacket(ToParsePacket_t * p_rx_packet, 
                          uint8_t * p_rx_buf, 
                          uint16_t rx_buf_capacity, 
                          uint32_t timeout_ms){
    Feedback_t fb;
    uint32_t pre_time_ms;

    fb = begin_parse_packet(p_rx_packet, p_rx_buf, rx_buf_capacity);
    if(fb == FEEDBACK_OK){
        pre_time_ms = osKernelSysTick();    // need configTICK_RATE_HZ to be 1000
        while(1){
            if(usb_available(usb) > 0){
                fb = parse_packet(p_rx_packet, usb_read(usb));
                if(fb != FEEDBACK_PROCEEDING){
                    break;
                }
            }

            if(osKernelSysTick()-pre_time_ms >= timeout_ms){
                fb = FEEDBACK_ERROR_TIMEOUT;
                break;
            }

            osDelay(1);     // we must use it to avoid blocking.
        } // end while
    }


    return fb;
}


void usb_task(void const * argument){

    while(1){
        uint8_t tx_data[7] = {99, 0xFF, 0xFF, 0xFD, 12, 147, 25};

        // *************************************************************
        // Receive data
        // *************************************************************
        Feedback_t fb = rxPacket(&rx_packet, rx_buf, RX_BUF_SIZE, 5000);
        
        // *************************************************************
        // Write data
        // *************************************************************
        if(fb == FEEDBACK_OK){
            tx_data[0] = (uint8_t)(fb);
            fb = begin_make_packet(&tx_packet, 1, 1, tx_buf, TX_BUF_SIZE);
            if(fb == FEEDBACK_OK){
                fb = add_data_to_packet(&tx_packet, tx_data, 7);
                fb = add_data_to_packet(&tx_packet, rx_buf, rx_packet.recv_data_len);
                if(fb == FEEDBACK_OK){
                    fb = end_make_packet(&tx_packet);
                    if(fb == FEEDBACK_OK){
                        usb_write(tx_buf, tx_packet.generated_packet_length);
                    }
                }
            }
        } // end if

        osDelay(1);
    }

}

