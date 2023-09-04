/*
 * Control panel for IC Simulation
 *
 * OpenGarages 
 *
 * craig@theialabs.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdbool.h>

#ifndef DATA_DIR
#define DATA_DIR "./data/"
#endif
#define DEFAULT_CAN_TRAFFIC DATA_DIR "sample-can.log"

#define DEFAULT_DIFFICULTY 1
// 0 = No randomization added to the packets other than location and ID
// 1 = Add NULL padding
// 2 = Randomize unused bytes

#define DEFAULT_SPEED_ID 580
#define DEFAULT_SPEED_POS 3
#define SCREEN_WIDTH 835
#define SCREEN_HEIGHT 608
#define MAX_SPEED 240.0 // Limiter 260.0 is full guage speed
#define ACCEL_RATE 8.0 // 0-MAX_SPEED in seconds


// For now, specific models will be done as constants.  Later
// We should use a config file
#define MODEL_BMW_X1_SPEED_ID 0x1B4
#define MODEL_BMW_X1_SPEED_BYTE 0
#define MODEL_BMW_X1_RPM_ID 0x0AA
#define MODEL_BMW_X1_RPM_BYTE 4
#define MODEL_BMW_X1_HANDBRAKE_ID 0x1B4  // Not implemented yet
#define MODEL_BMW_X1_HANDBRAKE_BYTE 5



// Acelleromoter axis info


//Analog joystick dead zone

int gLastAccelValue = 0; // Non analog R2

int s; // socket
struct canfd_frame cf;
char *traffic_log = DEFAULT_CAN_TRAFFIC;
struct ifreq ifr;

int speed_pos = DEFAULT_SPEED_POS;
int speed_len = DEFAULT_SPEED_POS + 2;
int difficulty = DEFAULT_DIFFICULTY;
char *model = NULL;





int throttle = 0;
float current_speed = 0;
int turning = 0;
int speed_id;
int currentTime;
int lastAccel = 0;
int lastTurnSignal = 0;

int seed = 0;
int debug = 0;

int play_id;
int kk = 0;
char data_file[256];
SDL_GameController *gGameController = NULL;

SDL_Haptic *gHaptic = NULL;
SDL_Renderer *renderer = NULL;
SDL_Texture *base_texture = NULL;

/**
* Listens for signals on the bus
*/


// Adds data dir to file name
// Uses a single pointer so not to have a memory leak
// returns point to data_files or NULL if append is too large
char *get_data(char *fname) {
  if(strlen(DATA_DIR) + strlen(fname) > 255) return NULL;
  strncpy(data_file, DATA_DIR, 255);
  strncat(data_file, fname, 255-strlen(data_file));
  return data_file;
}


void send_pkt(int mtu) {
  if(write(s, &cf, mtu) != mtu) {
	perror("write");
  }
}

// Randomizes bytes in CAN packet if difficulty is hard enough
void randomize_pkt(int start, int stop) {
	if (difficulty < 2) return;
	int i = start;
	for(;i < stop;i++) {
		if(rand() % 3 < 1) cf.data[i] = rand() % 255;
	}
}


void send_speed() {
	if (model) {
		if (!strncmp(model, "bmw", 3)) {
		        int b = ((16 * current_speed)/256) + 208;
			int a = 16 * current_speed - ((b-208) * 256);
		        memset(&cf, 0, sizeof(cf));
		        cf.can_id = speed_id;
		        cf.len = speed_len;
		        cf.data[speed_pos+1] = (char)b & 0xff;
		        cf.data[speed_pos] = (char)a & 0xff;
		        if(current_speed == 0) { // IDLE
		                cf.data[speed_pos] = rand() % 80;
		                cf.data[speed_pos+1] = 208;
		        }
		        if (speed_pos) randomize_pkt(0, speed_pos);
		        if (speed_len != speed_pos + 2) randomize_pkt(speed_pos+2, speed_len);
		        send_pkt(CAN_MTU);
		}
	} else {
		int kph = (current_speed / 0.6213751) * 100;
		memset(&cf, 0, sizeof(cf));
		cf.can_id = speed_id;
		cf.len = speed_len;
		cf.data[speed_pos+1] = (char)kph & 0xff;
		cf.data[speed_pos] = (char)(kph >> 8) & 0xff;
		if(kph == 0) { // IDLE
			cf.data[speed_pos] = 1;
			cf.data[speed_pos+1] = rand() % 255+100;
		}
		if (speed_pos) randomize_pkt(0, speed_pos);
		if (speed_len != speed_pos + 2) randomize_pkt(speed_pos+2, speed_len);
		send_pkt(CAN_MTU);
	}
}


// Checks throttle to see if we should accelerate or decelerate the vehicle
void checkAccel() {
	float rate = MAX_SPEED / (ACCEL_RATE * 100);
	// Updated every 10 ms
	if(currentTime > lastAccel + 10) {
		if(throttle < 0) {
			current_speed -= rate;
			if(current_speed < 1) current_speed = 0;
		} else if(throttle > 0) {
			current_speed += rate;
			if(current_speed > MAX_SPEED) { // Limiter
				current_speed = MAX_SPEED;
				if(gHaptic != NULL) {SDL_HapticRumblePlay( gHaptic, 0.5, 1000); printf("DEBUG HAPTIC\n"); }
			}
		}
		send_speed();
		lastAccel = currentTime;
	}
}








void kkpay() {
  printf("KK\n");
}

void kk_check(int k) {
  switch(k) {
    case SDLK_RETURN:
	if(kk == 0xa) kkpay();
	kk = 0;
	break;
    case SDLK_UP:
	kk = (kk < 2) ? kk+1 : 0;
	break;
    case SDLK_DOWN:
	kk = (kk > 1 && kk < 4) ? kk+1 : 0;
	break;
    case SDLK_LEFT:
	kk = (kk == 4 || kk == 6) ? kk+1 : 0;
	break;
    case SDLK_RIGHT:
	kk = (kk == 5 || kk == 7) ? kk+1 : 0;
	break;
    case SDLK_a:
	kk = kk == 9 ? kk+1 : 0;
	break;
    case SDLK_b:
	kk = kk == 8 ? kk+1 : 0;
	break;
    default:
	kk == 0;
  }
}

// Plays background can traffic
void play_can_traffic() {
	char can2can[50];
	snprintf(can2can, 49, "%s=can0", ifr.ifr_name);
	if(execlp("canplayer", "canplayer", "-I", traffic_log, "-l", "i", can2can, NULL) == -1) printf("WARNING: Could not execute canplayer. No bg data\n");
}

void kill_child() {
	kill(play_id, SIGINT);
}

void redraw_screen() {
  SDL_RenderCopy(renderer, base_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

// Maps the controllers buttons



void usage(char *msg) {
  if(msg) printf("%s\n", msg);
  printf("Usage: controls [options] <can>\n");
  printf("\t-s\tseed value from IC\n");
  printf("\t-l\tdifficulty level. 0-2 (default: %d)\n", DEFAULT_DIFFICULTY);
  printf("\t-t\ttraffic file to use for bg CAN traffic\n");
  printf("\t-m\tModel (Ex: -m bmw)\n");
  printf("\t-X\tDisable background CAN traffic.  Cheating if doing RE but needed if playing on a real CANbus\n");
  printf("\t-d\tdebug mode\n");
  exit(1);
}

int main(int argc, char *argv[]) {
  int opt;
  struct sockaddr_can addr;
  struct canfd_frame frame;
  int running = 1;
  int enable_canfd = 1;
  int play_traffic = 1;
  struct stat st;
  
  SDL_Event event;

  while ((opt = getopt(argc, argv, "Xdl:s:t:m:h?")) != -1) {
    switch(opt) {
	case 'l':
		difficulty = atoi(optarg);
		break;
	case 's':
		seed = atoi(optarg);
		break;
	case 't':
		traffic_log = optarg;
		break;
	case 'd':
		debug = 1;
		break;
	case 'm':
		model = optarg;
		break;
	case 'X':
		play_traffic = 0;
		break;
	case 'h':
	case '?':
	default:
		usage(NULL);
		break;
    }
  }

  if (optind >= argc) usage("You must specify at least one can device");

  if(stat(traffic_log, &st) == -1) {
	char msg[256];
	snprintf(msg, 255, "CAN Traffic file not found: %s\n", traffic_log);
	usage(msg);
  }

  /* open socket */
  if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
       perror("socket");
       return 1;
  }

  addr.can_family = AF_CAN;

  strcpy(ifr.ifr_name, argv[optind]);
  if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
       perror("SIOCGIFINDEX");
       return 1;
  }
  addr.can_ifindex = ifr.ifr_ifindex;

  if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                 &enable_canfd, sizeof(enable_canfd))){
       printf("error when enabling CAN FD support\n");
       return 1;
  }

  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
       perror("bind");
       return 1;
  }

  
  speed_id = DEFAULT_SPEED_ID;
  
  if (seed) {
	srand(seed);
       
        speed_id = (rand() % 2046) + 1;
        speed_pos = rand() % 8;
        printf("Seed: %d\n", seed);
	speed_len = speed_len + 2;
  } else if (model) {
	if (!strncmp(model, "bmw", 3)) {
		speed_id = MODEL_BMW_X1_SPEED_ID;
		speed_pos = MODEL_BMW_X1_SPEED_BYTE;
	} else {
		printf("Invalid model.  Valid entries are: bmw\n");
	}
  }

  if(difficulty > 0) {
	
	if (speed_len < 8) {
		speed_len += rand() % (8 - speed_len);
	} else {
		speed_len = 0;
	}
  }

  if(play_traffic) {
	play_id = fork();
	if((int)play_id == -1) {
		printf("Error: Couldn't fork bg player\n");
		exit(-1);
	} else if (play_id == 0) {
		play_can_traffic();
		// Shouldn't return
		exit(0);
	}
	atexit(kill_child);
  }

  // GUI Setup
  SDL_Window *window = NULL;
  SDL_Surface *screenSurface = NULL;
  
  window = SDL_CreateWindow("CANBus Control Panel", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if(window == NULL) {
        printf("Window could not be shown\n");
  }
  renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Surface *image = IMG_Load(get_data("speed.png"));
  base_texture = SDL_CreateTextureFromSurface(renderer, image);
  SDL_RenderCopy(renderer, base_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
  int button, axis; // Used for checking dynamic joystick mappings

  while(running) {
    while( SDL_PollEvent(&event) != 0 ) {
        switch(event.type) {
            case SDL_QUIT:
                running = 0;
                break;
	    case SDL_WINDOWEVENT:
		switch(event.window.event) {
		case SDL_WINDOWEVENT_ENTER:
		case SDL_WINDOWEVENT_RESIZED:
			redraw_screen();
			break;
		}
	    case SDL_KEYDOWN:
		switch(event.key.keysym.sym) {
		    case SDLK_UP:
			throttle = 1;
			break;
		    
		}
		kk_check(event.key.keysym.sym);
	   	break;
	    case SDL_KEYUP:
		switch(event.key.keysym.sym) {
		    case SDLK_UP:
			throttle = -1;
			break;
		    
		}
		break;
	    
        }
    }
    currentTime = SDL_GetTicks();
    checkAccel();
    
    SDL_Delay(5);
  }

  close(s);
  SDL_DestroyTexture(base_texture);
  SDL_FreeSurface(image);
  SDL_GameControllerClose(gGameController);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

}
