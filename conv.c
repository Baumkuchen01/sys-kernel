#include "conv.h"

typedef unsigned long long int size_t;
uint64_t* CONV_BASE = (uint64_t*)0x10001000L;
const size_t CONV_KERNEL_OFFSET = 0;
const size_t CONV_DATA_OFFSET = 1;
const size_t CONV_RESULT_LO_OFFSET = 0;
const size_t CONV_RESULT_HI_OFFSET = 1;
const size_t CONV_STATE_OFFSET = 2;
const unsigned char READY_MASK = 0b01;
const size_t CONV_ELEMENT_LEN = 4;

uint64_t* MISC_BASE = (uint64_t*)0x10002000L;
const size_t MISC_TIME_OFFSET = 0;

uint64_t get_time(void){
    return MISC_BASE[MISC_TIME_OFFSET];
}

void conv_kernel_init(const uint64_t* kernel_array, size_t kernel_len){
    for(size_t i = 0; i < kernel_len; i++)
        CONV_BASE[CONV_KERNEL_OFFSET] = kernel_array[i];
}

void conv_compute_one_byte(uint64_t data){
    CONV_BASE[CONV_DATA_OFFSET] = data;
}

void conv_compute(const uint64_t* data_array, size_t data_len, const uint64_t* kernel_array, size_t kernel_len, uint64_t* dest){
    // fill the code
    conv_kernel_init(kernel_array, kernel_len);
    for (int i = -3; i < (int)(data_len + kernel_len - 1); i++)
    {
        // while (!(CONV_BASE[CONV_STATE_OFFSET] & READY_MASK))
        //     ;
        uint64_t data = (i >= 0 && i < data_len) ? data_array[i] : 0;
        conv_compute_one_byte(data);
        if (i >= 0)
        {
            dest[i * 2] = CONV_BASE[CONV_RESULT_HI_OFFSET];
            dest[i * 2 + 1] = CONV_BASE[CONV_RESULT_LO_OFFSET];
        }
    }
}

void mul_compute(const uint64_t* data_array, size_t data_len, const uint64_t* kernel_array, size_t kernel_len, uint64_t* dest){
    // fill the code
    uint64_t data, buffer[2], carry;
    for (size_t i = 0; i < data_len + kernel_len - 1; i++)
    {   
        dest[i * 2] = dest[i * 2 + 1] = 0;
        for (size_t j = 0; j < kernel_len; j++)
        {
            buffer[0] = 0;
            buffer[1] = kernel_array[j];
            if (i + j - kernel_len + 1 < 0 || i + j - kernel_len + 1 >= data_len)
            {
                buffer[1] = 0;
                continue;
            }
            data = data_array[i + j - kernel_len + 1];
            for (int k = 0; k < 64; k++)
            {
                if (buffer[1] & 1)
                {
                    carry = buffer[0] + data < buffer[0];
                    buffer[0] += data;
                }
                else
                    carry = 0;
                buffer[1] >>= 1;
                buffer[1] |= buffer[0] << 63;
                buffer[0] >>= 1;
                buffer[0] |= carry << 63;
            }
            carry = dest[i * 2 + 1] + buffer[1] < dest[i * 2 + 1];
            dest[i * 2 + 1] += buffer[1];
            dest[i * 2] += buffer[0] + carry;
        }
    }
}
