#define DEBUG macro in Makefile, use -D option
#all: sender receiver
all: receiver
#sender: arp.o sender.o
#	gcc -DDEBUG -o sender arp.c sender.c -lnsl
receiver: receiver.o func.o
	gcc -DDEBUG -o receiver func.c receiver.c -lnsl -lpthread
clean:
	rm -f receiver receiver.o func.o
