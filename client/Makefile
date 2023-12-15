CC = g++
CFLAGS = -Wall
DEPS = client.h
OBJ = client.o handlers.o utilities.o
%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<

client: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lssl -lcrypto -pthread 
 
clean:
	rm -rf *o client