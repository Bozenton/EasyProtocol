/**
 * **************************************************************
 * @file bsp_usb_handler.c
 * @author xiaobaige (buaaxiaobaige@163.com)
 * @brief 
 * @version 0.1
 * @date 2022-12-29
 * 
 * @copyright Copyright (c) 2022
 * 
 * **************************************************************
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "bsp_usb_handler.h"
#include "circular_buffer.h"

extern uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);

struct usb_handler_t
{
    uint8_t * buffer;
    cbuf_handle_t cbuf;

};


usb_handler_t * usb_begin(void){
    
    usb_handler_t * usb = malloc(sizeof(usb_handler_t));
    assert(usb);

    usb->buffer = malloc(CIRCULAR_BUF_SIZE * sizeof(uint8_t));
    usb->cbuf = circular_buf_init(usb->buffer, CIRCULAR_BUF_SIZE);
		
	return usb;
}


void usb_end(usb_handler_t * usb){
    free(usb->buffer);
    circular_buf_free(usb->cbuf);
}


size_t usb_available(usb_handler_t * usb){

    return circular_buf_capacity(usb->cbuf);
}


void usb_load(usb_handler_t * usb, uint8_t * buf, uint32_t * len){
    int success;
    for(size_t i=0; i<(*len); i++){
        success = circular_buf_try_put(usb->cbuf, buf[i]);  // for thread safety, use this function
        if(success != 0){
            break;  // The buffer is full, we should take the data as soon as possible. 
        }
    }
}


int usb_read(usb_handler_t * usb){
    uint8_t byte;
    int r = circular_buf_get(usb->cbuf, &byte);
    if(!r){
        return byte;
    }
    return -1;
}


uint8_t usb_write(uint8_t * buf, uint16_t len){
    return CDC_Transmit_FS(buf, len);
}

uint8_t * usb_debug_get_buffer(usb_handler_t * usb){
    return usb->buffer;
}
