CC=gcc
CFLAGS=-I/usr/include/SDL2
LDFLAGS=-lSDL2 -lSDL2_image -lSDL2_ttf
OV=-fno-stack-protector -z execstack -no-pie

all: AC Battery Brake Cardoor Dashboard ICSim ICSim_victim Park Seatbelt Turnsignal CVE-2022-33218

AC: AC.c 
	$(CC) $(CFLAGS) -o AC AC.c   $(LDFLAGS)

Battery: Battery.c cJSON.c
	$(CC) $(CFLAGS) -o Battery Battery.c cJSON.c  $(LDFLAGS)

Brake: Brake.c
	$(CC) $(CFLAGS) -o Brake Brake.c $(LDFLAGS)

Cardoor: Cardoor.c
	$(CC) $(CFLAGS) -o Cardoor Cardoor.c  $(LDFLAGS)

Dashboard: Dashboard.c
	$(CC) $(CFLAGS) -o Dashboard Dashboard.c  $(LDFLAGS)

ICSim: ICSim.c
	$(CC) $(CFLAGS) -o ICSim ICSim.c  $(LDFLAGS)

ICSim_victim: ICSim_victim.c
	$(CC) $(CFLAGS) -o ICSim_victim ICSim_victim.c  $(LDFLAGS)

Park: Park.c
	$(CC) $(CFLAGS) -o Park Park.c  $(LDFLAGS)

Seatbelt: Seatbelt.c
	$(CC) $(CFLAGS) -o Seatbelt Seatbelt.c $(LDFLAGS)

Turnsignal: Turnsignal.c
	$(CC) $(CFLAGS) -o Turnsignal Turnsignal.c  $(LDFLAGS)

CVE-2022-33218: CVE-2022-33218.c
	$(CC) $(CFLAGS) -o CVE-2022-33218 CVE-2022-33218.c $(LDFLAGS) $(OV)

clean:
	rm -rf AC Battery Brake Cardoor Dashboard ICSim ICSim_victim Park Seatbelt Turnsignal CVE-2022-33218
