#ifndef _JOYCON_H
#define _JOYCON_H

#include <hidapi/hidapi.h>

#define IS_JOYCON(dev) ((dev)->vendor_id == 0x057e && ((dev)->product_id == 0x2006) || ((dev)->product_id == 0x2007))
#define JOYCON_VENDOR_ID 0x057e
#define PROD_ID_JOYCON_L 0x2006
#define PROD_ID_JOYCON_R 0x2007

#define JOYCON_BTN_LEFT     0b00000001
#define JOYCON_BTN_DOWN     0b00000010
#define JOYCON_BTN_UP       0b00000100
#define JOYCON_BTN_RIGHT    0b00001000

#define TYP_JOYCON 1
#define TYP_PRO 2

struct controller* new_controller();
int init_controller(struct controller *ctrl);
void destroy_controller();
void *joycon_read(void *arg);

typedef struct controller {
    // Fuck, hid API doesn't provide the fd for those device
    // we need to use open to get them
    // hid_device *joycon_L;
    // hid_device *joycon_R;
    // hid_device *pro_control;
    int joycon_L;
    int joycon_R;
    int pro_control;
    int type;
} Controller;

#endif
