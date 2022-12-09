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
        generated_packet_length;
        tx_is_init;

        % Receive


    end
    
    methods
        % Constructor
        function obj = EasyProtocol()
            obj.tx_packet_buf = uint8(zeros(1, obj.PACKET_TX_BUF_CAPACITY));

            var_load = load("crc_table.mat");
            obj.crc_table = var_load.crc_table;
        end
        
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
            obj.generated_packet_length = uint16(0);
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

            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitand(obj.tx_data_length, 0x00FF));
            generated_packet_len = generated_packet_len+1;
            obj.tx_packet_buf(generated_packet_len+1) = uint8(bitshift(obj.tx_data_length, -8));
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
            
            obj.generated_packet_length = generated_packet_len;
            
            ret = EP_Feedback.FEEDBACK_OK;
        end

        function crc = update_crc(obj, crc_current, data)
            assert(isa(crc_current, 'uint16'));
            assert(isa(data, 'uint8'));
            
            temp1 = uint8(bitsrl(crc_current, 8));
            temp2 = bitxor(temp1, data);
            idx = bitand(temp2, 0xFF);
            
            temp1 = bitsll(crc_current, 8);
            temp2 = obj.crc_table(idx+1);

            crc = bitxor(temp1, temp2);
        end
    end
end

