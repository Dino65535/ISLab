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
#define DATA_DIR "./img/"
#endif

#define DEFAULT_AC_ID 800
#define SCREEN_WIDTH 200
#define SCREEN_HEIGHT 150

int s; // socket
struct canfd_frame cf;
struct ifreq ifr;

int AC_pos = 0;
int AC_len = 1;
char AC_state = 1;
int AC_id = DEFAULT_AC_ID;

char data_file[256];

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

void send_AC_state(bool b) {
    if(b)
        AC_state = 1; //open
    else
        AC_state = 0; //close

    memset(&cf, 0, sizeof(cf));
    cf.can_id = AC_id;
    cf.len = AC_len;
    cf.data[AC_pos] = AC_state;
    send_pkt(CAN_MTU);
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
                   &enable_canfd, sizeof(enable_canfd))) {
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
    
    window = SDL_CreateWindow("AC", 470, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(window == NULL) {
        printf("Window could not be shown\n");
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface *image = IMG_Load(get_data("AC_ECU.png"));
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
                case SDLK_u:
                    send_AC_state(1);
                    break;
                case SDLK_i:
                    send_AC_state(0);
                    break;
                }
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
//123
