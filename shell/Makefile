OBJS = node.o main.o parser.o tokenizer.o interactive.o

ash: $(OBJS)
	cc -o ash $(OBJS)

main.o: tokenizer.h parser.h

tokenizer.o: parser.h

tokenizer.h tokenizer.c: tokenizer.l
	flex --header-file=tokenizer.h -o tokenizer.c tokenizer.l

parser.h parser.c: parser.y
	bison -d -o parser.c parser.y

.PHONY: clean
clean:
	rm -f *.o