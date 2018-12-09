#include <iostream>
#include <vector>
#include <cstdlib>
#include <limits>
#include <ctime>
#include <SDL.h>
#include <SDL_image.h>

namespace Settings {
    const Uint32 WIDTH = 960;
    const Uint32 HEIGHT = 720;
    const Uint32 HORZ_TILES = 16;
    const Uint32 VERT_TILES = 16;
    const Uint32 DRAW_TILE_WIDTH = 32;
    const Uint32 DRAW_TILE_HEIGHT = 32;
    const Uint32 ATLAS_TILESIZE = 16;
}

struct Pair {
    Uint16 x;
    Uint16 y;
};

// load a tileset image
SDL_Texture * load_tileset(SDL_Renderer *rend, const char *fname) {
    SDL_Texture * tset;
    SDL_Surface *temp;
    temp = IMG_Load(fname);
    if (!temp) {
        std::cerr << "Failed to load tileset: " << fname << std::endl;
        return NULL;
    }
    tset = SDL_CreateTextureFromSurface(rend, temp);
    SDL_FreeSurface(temp);
    return tset;
}

void delete_tileset(SDL_Texture * tset) {
    SDL_DestroyTexture(tset);
}

// create "count" float values between 0, 1.0f inclusive and store them into "outvec"
void generate_randoms(std::vector<float> & outvec, Uint16 count) {
    for (Uint32 i = 0; i < count; ++i) {
        outvec.push_back(static_cast<float>(rand() / static_cast<float>(RAND_MAX)));
        //std::cout << "generated random: " << outvec[i] << std::endl;
    }
}

// -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- -
// fill "outvec" with "count" elements such that the probability that any given
// element in "outvec" with value "i" is histogram[i].
// preconditions: sum(rev_hist) = 1.0f
void non_uniform_decisions(
    std::vector<Uint16> &       outvec,     // the vector which will hold the results
    Uint16                      count,      // number of elements to fill outvec with
    const std::vector<float> &  revhist     // basically a "reverse" histogram
) {  // -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- -
    std::vector<float> randvec;
    generate_randoms(randvec, count);
    for (auto rfv = randvec.begin(); rfv != randvec.end(); ++rfv) {
        for (Uint32 search = 0 ; search < revhist.size(); ++search) {
            if (*rfv < revhist[search]) {
                outvec.push_back(search);
                break;
            }
            else {
                *rfv -= revhist[search];
            }
        }
    }
}

/*
Maps these choices (shown in hexadecimal 0 to D) to atlas coordinates shown as where the
tile would appear visually in the tileset:

[0] [1] [2] [3] [4] <- high grass
[5] [6] [7] <- blue flower
[8] [9] [A] <- yellow flower
[B] [C] [D] <- purple flower

look at the png tileset you will see [9] is the location of a pink flower
*/
// outvec: result vector which holds x/y values indicating the top left coordinate of the tile in the atlas
// choices: the numbers indicating which tiles we desire the coordinates of
void decode_atlas(std::vector<Pair> & outvec, const std::vector<Uint16> & choices) {
    const Uint32 TS = Settings::ATLAS_TILESIZE;
    Pair pair;
    for (auto choices_itr = choices.begin(); choices_itr != choices.end(); ++choices_itr) {
        if (*choices_itr < 5) {
            pair.x = *choices_itr * TS;
            pair.y = 0;
        }
        else {
            pair.x = ((*choices_itr + 3) * TS) % (TS * 3);
            pair.y = (*choices_itr / 3) * TS - TS;
        }
        outvec.push_back(pair);
    }
}

// draw a set of tiles
void render_tiles(SDL_Renderer * const & rend, SDL_Texture * const & tileset, std::vector<SDL_Rect> tiles) {
    const Uint32 tile_width = Settings::DRAW_TILE_WIDTH;
    const Uint32 tile_height = Settings::DRAW_TILE_HEIGHT;

    const Pair translate = { // top left position where the tiles begin.
        Settings::WIDTH / 2 - (Settings::HORZ_TILES * tile_width)/2,
        Settings::HEIGHT / 2 - (Settings::VERT_TILES * tile_height)/2
    };

    Uint32 size = tiles.size();
    SDL_Rect destrect;  // where to draw the tile on the screen (rend)

    destrect.x = translate.x;
    destrect.y = translate.y;
    destrect.w = tile_width;
    destrect.h = tile_height;
    for (Uint32 i = 1; i <= size; ++i) {
        SDL_RenderCopy(rend, tileset, &tiles[i-1], &destrect);
        destrect.x = (i * tile_width) % (tile_width * Settings::HORZ_TILES) + translate.x;
        destrect.y = (i / Settings::VERT_TILES) * tile_height + translate.y;
    }
    return;
}

// the terrain is a vector of 
void randomize_terrain(std::vector<SDL_Rect> & terrain) {
    terrain.clear();
    std::vector<Uint16> temp_choices;
    std::vector<float> reverse_histogram{  // the sum of this vector should be 1.0f within float epsilon
        0.76875,      // chance of tile being default grass (loc 0,0 in tileset)
        0.025,        // chance of tile being high grass (loc 0,1 in tileset)
        0.025,
        0.025,
        0.025,
        0.025,        // chance of tile being high grass (loc 1,0 in tileset)
        0.003125,     // chance of tile being blue flower (loc 1,1 in tileset)
        0.003125,
        0.025,
        0.0125,
        0.0125,
        0.025,
        0.0125,
        0.0125
    };
    non_uniform_decisions(temp_choices, Settings::HORZ_TILES * Settings::VERT_TILES, reverse_histogram);
    std::vector<Pair> pairs;
    decode_atlas(pairs, temp_choices);
    temp_choices.clear();
    for (auto pairs_itr = pairs.begin(); pairs_itr != pairs.end(); ++pairs_itr) {
        SDL_Rect rect;
        rect.w = Settings::ATLAS_TILESIZE;
        rect.h = Settings::ATLAS_TILESIZE;
        rect.x = pairs_itr->x;
        rect.y = pairs_itr->y;
        terrain.push_back(rect);
    }
}

int main(int argc, char * argv[]) {
    srand(static_cast<unsigned int>(time(0)));
    bool running = true;
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *grassland;
    SDL_Event ev;
    std::vector<SDL_Rect> terrain;
    randomize_terrain(terrain);

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);

    window = SDL_CreateWindow(
        "game1",                // window title
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        Settings::WIDTH,
        Settings::HEIGHT,
        SDL_WINDOW_OPENGL
    );

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    grassland = load_tileset(renderer, "tileset.png");

    while (running) {
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) {
                running = false;
                continue;
            }
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                    continue;
                }
                else if (ev.key.keysym.sym == SDLK_r) {
                    randomize_terrain(terrain);
                    continue;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        render_tiles(renderer, grassland, terrain);

        SDL_RenderPresent(renderer);
    }
    delete_tileset(grassland);
    SDL_Quit();
    return 0;
}
