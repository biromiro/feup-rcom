#include "app.h"
#include "ll.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#define DATA 1
#define START 2
#define END 3

#define FILENAME 0
#define FILESIZE 1

typedef struct {
    int fileDescriptor;
    Source status;
    int sequenceNumber;
} applicationLayer;

typedef struct {
    size_t fileSize;
    char fileNameSz;
    char* fileName;
} fileInfo;

applicationLayer al;
fileInfo file_info;

int read_TLV(char *buffer, TLV *tlv)
{
    tlv->T = buffer[0];
    tlv->L = buffer[1];

    for (int i = 0; i < tlv->L; i++)
    {
        tlv->V[i] = buffer[i+2];
    }

    return tlv->L+2;
}

int update_file_info(const TLV *tlv)
{
    switch (tlv->T) {
        case FILENAME:
            file_info.fileNameSz = tlv->L;
            file_info.fileName = malloc(file_info.fileNameSz);
            memcpy(file_info.fileName, tlv->V, file_info.fileNameSz);
            break;

        case FILESIZE:
            file_info.fileSize = atoll((const char *)tlv->V);
            break;

        default:
            return -1;
    }

    return 0;
}

int app_start(int port, Source status)
{
    al.status = status;
    al.sequenceNumber = 0;
    int fd = llopen(port, al.status);
    if (fd > 0)
        al.fileDescriptor = fd;
    return fd;
}

int app_end()
{
    free(file_info.fileName);
    return llclose(al.fileDescriptor);
}

int send_control_packet(u_int8_t ctrl, TLV* tlv_arr, unsigned n_tlv)
{
    size_t size = 1;

    for (int i = 0; i < n_tlv; i++)
    {
        size += tlv_arr[i].L + 2;
    }

    char *packet = malloc(size);

    int curr_idx = 0;

    packet[curr_idx++] = ctrl;

    for (int i = 0; i < n_tlv; i++)
    {
        packet[curr_idx++] = tlv_arr[i].T;
        int len = tlv_arr[i].L;
        packet[curr_idx++] = len;
        memcpy(packet + curr_idx, tlv_arr[i].V, len);
        curr_idx += len;
    }

    int result = llwrite(al.fileDescriptor, packet, curr_idx) == curr_idx ? 0 : -1;

    free(packet);
    
    return result;
}

int receive_start_packet()
{
    char packet[MAX_PACK_SIZE];

    int size = llread(al.fileDescriptor, packet);

    if (size == -1)
        return -1;
    
    if (packet[0] != START)
        return -1;
    
    int curr_idx = 1;
    TLV curr_tlv;

    while(curr_idx < size)
    {
        curr_idx += read_TLV(packet + curr_idx , &curr_tlv);
        if (update_file_info(&curr_tlv) == -1)
            return -1;
    }

    return 0;
}

int send_data(FILE *file, size_t size)
{
    char packet[MAX_PACK_SIZE];

    int read_bytes = 0;

    while (read_bytes < size) 
    {
        if (ferror(file)) return -2;
        if (feof(file)) return -1;

        int read_size = fread(packet + 4, 1, MAX_PACK_SIZE - 4, file);

        packet[0] = DATA;
        packet[1] = al.sequenceNumber++ % 256;
        packet[2] = read_size / 256;
        packet[3] = read_size - packet[2]*256;
        
        llwrite(al.fileDescriptor, packet, read_size + 4);

        read_bytes += read_size;
        memset(packet, 0, MAX_PACK_SIZE);
    }

    return 0;
}

int send_file(char *filepath)
{
    FILE *file = fopen(filepath, "r");
    struct stat stats;
    fstat(fileno(file), &stats);
    size_t size = stats.st_size;
    char *name = basename(filepath);
    size_t filenameSize;
    if ((filenameSize = strnlen(name, MAX_FILENAME_SIZE)) == MAX_FILENAME_SIZE)
    {
        return -1;
    }

    TLV filename;
    filename.T = 0;
    filename.L = filenameSize;
    filename.V = (u_int8_t*) name;

    TLV filesize;
    filesize.T = 1;
    char fileSize[MAX_FILENAME_SIZE];
    snprintf(fileSize, MAX_FILENAME_SIZE, "%ld", size);
    filesize.L = 2;
    filesize.V = (u_int8_t*) fileSize;

    TLV tlvs[] = {filename, filesize};

    send_control_packet(START, tlvs, 2);

    int res = send_data(file, size);

    if (res != 0) return -1;

    send_control_packet(END, NULL, 0);

    fclose(file);

    return 0;
}

int receive_data(FILE* file) {
    
    u_int8_t packet[MAX_PACK_SIZE];

    do
    {   

        int read_size = llread(al.fileDescriptor, (char *) packet);
        
        if(packet[0] == END) break;

        if(read_size < 4 || packet[0] != DATA) return -1;

        if(packet[1] != (al.sequenceNumber++ % 256)) return -1;

        size_t received_data_bytes = 256 * packet[2] + packet[3]; 

        printf("received_bytes = %ld\n", received_data_bytes);
        for(int i = 0; i < read_size; i++){
            printf("0x%2hhx\t", packet[i]);
        }
        int res = fwrite(packet + 4, 1, received_data_bytes, file);

        if(res <= 0) return -1;

    } while (1);

    return 0;
}

int receive_file()
{
    receive_start_packet();

    char filename[file_info.fileNameSz + 1 + 1];

    filename[0] = 'c';
    memcpy(filename + 1, file_info.fileName, file_info.fileNameSz + 1);

    FILE* file = fopen(filename, "w");

    int res = receive_data(file);

    fclose(file);

    return res;
}