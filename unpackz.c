#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "macros.h"

static int tail_byte_num = 0;

static void unpack(char const *ifilename, char const *ofilename);

static void display_usage(){
    printf("usage:\n");
    printf("    unpackz -i [FILE] -o [FILE]\n");
}

int main(int argc,char *argv[]){
    int opt;
    opterr = 0;
    int opt_num = 0;
    char *ifilename = NULL;
    char *ofilename = NULL;
    while((opt = getopt(argc, argv, "i:o:")) != -1){
        switch(opt){
            case 'i':
                ifilename = optarg;
                opt_num++;
                break;
            case 'o':
                ofilename = optarg;
                opt_num++;
                break;
            case '?':
                printf("option -%c invalid or need arg\n", optopt);
                display_usage();
                exit(EXIT_FAILURE);
                break;
            default:
                printf("unknow option");
                display_usage();
                break;
        }
    }
    if(opt_num < 2){
        display_usage();
        exit(EXIT_FAILURE);
    }
    unpack(ifilename, ofilename);
    return EXIT_SUCCESS;
}

static void zero_unpack(uint8_t tag, FILE *ifile, FILE *ofile){
    int num = 0;
    uint8_t tag2 = tag;
    for(;tag;tag >>= 1)
        num += tag & 1;
    int tag_idx[num];
    int j = 0;
    for(int i = 0;tag2;tag2 >>= 1, i++){
        if(tag2 & 1)
            tag_idx[j++] = i;
    }
    uint8_t unpack_buffer[PACK_BYTE_NUM];
    memset(unpack_buffer, 0, sizeof(uint8_t) * PACK_BYTE_NUM);
    int pack_num = PACK_BYTE_NUM;
    if(num){
        uint8_t data_buffer[num];
        size_t read_num = fread(data_buffer, sizeof(uint8_t), num, ifile);
        if(read_num != num){
            perror("read ifile");
            exit(EXIT_FAILURE);
        }
        for(int i = 0;i < num;i++)
            unpack_buffer[tag_idx[i]] = data_buffer[i];
        read_num = fread(data_buffer, sizeof(uint8_t), 1, ifile);
        if(read_num == 1){ //未到文件尾
            if(fseek(ifile, -1, SEEK_CUR) != 0){
                perror("fseek ifile cur");
                exit(EXIT_FAILURE);
            }
        }
        else { //到了文件尾
            if(tail_byte_num > 0){
                pack_num = tail_byte_num;
            }
        }
    }
    fwrite(unpack_buffer, sizeof(uint8_t), pack_num, ofile);
    if(ferror(ofile)){
        perror("write ofile failed");
        exit(EXIT_FAILURE);   
    }
}

static void read_header(FILE *ifile){
    uint8_t header[HEADER_BYTE_NUM];
    size_t read_num = fread(header, sizeof(uint8_t), HEADER_BYTE_NUM, ifile);
    if(read_num != HEADER_BYTE_NUM){
        printf("read header failed,ifile is empty\n");
        exit(EXIT_FAILURE);
    }
    uint32_t file_size = 0;
    for(int i = 0;i < HEADER_BYTE_NUM;i++){
        file_size |= header[i] << ((HEADER_BYTE_NUM - i - 1)*8);
    }
    tail_byte_num = file_size % PACK_BYTE_NUM;
}

static void unpack(char const *ifilename, char const *ofilename){
    FILE *ifile = fopen(ifilename, "rb");
    if(ifile == NULL){
        perror("open ifile failed");
        exit(EXIT_FAILURE);
    }
    FILE *ofile = fopen(ofilename, "ab");
    if(ofile == NULL){
        perror("open ofile failed");
        exit(EXIT_FAILURE);
    }
    read_header(ifile);
    uint8_t tag_buffer[TAG_BYTE_NUM];
    for(;;){
        size_t read_num = fread(tag_buffer, sizeof(uint8_t), TAG_BYTE_NUM, ifile);
        if(read_num != TAG_BYTE_NUM)
            break;
        zero_unpack(tag_buffer[0], ifile, ofile);   
    }
    if(fclose(ifile) != 0){
        perror("fclose ifile");
        exit(EXIT_FAILURE);
    }
    if(fclose(ofile) != 0){
        perror("fclose ofile");
        exit(EXIT_FAILURE);
    }
}
