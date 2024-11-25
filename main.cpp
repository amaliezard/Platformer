/**
* Author: Amalie
* Assignment: Rise of the AI
* Date due: 2024-15-9, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define GL_SILENCE_DEPRECATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1
#define FIXED_TIMESTEP 0.0166666f
#define ENEMY_COUNT 3
#define LEVEL1_LEFT_EDGE 5.0f

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL_mixer.h>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"
#include "Map.h"
#include "Utility.h"
#include "Scene.h"
#include "LevelA.h"
#include "stb_image.h"
#include "LevelB.h"
#include "LevelC.h"
#include "Menu.h"

constexpr int WINDOW_WIDTH  = 640 * 2,
          WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 0.1922f,
            BG_BLUE    = 0.549f,
            BG_GREEN   = 0.9059f,
            BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
          VIEWPORT_Y = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;
constexpr char GAME_WINDOW_NAME[] = "Hello, Maps!";

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char SPRITESHEET_FILEPATH[] = "assets/images/char.png",
           MAP_TILESET_FILEPATH[] = "assets/images/tileset.png",
           BGM_FILEPATH[]         = "assets/audio/CloudDancer.mp3",
           JUMP_SFX_FILEPATH[]    = "assets/audio/jump.wav",
           ENEMY_SPRITESHEET_FILEPATH[] = "assets/images/BatPig.png";


// Global variables
SDL_Window* g_display_window = nullptr;
ShaderProgram g_shader_program;
float g_previous_ticks = 0.0f;
float g_accumulator = 0.0f;

Scene  *g_curr_scene = nullptr;
LevelA *g_levelA = nullptr;
LevelB *g_levelB = nullptr;
LevelC *g_levelC = nullptr;
Menu *g_menu = nullptr;

Scene *g_levels[4];

int g_lives =3;

glm::mat4 g_view_matrix, g_projection_matrix;
constexpr float MILLISECONDS_IN_SECOND = 1000.0;
bool g_is_colliding_bottom = false;

enum AppStatus { RUNNING, TERMINATED };
AppStatus g_app_status = RUNNING;

void switch_to_scene(Scene *scene);
void initialise();
void process_input();
void update();
void render();
void shutdown();

void switch_to_scene(Scene *scene)
{
    g_curr_scene = scene;
    g_curr_scene->initialise();
    
}


void initialise() {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    g_display_window = SDL_CreateWindow(GAME_WINDOW_NAME,
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // ————— VIDEO SETUP ————— //
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    g_menu = new Menu();
    g_levelA = new LevelA();
    g_levelB = new LevelB();
    g_levelC = new LevelC();
    g_levels[0] = g_menu;
    g_levels[1] = g_levelA;
    g_levels[2] = g_levelB;
    g_levels[3] = g_levelC;
    
    switch_to_scene(g_levels[0]);
    
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    g_curr_scene->get_state().jump_sfx = Mix_LoadWAV(JUMP_SFX_FILEPATH);
    if (!g_curr_scene->get_state().jump_sfx) {
        std::cerr << "Failed to load jump sound effect: " << Mix_GetError() << std::endl;
    }
}


void process_input()
{
    if (g_curr_scene->get_state().player != nullptr) {
        g_curr_scene->get_state().player->set_movement(glm::vec3(0.0f));
    }
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            case SDL_QUIT:
            case SDL_WINDOWEVENT_CLOSE:
                g_app_status = TERMINATED;
                break;
                
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_q:
                        g_app_status = TERMINATED;
                        break;
                        
                    case SDLK_SPACE:
                        if (g_curr_scene->get_state().player->get_collided_bottom())
                        {
                            g_curr_scene->get_state().player->jump();
                            Mix_PlayChannel(-1, g_curr_scene->get_state().jump_sfx, 0);
                        }
                        break;
                        
                    case SDLK_k:
                        for (int i = 0; i < ENEMY_COUNT; i++) {
                            Entity& enemy = g_curr_scene->get_state().enemies[i];
                            glm::vec3 player_position = g_curr_scene->get_state().player->get_position();
                            glm::vec3 enemy_position = enemy.get_position();
                            if (enemy.is_active() && glm::distance(player_position, enemy_position) < 1.0f) {
                                enemy.deactivate();
                            }
                        }
                        break;

                        
                    case SDLK_RETURN:
                        
                        if (g_curr_scene == g_menu) {
                            switch_to_scene(g_levels[1]);
                        }
                        break;
                        
                    default:
                        break;
                }
                
            default:
                break;
        }
    }
    
    const Uint8 *key_state = SDL_GetKeyboardState(NULL);
    
    if (key_state[SDL_SCANCODE_LEFT])      g_curr_scene->get_state().player->move_left();
    else if (key_state[SDL_SCANCODE_RIGHT])g_curr_scene->get_state().player->move_right();
    if (g_curr_scene->get_state().player != nullptr) {
        if (glm::length(g_curr_scene->get_state().player->get_movement()) > 1.0f)
            g_curr_scene->get_state().player->normalise_movement();
    }}



void update()
{
    if (g_curr_scene->get_state().player != nullptr) {
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    delta_time += g_accumulator;
    
    if (delta_time < FIXED_TIMESTEP)
    {
        g_accumulator = delta_time;
        return;
    }
    
    while (delta_time >= FIXED_TIMESTEP) {
        g_curr_scene->update(FIXED_TIMESTEP);
//        if (g_curr_scene->get_state().player != nullptr) {
//            g_is_colliding_bottom = g_curr_scene->get_state().player->get_collided_bottom();
//        }
        
        delta_time -= FIXED_TIMESTEP;
    }
    
    g_accumulator = delta_time;
    
    g_view_matrix = glm::mat4(1.0f);
    
        g_view_matrix = glm::translate(g_view_matrix,
                                       glm::vec3(-g_curr_scene->get_state().player->get_position().x,
                                                 3.75,
                                                 0));
        g_view_matrix = glm::mat4(1.0f);
        g_view_matrix = glm::translate(g_view_matrix, glm::vec3(-g_curr_scene->get_state().player->get_position().x, 1.0f, 0.0f));
        
    }
        
        
}


void render()
{
    g_shader_program.set_view_matrix(g_view_matrix);
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— RENDERING THE SCENE (i.e. map, character, enemies...) ————— //
    g_curr_scene->render(&g_shader_program);
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown()
{
    SDL_Quit();
    delete g_menu;
    delete g_levelA;
    delete g_levelB;
    delete g_levelC;
}



int main(int argc, char* argv[])
{
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        
        if (g_curr_scene->get_state().next_scene_id >= 0)
            switch_to_scene(g_levels[g_curr_scene->get_state().next_scene_id]);
        
        render();
    }
    
    shutdown();
    return 0;
}
