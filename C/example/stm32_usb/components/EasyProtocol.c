#include "EasyProtocol.h"

#define PACKET_IDX_HEADER_0         0
#define PACKET_IDX_HEADER_1         1
#define PACKET_IDX_HEADER_2         2
#define PACKET_IDX_RESERVED         3
#define PACKET_IDX_ID               4
#define PACKET_IDX_LENGTH_L         5
#define PACKET_IDX_LENGTH_H         6
#define PACKET_IDX_INST             7
#define PACKET_IDX_DATA             8

// internal functions
static void update_crc(uint16_t *p_crc_cur, uint8_t recv_data);

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
                              uint16_t data_buf_capacity){
    // check null ptr
    if(data_buf_capacity > 0 && p_data_buf == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    }

    p_parse_packet->p_data_buf = p_data_buf;
    p_parse_packet->data_buf_capacity = data_buf_capacity;
    p_parse_packet->is_init = true;

    return FEEDBACK_OK;
}

/**
 * Parse the packet by byte
 * @param p_parse_packet    pointer to the packet where parsing result is stored
 * @param recv_data         received byte
 * @return
 */
Feedback_t parse_packet(ToParsePacket_t * p_parse_packet, uint8_t recv_data){
    Feedback_t ret;
    if(p_parse_packet == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    } else if(p_parse_packet->is_init == false){
        return FEEDBACK_ERROR_NOT_INITIALIZED;
    }

    ret = FEEDBACK_PROCEEDING;
//    uint16_t byte_stuffing_cnt = 0;

    switch (p_parse_packet->parse_state) {
        case PACKET_PARSING_STATE_IDLE:
            if(p_parse_packet->header_cnt >= 3){
                p_parse_packet->header_cnt = 0;
            }
            p_parse_packet->header[p_parse_packet->header_cnt++] = recv_data;
            if(p_parse_packet->header_cnt == 3){
                // check if they match PACKET_HEADER
                if(p_parse_packet->header[0]==PACKET_HEADER_0
                && p_parse_packet->header[1]==PACKET_HEADER_1
                && p_parse_packet->header[2]==PACKET_HEADER_2) {
                    p_parse_packet->recv_data_len = 0;
                    p_parse_packet->calculated_crc = 0;
                    update_crc(&p_parse_packet->calculated_crc, PACKET_HEADER_0);
                    update_crc(&p_parse_packet->calculated_crc, PACKET_HEADER_1);
                    update_crc(&p_parse_packet->calculated_crc, PACKET_HEADER_2);
                    p_parse_packet->parse_state = PACKET_PARSING_STATE_RESERVED;
                } else {
                    // did not match those headers, shift them
                    p_parse_packet->header[0] = p_parse_packet->header[1];
                    p_parse_packet->header[1] = p_parse_packet->header[2];
                    p_parse_packet->header_cnt --;
                }
            }
            break;

        case PACKET_PARSING_STATE_RESERVED:
            // It is used to address the situation where PACKET HEADERS appear in the data field
            if(recv_data != PACKET_BYTE_STUFFING){
                p_parse_packet->reserved = recv_data;
                update_crc(&p_parse_packet->calculated_crc, recv_data);
                p_parse_packet->parse_state = PACKET_PARSING_STATE_ID;
            } else {
                ret = FEEDBACK_ERROR_WRONG_PACKET;
                p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            }
            break;

        case PACKET_PARSING_STATE_ID:
            if(recv_data < PACKET_MAX_ID || recv_data == PACKET_BROADCAST_ID){
                p_parse_packet->id = recv_data;
                update_crc(&p_parse_packet->calculated_crc, recv_data);
                p_parse_packet->parse_state = PACKET_PARSING_STATE_LENGTH_L;
            } else {
                ret = FEEDBACK_ERROR_INVALID_ID;
                p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            }
            break;

        case PACKET_PARSING_STATE_LENGTH_L:
            p_parse_packet->packet_len = recv_data;
            update_crc(&p_parse_packet->calculated_crc, recv_data);
            p_parse_packet->parse_state = PACKET_PARSING_STATE_LENGTH_H;
            break;

        case PACKET_PARSING_STATE_LENGTH_H:
            p_parse_packet->packet_len |= recv_data << 8;
            update_crc(&p_parse_packet->calculated_crc, recv_data);
            if(p_parse_packet->packet_len < 3){ // 3 = Instruction(1)+CRC(2)
                ret = FEEDBACK_ERROR_LENGTH;
                p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            } else {
                p_parse_packet->parse_state = PACKET_PARSING_STATE_INST;
            }
            break;

        case PACKET_PARSING_STATE_INST:
            p_parse_packet->inst_idx = recv_data;
            update_crc(&p_parse_packet->calculated_crc, recv_data);
            p_parse_packet->recv_data_len = 0;
            p_parse_packet->byte_stuffing_cnt = 0;
            if(p_parse_packet->packet_len > p_parse_packet->data_buf_capacity + 3){
                // 3 = Instruction(1)+CRC(2)
                ret = FEEDBACK_ERROR_BUFFER_OVERFLOW;
                p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            } else if(p_parse_packet->packet_len==3){// 3 = Instruction(1)+CRC(2)
                p_parse_packet->parse_state = PACKET_PARSING_STATE_CRC_L;
            } else {
                p_parse_packet->parse_state = PACKET_PARSING_STATE_DATA;
            }
            break;

        case PACKET_PARSING_STATE_DATA:
            if(p_parse_packet->p_data_buf == NULL){
                ret = FEEDBACK_ERROR_NULLPTR;
                p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
                break;
            }
            p_parse_packet->p_data_buf[p_parse_packet->recv_data_len++] = recv_data;
            update_crc(&p_parse_packet->calculated_crc, recv_data);

            //Remove byte stuffing
            if(p_parse_packet->recv_data_len >= 4){
                if(p_parse_packet->p_data_buf[p_parse_packet->recv_data_len-4] == PACKET_HEADER_0
                   && p_parse_packet->p_data_buf[p_parse_packet->recv_data_len-3] == PACKET_HEADER_1
                   && p_parse_packet->p_data_buf[p_parse_packet->recv_data_len-2] == PACKET_HEADER_2
                   && p_parse_packet->p_data_buf[p_parse_packet->recv_data_len-1] == PACKET_BYTE_STUFFING){
                    p_parse_packet->recv_data_len--;
                    p_parse_packet->byte_stuffing_cnt++;
                }
            }else if(p_parse_packet->recv_data_len == 3){
                if(p_parse_packet->inst_idx == PACKET_HEADER_0
                   && p_parse_packet->p_data_buf[0] == PACKET_HEADER_1
                   && p_parse_packet->p_data_buf[1] == PACKET_HEADER_2
                   && p_parse_packet->p_data_buf[2] == PACKET_BYTE_STUFFING){
                    p_parse_packet->recv_data_len--;
                    p_parse_packet->byte_stuffing_cnt++;
                }
            }
            if(p_parse_packet->recv_data_len+p_parse_packet->byte_stuffing_cnt+3 == p_parse_packet->packet_len){ // 3 = Instruction(1)+CRC(2)
                p_parse_packet->parse_state = PACKET_PARSING_STATE_CRC_L;
            }
            break;

        case PACKET_PARSING_STATE_CRC_L:
            p_parse_packet->recv_crc = recv_data;
            p_parse_packet->parse_state = PACKET_PARSING_STATE_CRC_H;
            break;

        case PACKET_PARSING_STATE_CRC_H:
            p_parse_packet->recv_crc |= recv_data << 8;
            if(p_parse_packet->calculated_crc == p_parse_packet->recv_crc){
                ret = FEEDBACK_OK;
            } else {
                ret = FEEDBACK_ERROR_CRC;
            }
            p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            break;

        default:
            p_parse_packet->parse_state = PACKET_PARSING_STATE_IDLE;
            break;
    }

    return ret;
}


/**
 * ***********************************************************************
 * Send Functions
 * ***********************************************************************
 */

Feedback_t begin_make_packet(ToMakePacket_t * p_make_packet,
                             uint8_t id,
                             uint8_t inst_idx,
                             uint8_t * p_packet_buf,
                             uint16_t packet_buf_capacity){
    if(p_packet_buf == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    }

    // check if ID is valid
    if(id != PACKET_BROADCAST_ID && id >= PACKET_MAX_ID ){
        return FEEDBACK_ERROR_INVALID_ID;
    }
    if(packet_buf_capacity < PACKET_MIN_SIZE){
        return FEEDBACK_ERROR_NOT_ENOUGH_BUFFER_SIZE;
    }

    p_make_packet->id = id;
    p_make_packet->inst_idx = inst_idx;
    p_make_packet->p_packet_buf = p_packet_buf;
    p_make_packet->packet_buf_capacity = packet_buf_capacity;
    p_make_packet->data_length = 0;
    p_make_packet->generated_packet_length = 0;
    p_make_packet->is_init = true;

    return FEEDBACK_OK;
}

Feedback_t add_data_to_packet(ToMakePacket_t * p_make_packet,
                              uint8_t * p_data,
                              uint16_t data_len){
    if(p_make_packet == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    } else if (p_make_packet->is_init == false) {
        return FEEDBACK_ERROR_NOT_INITIALIZED;
    }

    uint8_t *p_packet;      // pointer to the packet buffer
    uint8_t *p_data_buf;    // pointer to the data buffer
    uint16_t i, generated_data_len;
    // add in the basis of previous length so that we can call this function several times
    generated_data_len = p_make_packet->data_length;
    p_packet = p_make_packet->p_packet_buf;

    if(p_make_packet->packet_buf_capacity < generated_data_len + data_len + PACKET_MIN_SIZE) {
        return FEEDBACK_ERROR_BUFFER_OVERFLOW;
    }
    p_data_buf = &p_packet[PACKET_IDX_DATA];

    for(i=0; i<data_len; i++){
        p_data_buf[generated_data_len++] = p_data[i];

        // add byte stuffing
        if(generated_data_len >=3){
            if(p_data_buf[generated_data_len-3] == PACKET_HEADER_0
            && p_data_buf[generated_data_len-2] == PACKET_HEADER_1
            && p_data_buf[generated_data_len-1] == PACKET_HEADER_2){
                p_data_buf[generated_data_len++] = PACKET_BYTE_STUFFING;
            }
        } else if(generated_data_len == 2){
            if(p_packet[PACKET_IDX_INST] == PACKET_HEADER_0
               && p_data_buf[generated_data_len-2] == PACKET_HEADER_1
               && p_data_buf[generated_data_len-1] == PACKET_HEADER_2){
                p_data_buf[generated_data_len++] = PACKET_BYTE_STUFFING;
            }
        }
        if(p_make_packet->packet_buf_capacity < generated_data_len + PACKET_MIN_SIZE){
            return FEEDBACK_ERROR_BUFFER_OVERFLOW;
        }
    }
    p_make_packet->data_length = generated_data_len;
    return FEEDBACK_OK;
}

Feedback_t end_make_packet(ToMakePacket_t * p_make_packet){
    if(p_make_packet == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    }
    if(p_make_packet->is_init == false){
        return FEEDBACK_ERROR_NOT_INITIALIZED;
    }
    if(p_make_packet->p_packet_buf == NULL){
        return FEEDBACK_ERROR_NULLPTR;
    }

    uint8_t * p_packet;
    uint16_t i, generated_packet_len = 0, calculated_crc = 0;

    if(p_make_packet->packet_buf_capacity < p_make_packet->data_length + PACKET_MIN_SIZE){
        return FEEDBACK_ERROR_BUFFER_OVERFLOW;
    }

    p_packet = p_make_packet->p_packet_buf;

    p_packet[generated_packet_len++] = PACKET_HEADER_0;
    p_packet[generated_packet_len++] = PACKET_HEADER_1;
    p_packet[generated_packet_len++] = PACKET_HEADER_2;
    p_packet[generated_packet_len++] = PACKET_RESERVED;
    p_packet[generated_packet_len++] = p_make_packet->id;

    p_packet[generated_packet_len++] = (p_make_packet->data_length+3) >> 0; //3 = Instruction(1)+CRC(2)
    p_packet[generated_packet_len++] = (p_make_packet->data_length+3) >> 8; //3 = Instruction(1)+CRC(2)
    p_packet[generated_packet_len++] = p_make_packet->inst_idx;

    generated_packet_len += p_make_packet->data_length;

    for(i=0; i<generated_packet_len; i++){
        update_crc(&calculated_crc, p_packet[i]);
    }

    p_packet[generated_packet_len ++] = calculated_crc;
    p_packet[generated_packet_len ++] = calculated_crc >> 8;

    p_make_packet->generated_packet_length = generated_packet_len;

    return FEEDBACK_OK;
}




const uint16_t crc_table[256] = { 0x0000,
        0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
        0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027,
        0x0022, 0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D,
        0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B,
        0x004E, 0x0044, 0x8041, 0x80C3, 0x00C6, 0x00CC, 0x80C9,
        0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF,
        0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1, 0x00A0, 0x80A5,
        0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093,
        0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197,
        0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE,
        0x01A4, 0x81A1, 0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB,
        0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9,
        0x01C8, 0x81CD, 0x81C7, 0x01C2, 0x0140, 0x8145, 0x814F,
        0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176,
        0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162, 0x8123,
        0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104,
        0x8101, 0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D,
        0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B,
        0x032E, 0x0324, 0x8321, 0x0360, 0x8365, 0x836F, 0x036A,
        0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C,
        0x8359, 0x0348, 0x834D, 0x8347, 0x0342, 0x03C0, 0x83C5,
        0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3,
        0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7,
        0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E,
        0x0384, 0x8381, 0x0280, 0x8285, 0x828F, 0x028A, 0x829B,
        0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9,
        0x02A8, 0x82AD, 0x82A7, 0x02A2, 0x82E3, 0x02E6, 0x02EC,
        0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5,
        0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1, 0x8243,
        0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
        0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264,
        0x8261, 0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E,
        0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208,
        0x820D, 0x8207, 0x0202 };

static void update_crc(uint16_t *p_crc_cur, uint8_t recv_data)
{
    uint16_t crc;
    uint16_t i;

    crc = *p_crc_cur;

    i = ((unsigned short)(crc >> 8) ^ recv_data) & 0xFF;
    *p_crc_cur = (crc << 8) ^ (*(crc_table + i));
}

