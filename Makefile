.PHONY: all clean

all: foresinc

foresinc: main.o
	@gcc -Wall -pedantic $^ -o $@

main.o: main.c

%.o: %.c
	@gcc -Wall -Werror -pedantic -c $< -o $@

clean:
	@rm -f foresinc *.o
