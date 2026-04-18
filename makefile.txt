SOestimate: main.o process.o memory.o sensor.o controller.o server.o
	gcc main.o process.o memory.o sensor.o controller.o server.o -o bin/SOestimate

main.o: src/main.c inc/main.h
	gcc -c src/main.c -o obj/main.o

process.o: src/process.c inc/process.h
	gcc -c src/process.c -o obj/process.o

memory.o: src/memory.c inc/memory.h
	gcc -c src/memory.c -o obj/memory.o

sensor.o: src/sensor.c inc/sensor.h
	gcc -c src/sensor.c -o obj/sensor.o

controller.o: src/controller.c inc/controller.h
	gcc -c src/controller.c -o obj/controller.o

server.o: src/server.c inc/server.h
	gcc -c src/server.c -o obj/server.o

clean:
	rm -f -I obj/*.o bin/SOestimate