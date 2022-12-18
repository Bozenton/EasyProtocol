from enum import Enum
from numpy import uint8, uint16

# constants used for packet
PACKET_HEADER_0 = uint8(0xFF)
PACKET_HEADER_1 = uint8(0xFF)
PACKET_HEADER_2 = uint8(0xFD)
PACKET_RESERVED = uint8(0x00)
PACKET_BYTE_STUFFING = uint8(0xFD)

PACKET_MAX_ID = uint8(0xFD)
PACKET_BROADCAST_ID = uint8(0xFE)

PACKET_MIN_SIZE = 10  # header(3)+reserved(1)+id(1)+length(2)+instruction(1)+crc(2)
PACKET_TX_BUF_CAPACITY = 256
PACKET_RX_DATA_BUF_CAPACITY = 256

# Package index for different parts
PACKET_IDX_HEADER_0 = 0
PACKET_IDX_HEADER_1 = 1
PACKET_IDX_HEADER_2 = 2
PACKET_IDX_RESERVED = 3
PACKET_IDX_ID = 4
PACKET_IDX_LENGTH_L = 5
PACKET_IDX_LENGTH_H = 6
PACKET_IDX_INST = 7
PACKET_IDX_DATA = uint16(8)

crc_table = uint16([0x0000,
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
                    0x820D, 0x8207, 0x0202])


class PacketState(Enum):
    PACKET_PARSING_STATE_IDLE = 1
    PACKET_PARSING_STATE_RESERVED = 2
    PACKET_PARSING_STATE_ID = 3
    PACKET_PARSING_STATE_LENGTH_L = 4
    PACKET_PARSING_STATE_LENGTH_H = 5
    PACKET_PARSING_STATE_INST = 6
    PACKET_PARSING_STATE_DATA = 7
    PACKET_PARSING_STATE_CRC_L = 8
    PACKET_PARSING_STATE_CRC_H = 9


class Feedback(Enum):
    FEEDBACK_OK = 1
    FEEDBACK_PROCEEDING = 2
    FEEDBACK_ERROR_INVALID_ID = 3
    FEEDBACK_ERROR_NULLPTR = 4
    FEEDBACK_ERROR_NOT_INITIALIZED = 5
    FEEDBACK_ERROR_LENGTH = 6
    FEEDBACK_ERROR_BUFFER_OVERFLOW = 7
    FEEDBACK_ERROR_WRONG_PACKET = 8
    FEEDBACK_ERROR_CRC = 9
    FEEDBACK_ERROR_NOT_ENOUGH_BUFFER_SIZE = 10


def get_low_byte(w: uint16):
    return uint8(w & 0xFF)


def get_high_byte(w: uint16):
    return uint8((w >> 8) & 0xFF)


def make_word(low: uint8, high: uint8) -> uint16:
    return uint16(low & 0xFF) | ((high & 0xFF) << 8)


class EasyProtocol:
    def __init__(self):
        # CRC
        self.crc_table = crc_table
        # Send
        self.tx_id = 0
        self.tx_inst_idx = 0
        self.tx_packet_buf = [uint8(0)] * PACKET_TX_BUF_CAPACITY
        self.tx_data_length = uint16(0)
        self.tx_generated_packet_length = uint16(0)
        self.tx_is_init = False
        # Receive
        self.rx_header = [uint8(0)] * 3
        self.rx_header_cnt = 0
        self.rx_id = 0
        self.rx_inst_idx = 0
        self.rx_data_buf = [uint8(0)] * PACKET_RX_DATA_BUF_CAPACITY
        self.rx_data_buf_capacity = PACKET_RX_DATA_BUF_CAPACITY
        self.rx_recv_data_len = uint16(0)
        self.rx_packet_len = uint16(0)
        self.rx_calculated_crc = 0
        self.rx_recv_crc = 0
        self.rx_reserved = 0
        self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE
        self.rx_is_init = False
        self.rx_byte_stuffing_cnt = 0

    def update_crc(self, crc_current: uint16, data: uint8):
        i = (uint16(crc_current >> 8) ^ data) & 0xFF
        return uint16((crc_current << 8) ^ self.crc_table[i])

    def begin_make_packet(self, id: uint8, inst_idx: uint8) -> Feedback:
        if id != PACKET_BROADCAST_ID and id >= PACKET_MAX_ID:
            return Feedback.FEEDBACK_ERROR_INVALID_ID

        self.tx_id = id
        self.tx_inst_idx = inst_idx
        self.tx_data_length = uint16(0)
        self.tx_generated_packet_length = uint16(0)
        self.tx_is_init = True

        return Feedback.FEEDBACK_OK

    def add_data_to_packet(self, data: list[uint8]) -> Feedback:
        """
        Add data to the packet. This function can be called several times.
        :param data: list of bytes to be added
        :return:
        """
        try:
            _ = iter(data)
        except TypeError as te:
            print(data, 'is not iterable')

        if not self.tx_is_init:
            return Feedback.FEEDBACK_ERROR_NOT_INITIALIZED
        generated_data_len = self.tx_data_length
        ptr_data_buf = PACKET_IDX_DATA

        if generated_data_len + len(data) + PACKET_MIN_SIZE > PACKET_TX_BUF_CAPACITY:
            return Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW

        for i in range(len(data)):
            self.tx_packet_buf[ptr_data_buf + generated_data_len] = uint8(data[i])
            generated_data_len += 1
            # add byte stuffing
            if generated_data_len >= 3:
                if self.tx_packet_buf[ptr_data_buf + generated_data_len - 3] == PACKET_HEADER_0 and \
                        self.tx_packet_buf[ptr_data_buf + generated_data_len - 2] == PACKET_HEADER_1 and \
                        self.tx_packet_buf[ptr_data_buf + generated_data_len - 1] == PACKET_HEADER_2:
                    self.tx_packet_buf[ptr_data_buf + generated_data_len] = PACKET_BYTE_STUFFING
                    generated_data_len += 1
            elif generated_data_len == 2:
                if self.tx_packet_buf[PACKET_IDX_INST] == PACKET_HEADER_0 and \
                        self.tx_packet_buf[ptr_data_buf + generated_data_len - 2] == PACKET_HEADER_1 and \
                        self.tx_packet_buf[ptr_data_buf + generated_data_len - 1] == PACKET_HEADER_2:
                    self.tx_packet_buf[ptr_data_buf + generated_data_len] = PACKET_BYTE_STUFFING
                    generated_data_len += 1
            # check length
            if generated_data_len + len(data) + PACKET_MIN_SIZE > PACKET_TX_BUF_CAPACITY:
                return Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW
        self.tx_data_length = generated_data_len
        return Feedback.FEEDBACK_OK

    def end_make_packet(self):
        """
        End making packet
        :return:
        """
        if not self.tx_is_init:
            return Feedback.FEEDBACK_ERROR_NOT_INITIALIZED
        generated_packet_len = uint16(8)
        self.tx_packet_buf[0:generated_packet_len] = [uint8(PACKET_HEADER_0),
                                                      uint8(PACKET_HEADER_1),
                                                      uint8(PACKET_HEADER_2),
                                                      uint8(PACKET_RESERVED),
                                                      uint8(self.tx_id),
                                                      get_low_byte(self.tx_data_length + 3),
                                                      # 3 = Instruction(1)+CRC(2)
                                                      get_high_byte(self.tx_data_length + 3),
                                                      uint8(self.tx_inst_idx)]
        generated_packet_len += self.tx_data_length

        calculated_crc = uint16(0)
        for i in range(generated_packet_len):
            calculated_crc = self.update_crc(calculated_crc, self.tx_packet_buf[i])

        self.tx_packet_buf[generated_packet_len] = get_low_byte(calculated_crc)
        generated_packet_len += 1
        self.tx_packet_buf[generated_packet_len] = get_high_byte(calculated_crc)
        generated_packet_len += 1

        self.tx_generated_packet_length = generated_packet_len

        self.tx_is_init = False
        return Feedback.FEEDBACK_OK

    def begin_parse_packet(self):
        self.rx_is_init = True
        return Feedback.FEEDBACK_OK

    def parse_packet(self, recv_byte: uint8):
        recv_data = uint8(recv_byte)
        if not self.rx_is_init:
            return Feedback.FEEDBACK_ERROR_NOT_INITIALIZED

        ret = Feedback.FEEDBACK_PROCEEDING
        if self.rx_parse_state == PacketState.PACKET_PARSING_STATE_IDLE:
            if self.rx_header_cnt >= 3:
                self.rx_header_cnt = 0
            self.rx_header[self.rx_header_cnt] = recv_data
            self.rx_header_cnt += 1
            if self.rx_header_cnt == 3:
                # check if they match packet headers
                if self.rx_header[0] == PACKET_HEADER_0 and \
                        self.rx_header[1] == PACKET_HEADER_1 and \
                        self.rx_header[2] == PACKET_HEADER_2:
                    self.rx_recv_data_len = uint16(0)
                    self.rx_calculated_crc = uint16(0)
                    self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, PACKET_HEADER_0)
                    self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, PACKET_HEADER_1)
                    self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, PACKET_HEADER_2)
                    self.rx_parse_state = PacketState.PACKET_PARSING_STATE_RESERVED
                else:
                    self.rx_header[0] = self.rx_header[1]
                    self.rx_header[1] = self.rx_header[2]
                    self.rx_header_cnt -= 1

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_RESERVED:
            if recv_data != PACKET_BYTE_STUFFING:
                self.rx_reserved = recv_data
                self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_ID
            else:
                ret = Feedback.FEEDBACK_ERROR_WRONG_PACKET
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_ID:
            if recv_data < PACKET_MAX_ID or recv_data == PACKET_BROADCAST_ID:
                self.rx_id = recv_data
                self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_LENGTH_L
            else:
                ret = Feedback.FEEDBACK_ERROR_INVALID_ID
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_LENGTH_L:
            self.rx_packet_len = recv_data
            self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)
            self.rx_parse_state = PacketState.PACKET_PARSING_STATE_LENGTH_H

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_LENGTH_H:
            self.rx_packet_len = make_word(self.rx_packet_len, recv_data)
            self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)
            if self.rx_packet_len < 3:
                ret = Feedback.FEEDBACK_ERROR_LENGTH
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE
            else:
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_INST

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_INST:
            self.rx_inst_idx = recv_data
            self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)
            self.rx_recv_data_len = uint16(0)
            self.rx_byte_stuffing_cnt = uint16(0)
            if self.rx_packet_len > self.rx_data_buf_capacity + 3:
                # 3 = Instruction(1) + CRC(2)
                ret = Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE
            elif self.rx_packet_len == 3:  # 3 = Instruction(1) + CRC(2)
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_CRC_L
            else:
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_DATA

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_DATA:
            self.rx_data_buf[self.rx_recv_data_len] = recv_data
            self.rx_recv_data_len += 1
            self.rx_calculated_crc = self.update_crc(self.rx_calculated_crc, recv_data)

            # remove byte stuffing
            if self.rx_recv_data_len >= 4:
                if self.rx_data_buf[self.rx_recv_data_len - 4] == PACKET_HEADER_0 and \
                        self.rx_data_buf[self.rx_recv_data_len - 3] == PACKET_HEADER_1 and \
                        self.rx_data_buf[self.rx_recv_data_len - 2] == PACKET_HEADER_2 and \
                        self.rx_data_buf[self.rx_recv_data_len - 1] == PACKET_BYTE_STUFFING:
                    self.rx_recv_data_len -= 1
                    self.rx_byte_stuffing_cnt += 1
            elif self.rx_recv_data_len == 3:
                if self.rx_inst_idx == PACKET_HEADER_0 and \
                        self.rx_data_buf[0] == PACKET_HEADER_1 and \
                        self.rx_data_buf[1] == PACKET_HEADER_2 and \
                        self.rx_data_buf[2] == PACKET_BYTE_STUFFING:
                    self.rx_recv_data_len -= 1
                    self.rx_byte_stuffing_cnt += 1
            # check length
            if self.rx_recv_data_len + self.rx_byte_stuffing_cnt + 3 == self.rx_packet_len:
                # 3 = Instruction(1)+CRC(2)
                self.rx_parse_state = PacketState.PACKET_PARSING_STATE_CRC_L

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_CRC_L:
            self.rx_recv_crc = recv_data
            self.rx_parse_state = PacketState.PACKET_PARSING_STATE_CRC_H

        elif self.rx_parse_state == PacketState.PACKET_PARSING_STATE_CRC_H:
            self.rx_recv_crc = make_word(self.rx_recv_crc, recv_data)
            if self.rx_recv_crc == self.rx_calculated_crc:
                ret = Feedback.FEEDBACK_OK
            else:
                ret = Feedback.FEEDBACK_ERROR_CRC
            self.rx_is_init = False
            self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE
        else:
            self.rx_parse_state = PacketState.PACKET_PARSING_STATE_IDLE
            print("[Easy Protocol] Wrong rx parse state")
        return ret

    def get_tx_packet(self):
        if self.tx_generated_packet_length < 1:
            return []
        else:
            return self.tx_packet_buf[0:self.tx_generated_packet_length]

    def get_rx_data(self):
        if self.rx_recv_data_len < 1:
            return []
        else:
            return self.rx_data_buf[0:self.rx_recv_data_len]

    def get_rx_inst(self):
        return self.rx_inst_idx
