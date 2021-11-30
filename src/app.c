#include "app.h"
#include "ll.h"
#include "utils.h"

#define DATA 1
#define START 2
#define END 3

#define FILENAME 0
#define FILESIZE 1

typedef struct{
    int fileDescriptor;
    Source status;
} applicationLayer;

typedef struct {
    char *fileName;
    char fileNameSz;
    size_t fileSize;
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

    return 0;
}

int update_file_info(const TLV *tlv)
{
    switch (tlv->T) {
        case FILENAME:
            file_info.fileName = tlv->V;
            file_info.fileNameSz = tlv->L;
            break;

        case FILESIZE:
            memcpy(&file_info.fileSize, tlv->V, tlv->L);
            break;
        
        default:
            return -1;
    }

    return 0;
}

int app_start(int port, Source status)
{
    al.status = status;
    int fd = llopen(port, al.status);
    if (fd > 0)
        al.fileDescriptor = fd;
    return fd;
}

int app_end()
{
    return llclose(al.fileDescriptor);
}

int send_control_packet(char ctrl, TLV* tlv_arr, unsigned n_tlv)
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

    return llwrite(al.fileDescriptor, packet, curr_idx) == curr_idx ? 0 : -1;
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
        curr_idx += read_TLV(packet + curr_idx, &curr_tlv);
        if (update_file_info(&curr_tlv) == -1)
            return -1;
    }

    printf("Filename: %s, Filesize: %zu", file_info.fileName, file_info.fileSize);

    return 0;
}