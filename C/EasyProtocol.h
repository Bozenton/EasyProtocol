#ifndef EASY_PROTOCOL_H_
#define EASY_PROTOCOL_H_

#include <stdint.h>
#include <stdbool.h>

//#ifdef __cplusplus
//extern "C"{
//#endif

#define PACKET_HEADER_0         0xFF
#define PACKET_HEADER_1         0xFF
#define PACKET_HEADER_2         0xFD
#define PACKET_RESERVED         0x00
#define PACKET_BYTE_STUFFING    0xFD

// Package ID:
//  Range : 0 ~ 252 (0x00 ~ 0xFC)
//  Broadcast ID : 254 (0xFE)
#define PACKET_MAX_ID               0xFD        // not included
#define PACKET_BROADCAST_ID         0xFE

// header(3)+reserved(1)+id(1)+length(2)+instruction(1)+crc(2)
#define PACKET_MIN_SIZE     10

typedef enum{
    PACKET_PARSING_STATE_IDLE = 0,
    PACKET_PARSING_STATE_RESERVED,
    PACKET_PARSING_STATE_ID,
    PACKET_PARSING_STATE_LENGTH_L,
    PACKET_PARSING_STATE_LENGTH_H,
    PACKET_PARSING_STATE_INST,
    PACKET_PARSING_STATE_DATA,
    PACKET_PARSING_STATE_CRC_L,
    PACKET_PARSING_STATE_CRC_H
}PacketState_t;

typedef enum{
    FEEDBACK_OK = 0,
    FEEDBACK_PROCEEDING,
    FEEDBACK_ERROR_INVALID_ID,
    FEEDBACK_ERROR_NULLPTR,
    FEEDBACK_ERROR_NOT_INITIALIZED,
    FEEDBACK_ERROR_LENGTH,
    FEEDBACK_ERROR_BUFFER_OVERFLOW,
    FEEDBACK_ERROR_WRONG_PACKET,
    FEEDBACK_ERROR_CRC,
    FEEDBACK_ERROR_NOT_ENOUGH_BUFFER_SIZE
}Feedback_t;

typedef enum{
    INST_STATUS,
    /* you can define your own instructions here BEGIN*/

    /* you can define your own instructions here END */
}Instruction_t;

typedef struct ToParsePacket{
    uint8_t header[3];
    uint8_t header_cnt;
    uint8_t id;
    uint8_t inst_idx;
    uint8_t * p_data_buf;
    uint16_t data_buf_capacity;
    uint16_t recv_data_len;
    uint16_t packet_len;
    uint16_t calculated_crc;
    uint16_t recv_crc;
    uint8_t reserved;
    PacketState_t parse_state;
    bool is_init;
}ToParsePacket_t;


typedef struct ToMakePacket{
    uint8_t id;
    uint8_t inst_idx;
    uint8_t * p_packet_buf;
    uint16_t packet_buf_capacity;
    uint16_t data_length;
    uint16_t generated_packet_length;
    bool is_init;
}ToMakePacket_t;


/**
 * ***********************************************************************
 * Receive Functions
 * ***********************************************************************
 */

/**
 * Prepare a ToParsePacket_t for later parsing
 * @param p_parse_packet    pointer to the packet that is to be prepared
 * @param p_data_buf        pointer to the data buffer
 * @param data_buf_capacity length of the data buffer
 * @return
 */
Feedback_t begin_parse_packet(ToParsePacket_t * p_parse_packet,
                              uint8_t * p_data_buf,
                              uint16_t data_buf_capacity);
/**
 * Parse the packet by byte
 * @param p_parse_packet    pointer to the packet where parsing result is stored
 * @param recv_data         received byte
 * @return
 */
Feedback_t parse_packet(ToParsePacket_t * p_parse_packet, uint8_t recv_data);


/**
 * ***********************************************************************
 * Send Functions
 * ***********************************************************************
 */

Feedback_t begin_make_packet(ToMakePacket_t * p_make_packet,
                             uint8_t id,
                             uint8_t inst_idx,
                             uint8_t * p_packet_buf,
                             uint16_t packet_buf_capacity);

Feedback_t add_data_to_packet(ToMakePacket_t * p_make_packet,
                               uint8_t * p_data,
                               uint16_t data_len);

Feedback_t end_make_packet(ToMakePacket_t * p_make_packet);


//#ifdef __cplusplus
//}
//#endif

#endif
