all: joycon-demo

run: joycon-demo
	- @echo "You need root privilege to read the hid device"
	- @sudo ./joycon-demo

joycon-demo: main.c x.c
	- @gcc -lhidapi-hidraw -lX11 -lXtst main.c x.c -o joycon-demo

clean:
	- rm -f joycon-demo joycon-twinkle *.o

.PHONY: all

