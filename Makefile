CC=gcc
CFLAGS=-I/usr/include/SDL2
LDFLAGS=-lSDL2 -lSDL2_image -lSDL2_ttf
zz=pkg-config --cflags --libs dbus-1

all: icsim Turnsignal Park Brake Dashboard Cardoor Seatbelt Battery test AC

icsim: icsim.c
	$(CC) $(CFLAGS) -o icsim icsim.c `$(zz)` $(LDFLAGS)

Turnsignal: Turnsignal.c
	$(CC) $(CFLAGS) -o Turnsignal Turnsignal.c `$(zz)` $(LDFLAGS)

Park: Park.c
	$(CC) $(CFLAGS) -o Park Park.c `$(zz)` $(LDFLAGS)

Brake: Brake.c
	$(CC) $(CFLAGS) -o Brake Brake.c `$(zz)` $(LDFLAGS)

Dashboard: Dashboard.c
	$(CC) $(CFLAGS) -o Dashboard Dashboard.c `$(zz)` $(LDFLAGS)

Cardoor: Cardoor.c
	$(CC) $(CFLAGS) -o Cardoor Cardoor.c `$(zz)` $(LDFLAGS)

Seatbelt: Seatbelt.c
	$(CC) $(CFLAGS) -o Seatbelt Seatbelt.c `$(zz)` $(LDFLAGS)

Battery: Battery.c cJSON.c
	$(CC) $(CFLAGS) -o Battery Battery.c cJSON.c `$(zz)` $(LDFLAGS)

AC: AC.c 
	$(CC) $(CFLAGS) -o AC AC.c  `$(zz)` $(LDFLAGS)

test: test.c
	$(CC) $(CFLAGS) -o test test.c $(LDFLAGS)

clean:
	rm -rf icsim Turnsignal Park Brake Dashboard Cardoor Seatbelt Battery
