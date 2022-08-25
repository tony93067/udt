all: server background_server

% : %.c
	gcc -o $@ $< -lnsl -lpthread 
clean:
	rm -f server server.o
