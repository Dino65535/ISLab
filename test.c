#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
int a=0,b=0,c=0;

typedef struct {
    char str[100];
    SDL_Surface *text;
} StringInput;

void handle_input(StringInput *name, SDL_Event *event, TTF_Font *font, SDL_Color textColor);
void show_centered(StringInput *name, SDL_Renderer *renderer,StringInput *title);
void apply_surface(int x, int y, SDL_Surface *source, SDL_Renderer *destination);

int main(int argc, char* args[]) {
    bool quit = false;
    bool nameEntered = false;
    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        return 1;
    }
    
    SDL_Window* window = SDL_CreateWindow("String Input Example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (TTF_Init() == -1) {
        return 1;
    }
    
    TTF_Font *font = TTF_OpenFont("font.ttc", 28); // Replace with your font file path
    if (!font) {
        return 1;
    }
    
    SDL_Color textColor = {255, 255, 255};
    SDL_Color textColor1 = {204, 0, 0};
    StringInput name,title;
    name.str[0] = '\0';
    name.text = NULL;
    strcpy(title.str, "Please input url:");
    title.text = NULL;
    title.text = TTF_RenderText_Solid(font, title.str, textColor1);
    SDL_Rect rect1 = {200,100,400,50};
    SDL_Texture *texture1 = SDL_CreateTextureFromSurface(renderer, title.text);
    SDL_RenderCopy(renderer, texture1, NULL, &rect1);
    SDL_RenderPresent(renderer);
    while (!quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }

            if (!nameEntered) {
                
                handle_input(&name, &event, font, textColor);
                
                if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RETURN) {
                    quit = true;
                    nameEntered = true;

                    char vuln[30];
                    strcpy(vuln, name.str); 
                    vuln[strcspn(vuln,"\n")]='\0';
                    
                    if(strlen(vuln)>30){
                        printf("overflow...\n");
                        system("pkill icsim");
                    }
                    else{
                         char command[48];
                         snprintf(command, sizeof(command),"firefox %s",vuln);
                         system(command);
                    }
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture1, NULL, &rect1);
        if (!nameEntered) {
            show_centered(&name, renderer,&title);
        }

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}

void handle_input(StringInput *name, SDL_Event *event, TTF_Font *font, SDL_Color textColor) {
   
    if (event->type == SDL_KEYDOWN) {
        char temp[100];
        strcpy(temp, name->str);

        if (strlen(name->str) <= 100) {
            SDL_Keycode key = event->key.keysym.sym;
            if(key==SDLK_CAPSLOCK)
            {
                
                if(a==0)
                {
                    a=1;
                }
                else if(a==1)
                {
                    a=0;
                }
                
            }
             if(key==SDLK_LSHIFT)
            {
                b=1;
                
            }
            if (b==1 &&(key == SDLK_SLASH)) {

                
                name->str[strlen(name->str)] = '?';
                name->str[strlen(name->str) + 1] = '\0';
            }
            else if (b==1 &&(key == SDLK_SEMICOLON)) {

                
                name->str[strlen(name->str)] = ':';
                name->str[strlen(name->str) + 1] = '\0';
            }
            else if (b==1 &&(key == SDLK_7)) {

                
                name->str[strlen(name->str)] = '&';
                name->str[strlen(name->str) + 1] = '\0';
            }
            
            else if ((key!=SDLK_CAPSLOCK)&&(key == SDLK_SPACE) ||
                ((key >= SDLK_0) && (key <= SDLK_9)) ||
                ((key >= SDLK_a) && (key <= SDLK_z))||(key == SDLK_PERIOD)||(key == SDLK_SLASH)
                ||(key == SDLK_EQUALS)||(key == SDLK_SEMICOLON)||(key == SDLK_MINUS)) {

                
                if (a==1) {
                    key -=32 ;
                }
               
                name->str[strlen(name->str)] = (char)key;
                name->str[strlen(name->str) + 1] = '\0';
            }
        }

        if ((event->key.keysym.sym == SDLK_BACKSPACE) && (strlen(name->str) != 0)) {
            name->str[strlen(name->str) - 1] = '\0';
        }
       
        

        if (strcmp(name->str, temp) != 0) {
            if (name->text != NULL) {
                SDL_FreeSurface(name->text);
            }
            
            name->text = TTF_RenderText_Solid(font, name->str, textColor);
        }
    }
    else if(event->type == SDL_KEYUP)
    {
        SDL_Keycode key = event->key.keysym.sym;
        if(key==SDLK_LSHIFT)
            {
                b=0;
                
            }
    }
}



void show_centered(StringInput *name, SDL_Renderer *renderer,StringInput *title) {
    
    if (name->text != NULL) {
        SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, name->text);
        SDL_Rect rect = {(SCREEN_WIDTH - name->text->w) / 2, (SCREEN_HEIGHT - name->text->h) / 2, name->text->w, name->text->h};
        SDL_RenderCopy(renderer, texture, NULL, &rect);
       
        SDL_DestroyTexture(texture);
        
    }
}