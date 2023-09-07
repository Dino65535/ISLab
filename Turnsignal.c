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
#define DATA_DIR "./img/"
#endif

#define DEFAULT_SIGNAL_ID 392
#define DEFAULT_SIGNAL_POS 0
#define CAN_LEFT_SIGNAL 1
#define CAN_RIGHT_SIGNAL 2
#define ON 1
#define OFF 0
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 250

int s; // socket
struct canfd_frame cf;
struct ifreq ifr;

int signal_pos = DEFAULT_SIGNAL_POS;
int signal_len = 3;
char signal_state = 0;

int turning = 0;
int signal_id = DEFAULT_SIGNAL_ID;
int currentTime;
int lastTurnSignal = 0;

int kk = 0;
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

void send_turn_signal() {
    memset(&cf, 0, sizeof(cf));
    cf.can_id = signal_id;
    cf.len = signal_len;
    cf.data[signal_pos] = signal_state;
    send_pkt(CAN_MTU);
}

void checkTurn() {
    if(currentTime > lastTurnSignal + 500) {
        if(turning < 0) {
            signal_state ^= CAN_LEFT_SIGNAL;
        } else if(turning > 0) {
            signal_state ^= CAN_RIGHT_SIGNAL;
        } else {
            signal_state = 0;
        }
        send_turn_signal();
        lastTurnSignal = currentTime;
    }
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

    window = SDL_CreateWindow("Turnsignal", 0, 320, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(window == NULL) {
        printf("Window could not be shown\n");
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface *image = IMG_Load(get_data("turnSignal_ECU.png"));
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

                case SDLK_LEFT:
                    turning = -1;
                    break;
                case SDLK_RIGHT:
                    turning = 1;
                    break;

                }
                kk_check(event.key.keysym.sym);
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym) {

                case SDLK_LEFT:
                case SDLK_RIGHT:
                    turning = 0;
                    break;

                }
                break;

            }
        }
        currentTime = SDL_GetTicks();

        checkTurn();
        SDL_Delay(5);
    }

    close(s);
    SDL_DestroyTexture(base_texture);
    SDL_FreeSurface(image);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
