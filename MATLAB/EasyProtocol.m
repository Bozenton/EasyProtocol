classdef EasyProtocol < handle
    %EASYPROTOCOL Summary of this class goes here
    %   Detailed explanation goes here
    
    properties (Constant)
        % Package Value
        PACKET_HEADER_0=0xFF;
        PACKET_HEADER_1=0xFF;
        PACKET_HEADER_2=0xFD;
        PACKET_RESERVED=0x00;
        PACKET_BYTE_STUFFING=0xFD;

        PACKET_MAX_ID=0xFD;
        PACKET_BROADCAST_ID=0xFE;
        
        % header(3)+reserved(1)+id(1)+length(2)+instruction(1)+crc(2)
        PACKET_MIN_SIZE=10;
        PACKET_TX_BUF_CAPACITY = 256;
        PACKET_RX_DATA_BUF_CAPACITY = 256;

        % Package index for different parts
        PACKET_IDX_HEADER_0=1;
        PACKET_IDX_HEADER_1=2;
        PACKET_IDX_HEADER_2=3;
        PACKET_IDX_RESERVED=4;
        PACKET_IDX_ID=5;
        PACKET_IDX_LENGTH_L=6;
        PACKET_IDX_LENGTH_H=7;
        PACKET_IDX_INST=8;
        PACKET_IDX_DATA=9;
    end

    properties
        % CRC
        crc_table;

        % Send
        tx_id;
        tx_inst_idx;
        tx_packet_buf;
        tx_data_length;
        tx_generated_packet_length;
        tx_is_init;

        % Receive
        rx_header;
        rx_header_cnt;
        rx_id;
        rx_inst_idx;
        rx_data_buf;
        rx_data_buf_capacity;
        rx_recv_data_len;
        rx_packet_len;
        rx_calculated_crc;
        rx_recv_crc;
        rx_reserved;
        rx_parse_state;
        rx_is_init;
        rx_byte_stuffing_cnt;
    end
    
    methods
        % =================================================================
        % Constructor
        % =================================================================
        function obj = EasyProtocol()
            obj.tx_packet_buf = uint8(zeros(1, obj.PACKET_TX_BUF_CAPACITY));
            obj.tx_is_init = false;

            var_load = load("crc_table.mat");
            obj.crc_table = var_load.crc_table;
            
            obj.rx_header = uint8([0, 0, 0]);
            obj.rx_header_cnt = 0;
            obj.rx_data_buf = uint8(zeros(1, obj.PACKET_RX_DATA_BUF_CAPACITY));
            obj.rx_data_buf_capacity = obj.PACKET_RX_DATA_BUF_CAPACITY;
            obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
            obj.rx_is_init = false;
        end

        % =================================================================
        % Send Functions 
        % =================================================================
        function ret = begin_make_packet(obj, id, inst_idx)
            assert(isa(id, 'uint8'), "begin_make_packet: input argument 'id' must be uint8");
            assert(isa(inst_idx, 'uint8'), "begin_make_packet: input argument 'inst_idx' must be uint8");

            if id ~= obj.PACKET_BROADCAST_ID && id >= obj.PACKET_MAX_ID
                ret = EP_Feedback.FEEDBACK_ERROR_INVALID_ID;
                return;
            end
            obj.tx_id = id;
            obj.tx_inst_idx = inst_idx;
            obj.tx_data_length = uint16(0);
            obj.tx_generated_packet_length = uint16(0);
            obj.tx_is_init = true;

            ret = EP_Feedback.FEEDBACK_OK;
        end

        function ret = add_data_to_packet(obj, data)
            assert(isa(data, 'uint8'), "add_data_to_packet: Input argument 'data' must be an uint8 vector")
            assert(isvector(data), "add_data_to_packet: Input argument 'data' must be a vector");
            
            if obj.tx_is_init == false
                ret = EP_Feedback.FEEDBACK_ERROR_NOT_INITIALIZED;
                return;
            end
            generated_data_len = obj.tx_data_length;
            ptr_data_buf = obj.PACKET_IDX_DATA;
            
            if generated_data_len+length(data)+obj.PACKET_MIN_SIZE > ... 
                    obj.PACKET_TX_BUF_CAPACITY
                ret = EP_Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW;
                return;
            end

            for i=1:length(data)
                obj.tx_packet_buf(ptr_data_buf+generated_data_len) = data(i);
                generated_data_len = generated_data_len + 1;
                
                % add byte stuffing
                if generated_data_len >= 3
                    if obj.tx_packet_buf(ptr_data_buf+generated_data_len-3) == obj.PACKET_HEADER_0 && ...
                            obj.tx_packet_buf(ptr_data_buf+generated_data_len-2) == obj.PACKET_HEADER_1 && ...
                            obj.tx_packet_buf(ptr_data_buf+generated_data_len-1) == obj.PACKET_HEADER_2
                        obj.tx_packet_buf(ptr_data_buf+generated_data_len) = obj.PACKET_BYTE_STUFFING;
                        generated_data_len = generated_data_len + 1;
                    end
                elseif generated_data_len == 2
                    if obj.tx_packet_buf(obj.PACKET_IDX_INST) == obj.PACKET_HEADER_0 && ...
                            obj.tx_packet_buf(ptr_data_buf+generated_data_len-2) == obj.PACKET_HEADER_1 && ...
                            obj.tx_packet_buf(ptr_data_buf+generated_data_len-1) == obj.PACKET_HEADER_2
                        obj.tx_packet_buf(ptr_data_buf+generated_data_len) = obj.PACKET_BYTE_STUFFING;
                        generated_data_len = generated_data_len + 1;
                    end
                end
                % Check length
                if generated_data_len+length(data)+obj.PACKET_MIN_SIZE > ... 
                        obj.PACKET_TX_BUF_CAPACITY
                    ret = EP_Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW;
                    return;
                end
            end % end for 
            obj.tx_data_length = generated_data_len;
            ret = EP_Feedback.FEEDBACK_OK;
        end

        function ret = end_make_packet(obj)
            if obj.tx_is_init == false
                ret = EP_Feedback.FEEDBACK_ERROR_NOT_INITIALIZED;
                return ;
            end
            generated_packet_len = uint16(0);
            obj.tx_packet_buf(generated_packet_len+1) = obj.PACKET_HEADER_0;
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = obj.PACKET_HEADER_1;
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = obj.PACKET_HEADER_2;
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = obj.PACKET_RESERVED;
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = obj.tx_id;
            generated_packet_len = generated_packet_len+1;
            % 3 = Instruction(1)+CRC(2)
            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitand(obj.tx_data_length+3, 0x00FF));
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitshift(obj.tx_data_length+3, -8));
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = obj.tx_inst_idx;
            generated_packet_len = generated_packet_len+1;

            generated_packet_len = generated_packet_len + obj.tx_data_length;
            
            calculated_crc = uint16(0);
            for i=1:generated_packet_len
                calculated_crc = update_crc(obj, calculated_crc, obj.tx_packet_buf(i));
            end

            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitand(calculated_crc, 0x00FF));
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitshift(calculated_crc, -8));
            generated_packet_len = generated_packet_len+1;
            
            obj.tx_generated_packet_length = generated_packet_len;
            
            ret = EP_Feedback.FEEDBACK_OK;
            obj.tx_is_init = false;
        end

        function crc = update_crc(obj, crc_current, data)
            assert(isa(crc_current, 'uint16'));
            assert(isa(data, 'uint8'));
            
            temp1 = uint8(bitsrl(crc_current, 8));
            temp2 = bitxor(temp1, data);
            idx = bitand(temp2, 0xFF);
            
            temp1 = bitsll(crc_current, 8);
            temp2 = obj.crc_table(uint16(idx)+1);

            crc = bitxor(temp1, temp2);
        end

        % =================================================================
        % Receive Functions 
        % =================================================================
        function ret = begin_parse_packet(obj)
            % ???????????????????????????????????????????????????
            obj.rx_is_init = true;
            ret = EP_Feedback.FEEDBACK_OK;
        end

        function ret = parse_packet(obj, recv_data)
            assert(isa(recv_data, 'uint8'), "parse_packet: Input argument 'recv_data' must be an uint8 Integer");
            if obj.rx_is_init == false
                ret = EP_Feedback.FEEDBACK_ERROR_NOT_INITIALIZED;
                return;
            end

            ret = EP_Feedback.FEEDBACK_PROCEEDING;
            switch obj.rx_parse_state
                case EP_PacketState.PACKET_PARSING_STATE_IDLE
                    if obj.rx_header_cnt >= 3
                        obj.rx_header_cnt = 0;
                    end
                    obj.rx_header(obj.rx_header_cnt+1) = recv_data;
                    obj.rx_header_cnt = obj.rx_header_cnt + 1;
                    if obj.rx_header_cnt == 3 
                        % Check if they match packet headers
                        if obj.rx_header(1) == obj.PACKET_HEADER_0 && ...
                                obj.rx_header(2) == obj.PACKET_HEADER_1 && ...
                                obj.rx_header(3) == obj.PACKET_HEADER_2
                            obj.rx_recv_data_len = 0;
                            obj.rx_calculated_crc = uint16(0);
                            obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, obj.PACKET_HEADER_0);
                            obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, obj.PACKET_HEADER_1);
                            obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, obj.PACKET_HEADER_2);
                            
                            obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_RESERVED;
                        else
                            % did not match those headers, shift them
                            obj.rx_header(1) = obj.rx_header(2);
                            obj.rx_header(2) = obj.rx_header(3);
                            obj.rx_header_cnt = obj.rx_header_cnt -1;
                        end
                    end % end if obj.rx_header_cnt == 3 

                case EP_PacketState.PACKET_PARSING_STATE_RESERVED
                    % It is used to address the situation where PACKET HEADERS appear in the data field
                    if(recv_data ~= obj.PACKET_BYTE_STUFFING)
                        obj.rx_reserved = recv_data;
                        obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_ID;
                    else
                        ret = EP_Feedback.FEEDBACK_ERROR_WRONG_PACKET;
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
                    end
                    
                case EP_PacketState.PACKET_PARSING_STATE_ID
                    if recv_data < obj.PACKET_MAX_ID || recv_data == obj.PACKET_BROADCAST_ID
                        obj.rx_id = recv_data;
                        obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_LENGTH_L;
                    else
                        ret = EP_Feedback.FEEDBACK_ERROR_INVALID_ID;
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
                    end

                case EP_PacketState.PACKET_PARSING_STATE_LENGTH_L
                    obj.rx_packet_len = uint16(recv_data);
                    obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);
                    obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_LENGTH_H;

                case EP_PacketState.PACKET_PARSING_STATE_LENGTH_H
                    obj.rx_packet_len = bitor(obj.rx_packet_len, bitsll(uint16(recv_data), 8));
                    obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);
                    if obj.rx_packet_len < 3
                        ret = EP_Feedback.FEEDBACK_ERROR_LENGTH;
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
                    else
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_INST;
                    end

                case EP_PacketState.PACKET_PARSING_STATE_INST
                    obj.rx_inst_idx = recv_data;
                    obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);
                    obj.rx_recv_data_len = uint16(0);
                    obj.rx_byte_stuffing_cnt = uint16(0);
                    if obj.rx_packet_len > obj.rx_data_buf_capacity + 3
                        % 3 = Instruction(1)+CRC(2)
                        ret = EP_Feedback.FEEDBACK_ERROR_BUFFER_OVERFLOW;
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
                    elseif obj.rx_packet_len == 3   % 3 = Instruction(1)+CRC(2)
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_CRC_L;
                    else 
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_DATA;
                    end

                case EP_PacketState.PACKET_PARSING_STATE_DATA
                    obj.rx_data_buf(obj.rx_recv_data_len+1) = recv_data;
                    obj.rx_recv_data_len = obj.rx_recv_data_len + 1;
                    obj.rx_calculated_crc = update_crc(obj, obj.rx_calculated_crc, recv_data);

                    % remove byte stuffing
                    if obj.rx_recv_data_len >=4 
                        if obj.rx_data_buf(1+obj.rx_recv_data_len-4) == obj.PACKET_HEADER_0 && ...
                                obj.rx_data_buf(1+obj.rx_recv_data_len-3) == obj.PACKET_HEADER_1 && ...
                                obj.rx_data_buf(1+obj.rx_recv_data_len-2) == obj.PACKET_HEADER_2 && ...
                                obj.rx_data_buf(1+obj.rx_recv_data_len-1) == obj.PACKET_BYTE_STUFFING
                            obj.rx_recv_data_len = obj.rx_recv_data_len - 1;
                            obj.rx_byte_stuffing_cnt = obj.rx_byte_stuffing_cnt + 1;
                        end
                    elseif obj.rx_recv_data_len == 3
                        if obj.rx_inst_idx == obj.PACKET_HEADER_0 && ...
                                obj.rx_data_buf(1) == obj.PACKET_HEADER_1 && ...
                                obj.rx_data_buf(2) == obj.PACKET_HEADER_2 && ...
                                obj.rx_data_buf(3) == obj.PACKET_BYTE_STUFFING
                            obj.rx_recv_data_len = obj.rx_recv_data_len - 1;
                            obj.rx_byte_stuffing_cnt = obj.rx_byte_stuffing_cnt + 1;
                        end
                    end
                    if obj.rx_recv_data_len + obj.rx_byte_stuffing_cnt + 3 == obj.rx_packet_len
                        % 3 = Instruction(1)+CRC(2)
                        obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_CRC_L;
                    end

                case EP_PacketState.PACKET_PARSING_STATE_CRC_L
                    obj.rx_recv_crc = uint16(recv_data);
                    obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_CRC_H;

                case EP_PacketState.PACKET_PARSING_STATE_CRC_H
                    obj.rx_recv_crc = bitor(obj.rx_recv_crc, bitsll(uint16(recv_data), 8));
                    if obj.rx_calculated_crc == obj.rx_recv_crc
                        ret = EP_Feedback.FEEDBACK_OK;
                    else
                        ret = EP_Feedback.FEEDBACK_ERROR_CRC;
                    end
                    obj.rx_is_init = false;
                    obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
                otherwise
                    obj.rx_parse_state = EP_PacketState.PACKET_PARSING_STATE_IDLE;
            end % end switch
        end

        % =================================================================
        % Interface Functions 
        % =================================================================
        function data = get_rx_data(obj)
            if obj.rx_recv_data_len < 1
                data = [];
            else
                data = obj.rx_data_buf(1:obj.rx_recv_data_len);

            end
        end
        function inst = get_rx_inst(obj)
            inst = obj.rx_inst_idx;
        end
        function packet = get_tx_packet(obj)
            if obj.tx_generated_packet_length < 1
                packet = [];
            else
                packet = obj.tx_packet_buf(1:obj.tx_generated_packet_length);
            end
        end
    end
end

