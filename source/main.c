#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <math.h>

#define TOP_WIDTH      400
#define BOT_WIDTH      320
#define SCREEN_HEIGHT  240

// Vehicle State & Physics Parameters
typedef struct {
    float posX, posY;
    float speed;
    float steeringAngle; // Range: -35.0 to +35.0 degrees
    int gear;            // -1: Reverse, 0: Neutral, 1: Drive
    float rpm;
} Vehicle;

int main(int argc, char* argv[]) {
    // Enable New 3DS 804MHz CPU clock speedup & L2 cache
    osSetSpeedupEnable(true);

    // Initialize 3DS Graphics Libraries
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create rendering targets for Dual Screens
    C3D_RenderTarget* topTarget = C2D_CreateTarget(TOP_WIDTH, SCREEN_HEIGHT, GFX_TOP, GFX_LEFT);
    C3D_RenderTarget* botTarget = C2D_CreateTarget(BOT_WIDTH, SCREEN_HEIGHT, GFX_BOTTOM, GFX_LEFT);

    // Colors
    u32 clrSky      = C2D_Color32(135, 206, 235, 255);
    u32 clrRoad     = C2D_Color32(50, 50, 50, 255);
    u32 clrLane     = C2D_Color32(255, 255, 255, 255);
    u32 clrCar      = C2D_Color32(230, 40, 40, 255);
    u32 clrDash     = C2D_Color32(30, 30, 35, 255);
    u32 clrWheel    = C2D_Color32(80, 80, 80, 255);
    u32 clrPedalGas = C2D_Color32(40, 180, 40, 255);
    u32 clrPedalBrk = C2D_Color32(180, 40, 40, 255);

    Vehicle car = { .posX = 200.0f, .posY = 0.0f, .speed = 0.0f, .steeringAngle = 0.0f, .gear = 1, .rpm = 800.0f };
    touchPosition touch;

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if (kDown & KEY_START) break; // Exit to Homebrew Launcher

        // --- TOUCHSCREEN CONTROL ENGINE ---
        if (kHeld & KEY_TOUCH) {
            hidTouchRead(&touch);

            // Left side touch zone: Steering Wheel Math (Center at X=80, Y=140)
            if (touch.px < 160) {
                float dx = (float)touch.px - 80.0f;
                float dy = (float)touch.py - 140.0f;
                car.steeringAngle = (dx / 80.0f) * 35.0f;
            }
            
            // Right side touch zone: Gas / Brake Pedals
            if (touch.px >= 200) {
                if (touch.py > 130) { 
                    // Accelerator pedal pressed
                    if (car.gear == 1) car.speed += 0.08f;      // Drive forward
                    else if (car.gear == -1) car.speed -= 0.04f; // Reverse
                } else if (touch.py <= 130 && touch.py > 50) {
                    // Brake pedal pressed
                    if (car.speed > 0) car.speed -= 0.15f;
                    if (car.speed < 0) car.speed += 0.15f;
                }
            }
        } else {
            // Auto-center steering when touch is released
            car.steeringAngle *= 0.85f;
        }

        // Apply friction decay
        car.speed *= 0.98f;
        car.posX += (car.steeringAngle * 0.05f) * (car.speed * 0.5f);

        // --- RENDER TOP SCREEN (3D Perspective View) ---
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C2D_TargetClear(topTarget, clrSky);
        C2D_SceneBegin(topTarget);

        // Road rendering
        C2D_DrawRectSolid(60, 100, 0, 280, 140, clrRoad);
        
        // Dynamic center road lines based on car position
        float lineX = 200.0f - (car.posX - 200.0f);
        C2D_DrawRectSolid(lineX - 2, 100, 0, 4, 140, clrLane);

        // Player Vehicle Body
        C2D_DrawRectSolid(185, 170, 0, 30, 45, clrCar);

        // --- RENDER BOTTOM SCREEN (Cockpit & Dashboard Controls) ---
        C2D_TargetClear(botTarget, clrDash);
        C2D_SceneBegin(botTarget);

        // Steering Wheel Representation (Left Side)
        C2D_DrawCircleSolid(80 + (car.steeringAngle * 0.8f), 140, 0, 45, clrWheel);

        // Pedals (Right Side)
        C2D_DrawRectSolid(210, 60, 0, 40, 60, clrPedalBrk); // Brake
        C2D_DrawRectSolid(260, 130, 0, 40, 80, clrPedalGas); // Gas

        C3D_FrameEnd(0);
    }

    // Cleanup & Exit
    C2D_Fini();
    C3D_Fini();
    gfxExit();
    return 0;
}
