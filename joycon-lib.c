#include <stdlib.h>
#include <stdio.h>
#include <hidapi/hidapi.h>
#include <joycon.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

void parse_keycode(const uint8_t cur_keymap, const uint8_t pre_keymap);

// Allocate memory, and open the device
int init_controller(struct controller *ctrl) {
    struct hid_device_info *p, *devs;
    struct hid_device_info *l = NULL, *r = NULL, *pro = NULL;
    devs = hid_enumerate(JOYCON_VENDOR_ID, 0x00);
    p = devs;
    while(p && !((r && l) || pro)) {
        if(IS_JOYCON(p)) {
            printf("we found a joycon\n");
            if(p->product_id == PROD_ID_JOYCON_L) {
                l = p;
            }
            if(p->product_id == PROD_ID_JOYCON_R) {
                r = p;
            }
        }
        p = p->next;
    }
    /* Open device and return them */
    if(r != NULL && l != NULL) {
        ctrl->joycon_L = open(l->path, O_RDWR);
        ctrl->joycon_R = open(r->path, O_RDWR);
        if(ctrl->joycon_L < 0 || ctrl->joycon_R < 0) {
            fprintf(stderr, "controller init failed: cannot open hid device joycon\n");
            return -1;
        }
        printf("DBG: L fd = %d, R fd = %d\n", ctrl->joycon_L, ctrl->joycon_R);
        ctrl->type = TYP_JOYCON;
    } else if(pro != NULL) {
        ctrl->pro_control = open(pro->path, O_RDWR | O_NONBLOCK); 
        if(ctrl->pro_control < 0) {
            fprintf(stderr, "controller init failed: cannot open hid device pro controller\n");
            return -1;
        }
        ctrl->type = TYP_PRO;
    } else {
        fprintf(stderr, "controller init failed: not paired joycon, or cannot find pro controller\n");
        return -1;
    }
    hid_free_enumeration(devs);
    return 0;
}

struct controller* new_controller() {
    Controller *ctrl = (Controller *)malloc(sizeof(Controller));
    ctrl->joycon_L = 0;
    ctrl->joycon_R = 0;
    ctrl->pro_control = 0;
    return ctrl;
}

void *joycon_read(void *arg) {
    uint64_t fd = (uint64_t)arg;
    uint8_t buf[12];
    uint8_t pre = 0, cur = 0;
    int n = read(fd, buf, 12);
    if(n < 0) {
        fprintf(stderr, "Joycon read error, code %d\n", n);
    }
    cur = (uint8_t)buf[1];
    parse_keycode(cur, pre);
    return NULL;
}

void destroy_controller(struct controller *ctrl) {
    if(ctrl->joycon_L != 0) {
        close(ctrl->joycon_L);
    }
    if(ctrl->joycon_R != 0) {
        close(ctrl->joycon_R);
    }
    if(ctrl->pro_control != 0) {
        close(ctrl->pro_control);
    }
    free(ctrl);
}


void parse_keycode(const uint8_t cur_keymap, const uint8_t pre_keymap) {
    uint8_t diff = (cur_keymap ^ pre_keymap);
    int op = 0; // 0 for RELEASE, 1 for DOWN
    if(cur_keymap > pre_keymap) {
        op = 1;
        // pressed down
    } else {
        // released 
        op = 0;
    }
    if(diff & JOYCON_BTN_LEFT) {
        if(op == 0) {
            printf("release ");
        } else {
            printf("press down ");
        }
        printf(" LEFT\n");
        printf(" LEFT DONE\n");
    }
    if(diff & JOYCON_BTN_RIGHT) {
        if(op == 0) {
            printf("release ");
        } else {
            printf("press down ");
        }
        printf(" RIGHT\n");
        send_key("Right", op);
    }
    if(diff & JOYCON_BTN_UP) {
        if(op == 0) {
            printf("release ");
        } else {
            printf("press down ");
        }
        printf(" UP\n");
        send_key("Up", op);
    }
    if(diff & JOYCON_BTN_DOWN) {
        if(op == 0) {
            printf("release ");
        } else {
            printf("press down ");
        }
        printf(" DOWN\n");
        send_key("Down", op);
    }
    return;
}

void print_hex(const uint8_t *buf, int n) {
    for(int i = 0; i < n; i++) {
        if(i != 0) {
            printf(" ");
        }
        printf("0x%X", buf[i] & 0xFF);
    }
    printf("\n");
}

int send_led_packet(hid_device *handle, uint8_t led_data) {
    uint8_t packet[0x40];
    bzero(packet, 0x40);
    packet[0] = 0x01;
    packet[1] = 0x00;
    uint8_t rumbleData[8] = {0x04, 0xB1, 0x01, 0x01, 0xFF, 0xF1, 0xFF, 0xFF};
    memcpy(packet + 2, rumbleData, 8);
    packet[10] = 0x30;
    packet[11] = led_data;
    int n = hid_write(handle, packet, 0x40);
    if(n < 0) {
        fprintf(stderr, "write to hid device failed, %ls\n", hid_error(handle));
    } else {
        printf("send led command 0x%X to hid\n", led_data);
    }
    return n;
}
