#include "app.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int print_usage()
{
    printf("Usage:\tapp serialport role [filepath]\n\tex: app /dev/ttyS1 sender pinguim.gif\n");
    return -1;
}

int main(int argc, char **argv) {
    if (argc < 3)
        return print_usage();

    if (strcmp("/dev/ttyS0", argv[1]) != 0 && strcmp("/dev/ttyS1", argv[1]) != 0 && strcmp("/dev/ttyS10", argv[1]) != 0 && strcmp("/dev/ttyS11", argv[1]) != 0)
        return print_usage();

    int port = atoi(argv[1] + 9);
        
    if (strcmp("sender", argv[2]) != 0 && strcmp("receiver", argv[2]) != 0)
        return print_usage();
    
    Source role = (strcmp("sender", argv[2]) == 0) ? SENDER : RECEIVER;

    if ((role == SENDER && argc != 4) || (role == RECEIVER && argc != 3))
        return print_usage();

    printf("Starting app...\n");

    app_start(port, role);

    int result;

    if (role == SENDER)
    {
        printf("Assembling and sending...\n"); 
        char *filepath = argv[3];
        result = send_file(filepath);

        if (result != 0)
        {
            printf("Error sending data.\n");
            return -1;
        }
    }

    if (role == RECEIVER)
    {
        printf("Waiting to receive file.\n");

        result = receive_file();

        if (result != 0)
        {
            printf("Error receiving data.\n");
            return -1;
        }
    }

    printf("Closing...\n");

    app_end();

    printf("App is closed.\n");

    return 0;
}