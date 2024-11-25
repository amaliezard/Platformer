#include "LevelB.h"
#include "Utility.h"
#include <vector>
#define LEVEL1_WIDTH 20
#define LEVEL1_HEIGHT 5
#define FIXED_TIMESTEP 0.0166666f

#define ENEMY_COUNT 1
extern int g_lives;

unsigned int LEVELB_DATA[] =
{
    0, 0, 0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 3, 3, 0, 0, 0, 3, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0,
    3, 3, 1, 1, 1, 3, 3, 3, 3, 3, 1, 1, 1, 3, 3, 3, 1, 1, 1, 3,
    1, 1, 2, 2, 1, 0, 2, 2, 1, 1, 2, 0, 1, 1, 2, 2, 1, 1, 2, 2
};



LevelB::~LevelB()
{
    delete m_game_state.player;
    delete m_game_state.map;
    Mix_FreeChunk(m_game_state.jump_sfx);
    Mix_FreeMusic(m_game_state.bgm);
}

void LevelB::initialise() {
    m_game_state.next_scene_id = -1;

    GLuint map_texture_id = Utility::load_texture("assets/images/tileset.png");
    m_game_state.map = new Map(LEVEL_WIDTH, LEVEL_HEIGHT, LEVELB_DATA, map_texture_id, 1.0f, 4, 1);
    GLuint player_texture_id = Utility::load_texture("assets/images/char.png");
    int player_walking_animation[4][4] = {
        {1, 2, 3, 4},
        {5, 6, 7, 8},
        {9, 10, 11, 12},
        {13, 14, 15, 16}
    };
    m_game_state.player = new Entity(player_texture_id, 5.0f, glm::vec3(0.0f, -1.0f, 0.0f), 1.0f,
                                     player_walking_animation, 0.2f, 4, 0, 4, 4, 0.5f, 0.5f, PLAYER);
    
    GLuint enemy_texture_id = Utility::load_texture("assets/images/BatPig.png");
    
    m_game_state.player->set_jumping_power(2.5f);

    
    int batpig_fly_animation[1][4] = {
        { 0, 1, 2, 3 }
    };
    
    m_game_state.enemies = new Entity[1];
    

    m_game_state.enemies[0] = Entity(enemy_texture_id, 1.2f, glm::vec3(0.0f), 0.0f, batpig_fly_animation, 0.0f, 4, 0, 4, 1, 0.9f, 0.9f, ENEMY);
    m_game_state.enemies[0].set_position(glm::vec3(13.0f, 0.0f, 0.0f));
    m_game_state.enemies[0].set_ai_type(GUARD);
    m_game_state.enemies[0].set_ai_state(IDLE);
    m_game_state.enemies[0].activate();

    m_game_state.player->set_position(glm::vec3(5.0f, -1.0f, 0.0f));
    
  
    
    // ————— BLENDING ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Setup background music and sound effects
    
    Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 4096);
    Mix_PlayMusic(m_game_state.bgm, -1);
    Mix_VolumeMusic(MIX_MAX_VOLUME / 16.0f);
    
    m_game_state.jump_sfx = Mix_LoadWAV("assets/audio/jump.wav");
    m_game_state.bgm = Mix_LoadMUS("assets/audio/CloudDancer.mp3");
    Mix_PlayMusic(m_game_state.bgm, -1);
    
}

void LevelB::update(float delta_time) {
    m_game_state.player->update(delta_time, nullptr, nullptr, 0, m_game_state.map);

    for (int i = 0; i < ENEMY_COUNT; i++) {
        m_game_state.enemies[i].update(delta_time, m_game_state.player, m_game_state.enemies, ENEMY_COUNT, m_game_state.map);
    }

    if (m_game_state.player->get_position().y < -10.0f) {
        m_game_state.next_scene_id = 1;
    }

    while (delta_time >= FIXED_TIMESTEP) {
        m_game_state.player->update(FIXED_TIMESTEP, m_game_state.player, m_game_state.enemies, ENEMY_COUNT, m_game_state.map);

        for (int i = 0; i < ENEMY_COUNT; ++i) {
            m_game_state.enemies[i].update(FIXED_TIMESTEP, m_game_state.player, m_game_state.enemies, ENEMY_COUNT, m_game_state.map);
        }

        delta_time -= FIXED_TIMESTEP;
    }
    
    bool player_lost = false;
    bool enemy_defeated = false;

    for (int i = 0; i < ENEMY_COUNT; i++) {
        if (m_game_state.player->check_collision(&m_game_state.enemies[i])) {
            if (m_game_state.player->get_velocity().y < 0) {
                m_game_state.enemies[i].deactivate();
                enemy_defeated = true;
            } else {
                g_lives--;
                player_lost = true;
                break;
            }
        }
    }

    if (player_lost) {
        if (g_lives > 0) {
            initialise();
        } else {
            m_game_state.next_scene_id = -1;
        }
    } else if (enemy_defeated) {
        m_game_state.next_scene_id = 3;
    }
}


void LevelB::render(ShaderProgram *program) {
    m_game_state.map->render(program);
    m_game_state.player->render(program);
    for (int i = 0; i < 1; i++) {
        if (m_game_state.enemies[i].is_active()) {
            m_game_state.enemies[i].render(program);
        }
    }
   if (g_lives <= 0) {
       glm::vec3 player_position = m_game_state.player->get_position();
       Utility::draw_text(program, Utility::load_texture("assets/fonts/font1.png"), "YOU LOSE", 0.6f, 0.1f, glm::vec3(player_position.x - 2.5f, player_position.y + 0.5f, 0.0f));
       SDL_GL_SwapWindow(SDL_GL_GetCurrentWindow());
       SDL_Delay(3000);
       exit(0);
   }
      
}
