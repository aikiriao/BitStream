CC 		 = gcc
CFLAGS = -Wall -Wextra 

OBJS	 = bit_stream.o bit_stream_test.o
TARGET = bit_stream_test

all: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $<
