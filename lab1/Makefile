CC = gcc
CFLAGS = -Wall -Wextra -std=c99
OBJS = Myshell.o Utility.o

myshell: $(OBJS)
	$(CC) $(CFLAGS) -o myshell $(OBJS)

Myshell.o: Myshell.c Myshell.h
	$(CC) $(CFLAGS) -c Myshell.c

Utility.o: Utility.c Myshell.h
	$(CC) $(CFLAGS) -c Utility.c

clean:
	rm -f myshell $(OBJS)

