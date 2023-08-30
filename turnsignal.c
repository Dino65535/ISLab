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
#include <dbus-1.0/dbus/dbus.h>
#include <stdbool.h>

#ifndef DATA_DIR
#define DATA_DIR "./data/"
#endif
#define DEFAULT_CAN_TRAFFIC DATA_DIR "sample-can.log"

#define DEFAULT_DIFFICULTY 1
// 0 = No randomization added to the packets other than location and ID
// 1 = Add NULL padding
// 2 = Randomize unused bytes

#define DEFAULT_SIGNAL_ID 392
#define DEFAULT_DOOR_POS 2
#define DEFAULT_SIGNAL_POS 0
#define CAN_LEFT_SIGNAL 1
#define CAN_RIGHT_SIGNAL 2
#define ON 1
#define OFF 0
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

int signal_pos = DEFAULT_SIGNAL_POS;
int signal_len = DEFAULT_DOOR_POS + 1;
int difficulty = DEFAULT_DIFFICULTY;
char *model = NULL;


char signal_state = 0;

float current_speed = 0;
int turning = 0;
int signal_id;
int currentTime;
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
void sendsignal(char* sigvalue)
{
	DBusMessage* msg;
	DBusMessageIter args;
	DBusConnection* conn;
	DBusError err;
	int ret;
	dbus_uint32_t serial = 0;
	printf("Sending signal with value %s\n", sigvalue);
	// initialise the error value
	dbus_error_init(&err);
	// connect to the DBUS system bus, and check for errors
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
	fprintf(stderr, "Connection Error (%s)\n", err.message);
	dbus_error_free(&err);
	}
	if (NULL == conn) {
	exit(1);
	}
// register our name on the bus, and check for errors
	ret	=dbus_bus_request_name(conn,"test.signal.source",\
	DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (dbus_error_is_set(&err)) {
	fprintf(stderr, "Name Error (%s)\n", err.message);
	dbus_error_free(&err);
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
	exit(1);
	}
// create a signal & check for errors
	msg = dbus_message_new_signal("/test/signal/Object", // object name of the signal
	"test.signal.Type", // interface name of the signal
	"Test");
	// name of the signal
	if (NULL == msg)
	{
	fprintf(stderr, "Message Null\n");
	exit(1);
	}
// append arguments onto signal
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &sigvalue)) {
	fprintf(stderr, "Out Of Memory!\n");
	exit(1);
	}
	// send the message and flush the connection
	if (!dbus_connection_send(conn, msg, &serial)) {
	fprintf(stderr, "Out Of Memory!\n");
	exit(1);
	}
	dbus_connection_flush(conn);
	printf("Signal Sent\n");
	// free the message
	dbus_message_unref(msg);
}


/**
* Call a method on a remote object
*/
void query(char* param)
{
    DBusMessage* msg;
    DBusMessageIter args;
    DBusConnection* conn;
    DBusError err;
    int ret;
    printf("Calling remote method with %s\n", param);

    // initialiset the errors
    dbus_error_init(&err);

    // connect to the system bus and check for errors
    conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Connection Error (%s)\n", err.message);
        dbus_error_free(&err);
        exit(1);
    }

    if (NULL == conn) {
        fprintf(stderr, "Connection Null\n");
        exit(1);
    }

    // request our name on the bus
    ret = dbus_bus_request_name(conn, "test.method.caller",
                                DBUS_NAME_FLAG_REPLACE_EXISTING, &err);
    if (dbus_error_is_set(&err)) {
        fprintf(stderr, "Name Error (%s)\n", err.message);
        dbus_error_free(&err);
        exit(1);
    }

    if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
        fprintf(stderr, "Not Primary Owner (%d)\n", ret);
        exit(1);
    }

    // create a new method call and check for errors
    msg = dbus_message_new_method_call("seatbelt.method.server", // target for the method call
                                       "/ICSim", // object to call on
                                       "test.method.Type",    // interface to call on
                                       "Method");             // method name

    // method name
    if (NULL == msg) {
        fprintf(stderr, "Message Null\n");
        exit(1);
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param)) {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }

    // send message without waiting for reply
    if (!dbus_connection_send(conn, msg, NULL)) {
        fprintf(stderr, "Out Of Memory!\n");
        exit(1);
    }

    dbus_connection_flush(conn);
    printf("Request Sent\n");

    // free message
    dbus_message_unref(msg);
}

/**
* Server that exposes a method call and waits for it to be called
*/
void listen1()
{
	DBusMessage* msg;
	DBusMessage* reply;
	DBusMessageIter args;
	DBusConnection* conn;
	DBusError err;
	int ret;
	char* param;
	printf("Listening for method calls\n");
	// initialise the error
	dbus_error_init(&err);
	// connect to the bus and check for errors
	conn = dbus_bus_get(DBUS_BUS_SESSION, &err);
	if (dbus_error_is_set(&err)) {
	fprintf(stderr, "Connection Error (%s)\n", err.message);
	dbus_error_free(&err);
	}
	if (NULL == conn) {
	fprintf(stderr, "Connection Null\n");
	exit(1);
	}
	// request our name on the bus and check for errors
	ret = dbus_bus_request_name(conn, "test.method.server",
	DBUS_NAME_FLAG_REPLACE_EXISTING , &err);
	if (dbus_error_is_set(&err)) {
	fprintf(stderr, "Name Error (%s)\n", err.message);
	dbus_error_free(&err);
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
	fprintf(stderr, "Not Primary Owner (%d)\n", ret);
	exit(1);
	}
	// loop, testing for new messages
	while (true) {
	// non blocking read of the next available message
	dbus_connection_read_write(conn, 0);
	msg = dbus_connection_pop_message(conn);
	// loop again if we haven't got a message
	if (NULL == msg) {
	sleep(1);
	continue;
	}
	// check this is a method call for the right interface & method
	if (dbus_message_is_method_call(msg, "test.method.Type", "Method")){
		break;
	}
	
	// free the message
	dbus_message_unref(msg);
}
}
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


void send_turn_signal() {
	memset(&cf, 0, sizeof(cf));
	cf.can_id = signal_id;
	cf.len = signal_len;
	cf.data[signal_pos] = signal_state;
	if(signal_pos) randomize_pkt(0, signal_pos);
	if(signal_len != signal_pos + 1) randomize_pkt(signal_pos+1, signal_len);
	send_pkt(CAN_MTU);
}




// Checks if turning and activates the turn signal
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
  char* param = "no param";
	if (3 >= argc && NULL != argv[2]) param = argv[2];
	if (0 == strcmp(argv[1], "send"))
	sendsignal(param);
	
	else if (0 == strcmp(argv[1], "listen"))
	listen1();
	else if (0 == strcmp(argv[1], "query"))
	query(param);
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

  signal_id = DEFAULT_SIGNAL_ID;
  if (seed) {
	srand(seed);
       
        signal_pos = rand() % 9;
        
        printf("Seed: %d\n", seed);
	
	signal_len = signal_pos + 1;
	
  } 

  if(difficulty > 0) {
	
	if (signal_len < 8) {
		signal_len += rand() % (8 - signal_len);
	} else {
		signal_len = 0;
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
  SDL_Surface *image = IMG_Load(get_data("Turn-Signals.png"));
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
  SDL_GameControllerClose(gGameController);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();

}
