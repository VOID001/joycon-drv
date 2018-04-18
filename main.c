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
void send_key(const char *keystr, int pressed);
void parse_keycode(const uint8_t cur_keymap, const uint8_t pre_keymap);

int main(int argc, char **argv) {
    int res;
    wchar_t wstr[MAX_STR];
    fd_set rset;
    int nfds= 0;

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

