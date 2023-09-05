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

#define DEFAULT_DOOR_ID 411
#define DEFAULT_DOOR_POS 2
#define CAN_DOOR1_LOCK 1
#define CAN_DOOR2_LOCK 2 
#define CAN_DOOR3_LOCK 4
#define CAN_DOOR4_LOCK 8
#define ON 1
#define OFF 0
#define DOOR_LOCKED 0
#define DOOR_UNLOCKED 1
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 250
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
int door_pos = DEFAULT_DOOR_POS;
int door_len = DEFAULT_DOOR_POS + 1;
int difficulty = DEFAULT_DIFFICULTY;
char *model = NULL;

int lock_enabled = 0;
int unlock_enabled = 0;
char door_state = 0xf;

float current_speed = 0;
int turning = 0;
int door_id;
int currentTime;


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

void send_lock(char door) {
	door_state |= door;
	memset(&cf, 0, sizeof(cf));
	cf.can_id = door_id;
	cf.len = door_len;
	cf.data[door_pos] = door_state;
	if (door_pos) randomize_pkt(0, door_pos);
	if (door_len != door_pos + 1) randomize_pkt(door_pos + 1, door_len);
	send_pkt(CAN_MTU);
}

void send_unlock(char door) {
	door_state &= ~door;
	memset(&cf, 0, sizeof(cf));
	cf.can_id = door_id;
	cf.len = door_len;
	cf.data[door_pos] = door_state;
	if (door_pos) randomize_pkt(0, door_pos);
	if (door_len != door_pos + 1) randomize_pkt(door_pos + 1, door_len);
	send_pkt(CAN_MTU);
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

  door_id = DEFAULT_DOOR_ID;
  
  if (seed) {
	srand(seed);
        door_id = (rand() % 2046) + 1;
        door_pos = rand() % 9;
        printf("Seed: %d\n", seed);
	door_len = door_pos + 1;
	
  } 

  if(difficulty > 0) {
	if (door_len < 8) {
		door_len += rand() % (8 - door_len);
	} else {
		door_len = 0;
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
  
  window = SDL_CreateWindow("cardoor",0, 600, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if(window == NULL) {
        printf("Window could not be shown\n");
  }
  renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Surface *image = IMG_Load(get_data("door.png"));
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
		    
		    case SDLK_LSHIFT:
			lock_enabled = 1;
			if(unlock_enabled) send_lock(CAN_DOOR1_LOCK | CAN_DOOR2_LOCK | CAN_DOOR3_LOCK | CAN_DOOR4_LOCK);
			break;
		    case SDLK_RSHIFT:
			unlock_enabled = 1;
			if(lock_enabled) send_unlock(CAN_DOOR1_LOCK | CAN_DOOR2_LOCK | CAN_DOOR3_LOCK | CAN_DOOR4_LOCK);
			break;
			  case SDLK_a:
			if(lock_enabled) {
				send_lock(CAN_DOOR1_LOCK);
			} else if(unlock_enabled) {
				send_unlock(CAN_DOOR1_LOCK);
			}
			break;
		    case SDLK_b:
			if(lock_enabled) {
				send_lock(CAN_DOOR2_LOCK);
			} else if(unlock_enabled) {
				send_unlock(CAN_DOOR2_LOCK);
			}
			break;
		    case SDLK_x:
			if(lock_enabled) {
				send_lock(CAN_DOOR3_LOCK);
			} else if(unlock_enabled) {
				send_unlock(CAN_DOOR3_LOCK);
			}
			break;
		    case SDLK_y:
			if(lock_enabled) {
				send_lock(CAN_DOOR4_LOCK);
			} else if(unlock_enabled) {
				send_unlock(CAN_DOOR4_LOCK);
			}
		    
		}
		kk_check(event.key.keysym.sym);
	   	break;
	    case SDL_KEYUP:
		switch(event.key.keysym.sym) {
		   
		    case SDLK_LSHIFT:
			lock_enabled = 0;
			break;
		    case SDLK_RSHIFT:
			unlock_enabled = 0;
			break;
		}
		break;
	    
        }
    }
    currentTime = SDL_GetTicks();
    
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
