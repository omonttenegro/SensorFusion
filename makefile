TARGET = bin/SOestimate

CC = gcc
CFLAGS = -Wall -Iinc

OBJ = obj/main.o obj/process.o obj/memory.o obj/sensor.o obj/controller.o obj/server.o obj/random_measurement.o

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

obj/main.o: src/main.c inc/main.h
	$(CC) $(CFLAGS) -c src/main.c -o obj/main.o

obj/process.o: src/process.c inc/process.h
	$(CC) $(CFLAGS) -c src/process.c -o obj/process.o

obj/memory.o: src/memory.c inc/memory.h
	$(CC) $(CFLAGS) -c src/memory.c -o obj/memory.o

obj/sensor.o: src/sensor.c inc/sensor.h
	$(CC) $(CFLAGS) -c src/sensor.c -o obj/sensor.o

obj/controller.o: src/controller.c inc/controller.h
	$(CC) $(CFLAGS) -c src/controller.c -o obj/controller.o

obj/server.o: src/server.c inc/server.h
	$(CC) $(CFLAGS) -c src/server.c -o obj/server.o

obj/random_measurement.o: src/random_measurement.c inc/random_measurement.h
	$(CC) $(CFLAGS) -c src/random_measurement.c -o obj/random_measurement.o

clean:
	rm -f obj/*.o $(TARGET)