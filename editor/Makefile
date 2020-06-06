OBJS = vi.o

vi: $(OBJS)
	cc -Wall -o $@ $(OBJS) -lncurses

.PHONY: clean
clean:
	rm -f *.o