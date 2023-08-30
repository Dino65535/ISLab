CC=gcc
CFLAGS=-I/usr/include/SDL2
LDFLAGS=-lSDL2 -lSDL2_image -lSDL2_ttf
zz=pkg-config --cflags --libs dbus-1

all: icsim  turnsignal park brake Dashboard cardoor seatbelt Battery test AC

icsim: icsim.c
	$(CC) $(CFLAGS) -o icsim icsim.c `$(zz)` $(LDFLAGS)

turnsignal: turnsignal.c
	$(CC) $(CFLAGS) -o turnsignal turnsignal.c `$(zz)` $(LDFLAGS)

park: park.c
	$(CC) $(CFLAGS) -o park park.c `$(zz)` $(LDFLAGS)

brake: brake.c
	$(CC) $(CFLAGS) -o brake brake.c `$(zz)` $(LDFLAGS)

Dashboard: Dashboard.c
	$(CC) $(CFLAGS) -o Dashboard Dashboard.c `$(zz)` $(LDFLAGS)

cardoor: cardoor.c
	$(CC) $(CFLAGS) -o cardoor cardoor.c `$(zz)` $(LDFLAGS)

seatbelt: seatbelt.c
	$(CC) $(CFLAGS) -o seatbelt seatbelt.c `$(zz)` $(LDFLAGS)

Battery: Battery.c cJSON.c
	$(CC) $(CFLAGS) -o Battery Battery.c cJSON.c `$(zz)` $(LDFLAGS)

AC: AC.c 
	$(CC) $(CFLAGS) -o AC AC.c  `$(zz)` $(LDFLAGS)

test: test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS)

clean:
	rm -rf icsim controls turnsignal park brake Dashboard cardoor seatbelt Battery
