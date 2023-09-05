#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

#ifndef DATA_DIR
#define DATA_DIR "./data/"
#endif

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

int s; // socket
struct canfd_frame cf;
struct ifreq ifr;
int door_pos = DEFAULT_DOOR_POS;
int door_len = DEFAULT_DOOR_POS + 1;

int lock_enabled = 0;
int unlock_enabled = 0;
char door_state = 0xf;
int door_id = DEFAULT_DOOR_ID;

char data_file[256];
int kk = 0;

SDL_Renderer *renderer = NULL;
SDL_Texture *base_texture = NULL;

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

void send_lock(char door) {
	door_state |= door;
	memset(&cf, 0, sizeof(cf));
	cf.can_id = door_id;
	cf.len = door_len;
	cf.data[door_pos] = door_state;
	send_pkt(CAN_MTU);
}

void send_unlock(char door) {
	door_state &= ~door;
	memset(&cf, 0, sizeof(cf));
	cf.can_id = door_id;
	cf.len = door_len;
	cf.data[door_pos] = door_state;
	send_pkt(CAN_MTU);
}

void kk_check(int k) {
  switch(k) {
    case SDLK_RETURN:
	if(kk == 0xa) printf("KK\n");
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

void redraw_screen() {
  SDL_RenderCopy(renderer, base_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}



int main(int argc, char *argv[]) {
  struct sockaddr_can addr;
  int running = 1;
  int enable_canfd = 1;
  SDL_Event event;

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

  // GUI Setup
  SDL_Window *window = NULL;
  SDL_Surface *screenSurface = NULL;
  
  window = SDL_CreateWindow("Cardoor",0, 600, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
  if(window == NULL) {
        printf("Window could not be shown\n");
  }
  renderer = SDL_CreateRenderer(window, -1, 0);
  SDL_Surface *image = IMG_Load(get_data("door.png"));
  base_texture = SDL_CreateTextureFromSurface(renderer, image);
  SDL_RenderCopy(renderer, base_texture, NULL, NULL);
  SDL_RenderPresent(renderer);

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
    SDL_Delay(5);
  }
  close(s);
  SDL_DestroyTexture(base_texture);
  SDL_FreeSurface(image);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}