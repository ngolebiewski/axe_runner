#include "raylib.h"
#include <math.h>
#include <stdlib.h>

#define GAME_WIDTH 800
#define GAME_HEIGHT 450
#define SPRITE_SIZE 16
#define SCALE 3.0f
#define RENDER_SIZE (SPRITE_SIZE * SCALE)

#define MAX_AXES 20
#define MAX_GOBLINS 10
#define MAX_WALLS 10
#define MAX_PARTICLES 120
#define MAX_DARTS 20
#define SOUND true

typedef enum {
    STATE_TITLE,
    STATE_GAMEPLAY,
    STATE_GAMEOVER,
    STATE_VICTORY
} GameState;

typedef struct { Vector2 position; Vector2 velocity; float rotation; bool active; } Axe;
typedef struct { Vector2 position; Vector2 velocity; bool active; } Dart;
typedef struct { Vector2 position; bool active; bool isDead; float shootTimer; int animFrame; float animTimer; int moveDirY; } Goblin;
typedef struct { Rectangle rect; bool active; bool isSpike; } Wall;
typedef struct { Vector2 position; Vector2 velocity; float size; float alpha; Color color; bool active; } Particle;

Texture2D spriteSheet;
Vector2 playerPos;
float throwTimer;
Axe axes[MAX_AXES];
Dart darts[MAX_DARTS];
Goblin goblins[MAX_GOBLINS];
Wall walls[MAX_WALLS];
Particle particles[MAX_PARTICLES];
Vector2 heartPos;
bool heartActive;
float spikeWallX;

void DrawSpriteFrame(int frameIndex, Vector2 pos, float rotation, bool flipX) {
    Rectangle src = { (frameIndex - 1) * SPRITE_SIZE, 0, SPRITE_SIZE, SPRITE_SIZE };
    if (flipX) src.width = -SPRITE_SIZE;
    Rectangle dest = { pos.x, pos.y, RENDER_SIZE, RENDER_SIZE };
    Vector2 origin = { RENDER_SIZE / 2.0f, RENDER_SIZE / 2.0f };
    DrawTexturePro(spriteSheet, src, dest, origin, rotation, WHITE);
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

void ResetGame(void) {
    playerPos = (Vector2){ 150, GAME_HEIGHT / 2.0f };
    throwTimer = 0.0f;

    for (int i = 0; i < MAX_AXES; i++) axes[i].active = false;
    for (int i = 0; i < MAX_DARTS; i++) darts[i].active = false;
    for (int i = 0; i < MAX_GOBLINS; i++) goblins[i].active = false;
    for (int i = 0; i < MAX_WALLS; i++) walls[i].active = false;
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false;

    walls[0] = (Wall){ (Rectangle){ 600, 100, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };
    walls[1] = (Wall){ (Rectangle){ 900, 300, RENDER_SIZE, RENDER_SIZE * 2 }, true, false };
    walls[2] = (Wall){ (Rectangle){ 1100, 0, RENDER_SIZE, RENDER_SIZE * 7 }, true, false };
    walls[3] = (Wall){ (Rectangle){ 1250, 300, RENDER_SIZE, RENDER_SIZE * 2 }, true, true };

    goblins[0] = (Goblin){ (Vector2){ 750, 150 }, true, false, 0.0f, 7, 0.0f, 0 };
    goblins[1] = (Goblin){ (Vector2){ 1050, 300 }, true, false, 0.0f, 7, 0.0f, 0 };
    goblins[2] = (Goblin){ (Vector2){ 450, 200 }, true, false, 0.0f, 7, 0.0f, 0 };

    heartPos = (Vector2){ 1500, GAME_HEIGHT / 2.0f };
    heartActive = true;
    spikeWallX = 1650.0f;
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

    //init the music stream for the looping background music;
    Music musicDirge = LoadMusicStream("sound/song_loop.wav");
    float timePlayed = 0.0f;        // Time played normalized [0.0f..1.0f]
    float volume = 0.8f;
    SetMusicVolume(musicDirge, volume);
    // Start playing the stream ONCe before the game loop
    if (SOUND) PlayMusicStream(musicDirge);
    SetTargetFPS(60);
    

    spriteSheet = LoadTexture("axe_demo.png");

    RenderTexture2D target = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(target.texture, TEXTURE_FILTER_POINT);

    GameState currentState = STATE_TITLE;
    Color darkBrown = (Color){ 25, 15, 10, 255 };

    float playerSpeed = 300.0f;
    float scrollSpeed = 120.0f;
    int playerAnimFrame = 3;
    float playerAnimTimer = 0.0f;
    float groundOffset = 0.0f;
    int titleAnimFrame = 1;
    float titleAnimTimer = 0.0f;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_F)) {
            ToggleFullscreen();
        }

        // Play some background music!
        // Later on, add this to each switch case so each level can have a different tune.
        UpdateMusicStream(musicDirge);
        timePlayed = GetMusicTimePlayed(musicDirge)/GetMusicTimeLength(musicDirge);
        if (timePlayed > 1.0f) timePlayed = 1.0f;

        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                particles[i].position.x += particles[i].velocity.x * deltaTime;
                particles[i].position.y += particles[i].velocity.y * deltaTime;
                particles[i].alpha -= 1.2f * deltaTime;
                if (particles[i].alpha <= 0.0f) particles[i].active = false;
            }
        }

        switch (currentState) {
            case STATE_TITLE:
                titleAnimTimer += deltaTime;
                if (titleAnimTimer >= 0.4f) {
                    titleAnimTimer = 0.0f;
                    titleAnimFrame = (titleAnimFrame == 1) ? 2 : 1;
                }

                if (IsKeyPressed(KEY_SPACE) || GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    ResetGame();
                    currentState = STATE_GAMEPLAY;
                }
                break;

            case STATE_GAMEPLAY:
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

                    if (!verticalBlocked) {
                        playerPos.y = newY;
                    }
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

                for (int i = 0; i < MAX_DARTS; i++) {
                    if (darts[i].active) {
                        darts[i].position.x += darts[i].velocity.x * deltaTime;
                        darts[i].position.y += darts[i].velocity.y * deltaTime;

                        Rectangle dartRect = { darts[i].position.x - 4, darts[i].position.y - 4, 8, 8 };
                        
                        for (int w = 0; w < MAX_WALLS; w++) {
                            if (walls[w].active && CheckCollisionRecs(dartRect, walls[w].rect)) {
                                darts[i].active = false;
                                break;
                            }
                        }

                        Rectangle playerRect = { playerPos.x - RENDER_SIZE/2, playerPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                        if (darts[i].active && CheckCollisionRecs(playerRect, dartRect)) {
                            SpawnParticles(playerPos, RED);
                            PlaySound(fxNoiseWave);
                            currentState = STATE_GAMEOVER;
                        }

                        if (darts[i].position.x < -20) darts[i].active = false;
                    }
                }

                Rectangle playerRect = { playerPos.x - RENDER_SIZE/2, playerPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                bool pushedByWall = false;

                for (int i = 0; i < MAX_WALLS; i++) {
                    if (walls[i].active) {
                        walls[i].rect.x -= scrollSpeed * deltaTime;

                        if (CheckCollisionRecs(playerRect, walls[i].rect)) {
                            if (walls[i].isSpike) {
                                SpawnParticles(playerPos, RED);
                                PlaySound(fxNoiseWave);
                                currentState = STATE_GAMEOVER;
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
                    SpawnParticles(playerPos, RED);
                    PlaySound(fxNoiseWave);
                    currentState = STATE_GAMEOVER;
                }

                for (int i = 0; i < MAX_GOBLINS; i++) {
                    if (goblins[i].active) {
                        Rectangle leftSpikesRect = { 0, 0, RENDER_SIZE, GAME_HEIGHT };

                        if (goblins[i].isDead) {
                            goblins[i].position.x -= scrollSpeed * deltaTime;
                            Rectangle deadGoblinRect = { goblins[i].position.x - RENDER_SIZE/2, goblins[i].position.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                            if (CheckCollisionRecs(deadGoblinRect, leftSpikesRect)) {
                                SpawnParticles(goblins[i].position, LIME);
                                goblins[i].active = false;
                            }
                            continue;
                        }

                        goblins[i].animTimer += deltaTime;
                        if (goblins[i].animTimer >= 0.15f) {
                            goblins[i].animTimer = 0.0f;
                            goblins[i].animFrame = (goblins[i].animFrame == 7) ? 8 : 7;
                        }

                        float gobSpeedX = scrollSpeed + 110.0f;
                        float gobSpeedY = 150.0f;

                        Vector2 nextPos = goblins[i].position;

                        if (goblins[i].moveDirY == 0) {
                            nextPos.x -= gobSpeedX * deltaTime;
                        } else {
                            nextPos.x -= scrollSpeed * deltaTime;
                            nextPos.y += goblins[i].moveDirY * gobSpeedY * deltaTime;
                        }

                        Rectangle goblinRect = { nextPos.x - RENDER_SIZE/2, nextPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };

                        if (CheckCollisionRecs(goblinRect, leftSpikesRect)) {
                            SpawnParticles(goblins[i].position, LIME);
                            goblins[i].isDead = true;
                            goblins[i].animFrame = 16;
                            continue;
                        }

                        bool hitSpikeWall = false;
                        bool blockedByWall = false;

                        for (int w = 0; w < MAX_WALLS; w++) {
                            if (walls[w].active && CheckCollisionRecs(goblinRect, walls[w].rect)) {
                                if (walls[w].isSpike) {
                                    hitSpikeWall = true;
                                    break;
                                } else {
                                    blockedByWall = true;
                                    break;
                                }
                            }
                        }

                        if (hitSpikeWall) {
                            SpawnParticles(goblins[i].position, LIME);
                            goblins[i].isDead = true;
                            goblins[i].animFrame = 16;
                            continue;
                        }

                        if (blockedByWall) {
                            if (goblins[i].moveDirY == 0) {
                                goblins[i].moveDirY = (goblins[i].position.y > GAME_HEIGHT / 2) ? -1 : 1;
                            }
                            goblins[i].position.x -= scrollSpeed * deltaTime;
                        } else {
                            goblins[i].position = nextPos;
                            
                            if (goblins[i].moveDirY != 0) {
                                Vector2 testLeftPos = { goblins[i].position.x - 10.0f, goblins[i].position.y };
                                Rectangle testLeftRect = { testLeftPos.x - RENDER_SIZE/2, testLeftPos.y - RENDER_SIZE/2, RENDER_SIZE, RENDER_SIZE };
                                bool leftBlocked = false;

                                for (int w = 0; w < MAX_WALLS; w++) {
                                    if (walls[w].active && CheckCollisionRecs(testLeftRect, walls[w].rect)) {
                                        leftBlocked = true;
                                        break;
                                    }
                                }
                                if (!leftBlocked) goblins[i].moveDirY = 0;
                            }
                        }

                        if (goblins[i].position.y < RENDER_SIZE / 2) {
                            goblins[i].position.y = RENDER_SIZE / 2;
                            goblins[i].moveDirY = 1;
                        }
                        if (goblins[i].position.y > GAME_HEIGHT - RENDER_SIZE * 1.5f) {
                            goblins[i].position.y = GAME_HEIGHT - RENDER_SIZE * 1.5f;
                            goblins[i].moveDirY = -1;
                        }

                        goblins[i].shootTimer += deltaTime;
                        if (goblins[i].shootTimer >= 1.2f) {
                            goblins[i].shootTimer = 0.0f;
                            for (int d = 0; d < MAX_DARTS; d++) {
                                if (!darts[d].active) {
                                    darts[d].active = true;
                                    darts[d].position = goblins[i].position;
                                    darts[d].velocity = (Vector2){ -350.0f, 0.0f };
                                    break;
                                }
                            }
                        }

                        if (CheckCollisionRecs(playerRect, goblinRect)) {
                            SpawnParticles(playerPos, RED);
                            PlaySound(fxNoiseWave);
                            currentState = STATE_GAMEOVER;
                        }

                        for (int j = 0; j < MAX_AXES; j++) {
                            if (axes[j].active) {
                                Vector2 axeCenter = axes[j].position;
                                if (CheckCollisionCircleRec(axeCenter, RENDER_SIZE / 3, goblinRect)) {
                                    SpawnParticles(goblins[i].position, LIME);
                                    PlaySound(fxScreamMike);
                                    goblins[i].isDead = true;
                                    goblins[i].animFrame = 16;
                                    axes[j].active = false;
                                    break;
                                }
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
                      if (SOUND) PlaySound(fx1up); // Play 1up WAV sound 
                      currentState = STATE_VICTORY;
                    } else if (CheckCollisionRecs(playerRect, endSpikeRect)) {
                        SpawnParticles(playerPos, RED);
                        PlaySound(fxNoiseWave);
                        currentState = STATE_GAMEOVER;
                    }
                }
                break;

            case STATE_GAMEOVER:
            case STATE_VICTORY:
                if (IsKeyPressed(KEY_SPACE) || GetTouchPointCount() > 0 || IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    currentState = STATE_TITLE;
                }
                break;
        }

        BeginTextureMode(target);
            ClearBackground(darkBrown);

            for (float x = groundOffset; x < GAME_WIDTH + RENDER_SIZE; x += RENDER_SIZE) {
                DrawSpriteFrame(10, (Vector2){ x + RENDER_SIZE/2, GAME_HEIGHT - RENDER_SIZE/2 }, 0, false);
            }

            if (currentState == STATE_TITLE) {
                DrawText("AXE RUNNER", GAME_WIDTH / 2 - MeasureText("AXE RUNNER", 40) / 2, 150, 40, RAYWHITE);
                DrawText("Touch / Any key to start", GAME_WIDTH / 2 - MeasureText("Touch / Any key to start", 20) / 2, 250, 20, LIGHTGRAY);
                DrawSpriteFrame(titleAnimFrame, (Vector2){ GAME_WIDTH / 2, 320 }, 0, false);
            }
            else {
                for (int y = 0; y < GAME_HEIGHT - RENDER_SIZE; y += RENDER_SIZE) {
                    DrawSpriteFrame(9, (Vector2){ RENDER_SIZE/2, y + RENDER_SIZE/2 }, 180.0f, false);
                }

                if (heartActive) {
                    DrawSpriteFrame(11, heartPos, 0, false);
                    for (int y = 0; y < GAME_HEIGHT - RENDER_SIZE; y += RENDER_SIZE) {
                        DrawSpriteFrame(9, (Vector2){ spikeWallX, y + RENDER_SIZE/2 }, 0, false);
                    }
                }

                for (int i = 0; i < MAX_WALLS; i++) {
                    if (walls[i].active) {
                        int spriteIdx = walls[i].isSpike ? 9 : 6;
                        for (int y = 0; y < walls[i].rect.height; y += RENDER_SIZE) {
                            Vector2 wPos = { walls[i].rect.x + RENDER_SIZE/2, walls[i].rect.y + y + RENDER_SIZE/2 };
                            DrawSpriteFrame(spriteIdx, wPos, 0, false);
                        }
                    }
                }

                for (int i = 0; i < MAX_GOBLINS; i++) {
                    if (goblins[i].active) DrawSpriteFrame(goblins[i].animFrame, goblins[i].position, 0, false);
                }

                for (int i = 0; i < MAX_DARTS; i++) {
                    if (darts[i].active) DrawRectangleV(darts[i].position, (Vector2){ 8, 4 }, PURPLE);
                }

                for (int i = 0; i < MAX_AXES; i++) {
                    if (axes[i].active) DrawSpriteFrame(12, axes[i].position, axes[i].rotation, false);
                }

                if (currentState == STATE_GAMEPLAY) DrawSpriteFrame(playerAnimFrame, playerPos, 0, false);

                for (int i = 0; i < MAX_PARTICLES; i++) {
                    if (particles[i].active) {
                        DrawRectangleV(particles[i].position, (Vector2){ particles[i].size, particles[i].size }, ColorAlpha(particles[i].color, particles[i].alpha));
                    }
                }

                if (currentState == STATE_GAMEOVER) {
                    DrawText("GAME OVER", GAME_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, 180, 40, RED);
                    DrawText("Touch / Press key to try again", GAME_WIDTH / 2 - MeasureText("Touch / Press key to try again", 20) / 2, 300, 20, LIGHTGRAY);
                } else if (currentState == STATE_VICTORY) {
                    DrawText("STAGE CLEARED!", GAME_WIDTH / 2 - MeasureText("STAGE CLEARED!", 40) / 2, 180, 40, GOLD);
                    DrawText("Touch / Press key to play again", GAME_WIDTH / 2 - MeasureText("Touch / Press key to play again", 20) / 2, 300, 20, LIGHTGRAY);
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
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadSound(fx1up); // unloads the sound data initialized in main
    UnloadSound(fxScreamMike);

    UnloadMusicStream(musicDirge);
    CloseAudioDevice(); // finish audio cleanup
  
    UnloadRenderTexture(target);
    UnloadTexture(spriteSheet);
    CloseWindow();
    return 0;
}
