/**
 * Author: E.K.Jithendiran
 * Date  : 29 Apr 2026 
 * 
 * This code implements the reversal logic using bitwise operations for fixed-width types and 
 * a buffer-based approach for arbitrary lengths (N-bytes).
 * 
 * 
 * gcc byte_swapper.c -o /tmp/byte_swapper.o
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* 
 * 8-bit: No conversion required.
 * 8-bit values consist of a single byte; therefore, the byte order is 
 * always consistent regardless of architecture.
 */
static inline uint8_t swap8(uint8_t val) {
    return val;
}

/* 16-bit: Swap two bytes */
static inline uint16_t swap16(uint16_t val) {
    /*
    Given the input 0x1234 (where 0x12 is the high byte and 0x34 is the low byte):
    In little endian system at RAM the data stored as 34, 12, when loaded into register as 16bytes it will be 1234

    Memory: 34 12
    Register: 0x1234

    hex     =    12         | 34
    binary  =    00010010    00110100    

    1. val << 8 move  8bits right
    12          34          34       00
    00010010    00110100  = 00110100 00000000
    2.  val >> 8 move 8bits left
    12          34          00       12
    00010010    00110100  = 00000000 00010010
    
    3. now add both (|)
    00110100 00000000
    00000000 00010010
    ------------------
    00110100 00010010
    34       12
    */
    return (uint16_t)((val << 8) | (val >> 8));
}

/* 32-bit: Swap four bytes */
static inline uint32_t swap32(uint32_t val) {
    /*
    0x12345678

    12 -> 00010010
    34 -> 00110100
    56 -> 01010110
    78 -> 01111000

    (val << 24) & 0xff000000
        1. (val << 24) & 0xff000000)
            move 24bits left
            12          34          56          78
            00010010    00110100    01010110    01111000

                1. after moving 8 bits left (8bit)
                    34          56          78          00
                    00110100    01010110    01111000    00000000
                2. after mving 8 bits left (16bit)
                    56          78          00          00
                    01010110    01111000    00000000    00000000
                2. after mving 8 bits left (24bit)
                    78          00          00          00
                    01111000    00000000    00000000    00000000
            (val << 24) = 01111000    00000000    00000000    00000000

        2. & 0xff000000 -> and operation with 11111111    00000000    00000000    00000000 
            zero out other than MSB

            01111000    00000000    00000000    00000000
            11111111    00000000    00000000    00000000 
            -----------------------------------------------
            01111000    00000000    00000000    00000000
            78          00          00          00
    
    (val << 8)  & 0x00ff0000
        1. (val << 8)

            12          34          56          78
            00010010    00110100    01010110    01111000

                after moving 8 bits left (8bit)
                    34          56          78          00
                    00110100    01010110    01111000    00000000
        2.  & 0x00ff0000

            00110100    01010110    01111000    00000000
            00000000    11111111    00000000    00000000
            ---------------------------------------------
            00000000    01010110    00000000    00000000    
            00          56          00          00

    (val >> 8)  & 0x0000ff00

        1. (val >> 8) move 8 bits right
            12          34          56          78
            00010010    00110100    01010110    01111000

            00          12          34          56          
            00000000    00010010    00110100    01010110
            
        2. & 0x0000ff00
            00          12          34          56   
            00000000    00010010    00110100    01010110
            00000000    00000000    11111111    00000000
            ---------------------------------------------
            00000000    00000000    00110100    00000000
            00          00          34          00
    
    (val >> 24) & 0x000000ff

        1. (val >> 24)

            12          34          56          78
            00010010    00110100    01010110    01111000

            after shift

            00          00          00          12
            00000000    00000000    00000000    00010010
        2. & 0x000000ff
            00000000    00000000    00000000    00010010
            00000000    00000000    00000000    11111111
            ---------------------------------------------
            00000000    00000000    00000000    00010010
    
    (|) add all together

    01111000    00000000    00000000    00000000
    00000000    01010110    00000000    00000000 
    00000000    00000000    00110100    00000000 
    00000000    00000000    00000000    00010010
    ---------------------------------------------
    01111000    01010110    00110100    00010010
    78          56          34          12
    */

    return ((val << 24) & 0xff000000) |
           ((val << 8)  & 0x00ff0000) |
           ((val >> 8)  & 0x0000ff00) |
           ((val >> 24) & 0x000000ff);
}

/* 64-bit: Swap eight bytes */
static inline uint64_t swap64(uint64_t val) {
    return ((val << 56) & 0xff00000000000000ULL) |
           ((val << 40) & 0x00ff000000000000ULL) |
           ((val << 24) & 0x0000ff0000000000ULL) |
           ((val << 8)  & 0x000000ff00000000ULL) |
           ((val >> 8)  & 0x00000000ff000000ULL) |
           ((val >> 24) & 0x0000000000ff0000ULL) |
           ((val >> 40) & 0x000000000000ff00ULL) |
           ((val >> 56) & 0x00000000000000ffULL);
}

/* * N-bytes: Arbitrary length reversal.
 * Uses a two-pointer approach to reverse the byte array in-place.
 */
void swap_n(uint8_t *buffer, size_t n) {
    size_t i = 0;
    size_t j = n - 1;
    uint8_t temp;
    
    while (i < j) {
        temp = buffer[i];
        buffer[i] = buffer[j];
        buffer[j] = temp;
        i++;
        j--;
    }
}

int main() {
    uint16_t test16 = 0x1234;
    uint32_t test32 = 0x12345678;
    uint8_t testN[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

    printf("Original 16: 0x%04x -> Swapped: 0x%04x\n", test16, swap16(test16)); // Original 16: 0x1234 -> Swapped: 0x3412
    printf("Original 32: 0x%08x -> Swapped: 0x%08x\n", test32, swap32(test32)); // Original 32: 0x12345678 -> Swapped: 0x78563412
    
    printf("Original N: ");
    for(int i=0; i<5; i++) printf("%02x ", testN[i]);
    
    swap_n(testN, 5);
    
    printf("\nSwapped N:  ");
    for(int i=0; i<5; i++) printf("%02x ", testN[i]);
    printf("\n");

    return 0;
}