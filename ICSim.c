#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

#ifndef DATA_DIR
#define DATA_DIR "./img/"  // Needs trailing slash
#endif

#define DEFAULT_battery_ID 700
#define DEFAULT_save_ID 950
#define DEFAULT_AC_ID 800
#define DEFAULT_brake_ID 600
#define DEFAULT_park_ID 900
#define DEFAULT_BYTE 0

#define SCREEN_WIDTH 692
#define SCREEN_HEIGHT 329

#define DOOR_LOCKED 0
#define DOOR_UNLOCKED 1
#define OFF 0
#define ON 1
#define DEFAULT_DOOR_ID 411 // 0x19b
#define DEFAULT_DOOR_BYTE 2
#define CAN_DOOR1_LOCK 1
#define CAN_DOOR2_LOCK 2
#define CAN_DOOR3_LOCK 4
#define CAN_DOOR4_LOCK 8
#define DEFAULT_SIGNAL_ID 392 // 0x188
#define DEFAULT_SIGNAL_BYTE 0
#define CAN_LEFT_SIGNAL 1
#define CAN_RIGHT_SIGNAL 2
#define DEFAULT_SPEED_ID 580 // 0x244
#define DEFAULT_SPEED_BYTE 3 // bytes 3,4

const int canfd_on = 1;
int debug = 0;
int ac=0;
int word=0;
int battery=0;
int seatbelt=0;
int Brake=0;
int Park=0;
int randomize = 0;
int seed = 0;
int door_pos = DEFAULT_DOOR_BYTE;
int signal_pos = DEFAULT_SIGNAL_BYTE;
int speed_pos = DEFAULT_SPEED_BYTE;
int AC_pos = DEFAULT_BYTE;
int battery_pos = DEFAULT_BYTE;
int save_pos = DEFAULT_BYTE;
int brake_pos = DEFAULT_BYTE;
int park_pos = DEFAULT_BYTE;
long current_speed = 0;
int door_status[4];
int turn_status[2];
char data_file[256];
SDL_Renderer *renderer = NULL;
SDL_Texture *base_texture = NULL;
SDL_Texture *needle_tex = NULL;
SDL_Texture *sprite_tex = NULL;
SDL_Rect speed_rect;
//文字
SDL_Texture *word100_texture = NULL;
SDL_Texture *word100_texture1 = NULL;
SDL_Texture *word80_texture = NULL;
SDL_Texture *word80_texture1 = NULL;
SDL_Texture *word60_texture = NULL;
SDL_Texture *word60_texture1 = NULL;
SDL_Texture *word40_texture = NULL;
SDL_Texture *word40_texture1 = NULL;
SDL_Texture *word20_texture = NULL;
SDL_Texture *word20_texture1 = NULL;
SDL_Texture *word0_texture = NULL;
SDL_Texture *word0_texture1 = NULL;
//new
int battery_level = 10;
SDL_Texture *AC_tex = NULL;
SDL_Texture *AC_tex1 = NULL;
SDL_Texture *battery_empty_tex = NULL;
SDL_Texture *battery_green_tex = NULL;
SDL_Texture *battery_green_tex1 = NULL;
SDL_Texture *battery_green_tex2 = NULL;
SDL_Texture *battery_green_tex3 = NULL;
SDL_Texture *battery_green_tex4 = NULL;
SDL_Texture *battery_green_tex5 = NULL;
SDL_Texture *brake_tex = NULL;
SDL_Texture *brake_tex1 = NULL;
SDL_Texture *park_tex1 = NULL;
SDL_Texture *save_tex = NULL;
SDL_Texture *save_tex1 = NULL;
SDL_Texture *park_tex = NULL;
//文字
SDL_Rect word_dis = { 550, 0, 80, 80 };
SDL_Rect word1_dis = { 550, 240, 80, 80 };
//new
SDL_Rect battery_empty_dis = { 0, 100, 0, 0};
SDL_Rect battery_green_dis = { 0, 100, 80, 150};
SDL_Rect batterysize = { 550, 90, 80, 150};
SDL_Rect AC_dis = { 0, 0, 140, 90};
SDL_Rect brake_dis = { 0, 250, 70, 70};
SDL_Rect park_dis = { 90, 250, 70, 70};
SDL_Rect save_dis = { 180, 250, 80, 80};


long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

char *get_data(char *fname) {
    if(strlen(DATA_DIR) + strlen(fname) > 255) return NULL;
    strncpy(data_file, DATA_DIR, 255);
    strncat(data_file, fname, 255-strlen(data_file));
    return data_file;
}

void init_car_state() {
    door_status[0] = DOOR_LOCKED;
    door_status[1] = DOOR_LOCKED;
    door_status[2] = DOOR_LOCKED;
    door_status[3] = DOOR_LOCKED;
    turn_status[0] = OFF;
    turn_status[1] = OFF;
}

void blank_ic() {
    SDL_RenderCopy(renderer, base_texture, NULL, NULL);
}

void update_speed() {
    SDL_Rect dial_rect;
    SDL_Point center;
    double angle = 0;
    dial_rect.x = 200;
    dial_rect.y = 80;
    dial_rect.h = 130;
    dial_rect.w = 300;
    SDL_RenderCopy(renderer, base_texture, &dial_rect, &dial_rect);
    /* Because it's a curve we do a smaller rect for the top */
    dial_rect.x = 250;
    dial_rect.y = 30;
    dial_rect.h = 65;
    dial_rect.w = 200;
    SDL_RenderCopy(renderer, base_texture, &dial_rect, &dial_rect);
    // And one more smaller box for the pivot point of the needle
    dial_rect.x = 323;
    dial_rect.y = 171;
    dial_rect.h = 52;
    dial_rect.w = 47;
    SDL_RenderCopy(renderer, base_texture, &dial_rect, &dial_rect);
    center.x = 135;
    center.y = 20;
    angle = map(current_speed, 0, 280, 0, 180);
    if(angle < 0) angle = 0;
    if(angle > 180) angle = 180;
    SDL_RenderCopyEx(renderer, needle_tex, NULL, &speed_rect, angle, &center, SDL_FLIP_NONE);
}

void update_AC() {
    if(ac == 0) {
        SDL_RenderFillRect(renderer, &AC_dis);
        SDL_RenderCopy(renderer, AC_tex, NULL, &AC_dis);
    } else if(ac == 1) {
        SDL_RenderFillRect(renderer, &AC_dis);
        SDL_RenderCopy(renderer, AC_tex1, NULL, &AC_dis);
    }
}

void update_brake() {
    if(Brake == 0) { //close
        SDL_RenderFillRect(renderer, &brake_dis);
    } else if(Brake == 1) { //yellow
        SDL_RenderFillRect(renderer, &brake_dis);
        SDL_RenderCopy(renderer, brake_tex1, NULL, &brake_dis);
    } else if(Brake == 2) { //red
        SDL_RenderFillRect(renderer, &brake_dis);
        SDL_RenderCopy(renderer, brake_tex, NULL, &brake_dis);
    }
}

void update_seatbelt() {
    if(seatbelt == 0)
        SDL_RenderFillRect(renderer, &save_dis);
    if(seatbelt == 1) {
        SDL_RenderFillRect(renderer, &save_dis);
        SDL_RenderCopy(renderer, save_tex, NULL, &save_dis);
    }
}

void update_park() {
    if(Park == 0) {
        SDL_RenderFillRect(renderer, &park_dis);
    } else if(Park == 1) {
        SDL_RenderFillRect(renderer, &park_dis);
        SDL_RenderCopy(renderer, park_tex1, NULL, &park_dis);
    } else if(Park == 2) {
        SDL_RenderFillRect(renderer, &park_dis);
        SDL_RenderCopy(renderer, park_tex, NULL, &park_dis);
    }
}

void update_battery() {
    if(battery==0)
        SDL_RenderCopy(renderer, battery_green_tex, NULL, &batterysize);
    SDL_RenderCopy(renderer, battery_empty_tex, NULL, &batterysize);

    if(battery==1) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word100_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word100_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex,  NULL, &batterysize);
    } else if(battery==2) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word80_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word80_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex1,  NULL,&batterysize);
    } else if(battery==3) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word60_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word60_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex2,  NULL, &batterysize);
    } else if(battery==4) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word40_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word40_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex3,  NULL, &batterysize);
    } else if(battery==5) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word20_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word20_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex4,  NULL, &batterysize);
    } else if(battery==6) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        SDL_RenderCopy(renderer, word0_texture1, NULL, &word1_dis);
        SDL_RenderCopy(renderer, word0_texture, NULL, &word_dis);
        SDL_RenderCopy(renderer, battery_green_tex5,  NULL, &batterysize);
    } else if(battery==100) {
        SDL_RenderFillRect(renderer, &word1_dis);
        SDL_RenderFillRect(renderer, &word_dis);
        battery=50;
        SDL_RenderCopy(renderer, battery_green_tex, NULL, &batterysize);
    }
}

void update_doors() {
    SDL_Rect door_area, update, pos;
    door_area.x = 390;
    door_area.y = 215;
    door_area.w = 110;
    door_area.h = 85;
    SDL_RenderCopy(renderer, base_texture, &door_area, &door_area);
    // No update if all doors are locked
    if(door_status[0] == DOOR_LOCKED && door_status[1] == DOOR_LOCKED &&
            door_status[2] == DOOR_LOCKED && door_status[3] == DOOR_LOCKED) return;
    // Make the base body red if even one door is unlocked
    update.x = 440;
    update.y = 239;
    update.w = 45;
    update.h = 83;
    memcpy(&pos, &update, sizeof(SDL_Rect));
    pos.x -= 22;
    pos.y -= 22;
    SDL_RenderCopy(renderer, sprite_tex, &update, &pos);
    if(door_status[0] == DOOR_UNLOCKED) {
        update.x = 420;
        update.y = 263;
        update.w = 21;
        update.h = 22;
        memcpy(&pos, &update, sizeof(SDL_Rect));
        pos.x -= 22;
        pos.y -= 22;
        SDL_RenderCopy(renderer, sprite_tex, &update, &pos);
    }
    if(door_status[1] == DOOR_UNLOCKED) {
        update.x = 484;
        update.y = 261;
        update.w = 21;
        update.h = 22;
        memcpy(&pos, &update, sizeof(SDL_Rect));
        pos.x -= 22;
        pos.y -= 22;
        SDL_RenderCopy(renderer, sprite_tex, &update, &pos);
    }
    if(door_status[2] == DOOR_UNLOCKED) {
        update.x = 420;
        update.y = 284;
        update.w = 21;
        update.h = 22;
        memcpy(&pos, &update, sizeof(SDL_Rect));
        pos.x -= 22;
        pos.y -= 22;
        SDL_RenderCopy(renderer, sprite_tex, &update, &pos);
    }
    if(door_status[3] == DOOR_UNLOCKED) {
        update.x = 484;
        update.y = 287;
        update.w = 21;
        update.h = 22;
        memcpy(&pos, &update, sizeof(SDL_Rect));
        pos.x -= 22;
        pos.y -= 22;
        SDL_RenderCopy(renderer, sprite_tex, &update, &pos);
    }
}

void update_turn_signals() {
    SDL_Rect left, right, lpos, rpos;
    left.x = 213;
    left.y = 51;
    left.w = 45;
    left.h = 45;
    memcpy(&right, &left, sizeof(SDL_Rect));
    right.x = 482;
    memcpy(&lpos, &left, sizeof(SDL_Rect));
    memcpy(&rpos, &right, sizeof(SDL_Rect));
    lpos.x -= 22;
    lpos.y -= 22;
    rpos.x -= 22;
    rpos.y -= 22;
    if(turn_status[0] == OFF) {
        SDL_RenderCopy(renderer, base_texture, &lpos, &lpos);
    } else {
        SDL_RenderCopy(renderer, sprite_tex, &left, &lpos);
    }
    if(turn_status[1] == OFF) {
        SDL_RenderCopy(renderer, base_texture, &rpos, &rpos);
    } else {
        SDL_RenderCopy(renderer, sprite_tex, &right, &rpos);
    }
}

void redraw_ic() {
    blank_ic();
    update_speed();
    update_doors();
    update_turn_signals();
    update_AC();
    update_battery();
    update_brake();
    update_seatbelt();
    update_park();

    SDL_RenderPresent(renderer);
}

void update_brake_state(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < brake_pos) return;

    Brake = cf->data[brake_pos];

    update_brake();
    SDL_RenderPresent(renderer);
}

void update_park_state(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < park_pos) return;

    Park = cf->data[park_pos];

    update_park();
    SDL_RenderPresent(renderer);
}

void update_AC_state(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < AC_pos) return;

    ac = cf->data[AC_pos];

    update_AC();
    SDL_RenderPresent(renderer);
}

void update_seatbelt_state(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < save_pos) return;

    seatbelt = cf->data[save_pos];

    update_seatbelt();
    SDL_RenderPresent(renderer);
}

void update_battery_state(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < battery_pos) return;
    if(cf->data[battery_pos]& CAN_LEFT_SIGNAL) {
        if(battery == 50)
            battery = 0;
        battery = battery+1;
    } else {
        battery = 100;
    }
    update_battery();
    SDL_RenderPresent(renderer);
}

void update_speed_status(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < speed_pos + 1) return;

    int speed = cf->data[speed_pos] << 8;
    speed += cf->data[speed_pos + 1];
    speed = speed / 100; // speed in kilometers
    current_speed = speed * 0.6213751; // mph

    update_speed();
    SDL_RenderPresent(renderer);
}

void update_signal_status(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < signal_pos) return;
    if(cf->data[signal_pos] & CAN_LEFT_SIGNAL) {
        turn_status[0] = ON;
    } else {
        turn_status[0] = OFF;
    }
    if(cf->data[signal_pos] & CAN_RIGHT_SIGNAL) {
        turn_status[1] = ON;
    } else {
        turn_status[1] = OFF;
    }
    update_turn_signals();
    SDL_RenderPresent(renderer);
}

void update_door_status(struct canfd_frame *cf, int maxdlen) {
    int len = (cf->len > maxdlen) ? maxdlen : cf->len;
    if(len < door_pos) return;
    if(cf->data[door_pos] & CAN_DOOR1_LOCK) {
        door_status[0] = DOOR_LOCKED;
    } else {
        door_status[0] = DOOR_UNLOCKED;
    }
    if(cf->data[door_pos] & CAN_DOOR2_LOCK) {
        door_status[1] = DOOR_LOCKED;
    } else {
        door_status[1] = DOOR_UNLOCKED;
    }
    if(cf->data[door_pos] & CAN_DOOR3_LOCK) {
        door_status[2] = DOOR_LOCKED;
    } else {
        door_status[2] = DOOR_UNLOCKED;
    }
    if(cf->data[door_pos] & CAN_DOOR4_LOCK) {
        door_status[3] = DOOR_LOCKED;
    } else {
        door_status[3] = DOOR_UNLOCKED;
    }
    update_doors();
    SDL_RenderPresent(renderer);
}

int main(int argc, char *argv[]) {
    int can;
    int n;
    struct ifreq ifr;
    struct sockaddr_can addr;
    struct canfd_frame frame;
    struct iovec iov;
    struct msghdr msg;
    struct cmsghdr *cmsg;
    struct timeval tv, timeout_config = { 0, 0 };
    struct stat dirstat;
    fd_set rdfs;
    char ctrlmsg[CMSG_SPACE(sizeof(struct timeval)) + CMSG_SPACE(sizeof(__u32))];
    int running = 1;
    int nbytes, maxdlen;
    int ret;
    int seed = 0;
    int door_id, signal_id, speed_id,AC_id,battery_id,brake_id,park_id,save_id;
    TTF_Init();
    TTF_Font *font = TTF_OpenFont("font.ttc", 30);
    int ww=0, hh=0;
    SDL_Color col= {255, 255, 255};
    SDL_Rect rt= {0, 100, 0, 0};
    const char* cc="ok";
    const char* cc1="100%";
    const char* cc2="20m";
    const char* cc3="80%";
    const char* cc4="40m";
    const char* cc5="60%";
    const char* cc6="1h";
    const char* cc7="40%";
    const char* cc8="1h20m";
    const char* cc9="20%";
    const char* cc10="1h40m";
    const char* cc11="0%";
    TTF_SizeUTF8(font, cc, &ww, &hh);
    TTF_SizeUTF8(font, cc1, &ww, &hh);
    TTF_SizeUTF8(font, cc2, &ww, &hh);
    TTF_SizeUTF8(font, cc3, &ww, &hh);
    TTF_SizeUTF8(font, cc4, &ww, &hh);
    TTF_SizeUTF8(font, cc5, &ww, &hh);
    TTF_SizeUTF8(font, cc6, &ww, &hh);
    TTF_SizeUTF8(font, cc7, &ww, &hh);
    TTF_SizeUTF8(font, cc8, &ww, &hh);
    TTF_SizeUTF8(font, cc9, &ww, &hh);
    TTF_SizeUTF8(font, cc10, &ww, &hh);
    TTF_SizeUTF8(font, cc11, &ww, &hh);
    SDL_Event event;

    // Create a new raw CAN socket
    can = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if(can < 0) exit(1);

    addr.can_family = AF_CAN;
    memset(&ifr.ifr_name, 0, sizeof(ifr.ifr_name));
    strncpy(ifr.ifr_name, argv[optind], strlen(argv[optind]));
    printf("Using CAN interface %s\n", ifr.ifr_name);
    if (ioctl(can, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        exit(1);
    }
    addr.can_ifindex = ifr.ifr_ifindex;
    // CAN FD Mode
    setsockopt(can, SOL_CAN_RAW, CAN_RAW_FD_FRAMES, &canfd_on, sizeof(canfd_on));

    iov.iov_base = &frame;
    iov.iov_len = sizeof(frame);
    msg.msg_name = &addr;
    msg.msg_namelen = sizeof(addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &ctrlmsg;
    msg.msg_controllen = sizeof(ctrlmsg);
    msg.msg_flags = 0;

    if (bind(can, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    init_car_state();

    door_id = DEFAULT_DOOR_ID;
    signal_id = DEFAULT_SIGNAL_ID;
    speed_id = DEFAULT_SPEED_ID;
    battery_id= DEFAULT_battery_ID;
    AC_id= DEFAULT_AC_ID;
    brake_id= DEFAULT_brake_ID;
    park_id= DEFAULT_park_ID;
    save_id= DEFAULT_save_ID;

    SDL_Window *window = NULL;
    SDL_Surface *screenSurface = NULL;
    if(SDL_Init ( SDL_INIT_VIDEO ) < 0 ) {
        printf("SDL Could not initializes\n");
        exit(40);
    }
    window = SDL_CreateWindow("IC Simulator", 1000, 300, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN); // | SDL_WINDOW_RESIZABLE);
    if(window == NULL) {
        printf("Window could not be shown\n");
    }
    renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_Surface *image = IMG_Load(get_data("ic.png"));
    SDL_Surface *needle = IMG_Load(get_data("needle.png"));
    SDL_Surface *sprites = IMG_Load(get_data("spritesheet.png"));
    base_texture = SDL_CreateTextureFromSurface(renderer, image);
    needle_tex = SDL_CreateTextureFromSurface(renderer, needle);
    sprite_tex = SDL_CreateTextureFromSurface(renderer, sprites);
    //文字
    SDL_Surface *mes = TTF_RenderUTF8_Solid(font, cc, col);
    word100_texture = SDL_CreateTextureFromSurface(renderer, mes);
    SDL_Surface *mes1 = TTF_RenderUTF8_Solid(font, cc1, col);
    word100_texture1 = SDL_CreateTextureFromSurface(renderer, mes1);
    SDL_Surface *mes2 = TTF_RenderUTF8_Solid(font, cc2, col);
    word80_texture = SDL_CreateTextureFromSurface(renderer, mes2);
    SDL_Surface *mes3 = TTF_RenderUTF8_Solid(font, cc3, col);
    word80_texture1 = SDL_CreateTextureFromSurface(renderer, mes3);
    SDL_Surface *mes4 = TTF_RenderUTF8_Solid(font, cc4, col);
    word60_texture = SDL_CreateTextureFromSurface(renderer, mes4);
    SDL_Surface *mes5 = TTF_RenderUTF8_Solid(font, cc5, col);
    word60_texture1 = SDL_CreateTextureFromSurface(renderer, mes5);
    SDL_Surface *mes6 = TTF_RenderUTF8_Solid(font, cc6, col);
    word40_texture = SDL_CreateTextureFromSurface(renderer, mes6);
    SDL_Surface *mes7 = TTF_RenderUTF8_Solid(font, cc7, col);
    word40_texture1 = SDL_CreateTextureFromSurface(renderer, mes7);
    SDL_Surface *mes8 = TTF_RenderUTF8_Solid(font, cc8, col);
    word20_texture = SDL_CreateTextureFromSurface(renderer, mes8);
    SDL_Surface *mes9 = TTF_RenderUTF8_Solid(font, cc9, col);
    word20_texture1 = SDL_CreateTextureFromSurface(renderer, mes9);
    SDL_Surface *mes10 = TTF_RenderUTF8_Solid(font, cc10, col);
    word0_texture = SDL_CreateTextureFromSurface(renderer, mes10);
    SDL_Surface *mes11 = TTF_RenderUTF8_Solid(font, cc11, col);
    word0_texture1 = SDL_CreateTextureFromSurface(renderer, mes11);
    //new
    SDL_Surface *AC = IMG_Load(get_data("AC.png"));
    AC_tex = SDL_CreateTextureFromSurface(renderer, AC);
    SDL_Surface *AC1 = IMG_Load(get_data("AC1.png"));
    AC_tex1 = SDL_CreateTextureFromSurface(renderer, AC1);
    SDL_Surface *battery_empty = IMG_Load(get_data("battery_empty.png"));
    battery_empty_tex = SDL_CreateTextureFromSurface(renderer, battery_empty);
    SDL_Surface *battery_green = IMG_Load(get_data("battery_green.png"));
    battery_green_tex = SDL_CreateTextureFromSurface(renderer, battery_green);
    SDL_Surface *battery_green1 = IMG_Load(get_data("battery_green1.png"));
    battery_green_tex1 = SDL_CreateTextureFromSurface(renderer, battery_green1);
    SDL_Surface *battery_green2 = IMG_Load(get_data("battery_green2.png"));
    battery_green_tex2 = SDL_CreateTextureFromSurface(renderer, battery_green2);
    SDL_Surface *battery_green3 = IMG_Load(get_data("battery_green3.png"));
    battery_green_tex3 = SDL_CreateTextureFromSurface(renderer, battery_green3);
    SDL_Surface *battery_green4 = IMG_Load(get_data("battery_green4.png"));
    battery_green_tex4 = SDL_CreateTextureFromSurface(renderer, battery_green4);
    SDL_Surface *battery_green5 = IMG_Load(get_data("battery_green5.png"));
    battery_green_tex5 = SDL_CreateTextureFromSurface(renderer, battery_green5);
    SDL_Surface *park = IMG_Load(get_data("park.png"));
    park_tex = SDL_CreateTextureFromSurface(renderer, park);
    SDL_Surface *park1 = IMG_Load(get_data("park1.png"));
    park_tex1 = SDL_CreateTextureFromSurface(renderer, park1);
    SDL_Surface *save = IMG_Load(get_data("save.png")); //change name
    save_tex = SDL_CreateTextureFromSurface(renderer, save);
    SDL_Surface *save1 = IMG_Load(get_data("save1.png"));
    save_tex1 = SDL_CreateTextureFromSurface(renderer, save1);
    SDL_Surface *brake = IMG_Load(get_data("brake.png"));
    brake_tex = SDL_CreateTextureFromSurface(renderer, brake);
    SDL_Surface *brake1 = IMG_Load(get_data("brake1.png"));
    brake_tex1 = SDL_CreateTextureFromSurface(renderer, brake1);

    speed_rect.x = 212;
    speed_rect.y = 175;
    speed_rect.h = needle->h;
    speed_rect.w = needle->w;

    // Draw the IC
    redraw_ic();
    bool rec = true;
    /* For now we will just operate on one CAN interface */
    while(running) {
        while( SDL_PollEvent(&event) != 0 ) {
            switch(event.type) {
            case SDL_QUIT:
                running = 0;
                break;
            case SDL_WINDOWEVENT:
                switch(event.window.event) {

                case SDL_WINDOWEVENT_RESIZED:
                    redraw_ic();
                    break;
                }

            }
            SDL_Delay(3);
        }
        
        if(rec){
	        nbytes = recvmsg(can, &msg, 0);
	        if (nbytes < 0) {
	            perror("read");
	            return 1;
	        }
	        if ((size_t)nbytes == CAN_MTU)
	            maxdlen = CAN_MAX_DLEN;
	        else if ((size_t)nbytes == CANFD_MTU)
	            maxdlen = CANFD_MAX_DLEN;
	        else {
	            fprintf(stderr, "read: incomplete CAN frame\n");
	            return 1;
	        }
	        for (cmsg = CMSG_FIRSTHDR(&msg);
	                cmsg && (cmsg->cmsg_level == SOL_SOCKET);
	                cmsg = CMSG_NXTHDR(&msg,cmsg)) {
	            if (cmsg->cmsg_type == SO_TIMESTAMP)
	                tv = *(struct timeval *)CMSG_DATA(cmsg);
	            else if (cmsg->cmsg_type == SO_RXQ_OVFL)
	                //dropcnt[i] = *(__u32 *)CMSG_DATA(cmsg);
	                fprintf(stderr, "Dropped packet\n");
	        }

	        if(frame.can_id == door_id) update_door_status(&frame, maxdlen);
	        if(frame.can_id == signal_id) update_signal_status(&frame, maxdlen);
	        if(frame.can_id == speed_id) update_speed_status(&frame, maxdlen);
	        if(frame.can_id == AC_id) update_AC_state(&frame, maxdlen);
	        if(frame.can_id == battery_id) update_battery_state(&frame, maxdlen);
	        if(frame.can_id == brake_id) update_brake_state(&frame, maxdlen);
	        if(frame.can_id == park_id) update_park_state(&frame, maxdlen);
	        if(frame.can_id == save_id) update_seatbelt_state(&frame, maxdlen);

	        if(frame.can_id == 0){
	            rec = false;
	        }
    	}
    }
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_DestroyTexture(battery_empty_tex);
    SDL_DestroyTexture(battery_green_tex);
    SDL_DestroyTexture(battery_green_tex1);
    SDL_DestroyTexture(battery_green_tex2);
    SDL_DestroyTexture(battery_green_tex3);
    SDL_DestroyTexture(battery_green_tex4);
    SDL_DestroyTexture(battery_green_tex5);
    SDL_DestroyTexture(AC_tex);
    SDL_DestroyTexture(AC_tex1);
    SDL_DestroyTexture(save_tex);
    SDL_DestroyTexture(save_tex1);
    SDL_DestroyTexture(brake_tex);
    SDL_DestroyTexture(brake_tex1);
    SDL_DestroyTexture(park_tex);
    SDL_DestroyTexture(park_tex1);
    SDL_DestroyTexture(base_texture);
    SDL_DestroyTexture(needle_tex);
    SDL_DestroyTexture(sprite_tex);
    SDL_DestroyTexture(word100_texture);
    SDL_DestroyTexture(word100_texture1);
    SDL_FreeSurface(mes);
    SDL_FreeSurface(mes1);
    SDL_FreeSurface(battery_empty);
    SDL_FreeSurface(battery_green);
    SDL_FreeSurface(battery_green1);
    SDL_FreeSurface(battery_green2);
    SDL_FreeSurface(battery_green3);
    SDL_FreeSurface(battery_green4);
    SDL_FreeSurface(battery_green5);
    SDL_FreeSurface(brake);
    SDL_FreeSurface(brake1);
    SDL_FreeSurface(park);
    SDL_FreeSurface(park1);
    SDL_FreeSurface(AC);
    SDL_FreeSurface(AC1);
    SDL_FreeSurface(save);
    SDL_FreeSurface(save1);
    SDL_FreeSurface(image);
    SDL_FreeSurface(needle);
    SDL_FreeSurface(sprites);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();

    return 0;
}
