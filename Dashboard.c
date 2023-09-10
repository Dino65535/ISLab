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

#define DEFAULT_SPEED_ID 580
#define DEFAULT_SPEED_POS 3
#define SCREEN_WIDTH 300
#define SCREEN_HEIGHT 250
#define MAX_SPEED 300.0
#define ACCEL_RATE 8.0 // 0-MAX_SPEED in seconds

int s; // socket
struct canfd_frame cf;
struct ifreq ifr;

int speed_pos = DEFAULT_SPEED_POS;
int speed_len = DEFAULT_SPEED_POS + 2;
int throttle = 0;
float current_speed = 0;
int speed_id = DEFAULT_SPEED_ID;
int currentTime;
int lastAccel = 0;

int kk = 0;
char data_file[256];

SDL_Haptic *gHaptic = NULL;
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

void send_speed() {

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
    send_pkt(CAN_MTU);

}

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
                if(gHaptic != NULL) {
                    SDL_HapticRumblePlay( gHaptic, 0.5, 1000);
                    printf("DEBUG HAPTIC\n");
                }
            }
        }
        send_speed();
        lastAccel = currentTime;
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
    
    window = SDL_CreateWindow("Dasboard", 310, 600, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if(window == NULL) {
        printf("Window could not be shown\n");
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface *image = IMG_Load(get_data("dashboard_ECU.png"));
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
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}