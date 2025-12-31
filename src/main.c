#include "raylib.h"
#include <math.h>

#define SCREEN_W 800
#define SCREEN_H 600
#define GRAVITY 1200.0f
#define JUMP_FORCE -700.0f
#define MOVE_ACCEL 800.0f
#define AIR_ACCEL 600.0f
#define FRICTION 600.0f
#define ICE_FRICTION 120.0f
#define MAX_SPEED 250.0f

typedef struct {
    Vector2 pos;
    Vector2 vel;
    float w, h;
    bool grounded;
    float groundTime;
} Player;

typedef struct {
    Rectangle rect;
} Platform;

typedef struct {
    Vector2 pos;
    float size;
} Spike;

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "ICED - Slippery Platformer");
    SetTargetFPS(60);
    
    Player p = {
        .pos = {100, 300},
        .vel = {0, 0},
        .w = 30,
        .h = 40,
        .grounded = false,
        .groundTime = 0
    };
    
    Platform platforms[] = {
        {{0, 500, 300, 20}},
        {{400, 450, 250, 20}},
        {{150, 350, 200, 20}},
        {{500, 300, 200, 20}},
        {{100, 200, 150, 20}},
        {{600, 200, 180, 20}},
        {{0, SCREEN_H - 20, SCREEN_W, 20}}
    };
    int platformCount = sizeof(platforms) / sizeof(Platform);
    
    Spike spikes[] = {
        {{320, 480}, 15},
        {{520, 430}, 15},
        {{670, 280}, 15},
        {{300, SCREEN_H - 40}, 15},
        {{500, SCREEN_H - 40}, 15},
        {{250, 330}, 15},
        {{350, 180}, 15}
    };
    int spikeCount = sizeof(spikes) / sizeof(Spike);
    
    Vector2 goalPos = {650, 150};
    float goalSize = 25;
    bool won = false;
    float timer = 0.0f;
    float winTime = 0.0f;
    
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // Update timer
        if (!won) {
            timer += dt;
        }
        
        // Input
        float inputX = 0;
        if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) inputX -= 1;
        if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) inputX += 1;
        
        // Horizontal movement
        float accel = p.grounded ? MOVE_ACCEL : AIR_ACCEL;
        p.vel.x += inputX * accel * dt;
        
        // Apply friction (reduced when on ground = slippery)
        if (p.grounded) {
            float friction = ICE_FRICTION;
            if (fabsf(p.vel.x) > friction * dt) {
                p.vel.x -= copysignf(friction * dt, p.vel.x);
            } else {
                p.vel.x = 0;
            }
        }
        
        // Clamp speed
        if (p.vel.x > MAX_SPEED) p.vel.x = MAX_SPEED;
        if (p.vel.x < -MAX_SPEED) p.vel.x = -MAX_SPEED;
        
        // Jump with coyote time
        bool jumpPressed = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
        if (jumpPressed && p.grounded) {
            p.vel.y = JUMP_FORCE;
            p.grounded = false;
        }
        
        // Gravity
        if (!p.grounded) {
            p.vel.y += GRAVITY * dt;
        }
        
        // Update position
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        
        // Collision detection
        Rectangle playerRect = {p.pos.x, p.pos.y, p.w, p.h};
        p.grounded = false;
        
        for (int i = 0; i < platformCount; i++) {
            if (CheckCollisionRecs(playerRect, platforms[i].rect)) {
                Rectangle overlap = GetCollisionRec(playerRect, platforms[i].rect);
                
                // Resolve collision
                if (overlap.width < overlap.height) {
                    // Horizontal collision
                    if (p.pos.x < platforms[i].rect.x) {
                        p.pos.x -= overlap.width;
                    } else {
                        p.pos.x += overlap.width;
                    }
                    p.vel.x = 0;
                } else {
                    // Vertical collision
                    if (p.pos.y < platforms[i].rect.y) {
                        p.pos.y -= overlap.height;
                        p.vel.y = 0;
                        p.grounded = true;
                    } else {
                        p.pos.y += overlap.height;
                        p.vel.y = 0;
                    }
                }
            }
        }
        
        if (p.grounded) {
            p.groundTime = 0;
        } else {
            p.groundTime += dt;
        }
        
        // Screen wrap
        if (p.pos.x > SCREEN_W) p.pos.x = -p.w;
        if (p.pos.x < -p.w) p.pos.x = SCREEN_W;
        
        // Check spike collision
        for (int i = 0; i < spikeCount; i++) {
            Vector2 playerCenter = {p.pos.x + p.w/2, p.pos.y + p.h/2};
            float dist = sqrtf(powf(playerCenter.x - spikes[i].pos.x, 2) + 
                             powf(playerCenter.y - spikes[i].pos.y, 2));
            if (dist < spikes[i].size + fminf(p.w, p.h)/2) {
                p.pos = (Vector2){100, 300};
                p.vel = (Vector2){0, 0};
                timer = 0.0f;
            }
        }
        
        // Check goal collision
        if (!won) {
            Vector2 playerCenter = {p.pos.x + p.w/2, p.pos.y + p.h/2};
            float dist = sqrtf(powf(playerCenter.x - goalPos.x, 2) + 
                             powf(playerCenter.y - goalPos.y, 2));
            if (dist < goalSize + fminf(p.w, p.h)/2) {
                won = true;
                winTime = timer;
            }
        }
        
        // Fall reset
        if (p.pos.y > SCREEN_H) {
            p.pos = (Vector2){100, 300};
            p.vel = (Vector2){0, 0};
            timer = 0.0f;
        }
        
        // Draw
        BeginDrawing();
        ClearBackground((Color){20, 30, 50, 255});
        
        // Draw platforms with ice effect
        for (int i = 0; i < platformCount; i++) {
            DrawRectangleRec(platforms[i].rect, (Color){150, 200, 255, 255});
            DrawRectangleLinesEx(platforms[i].rect, 2, (Color){200, 230, 255, 255});
            
            // Ice sparkles
            for (int j = 0; j < 3; j++) {
                float sparkleX = platforms[i].rect.x + (platforms[i].rect.width * (j + 1) / 4);
                float sparkleY = platforms[i].rect.y + 5;
                DrawCircle(sparkleX, sparkleY, 2, (Color){255, 255, 255, 180});
            }
        }
        
        // Draw spikes
        for (int i = 0; i < spikeCount; i++) {
            Vector2 tip = {spikes[i].pos.x, spikes[i].pos.y - spikes[i].size};
            Vector2 left = {spikes[i].pos.x - spikes[i].size, spikes[i].pos.y + spikes[i].size};
            Vector2 right = {spikes[i].pos.x + spikes[i].size, spikes[i].pos.y + spikes[i].size};
            DrawTriangle(left, tip, right, (Color){255, 60, 60, 255});
            DrawTriangleLines(left, tip, right, (Color){180, 0, 0, 255});
            // Add orange highlight
            Vector2 tipMid = {spikes[i].pos.x, spikes[i].pos.y - spikes[i].size * 0.6f};
            DrawTriangle(left, tipMid, right, (Color){255, 140, 0, 255});
        }
        
        // Draw goal star
        DrawPoly(goalPos, 5, goalSize, 0, GOLD);
        DrawPolyLines(goalPos, 5, goalSize, 0, YELLOW);
        
        // Draw player
        DrawRectangle(p.pos.x, p.pos.y, p.w, p.h, SKYBLUE);
        DrawRectangleLines(p.pos.x, p.pos.y, p.w, p.h, WHITE);
        
        // Draw velocity indicator
        if (fabsf(p.vel.x) > 10) {
            DrawLine(p.pos.x + p.w/2, p.pos.y + p.h/2, 
                    p.pos.x + p.w/2 + p.vel.x * 0.1f, p.pos.y + p.h/2,
                    RED);
        }
        
        // Instructions
        if (won) {
            int minutes = (int)winTime / 60;
            float seconds = winTime - (minutes * 60);
            DrawText(TextFormat("YOU WIN! Time: %d:%05.2f", minutes, seconds), SCREEN_W/2 - 180, SCREEN_H/2 - 20, 30, GOLD);
            DrawText("Press R to restart", SCREEN_W/2 - 100, SCREEN_H/2 + 20, 20, WHITE);
            if (IsKeyPressed(KEY_R)) {
                won = false;
                timer = 0.0f;
                p.pos = (Vector2){100, 300};
                p.vel = (Vector2){0, 0};
            }
        } else {
        
            DrawText("ARROW KEYS or WASD to move, SPACE to jump", 10, 35, 16, LIGHTGRAY);
            int minutes = (int)timer / 60;
            float seconds = timer - (minutes * 60);
            DrawText(TextFormat("Time: %d:%05.2f", minutes, seconds), 10, 60, 20, YELLOW);
        }
        
        EndDrawing();
    }
    
    CloseWindow();
    return 0;
}