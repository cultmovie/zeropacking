#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "macros.h"

static void pack(char const *ifilename, char const *ofilename);

static void display_usage(){
    printf("usage:\n");
    printf("    packz -i [FILE] -o [FILE]\n");
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
    pack(ifilename, ofilename);
    return EXIT_SUCCESS;
}

static void zero_pack(uint8_t read_buffer[], FILE *ofile){
    int total_nonzero_num = 0;
    for(int i = 0;i < PACK_BYTE_NUM;i++){
        if(read_buffer[i] != 0)
            total_nonzero_num++;
    }
    int pack_byte_num = total_nonzero_num + 1;
    uint8_t pack_buffer[pack_byte_num];
    memset(pack_buffer, 0, sizeof(uint8_t) * pack_byte_num);
    int nonzero_num = 0;
    for(int i = 0;i < PACK_BYTE_NUM;i++){
        if(read_buffer[i] != 0){
            pack_buffer[0] |= 1 << i;
            pack_buffer[++nonzero_num] = read_buffer[i];
        }
    }
    size_t write_num = fwrite(pack_buffer, sizeof(uint8_t), pack_byte_num, ofile);
    if(write_num != pack_byte_num){
        perror("write ofile failed");
        exit(EXIT_FAILURE);   
    }
}

static void align_read_buffer(uint8_t read_buffer[], size_t buffer_size){
    if(buffer_size < PACK_BYTE_NUM){
        for(int i = buffer_size;i < PACK_BYTE_NUM;i++)
            read_buffer[i] = 0;
    }
}

static void write_header(FILE *ifile, FILE *ofile){
    if(fseek(ifile, 0, SEEK_END) != 0){
        perror("fseek ifile end");
        exit(EXIT_FAILURE);
    }
    long ifile_size = ftell(ifile);
    if(!ifile_size){
        printf("pack failed,input file is empty\n");
        exit(EXIT_FAILURE);
    }
    if(ifile_size > UINT_MAX){
        printf("pack failed,input file is too big,max file size is 4G\n");
        exit(EXIT_FAILURE);
    }
    if(ferror(ifile)){
        perror("ftell ifile");
        exit(EXIT_FAILURE);
    }
    if(fseek(ifile, 0, SEEK_SET) != 0){
        perror("fseek ifile start");
        exit(EXIT_FAILURE);
    }
    uint8_t header[HEADER_BYTE_NUM];
    for(int i = HEADER_BYTE_NUM - 1;i >= 0;i--){
       header[i] = ifile_size % 256;
       ifile_size >>= 8;
    }
    size_t write_num = fwrite(header, sizeof(uint8_t), HEADER_BYTE_NUM, ofile);
    if(write_num != HEADER_BYTE_NUM){
        perror("write ofile header");
        exit(EXIT_FAILURE);
    }
}

static void pack(char const *ifilename, char const *ofilename){
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
    write_header(ifile, ofile);
    uint8_t read_buffer[PACK_BYTE_NUM];
    for(;;){
        memset(read_buffer, 0, sizeof(uint8_t) * PACK_BYTE_NUM);
        size_t read_num = fread(read_buffer, sizeof(uint8_t), PACK_BYTE_NUM, ifile);
        if(ferror(ifile)){
            perror("read file failed");
            exit(EXIT_FAILURE);
        }
        assert(read_num <= PACK_BYTE_NUM);
        if(read_num > 0){
            align_read_buffer(read_buffer, read_num);
            zero_pack(read_buffer, ofile);
        }
        if(feof(ifile) || read_num < PACK_BYTE_NUM)
            break;
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
