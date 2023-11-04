parameter = -I/usr/include/SDL2
lib = -lSDL2 -lSDL2_image -lSDL2_ttf
protect = -fno-stack-protector -z execstack -no-pie -g
dir = source/
all: AC Battery Brake Cardoor Dashboard ICSim ICSim_victim Park Seatbelt Turnsignal

AC: $(dir)AC.c 
	@gcc $(dir)AC.c -o AC $(lib) $(parameter)

Battery: $(dir)Battery.c $(dir)cJSON.c
	@gcc $(dir)Battery.c $(dir)cJSON.c -o Battery $(lib) $(parameter)

Brake: $(dir)Brake.c
	@gcc $(dir)Brake.c -o Brake $(lib) $(parameter)

Cardoor: $(dir)Cardoor.c
	@gcc $(dir)Cardoor.c -o Cardoor $(lib) $(parameter)

Dashboard: $(dir)Dashboard.c
	@gcc $(dir)Dashboard.c -o Dashboard $(lib) $(parameter)

ICSim: $(dir)ICSim.c
	@gcc $(dir)ICSim.c -o ICSim $(lib) $(parameter)

ICSim_victim: $(dir)ICSim_victim.c
	@gcc $(dir)ICSim_victim.c -o ICSim_victim $(lib) $(parameter)

Park: $(dir)Park.c
	@gcc $(dir)Park.c -o Park $(lib) $(parameter)

Seatbelt: $(dir)Seatbelt.c
	@gcc $(dir)Seatbelt.c -o Seatbelt $(lib) $(parameter)

Turnsignal: $(dir)Turnsignal.c
	@gcc $(dir)Turnsignal.c -o Turnsignal $(lib) $(parameter)
	
clean:
	@rm -rf AC Battery Brake Cardoor Dashboard ICSim ICSim_victim Park Seatbelt Turnsignal
	@echo "clean down"