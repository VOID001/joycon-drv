#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/extensions/XTest.h>

void send_key(const char *keystr, int pressed) {
	Display *dis;
	Bool is_press = False;
    if(pressed) {
        is_press = True;
    }
	dis = XOpenDisplay(NULL);
	KeyCode modcode = 0; //init value
	modcode = XKeysymToKeycode(dis, XStringToKeysym(keystr));
	XTestFakeKeyEvent(dis, modcode, is_press, 0);
	XFlush(dis);
    XCloseDisplay(dis);
	return;
}

