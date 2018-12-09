#include <iostream>
#include <SDL.h>

int main(int argc, char * argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    std::cout << "Hello SDL" << std::endl;
    SDL_Quit();
    return 0;
}