target = generate_file

all: $(target)

% : %.o

%.o: %.c
	gcc $< -c
