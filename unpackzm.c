#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "macros.h"

static void unpack(char const *ifilename, char const *ofilename);

static void display_usage(){
    printf("usage:\n");
    printf("    unpackzm -i [FILE] -o [FILE]\n");
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

static long get_ifile_size(FILE *file){
    if(fseek(file, 0, SEEK_END) != 0){
        perror("get file size,fseek end");
        exit(EXIT_FAILURE);
    }
    long file_size = ftell(file);
    if(file_size == -1){
        perror("ftell ifile");
        exit(EXIT_FAILURE);
    }
    if(file_size <= 0){
        printf("get file size failed,file is empty\n");
        exit(EXIT_FAILURE);
    }
    if(fseek(file, 0, SEEK_SET) != 0){
        perror("get file size,fseek start");
        exit(EXIT_FAILURE);
    }
    return file_size;
}

static long get_origin_file_size(uint8_t inbuffer[]){
    long file_size = 0;
    for(int i = 0;i < HEADER_BYTE_NUM;i++){
        file_size |= inbuffer[i] << ((HEADER_BYTE_NUM - i - 1)*CHAR_BIT);
    }
    return file_size;
}

static int get_nonzero_num(uint8_t tag){
    int num = 0;
    for(;tag;tag >>= 1)
        num += tag & 1;
    return num;
}

static void gen_nonzero_outbuffer_idxs(long outbuffer_idxs[], uint8_t tag, long offset){
    int j = 0;
    for(int i = 0;tag;tag >>= 1, i++){
        if(tag & 1)
            outbuffer_idxs[j++] = i + offset;
    }
}

static void unpack_zero(FILE *ifile, FILE *ofile){
    long ifile_size = get_ifile_size(ifile);
    uint8_t inbuffer[ifile_size];
    size_t read_num = fread(inbuffer, sizeof(uint8_t), ifile_size, ifile);
    assert(read_num == ifile_size);

    long origin_file_size = get_origin_file_size(inbuffer);
    uint8_t *outbuffer = malloc(sizeof(uint8_t)*origin_file_size);

    long inbuffer_idx = HEADER_BYTE_NUM;
    long tag_idx = 0;
    while(inbuffer_idx < ifile_size - 1){
        int num = get_nonzero_num(inbuffer[inbuffer_idx]);
        if(num){
            long nonzero_outbuffer_idxs[num];
            gen_nonzero_outbuffer_idxs(nonzero_outbuffer_idxs, inbuffer[inbuffer_idx], tag_idx*PACK_BYTE_NUM);

            for(int i = 0;i < num;i++){
                ++inbuffer_idx;
                outbuffer[nonzero_outbuffer_idxs[i]] = inbuffer[inbuffer_idx];
            }
            inbuffer_idx++;
            tag_idx++;
        }
        else{
            inbuffer_idx++;
            tag_idx++;
        }
    }
    size_t write_num = fwrite(outbuffer, sizeof(uint8_t), origin_file_size, ofile);
    free(outbuffer);
    if(write_num != origin_file_size){
        perror("fwrite outbuffer");
        exit(EXIT_FAILURE);
    }
}

static void unpack(char const *ifilename, char const *ofilename){
    FILE *ifile = fopen(ifilename, "rb");
    if(ifile == NULL){
        perror("open ifile failed");
        exit(EXIT_FAILURE);
    }
    FILE *ofile = fopen(ofilename, "wb");
    if(ofile == NULL){
        perror("open ofile failed");
        exit(EXIT_FAILURE);
    }

    unpack_zero(ifile, ofile);
    
    if(fclose(ifile) != 0){
        perror("fclose ifile");
        exit(EXIT_FAILURE);
    }
    if(fclose(ofile) != 0){
        perror("fclose ofile");
        exit(EXIT_FAILURE);
    }
}
