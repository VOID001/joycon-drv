#include <stdio.h>
#include <hidapi/hidapi.h>
#include <stdint.h>
#include <strings.h>
#include <string.h>
#include <sys/time.h>

#define MAX_STR 255

#define IS_JOYCON(dev) ((dev)->vendor_id == 0x057e && (dev)->product_id == 0x2006)

#define JOYCON_BTN_LEFT     0b00000001
#define JOYCON_BTN_DOWN     0b00000010
#define JOYCON_BTN_UP       0b00000100
#define JOYCON_BTN_RIGHT    0b00001000

int send_led_packet(hid_device *handle, uint8_t led_data);
void send_key(const char *keystr, int pressed);
void parse_keycode(const uint8_t cur_keymap, const uint8_t pre_keymap);

int main() {
    int res;
    unsigned char buf[65];
    wchar_t wstr[MAX_STR];
    hid_device *handle;
    int i;

    // Enumerate and print the HID devices on the system
    struct hid_device_info *devs, *cur_dev;

    devs = hid_enumerate(0x0, 0x0);
    cur_dev = devs;
    while (cur_dev) {
        printf("Device Found\n  type: %04hx %04hx\n  path: %s\n  serial_number: %ls",
               cur_dev->vendor_id, cur_dev->product_id, cur_dev->path, cur_dev->serial_number);
        printf("\n");
        printf("  Manufacturer: %ls\n", cur_dev->manufacturer_string);
        printf("  Product:      %ls\n", cur_dev->product_string);
        printf("\n");
        if(IS_JOYCON(cur_dev)) {
            printf("We found a joycon\n");
            break;
        }
        cur_dev = cur_dev->next;
    }
    printf("%04hx %04hx\n", cur_dev->vendor_id, cur_dev->product_id);
    handle = hid_open(cur_dev->vendor_id, cur_dev->product_id, NULL);

    if(!handle) {
        fprintf(stderr, "cannot open hid device\n");
        return 0;
    }
    printf("open joycon device success\n");

    // Let's try to let joycon rumble
   // We now open the hid device, let's write something
    uint8_t led_pattern[] = {
            0b00001111, 0b00000111, 0b00000011, 
            0b00000001, 0b00000000, 0b00001000,
            0b00001100, 0b00001110, 0b00001111,
            0b00000000, 0b00001001, 0b00000110,
            0b00001010, 0b00000101
    };
    int cnt = 0;
    // while(cnt < 1024) {
    //     send_led_packet(handle, led_pattern[(cnt++) % 15]);
    //     usleep(5 * 1000 * 100);
    // }
    int ret = 0;
    uint8_t recv_buf[12];
    uint8_t pre_keymap;
    bzero(recv_buf, 12);
    while(1) {
        ret = hid_read(handle, buf, 12);
        uint8_t cur_keymap = buf[1];
        // printf("Bytes = %d:\n", ret);
        // print_hex(buf, 12);
        parse_keycode(cur_keymap, pre_keymap);
        pre_keymap = cur_keymap;
    }

    return 0;
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
        send_key("Left", op);
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
