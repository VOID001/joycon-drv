#include <stdio.h>
#include <hidapi/hidapi.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <joycon.h>
#include <pthread.h>
#include <math.h>

#define MAX_STR 255

int send_led_packet(hid_device *handle, uint8_t led_data);
void debug_hexdump(uint8_t *data, int len);
void send_key(const char *keystr, int pressed);
void parse_keycode(const uint8_t cur_keymap, const uint8_t pre_keymap);
int send_subcommand_data_packet(hid_device *handle, const uint8_t *packet, const uint8_t len);
void interactive_io_shell();
int string2byte(const char *str, uint8_t *out_data);

int main(int argc, char **argv) {
    int res;
    wchar_t wstr[MAX_STR];
    fd_set rset;
    int nfds= 0;


    // Open controller here
    interactive_io_shell();
    // And exit

    struct controller *ctrl = new_controller();
    res = init_controller(ctrl);
    if(res < 0) {
        fprintf(stderr, "%s start failed", argv[0]);
        exit(res);
    }
    printf("joycon device init ok.\n");

    FD_ZERO(&rset);
    if(ctrl->type == TYP_JOYCON) {
        printf("DBG: Set joycon fd to fdset\n");
        FD_SET(ctrl->joycon_L, &rset);
        FD_SET(ctrl->joycon_R, &rset);
        nfds = (ctrl->joycon_L > ctrl->joycon_R ? ctrl->joycon_L : ctrl->joycon_R) + 1;
    }
    if(ctrl->type == TYP_PRO) {
        FD_SET(ctrl->pro_control, &rset);
        nfds = ctrl->pro_control + 1;
    }
    while(1) {
        res = select(nfds, &rset, NULL, NULL, NULL);
        if(res < 0) {
            fprintf(stderr, "select return error, code %d\n", res);
            exit(res);
        }
        if(res) {
            printf("DBG: data received\n");
            pthread_t threadL = 0, threadR = 0;
            if(FD_ISSET(ctrl->joycon_L, &rset)) {
                pthread_create(&threadL, NULL, joycon_read, (void *)ctrl->joycon_L);
            }
            if(FD_ISSET(ctrl->joycon_R, &rset)) {
                pthread_create(&threadR, NULL, joycon_read, (void *)ctrl->joycon_R);
            }
            if(threadL) pthread_join(threadL, NULL);
            if(threadR) pthread_join(threadR, NULL);
        }
    }
    return 0;
}

void interactive_io_shell() {
    hid_device *handle = hid_open(0x057e, 0x2006, NULL);
    char input_buf[4096];
    uint8_t out_data[0x40];
    while(1) {
        printf("> ");
        bzero(out_data, 0x40);
        fgets(input_buf, 4096, stdin);
        int err = string2byte(input_buf, out_data);
        if(err < 0) {
            fprintf(stderr, "string 2 byte argument error\n");
            continue;
        }
        debug_hexdump(out_data, 0x40);
        err = send_subcommand_data_packet(handle, out_data, 0x10);
        printf("< %d\n", err);
    }
}

void debug_hexdump(uint8_t *data, int len) {
    printf("--- BEGIN DEBUG HEXDUMP ---\n");
    for(int i = 0; i < len; i++) {
        printf("\\x%x", data[i]);
    }
    printf("--- END DEBUG HEXDUMP ---\n");
}

int string2byte(const char *str, uint8_t *out_data) {
    char *p = str;
    int state = 0;
    uint8_t onebyte = 0x00;
    int cnt = 0;
    if(str[0] == '\n') {
        return -2;
    }
    while(*p != '\n') {
        if(state == 0) {
            if(*p == '0') {
                state = 1;
                p++;
                if(*p != 'x') {
                    fprintf(stderr, "we need an x\n");
                    return -1;
                }
                p++;
                continue;
            } else {
                fprintf(stderr, "we need an 0\n");
                return -1;
            }
        }
        if(state == 1) {
            if(*p >= '0' && *p <= '9')  {
                onebyte = (onebyte << 4) + (*p - '0');
            } else if( *p >= 'A' && *p <= 'F') {
                onebyte = (onebyte << 4) + (*p - 'A' + 10);
            } else if(*p == ' ') {
                out_data[cnt++] = onebyte;
                printf("\\x%x", onebyte);
                onebyte = 0;
                state = 0;
            } else {
                fprintf(stderr, "invalid hex char\n");
                return -1;
            }
            p++;
        }
    }
    if(*p == '\n') {
        out_data[cnt++] = onebyte;
        printf("\\x%x", onebyte);
        onebyte = 0;
        state = 0;
    }
    printf("\n", onebyte);
    return 0;
}
