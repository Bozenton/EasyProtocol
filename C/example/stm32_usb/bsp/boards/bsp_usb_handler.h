/**
 * **************************************************************
 * @file bsp_usb_handler.h
 * @author xiaobaige (buaaxiaobaige@163.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 * @note If the data if coming continuously, it must be taken as soon as possible
 * 
 * **************************************************************
 */

#ifndef BSP_USB_HANDER_H_
#define BSP_USB_HANDER_H_

#include <stddef.h>
#include <stdint.h>

#define CIRCULAR_BUF_SIZE   32


typedef struct usb_handler_t usb_handler_t;

usb_handler_t * usb_begin(void);

void usb_end(usb_handler_t * usb);

size_t usb_available(usb_handler_t * usb);

/**
 * @brief Read data from circular buffer
 * 
 * @return The first byte (uint8_t) of data available (or -1 if no data is available)
 */
int usb_read(usb_handler_t * usb);

uint8_t usb_write(uint8_t * buf, uint16_t len);


/**
 * @brief This function is used in function `CDC_Receive_FS` of `usbd_cdc_if.c`
 * 
 * @param buf 
 * @param len 
 */
void usb_load(usb_handler_t * usb, uint8_t * buf, uint32_t * len);


uint8_t * usb_debug_get_buffer(usb_handler_t * usb);

#endif 
