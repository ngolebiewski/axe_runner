#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define GAME_WIDTH 800
#define GAME_HEIGHT 450
#define SPRITE_SIZE 16
#define SCALE 3.0f
#define RENDER_SIZE (SPRITE_SIZE * SCALE)

#define MAX_AXES 20
#define MAX_ENEMIES 12
#define MAX_WALLS 15
#define MAX_PARTICLES 120
#define MAX_PROJECTILES 30
#define SOUND true

typedef enum {
    STATE_TITLE,
    STATE_GAMEPLAY,
    STATE_PAUSE,
    STATE_GAMEOVER,
    STATE_VICTORY
} GameState;

typedef enum {
    ENEMY_GOBLIN,
    ENEMY_CROW,
    ENEMY_FROST_GIANT
} EnemyType;

typedef struct { Vector2 position; Vector2 velocity; float rotation; bool active; } Axe;
typedef struct { Vector2 position; Vector2 velocity; bool active; } Projectile;

typedef struct {
    EnemyType type;
    Vector2 position;
    float startY;
    float waveTimer;
    bool active;
    bool isDead;
    float shootTimer;
    int animFrame;
    float animTimer;
    int moveDirY;
    float scaleMultiplier;
    int health;
    int maxHealth;
} Enemy;

typedef struct { Rectangle rect; bool active; bool isSpike; } Wall;
typedef struct { Vector2 position; Vector2 velocity; float size; float alpha; Color color; bool active; } Particle;

Texture2D spriteSheet;
Vector2 playerPos;
float throwTimer;
Axe axes[MAX_AXES];
Projectile projectiles[MAX_PROJECTILES];
Enemy enemies[MAX_ENEMIES];
Wall walls[MAX_WALLS];
Particle particles[MAX_PARTICLES];
Vector2 heartPos;
bool heartActive;
float spikeWallX;

int currentLevel = 1;

// Invincibility & Konami Code Variables
bool isInvincible = false;
float invincibilityFlashTimer = 0.0f;

const KeyboardKey konamiCode[] = {
    KEY_UP, KEY_UP,
    KEY_DOWN, KEY_DOWN,
    KEY_LEFT, KEY_RIGHT,
    KEY_LEFT, KEY_RIGHT,
    KEY_B, KEY_A
};
int konamiIndex = 0;

void DrawSpriteFrameExTint(int frameIndex, Vector2 pos, float rotation, bool flipX, float customScale, Color tint) {
    Rectangle src = { (frameIndex - 1) * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE };
    if (flipX) src.width = -SPRITE_SIZE;
    float renderDim = SPRITE_SIZE * customScale;
    Rectangle dest = { pos.x, pos.y, renderDim, renderDim };
    Vector2 origin = { renderDim / 2.0f, renderDim / 2.0f };
    DrawTexturePro(spriteSheet, src, dest, origin, rotation, tint);
}

void DrawSpriteFrameEx(int frameIndex, Vector2 pos, float rotation, bool flipX, float customScale) {
    DrawSpriteFrameExTint(frameIndex, pos, rotation, flipX, customScale, WHITE);
}

void DrawSpriteFrame(int frameIndex, Vector2 pos, float rotation, bool flipX) {
    DrawSpriteFrameEx(frameIndex, pos, rotation, flipX, SCALE);
}

void SpawnParticles(Vector2 pos, Color color) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES && count < 30; i++) {
        if (!particles[i].active) {
            particles[i].active = true;
            particles[i].position = pos;
            float angle = (float)(rand() % 360) * DEG2RAD;
            float speed = (float)(rand() % 300 + 50);
            particles[i].velocity = (Vector2){ cosf(angle) * speed, sinf(angle) * speed };
            particles[i].size = (float)(rand() % 8 + 4);
            particles[i].alpha = 1.0f;
            particles[i].color = color;
            count++;
        }
    }
}

void LoadLevel(int level) {
    currentLevel = level;
    playerPos = (Vector2){ 150, GAME_HEIGHT / 2.0f };
    throwTimer = 0.0f;

    for (int i = 0; i < MAX_AXES; i++) axes[i].active = false;
    for (int i = 0; i < MAX_PROJECTILES; i++) projectiles[i].active = false;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].active = false;
    for (int i = 0; i < MAX_WALLS; i++) walls[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;

    if (currentLevel == 1) {
        walls[0] = (Wall){ (Rectangle){ 600, 100, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };
        walls[1] = (Wall){ (Rectangle){ 900, 300, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };
        walls[2] = (Wall){ (Rectangle){ 1100, 0, RENDER_SIZE, RENDER_SIZE * 7 }, true, false };
        walls[3] = (Wall){ (Rectangle){ 1250, 300, RENDER_SIZE, RENDER_SIZE * 2 }, true, true };

        enemies[0] = (Enemy){ ENEMY_GOBLIN, (Vector2){ 750, 150 }, 0, 0, true, false, 0.0f, 7, 0.0f, 0, 1.0f, 1, 1 };
        enemies[1] = (Enemy){ ENEMY_GOBLIN, (Vector2){ 1050, 300 }, 0, 0, true, false, 0.0f, 7, 0.0f, 0, 1.0f, 1, 1 };
        enemies[2] = (Enemy){ ENEMY_GOBLIN, (Vector2){ 450, 200 }, 0, 0, true, false, 0.0f, 7, 0.0f, 0, 1.0f, 1, 1 };

        heartPos = (Vector2){ 1500, GAME_HEIGHT / 2.0f };
        heartActive = true;
        spikeWallX = 1650.0f;
    } else {
        walls[0] = (Wall){ (Rectangle){ 600, 100, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };
        walls[1] = (Wall){ (Rectangle){ 900, 250, RENDER_SIZE, RENDER_SIZE * 3 }, true, false };
        walls[2] = (Wall){ (Rectangle){ 1200, 0, RENDER_SIZE, RENDER_SIZE * 6 }, true, false };
        walls[3] = (Wall){ (Rectangle){ 1500, 200, RENDER_SIZE, RENDER_SIZE * 3 }, true, false };
        walls[4] = (Wall){ (Rectangle){ 1900, 100, RENDER_SIZE, RENDER_SIZE * 4 }, true, false };
        walls[5] = (Wall){ (Rectangle){ 2300, 250, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };

        enemies[0] = (Enemy){ ENEMY_CROW, (Vector2){ 700, 200 }, 200.0f, 0.0f, true, false, 0.0f, 28, 0.0f, 0, 1.0f, 1, 1 };
        enemies[1] = (Enemy){ ENEMY_CROW, (Vector2){ 1100, 150 }, 150.0f, 1.0f, true, false, 0.0f, 28, 0.0f, 0, 1.0f, 1, 1 };
        enemies[2] = (Enemy){ ENEMY_CROW, (Vector2){ 1600, 250 }, 250.0f, 2.0f, true, false, 0.0f, 28, 0.0f, 0, 1.0f, 1, 1 };
        enemies[3] = (Enemy){ ENEMY_CROW, (Vector2){ 2100, 180 }, 180.0f, 0.5f, true, false, 0.0f, 28, 0.0f, 0, 1.0f, 1, 1 };

        enemies[4] = (Enemy){ ENEMY_FROST_GIANT, (Vector2){ 850, 150 }, 0, 0, true, false, 0.0f, 32, 0.0f, 1, 2.0f, 5, 5 };
        enemies[5] = (Enemy){ ENEMY_FROST_GIANT, (Vector2){ 1400, 300 }, 0, 0, true, false, 0.0f, 32, 0.0f, -1, 2.0f, 5, 5 };
        enemies[6] = (Enemy){ ENEMY_FROST_GIANT, (Vector2){ 2000, 100 }, 0, 0, true, false, 0.0f, 32, 0.0f, 1, 2.0f, 5, 5 };

        enemies[7] = (Enemy){ ENEMY_FROST_GIANT, (Vector2){ 2700, GAME_HEIGHT / 2.0f }, 0, 0, true, false, 0.0f, 32, 0.0f, 1, 4.0f, 10, 10 };

        heartPos = (Vector2){ 2800, GAME_HEIGHT / 2.0f };
        heartActive = true;
        spikeWallX = 2950.0f;
    }
}

void ResetGame(void) {
    isInvincible = false;
    konamiIndex = 0;
    LoadLevel(1);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(GAME_WIDTH, GAME_HEIGHT, "Axe Runner");

    InitAudioDevice();

    Sound fx1up = LoadSound("sound/1up.wav");
    Sound fxScreamMike = LoadSound("sound/screamMike.wav");
    SetSoundVolume(fxScreamMike, 0.3f);
    Sound fxNoiseWave = LoadSound("sound/sfx_4_downsweep.wav");
    SetSoundVolume(fxNoiseWave, 0.6f);

    Music musicDirge = LoadMusicStream("sound/song_loop.wav");
    if (SOUND) PlayMusicStream(musicDirge);
    SetTargetFPS(60);

    spriteSheet = LoadTexture("axe_demo.png");

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    GameState currentState = STATE_TITLE;

    Color castleBgColor = (Color){ 25, 15, 10, 255 };
    Color iceBgColor = (Color){ 100, 160, 220, 255 };

    float playerSpeed = 300.0f;
    float scrollSpeed = 120.0f;
    int playerAnimFrame = 3;
    float playerAnimTimer = 0.0f;
    float groundOffset = 0.0f;
    int titleAnimFrame = 1;
    float titleAnimTimer = 0.0f;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_F)) ToggleFullscreen();

        UpdateMusicStream(musicDirge);

        // Pause Toggle & Konami Input Sequence
        if (currentState == STATE_GAMEPLAY && (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
            currentState = STATE_PAUSE;
            konamiIndex = 0;
        } else if (currentState == STATE_PAUSE) {
            // Unpause triggers
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                if (konamiIndex == sizeof(konamiCode) / sizeof(konamiCode[0])) {
                    isInvincible = true;
                    if (SOUND) PlaySound(fx1up);
                }
                currentState = STATE_GAMEPLAY;
                konamiIndex = 0;
            } else {
                // Check sequence inputs during pause
                int key = GetKeyPressed();
                if (key > 0) {
                    if (konamiIndex < (int)(sizeof(konamiCode) / sizeof(konamiCode[0])) && key == konamiCode[konamiIndex]) {
                        konamiIndex++;
                    } else if (key == konamiCode[0]) {
                        konamiIndex = 1;
                    } else {
                        konamiIndex = 0;
                    }
                }
            }
        }

        if (currentState != STATE_PAUSE) {
            for (int i = 0; i < MAX_PARTICLES; i++) {
                if (particles[i].active) {
                    particles[i].position.x += particles[i].velocity.x * deltaTime;
                    particles[i].position.y += particles[i].velocity.y * deltaTime;
                    particles[i].alpha -= 1.2f * deltaTime;
                    if (particles[i].alpha <= 0.0f) particles[i].active = false;
                }
            }
        }

        switch (currentState) {
            case STATE_TITLE:
                titleAnimTimer += deltaTime;
                if (titleAnimTimer >= 0.4f) {
                    titleAnimTimer = 0.0f;
                    titleAnimFrame = (titleAnimFrame == 1) ? 2 : 1;
                }

                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ResetGame();
                    currentState = STATE_GAMEPLAY;
                }
                break;

            case STATE_PAUSE:
                break;

            case STATE_GAMEPLAY:
                if (isInvincible) {
                    invincibilityFlashTimer += deltaTime * 20.0f;
                }

                groundOffset -= scrollSpeed * deltaTime;
                if (groundOffset <= -RENDER_SIZE) groundOffset += RENDER_SIZE;

                float moveY = 0.0f;
                if (IsKeyDown(KEY_UP)) moveY -= playerSpeed * deltaTime;
                if (IsKeyDown(KEY_DOWN)) moveY += playerSpeed * deltaTime;

                if (GetTouchPointCount() > 0 || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    Vector2 touchPos = (GetTouchPointCount() > 0) ? GetTouchPosition(0) : GetMousePosition();
                    float screenMidY = GetScreenHeight() / 2.0f;

                    if (touchPos.y < screenMidY) moveY -= playerSpeed * deltaTime;
                    else                         moveY += playerSpeed * deltaTime;
                }

                if (moveY != 0.0f) {
                    float newY = playerPos.y + moveY;
                    Rectangle verticalTestRect = { playerPos.x - RENDER_SIZE/2, newY - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                    bool verticalBlocked = false;

                    for (int w = 0; w < MAX_WALLS; w++) {
                        if (walls[w].active && !walls[w].isSpike && CheckCollisionRecs(verticalTestRect, walls[w].rect)) {
                            verticalBlocked = true;
                            break;
                        }
                    }

                    if (!verticalBlocked) playerPos.y = newY;
                }

                if (playerPos.y < RENDER_SIZE / 2) playerPos.y = RENDER_SIZE / 2;
                if (playerPos.y > GAME_HEIGHT - RENDER_SIZE * 1.5f) playerPos.y = GAME_HEIGHT - RENDER_SIZE * 1.5f;

                playerAnimTimer += deltaTime;
                if (playerAnimTimer >= 0.1f) {
                    playerAnimTimer = 0.0f;
                    if (++playerAnimFrame > 5) playerAnimFrame = 3;
                }

                throwTimer += deltaTime;
                if (throwTimer >= 0.35f) {
                    throwTimer = 0.0f;
                    for (int i = 0; i < MAX_AXES; i++) {
                        if (!axes[i].active) {
                            axes[i].active = true;
                            axes[i].position = playerPos;
                            axes[i].velocity = (Vector2){ 400.0f, -250.0f };
                            axes[i].rotation = 0.0f;
                            break;
                        }
                    }
                }

                // Update Player Axes
                for (int i = 0; i < MAX_AXES; i++) {
                    if (axes[i].active) {
                        axes[i].position.x += axes[i].velocity.x * deltaTime;
                        axes[i].position.y += axes[i].velocity.y * deltaTime;
                        axes[i].velocity.y += 600.0f * deltaTime;
                        axes[i].rotation += 720.0f * deltaTime;

                        Rectangle axeRect = { axes[i].position.x - 8, axes[i].position.y - 8, 16, 16 };
                        for (int w = 0; w < MAX_WALLS; w++) {
                            if (walls[w].active && CheckCollisionRecs(axeRect, walls[w].rect)) {
                                axes[i].active = false;
                                break;
                            }
                        }

                        if (axes[i].position.x > GAME_WIDTH + 50 || axes[i].position.y > GAME_HEIGHT + 50) {
                            axes[i].active = false;
                        }
                    }
                }

                // Update Enemy Projectiles
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    if (projectiles[i].active) {
                        projectiles[i].position.x += projectiles[i].velocity.x * deltaTime;
                        projectiles[i].position.y += projectiles[i].velocity.y * deltaTime;

                        Rectangle projRect = { projectiles[i].position.x - 4, projectiles[i].position.y - 4, 8, 8 };
                        
                        for (int w = 0; w < MAX_WALLS; w++) {
                            if (walls[w].active && CheckCollisionRecs(projRect, walls[w].rect)) {
                                projectiles[i].active = false;
                                break;
                            }
                        }

                        Rectangle playerRect = { playerPos.x - RENDER_SIZE/2, playerPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                        if (projectiles[i].active && CheckCollisionRecs(playerRect, projRect)) {
                            if (!isInvincible) {
                                SpawnParticles(playerPos, RED);
                                PlaySound(fxNoiseWave);
                                currentState = STATE_GAMEOVER;
                            } else {
                                projectiles[i].active = false;
                            }
                        }

                        if (projectiles[i].position.x < -20) projectiles[i].active = false;
                    }
                }

                Rectangle playerRect = { playerPos.x - RENDER_SIZE/2, playerPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                bool pushedByWall = false;

                // Update Level Walls
                for (int i = 0; i < MAX_WALLS; i++) {
                    if (walls[i].active) {
                        walls[i].rect.x -= scrollSpeed * deltaTime;

                        if (CheckCollisionRecs(playerRect, walls[i].rect)) {
                            if (walls[i].isSpike) {
                                if (!isInvincible) {
                                    SpawnParticles(playerPos, RED);
                                    PlaySound(fxNoiseWave);
                                    currentState = STATE_GAMEOVER;
                                }
                            } else {
                                playerPos.x = walls[i].rect.x - RENDER_SIZE / 2;
                                pushedByWall = true;
                            }
                        }
                    }
                }

                if (!pushedByWall && playerPos.x < 150) {
                    playerPos.x += scrollSpeed * 0.8f * deltaTime;
                }

                if (playerPos.x - RENDER_SIZE / 2 <= 0) {
                    if (!isInvincible) {
                        SpawnParticles(playerPos, RED);
                        PlaySound(fxNoiseWave);
                        currentState = STATE_GAMEOVER;
                    } else {
                        playerPos.x = RENDER_SIZE / 2;
                    }
                }

                // Update Enemies
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        float renderSizeE = RENDER_SIZE * enemies[i].scaleMultiplier;
                        Rectangle leftHazardRect = { 0, 0, RENDER_SIZE, GAME_HEIGHT };
                        Color particleColor = (currentLevel == 1) ? LIME : (enemies[i].type == ENEMY_CROW ? SKYBLUE : BLUE);

                        if (enemies[i].isDead) {
                            enemies[i].position.x -= scrollSpeed * deltaTime;

                            Rectangle deadEnemyRect = { enemies[i].position.x - renderSizeE/2, enemies[i].position.y - renderSizeE/2, renderSizeE, renderSizeE };
                            if (CheckCollisionRecs(deadEnemyRect, leftHazardRect) || enemies[i].position.x < -RENDER_SIZE) {
                                SpawnParticles(enemies[i].position, particleColor);
                                enemies[i].active = false;
                            }
                            continue;
                        }

                        // AI Movement
                        if (enemies[i].type == ENEMY_GOBLIN) {
                            enemies[i].animTimer += deltaTime;
                            if (enemies[i].animTimer >= 0.15f) {
                                enemies[i].animTimer = 0.0f;
                                enemies[i].animFrame = (enemies[i].animFrame == 7) ? 8 : 7;
                            }

                            float gobSpeedX = scrollSpeed + 110.0f;
                            float gobSpeedY = 150.0f;
                            Vector2 nextPos = enemies[i].position;

                            if (enemies[i].moveDirY == 0) nextPos.x -= gobSpeedX * deltaTime;
                            else {
                                nextPos.x -= scrollSpeed * deltaTime;
                                nextPos.y += enemies[i].moveDirY * gobSpeedY * deltaTime;
                            }

                            Rectangle gobRect = { nextPos.x - renderSizeE/2, nextPos.y - renderSizeE/2, renderSizeE, renderSizeE };
                            bool hitSpikeWall = false, blockedByWall = false;

                            for (int w = 0; w < MAX_WALLS; w++) {
                                if (walls[w].active && CheckCollisionRecs(gobRect, walls[w].rect)) {
                                    if (walls[w].isSpike) hitSpikeWall = true;
                                    else blockedByWall = true;
                                    break;
                                }
                            }

                            if (CheckCollisionRecs(gobRect, leftHazardRect) || hitSpikeWall) {
                                SpawnParticles(enemies[i].position, particleColor);
                                enemies[i].isDead = true;
                                enemies[i].animFrame = 16;
                                continue;
                            }

                            if (blockedByWall) {
                                if (enemies[i].moveDirY == 0) enemies[i].moveDirY = (enemies[i].position.y > GAME_HEIGHT / 2) ? -1 : 1;
                                enemies[i].position.x -= scrollSpeed * deltaTime;
                            } else {
                                enemies[i].position = nextPos;
                                if (enemies[i].moveDirY != 0) {
                                    Rectangle testLeftRect = { enemies[i].position.x - 10.0f - renderSizeE/2, enemies[i].position.y - renderSizeE/2, renderSizeE, renderSizeE };
                                    bool leftBlocked = false;
                                    for (int w = 0; w < MAX_WALLS; w++) {
                                        if (walls[w].active && CheckCollisionRecs(testLeftRect, walls[w].rect)) { leftBlocked = true; break; }
                                    }
                                    if (!leftBlocked) enemies[i].moveDirY = 0;
                                }
                            }

                            if (enemies[i].position.y < renderSizeE / 2) { enemies[i].position.y = renderSizeE / 2; enemies[i].moveDirY = 1; }
                            if (enemies[i].position.y > GAME_HEIGHT - renderSizeE * 1.5f) { enemies[i].position.y = GAME_HEIGHT - renderSizeE * 1.5f; enemies[i].moveDirY = -1; }

                        } else if (enemies[i].type == ENEMY_CROW) {
                            enemies[i].waveTimer += deltaTime * 3.0f;
                            enemies[i].position.x -= (scrollSpeed + 90.0f) * deltaTime;
                            enemies[i].position.y = enemies[i].startY + sinf(enemies[i].waveTimer) * 60.0f;

                            enemies[i].animTimer += deltaTime;
                            if (enemies[i].animTimer >= 0.12f) {
                                enemies[i].animTimer = 0.0f;
                                enemies[i].animFrame = (enemies[i].animFrame == 28) ? 29 : 28;
                            }
                        } else if (enemies[i].type == ENEMY_FROST_GIANT) {
                            enemies[i].animTimer += deltaTime;
                            if (enemies[i].animTimer >= 0.2f) {
                                enemies[i].animTimer = 0.0f;
                                enemies[i].animFrame = (enemies[i].animFrame == 32) ? 33 : 32;
                            }

                            enemies[i].position.x -= scrollSpeed * deltaTime;
                            enemies[i].position.y += enemies[i].moveDirY * 100.0f * deltaTime;

                            if (enemies[i].position.y < renderSizeE / 2) { enemies[i].position.y = renderSizeE / 2; enemies[i].moveDirY = 1; }
                            if (enemies[i].position.y > GAME_HEIGHT - renderSizeE * 1.2f) { enemies[i].position.y = GAME_HEIGHT - renderSizeE * 1.2f; enemies[i].moveDirY = -1; }
                        }

                        Rectangle enemyRect = { enemies[i].position.x - renderSizeE/2, enemies[i].position.y - renderSizeE/2, renderSizeE, renderSizeE };

                        if (CheckCollisionRecs(enemyRect, leftHazardRect)) {
                            SpawnParticles(enemies[i].position, particleColor);
                            enemies[i].isDead = true;
                            enemies[i].animFrame = (enemies[i].type == ENEMY_GOBLIN) ? 16 : (enemies[i].type == ENEMY_CROW ? 30 : 34);
                            continue;
                        }

                        if (enemies[i].type == ENEMY_GOBLIN || enemies[i].type == ENEMY_CROW) {
                            enemies[i].shootTimer += deltaTime;
                            float shootInterval = (enemies[i].type == ENEMY_GOBLIN) ? 1.2f : 1.5f;
                            if (enemies[i].shootTimer >= shootInterval) {
                                enemies[i].shootTimer = 0.0f;
                                for (int p = 0; p < MAX_PROJECTILES; p++) {
                                    if (!projectiles[p].active) {
                                        projectiles[p].active = true;
                                        projectiles[p].position = enemies[i].position;
                                        projectiles[p].velocity = (enemies[i].type == ENEMY_GOBLIN) ? (Vector2){ -350.0f, 0.0f } : (Vector2){ -300.0f, 50.0f };
                                        break;
                                    }
                                }
                            }
                        }

                        if (CheckCollisionRecs(playerRect, enemyRect)) {
                            if (!isInvincible) {
                                SpawnParticles(playerPos, RED);
                                PlaySound(fxNoiseWave);
                                currentState = STATE_GAMEOVER;
                            } else {
                                enemies[i].isDead = true;
                                enemies[i].animFrame = (enemies[i].type == ENEMY_GOBLIN) ? 16 : (enemies[i].type == ENEMY_CROW ? 30 : 34);
                                SpawnParticles(enemies[i].position, particleColor);
                            }
                        }

                        for (int j = 0; j < MAX_AXES; j++) {
                            if (axes[j].active && CheckCollisionCircleRec(axes[j].position, renderSizeE / 3, enemyRect)) {
                                axes[j].active = false;
                                enemies[i].health--;

                                SpawnParticles(axes[j].position, particleColor);
                                PlaySound(fxScreamMike);

                                if (enemies[i].health <= 0) {
                                    enemies[i].isDead = true;
                                    enemies[i].animFrame = (enemies[i].type == ENEMY_GOBLIN) ? 16 : (enemies[i].type == ENEMY_CROW ? 30 : 34);
                                }
                                break;
                            }
                        }
                    }
                }

                if (heartActive) {
                    heartPos.x -= scrollSpeed * deltaTime;
                    spikeWallX -= scrollSpeed * deltaTime;

                    Rectangle heartRect = { heartPos.x - RENDER_SIZE/2, heartPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                    Rectangle endSpikeRect = { spikeWallX - RENDER_SIZE/2, 0, RENDER_SIZE, GAME_HEIGHT };

                    if (CheckCollisionRecs(playerRect, heartRect)) {
                        if (SOUND) PlaySound(fx1up);
                        if (currentLevel == 1) {
                            LoadLevel(2);
                        } else {
                            currentState = STATE_VICTORY;
                        }
                    } else if (CheckCollisionRecs(playerRect, endSpikeRect)) {
                        if (!isInvincible) {
                            SpawnParticles(playerPos, RED);
                            PlaySound(fxNoiseWave);
                            currentState = STATE_GAMEOVER;
                        }
                    }
                }
                break;

            case STATE_GAMEOVER:
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    LoadLevel(currentLevel);
                    currentState = STATE_GAMEPLAY;
                }
                break;

            case STATE_VICTORY:
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentState = STATE_TITLE;
                }
                break;
        }

        // Render Frame
        BeginTextureMode(target);
            ClearBackground((currentLevel == 1) ? castleBgColor : iceBgColor);

            // Draw Floor
            int groundTile = (currentLevel == 1) ? 10 : 17;
            for (float x = groundOffset; x < GAME_WIDTH + RENDER_SIZE; x += RENDER_SIZE) {
                DrawSpriteFrame(groundTile, (Vector2){ x + RENDER_SIZE/2, GAME_HEIGHT - RENDER_SIZE/2 }, 0, false);
            }

            if (currentState == STATE_TITLE) {
                DrawText("AXE RUNNER", GAME_WIDTH / 2 - MeasureText("AXE RUNNER", 40) / 2, 150, 40, RAYWHITE);
                DrawText("Touch / Press Space to Start", GAME_WIDTH / 2 - MeasureText("Touch / Press Space to Start", 20) / 2, 250, 20, LIGHTGRAY);
                DrawSpriteFrame(titleAnimFrame, (Vector2){ GAME_WIDTH / 2, 320 }, 0, false);
            }
            else {
                if (currentLevel == 1) {
                    for (int y = 0; y < GAME_HEIGHT - RENDER_SIZE; y += RENDER_SIZE) {
                        DrawSpriteFrame(9, (Vector2){ RENDER_SIZE/2, y + RENDER_SIZE/2 }, 180.0f, false);
                    }
                } else {
                    for (int y = 0; y < GAME_HEIGHT - RENDER_SIZE; y += RENDER_SIZE) {
                        DrawSpriteFrame(18, (Vector2){ RENDER_SIZE/2, y + RENDER_SIZE/2 }, 180.0f, false);
                    }
                }

                if (heartActive) {
                    DrawSpriteFrame(11, heartPos, 0, false);
                    int endSpikeTile = (currentLevel == 1) ? 9 : 18;
                    float endSpikeRot = (currentLevel == 1) ? 0.0f : 0.0f;
                    for (int y = 0; y < GAME_HEIGHT - RENDER_SIZE; y += RENDER_SIZE) {
                        DrawSpriteFrame(endSpikeTile, (Vector2){ spikeWallX, y + RENDER_SIZE/2 }, endSpikeRot, false);
                    }
                }

                // Render Level Obstacle Walls
                for (int i = 0; i < MAX_WALLS; i++) {
                    if (walls[i].active) {
                        int wallSpriteIdx = walls[i].isSpike ? 9 : ((currentLevel == 1) ? 6 : 19);
                        for (int y = 0; y < walls[i].rect.height; y += RENDER_SIZE) {
                            Vector2 wPos = { walls[i].rect.x + RENDER_SIZE/2, walls[i].rect.y + y + RENDER_SIZE/2 };
                            DrawSpriteFrame(wallSpriteIdx, wPos, 0, false);
                        }
                    }
                }

                // Render Enemies with Damage Darkening Tint
                for (int i = 0; i < MAX_ENEMIES; i++) {
                    if (enemies[i].active) {
                        Color renderTint = WHITE;

                        if (enemies[i].type == ENEMY_FROST_GIANT && enemies[i].health < enemies[i].maxHealth) {
                            float healthRatio = (float)enemies[i].health / (float)enemies[i].maxHealth;
                            unsigned char val = (unsigned char)(80 + 175 * healthRatio);
                            renderTint = (Color){ val, val, val + 20, 255 };
                        }

                        DrawSpriteFrameExTint(enemies[i].animFrame, enemies[i].position, 0, false, SCALE * enemies[i].scaleMultiplier, renderTint);
                    }
                }

                // Render Projectiles
                for (int i = 0; i < MAX_PROJECTILES; i++) {
                    if (projectiles[i].active) {
                        if (currentLevel == 1) DrawRectangleV(projectiles[i].position, (Vector2){ 8, 4 }, PURPLE);
                        else DrawSpriteFrame(31, projectiles[i].position, 0, false);
                    }
                }

                // Render Player Axes
                for (int i = 0; i < MAX_AXES; i++) {
                    if (axes[i].active) DrawSpriteFrame(12, axes[i].position, axes[i].rotation, false);
                }

                // Render Player with Mario Star Flashing when Invincible
                if (currentState == STATE_GAMEPLAY || currentState == STATE_PAUSE) {
                    Color playerTint = WHITE;
                    if (isInvincible) {
                        float flashVal = sinf(invincibilityFlashTimer);
                        if (flashVal > 0.0f) {
                            playerTint = (Color){ 255, 255, 255, 255 }; // Pure white flash
                        } else {
                            playerTint = (Color){ 255, 220, 100, 255 }; // Gold/yellow tint alternate
                        }
                    }
                    DrawSpriteFrameExTint(playerAnimFrame, playerPos, 0, false, SCALE, playerTint);
                }

                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (particles[i].active) {
                        DrawRectangleV(particles[i].position, (Vector2){ particles[i].size, particles[i].size }, ColorAlpha(particles[i].color, particles[i].alpha));
                    }
                }

                if (currentState == STATE_PAUSE) {
                    DrawRectangle(0, 0, GAME_WIDTH, GAME_HEIGHT, ColorAlpha(BLACK, 0.4f));
                    DrawText("PAUSED", GAME_WIDTH / 2 - MeasureText("PAUSED", 40) / 2, 180, 40, RAYWHITE);
                    DrawText("Press Space or Enter to Resume", GAME_WIDTH / 2 - MeasureText("Press Space or Enter to Resume", 20) / 2, 240, 20, LIGHTGRAY);
                } else if (currentState == STATE_GAMEOVER) {
                    DrawText("GAME OVER", GAME_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, 180, 40, RED);
                    DrawText("Touch / Press Space to Retry Level", GAME_WIDTH / 2 - MeasureText("Touch / Press Space to Retry Level", 20) / 2, 300, 20, LIGHTGRAY);
                } else if (currentState == STATE_VICTORY) {
                    DrawText("ALL STAGES CLEARED!", GAME_WIDTH / 2 - MeasureText("ALL STAGES CLEARED!", 40) / 2, 180, 40, GOLD);
                    DrawText("Touch / Press Space to Play Again", GAME_WIDTH / 2 - MeasureText("Touch / Press Space to Play Again", 20) / 2, 300, 20, LIGHTGRAY);
                }
            }
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);

            float screenW = (float)GetScreenWidth();
            float screenH = (float)GetScreenHeight();
            float scale = fminf(screenW / GAME_WIDTH, screenH / GAME_HEIGHT);

            Rectangle srcRec = { 0.0f, 0.0f, (float)GAME_WIDTH, (float)-GAME_HEIGHT };
            Rectangle destRec = {
                (screenW - ((float)GAME_WIDTH * scale)) * 0.5f,
                (screenH - ((float)GAME_HEIGHT * scale)) * 0.5f,
                (float)GAME_WIDTH * scale,
                (float)GAME_HEIGHT * scale
            };

            DrawTexturePro(target.texture, srcRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
        EndDrawing();
    }

    UnloadSound(fx1up);
    UnloadSound(fxScreamMike);
    UnloadSound(fxNoiseWave);

    UnloadMusicStream(musicDirge);
    CloseAudioDevice();

    UnloadRenderTexture(target);
    UnloadTexture(spriteSheet);
    CloseWindow();
    return 0;
}