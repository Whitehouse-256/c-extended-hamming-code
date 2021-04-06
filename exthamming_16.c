#include <stdio.h>

void print_test(unsigned char* bytes){
    // bytes is 2 B - 16 bits
    printf("%i %i %i %i\n", !!(bytes[0]&0x80), !!(bytes[0]&0x40), !!(bytes[0]&0x20), !!(bytes[0]&0x10));
    printf("%i %i %i %i\n", !!(bytes[0]&0x08), !!(bytes[0]&0x04), !!(bytes[0]&0x02), !!(bytes[0]&0x01));
    printf("%i %i %i %i\n", !!(bytes[1]&0x80), !!(bytes[1]&0x40), !!(bytes[1]&0x20), !!(bytes[1]&0x10));
    printf("%i %i %i %i\n", !!(bytes[1]&0x08), !!(bytes[1]&0x04), !!(bytes[1]&0x02), !!(bytes[1]&0x01));
}

/*void print_test(unsigned char byte){
    // bytes is 2 B - 16 bits
    printf("%i %i %i %i  ", !!(byte&0x80), !!(byte&0x40), !!(byte&0x20), !!(byte&0x10));
    printf("%i %i %i %i\n", !!(byte&0x08), !!(byte&0x04), !!(byte&0x02), !!(byte&0x01));
}*/

void encode(unsigned char* out, const unsigned char* in){
    // out is 2 B - 16 bits
    // in is 2 B - first 11 bits (0-10) are used
    //zero output
    out[0] = 0;
    out[1] = 0;
    out[0] |= (in[0] & 0x80) >> 3; //bit 0
    out[0] |= (in[0] & 0x70) >> 4; //bits 1,2,3
    out[1] |= (in[0] & 0x0E) << 3; //bits 4,5,6
    out[1] |= (in[0] & 0x01) << 3; //bit 7
    out[1] |= (in[1] & 0xE0) >> 5; //bits 8,9,10
    
    //compute parity
    unsigned char parity = (out[0] & 0x55) | ((out[1] << 1) & 0xAA);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    out[0] |= 0x40 & ((parity & 0x01) << 6);

    parity = (out[0] & 0x33) | ((out[1] << 2) & 0xCC);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    out[0] |= 0x20 & ((parity & 0x01) << 5);

    parity = (out[0] & 0x0F) | ((out[1] << 4) & 0xF0);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    out[0] |= 0x08 & ((parity & 0x01) << 3);

    parity = out[1];
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    out[1] |= 0x80 & ((parity & 0x01) << 7);
    
    //compute global parity
    parity = out[0] ^ out[1];
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    out[0] |= 0x80 & ((parity & 0x01) << 7);
}

unsigned char repair(unsigned char* in){
    // in is 2B - 16 bits
    // returns number of errors, tries to repair in array
    unsigned char error_index = 0;
    
    unsigned char parity = (in[0] & 0x55) | ((in[1] << 1) & 0xAA);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    error_index |= 0x10 & ~((parity & 0x01) << 4);

    parity = (in[0] & 0x33) | ((in[1] << 2) & 0xCC);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    error_index |= 0x20 & ~((parity & 0x01) << 5);

    parity = (in[0] & 0x0F) | ((in[1] << 4) & 0xF0);
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    error_index |= 0x40 & ~((parity & 0x01) << 6);

    parity = in[1];
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    error_index |= 0x01 & (parity & 0x01);

    unsigned char errors = 0;
    if(error_index != 0x70){
        //fix a 1 bit error
        in[error_index & 0x01] ^= (0x01 << ((error_index & 0xF0) >> 4));
        errors = 1;
    }
    
    //check global checksum
    parity = in[0] ^ in[1];
    parity ^= parity >> 4;
    parity ^= parity >> 2;
    parity ^= parity >> 1;
    if(parity & 0x01){
        if(errors == 0){
            in[0] ^= 0x80;
            errors = 1;
        }else errors = 2;
    }
    return errors;
}

int main(){
    unsigned char bytes[2];
    unsigned char in[] = {0x31, 0xC0};

    encode(bytes, in);

    for(unsigned int i=0; i<2; i++){
        for(unsigned int j=0; j<8; j++){
            unsigned char encoded[] = {bytes[0], bytes[1]};
            //make an error
            encoded[i] ^= (0x01 << j);
            //repair an error
            unsigned char errors = repair(encoded);
            printf("- run [%u,%u]: errors=%u", i, j, errors);
            if(encoded[0] != bytes[0] || encoded[1] != bytes[1]){
                printf(" ERROR NOT FIXED!");
            }
            printf("\n");
        }
    }

    return 0;
}
