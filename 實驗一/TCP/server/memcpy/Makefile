all: background_client_downlink background_client_uplink

% : %.c
	gcc -o $@ $< -lnsl -lpthread 
clean:
	rm -f server server.o
