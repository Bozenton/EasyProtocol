classdef EP_ToParsePacket
    properties
        header = [0 0 0];
        header_cnt;
        id;
        inst_idx;
        p_data_buf;
        data_buf_capacity;
        recv_data_len;
        packet_len;
        calculated_crc;
        recv_crc;
        reserved;
        parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
        is_init;
    end
end

