OBJS = scanner.o main.o parser.c

ash: $(OBJS)
	cc -Wall -o $@ $(OBJS)

.PHONY: clean
clean:
	rm -f *.o