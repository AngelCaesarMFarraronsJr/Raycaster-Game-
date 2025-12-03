#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>
#include <math.h>
#include <time.h> // Added for time-based seeding of random number generator
#include "Textures/T_1.ppm"
#include "Textures/T_2.ppm"
#include "Textures/T_3.ppm"
#include "Textures/T_4.ppm"
#include "Textures/start.ppm"
#include "Textures/win.ppm"
#include "Textures/intro.ppm"
#include "Textures/key.ppm"
#define PI 3.1415926535
#define P2 PI/2
#define P3 3*PI/2
#define DR 0.0174533

float px, py, pdx, pdy, pa;

int keyStates[256] = {0};
int gameStarted = 0;
int gameWon = 0;
int hasKey = 0;
int gameIntro = 0;
int playerPassedExitCheck = 0;

float depthBuffer[1024];

typedef struct {
    float x;
    float y;
    int active;
} Sprite;

Sprite keySprite;

float degToRad(float a) { return a*PI/180.0; }
float FixAng(float a) { if(a>359){ a-=360;} if(a<0){ a+=360;} return a; }

float dist(float ax, float ay, float bx, float by)
{
    return sqrtf((bx-ax)*(bx-ax) + (by-ay)*(by-ay));
}

// Helper macro for magenta pixel filter (R=255, G=0, B=255)
#define CHECK_MAGENTA_PIXEL(r, g, b) (r == 255 && g == 0 && b == 255)
// Original check for white, used for textures that might use it for transparency
#define CHECK_WHITE_PIXEL(r, g, b) (r == 255 && g == 255 && b == 255)


void drawStartScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = start[pixel+0];
                int green = start[pixel+1];
                int blue  = start[pixel+2];
                
                // Keep the white filter for other screens if needed
                if(!CHECK_WHITE_PIXEL(red, green, blue)) 
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}

void drawIntroScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = intro[pixel+0];
                int green = intro[pixel+1];
                int blue  = intro[pixel+2];
                
                if(!CHECK_WHITE_PIXEL(red, green, blue)) // --- FILTER ---
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}

void drawWinScreen()
{
    int x, y;
    for(y=0; y<512; y++)
    {
        for(x=0; x<1024; x++)
        {
            int pixel = (y * 1024 + x) * 3;
            
            if(pixel >= 0 && pixel < 1024*512*3 - 2)
            {
                int red   = win[pixel+0];
                int green = win[pixel+1];
                int blue  = win[pixel+2];
                
                if(!CHECK_WHITE_PIXEL(red, green, blue)) // --- FILTER ---
                {
                    glPointSize(1);
                    glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                    glBegin(GL_POINTS);
                    glVertex2i(x, y);
                    glEnd();
                }
            }
        }
    }
}


// --- MAP & COLLISION ---

int mapX=16, mapY=16, mapS=64;
int map[]=
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
    1,0,1,0,0,1,0,1,1,1,0,1,0,1,0,1,
    1,0,1,0,0,0,0,1,0,0,0,0,0,1,0,1,
    1,0,1,1,1,1,0,1,0,1,1,1,1,1,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,1,
    1,1,1,1,0,1,1,0,1,1,0,1,1,0,1,1,
    1,0,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
    1,0,1,1,1,1,1,1,0,1,1,1,0,1,0,1,
    1,0,1,0,0,1,0,0,0,0,0,1,0,1,0,1,
    1,0,0,0,1,1,1,1,1,1,0,1,0,1,0,1,
    1,0,1,0,1,0,0,0,0,1,0,0,0,1,0,1,
    1,0,1,0,0,0,0,0,0,1,0,1,1,1,0,1,
    1,1,1,0,1,1,1,0,1,1,0,0,0,1,0,1,
    1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,4,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

int mapFloor[]=
{
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
};

int mapCeiling[]=
{
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
};

/**
 * Finds a random cell on the map that is walkable (map value 0)
 * and returns the center coordinates.
 * This is crucial for dynamic key placement.
 */
void findRandomEmptySpot(float *outX, float *outY) {
    int rx, ry, mp;
    int found = 0;
    while (!found) {
        // Generate random coordinates within the map bounds (1 to mapX-2, 1 to mapY-2)
        // to avoid corners and border walls (which are always 1).
        rx = (rand() % (mapX - 2)) + 1;
        ry = (rand() % (mapY - 2)) + 1;
        
        mp = ry * mapX + rx;
        
        // Check if the cell is walkable (value 0)
        // Ensure the player doesn't spawn on the key location either, 
        // which is done implicitly by checking for 0 (player start is in map[1][1]).
        if (map[mp] == 0) {
            *outX = rx * mapS + mapS / 2.0f;
            *outY = ry * mapS + mapS / 2.0f;
            found = 1;
        }
    }
}


int checkCollision(float x, float y)
{
    int mx = (int)(x) >> 6;
    int my = (int)(y) >> 6;
    int mp = my * mapX + mx;

    if(mp < 0 || mp >= mapX * mapY) return 1;

    if(map[mp] == 1) return 1;
    
    // Door (4) logic modified to set playerPassedExitCheck
    if(map[mp] == 4) {
        if(hasKey) {
            // Player has key and hits the exit door.
            playerPassedExitCheck = 1; 
            return 0; // Allowed to pass through
        } else {
            return 1; // Blocked
        }
    }

    return 0; // Open space (0)
}

void drawMinimap()
{
    int xo, yo;
    int minimapScale = 4;
    int minimapSize = mapX * minimapScale;
    
    for(yo=0; yo<mapY; yo++)
    {
        for(xo=0; xo<mapX; xo++)
        {
            int mp = yo * mapX + xo;
            if(map[mp] == 1) {
                glColor3f(1, 1, 1);
            } else if(map[mp] == 4) {
                if(hasKey) {
                    glColor3f(0.0f, 1.0f, 0.0f); // Green for unlocked on minimap
                } else {
                    glColor3f(1.0f, 0.0f, 0.0f); // Red for locked on minimap
                }
            } else {
                glColor3f(0, 0, 0);
            }
            
            glBegin(GL_QUADS);
            glVertex2i(xo * minimapScale, yo * minimapScale);
            glVertex2i((xo + 1) * minimapScale, yo * minimapScale);
            glVertex2i((xo + 1) * minimapScale, (yo + 1) * minimapScale);
            glVertex2i(xo * minimapScale, (yo + 1) * minimapScale);
            glEnd();
        }
    }
    
    if(keySprite.active)
    {
        // Calculate key position in minimap coordinates
        int keyMinimapX = (int)(keySprite.x * minimapScale) / mapS;
        int keyMinimapY = (int)(keySprite.y * minimapScale) / mapS;
        
        glColor3f(1, 0.84, 0); // Gold color for the key
        glPointSize(6);
        glBegin(GL_POINTS);
        glVertex2i(keyMinimapX, keyMinimapY);
        glEnd();
    }
    
    {
        int playerMinimapX = (int)(px * minimapScale) / mapS;
        int playerMinimapY = (int)(py * minimapScale) / mapS;
        
        glColor3f(1, 0, 0);
        glPointSize(4);
        glBegin(GL_POINTS);
        glVertex2i(playerMinimapX, playerMinimapY);
        glEnd();
        
        glColor3f(1, 0, 0);
        glLineWidth(1);
        glBegin(GL_LINES);
        glVertex2i(playerMinimapX, playerMinimapY);
        glVertex2i(playerMinimapX + (int)(pdx * minimapScale / 5), 
                   playerMinimapY + (int)(pdy * minimapScale / 5));
        glEnd();
    }
}

// --- RAYCASTING & RENDERING ---
void drawRays3D()
{
    int r,mx,my,mp,dof,y,i;
    float rx,ry,ra,xo,yo,disT;

    ra = pa - DR*30.0f;
    if(ra < 0) ra += 2*PI;
    if(ra > 2*PI) ra -= 2*PI;

    for(r=0;r<128;r++)
    {
        float disH, hx, hy;
        int hMapValue;
        float aTan;
        float disV, vx, vy;
        int vMapValue;
        float nTan;
        float ca;
        float lineH;
        float lineOff;
        float shade;
        int texX;
        int wallType;

        // Horizontal Check
        dof=0;
        disH=1e6f;
        hx=0;
        hy=0;
        hMapValue=0;
        aTan = -1.0f / tanf(ra);

        if(ra > PI) { ry = (((int)py>>6)<<6) - 0.0001f; rx = (py - ry) * aTan + px; yo = -64; xo = -yo * aTan; }
        else { ry = (((int)py>>6)<<6) + 64; rx = (py - ry) * aTan + px; yo = 64; xo = -yo * aTan; }
        if(fabsf(ra - 0.0f) < 1e-6 || fabsf(ra - PI) < 1e-6) { rx = px; ry = py; dof = 8; }
        while(dof < 8)
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                // If the ray hits a wall (1) OR a door (4), it stops and records the hit.
                if(map[mp] == 1 || map[mp] == 4) { 
                    hx = rx; hy = ry; disH = dist(px,py,hx,hy); 
                    hMapValue = map[mp];
                    dof = 8; 
                } else { // Ray passes through empty space (0)
                    rx += xo; ry += yo; dof++; 
                }
            } else { rx += xo; ry += yo; dof++; }
        }

        // Vertical Check
        dof = 0;
        disV = 1e6f;
        vx = px;
        vy = py;
        vMapValue=0;
        nTan = -tanf(ra);

        if(ra > P2 && ra < P3) { rx = (((int)px>>6)<<6) - 0.0001f; ry = (px - rx) * nTan + py; xo = -64; yo = -xo * nTan; }
        else { rx = (((int)px>>6)<<6) + 64; ry = (px - rx) * nTan + py; xo = 64; yo = -xo * nTan; }
        if(fabsf(ra - P2) < 1e-6 || fabsf(ra - P3) < 1e-6) { rx = px; ry = py; dof = 8; }
        while(dof < 8)
        {
            mx = (int)(rx) >> 6; my = (int)(ry) >> 6; mp = my * mapX + mx;
            if(mp >= 0 && mp < mapX*mapY) {
                 // If the ray hits a wall (1) OR a door (4), it stops and records the hit.
                if(map[mp] == 1 || map[mp] == 4) { 
                    vx = rx; vy = ry; disV = dist(px,py,vx,vy); 
                    vMapValue = map[mp];
                    dof = 8; 
                } else { // Ray passes through empty space (0)
                    rx += xo; ry += yo; dof++; 
                }
            } else { rx += xo; ry += yo; dof++; }
        }

        // Select the closest hit. wallType will be 4 if it hit the door.
        if(disV < disH) { rx = vx; ry = vy; disT = disV; wallType = vMapValue; shade = 1.0f; }
        else { rx = hx; ry = hy; disT = disH; wallType = hMapValue; shade = 0.7f; }

        ca = pa - ra;
        if(ca < 0) ca += 2*PI;
        if(ca > 2*PI) ca -= 2*PI;
        disT *= cosf(ca);
        
        for(i = r*8; i < (r+1)*8 && i < 1024; i++) {
            depthBuffer[i] = disT;
        }

        lineH = (mapS * 512.0f) / disT;
        if(lineH > 512) lineH = 512;
        lineOff = 256.0f - lineH/2.0f;

        // Draw Ceiling (T_3)
        for(y = 0; y < (int)lineOff; y++)
        {
            float dy = 256.0f - y;
            float ceilingDist;
            float wx, wy;
            int localX, localY;
            int ceilingTexX, ceilingTexY;
            int ceilingPixel;
            
            if(fabsf(dy) < 1e-6f) continue;
            
            float currentRa = ra;
            float caFix = cosf(pa - currentRa); 
            ceilingDist = (mapS * 512.0f) / (dy * 2.0f * caFix);
            
            wx = px + cosf(currentRa) * ceilingDist;
            wy = py + sinf(currentRa) * ceilingDist;
            
            localX = ((int)wx) % 32;
            localY = ((int)wy) % 32;
            if(localX < 0) localX += 32;
            if(localY < 0) localY += 32;
            
            ceilingTexX = localX;
            ceilingTexY = localY;
            
            ceilingPixel = (ceilingTexY * 32 + ceilingTexX) * 3;
            
            if(ceilingPixel >= 0 && ceilingPixel < 32*32*3 - 2)
            {
                int red   = T_3[ceilingPixel+0];
                int green = T_3[ceilingPixel+1];
                int blue  = T_3[ceilingPixel+2];
                
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glPointSize(8);
                glBegin(GL_POINTS);
                glVertex2i(r*8, y);
                glEnd();
            }
        }
        // End Draw Ceiling

        // Wall Texture Calculation
        if(disV < disH) {
            texX = ((int)ry) % 32;
            if(texX < 0) texX += 32;
            shade = 1.0f;
        } else {
            texX = ((int)rx) % 32;
            if(texX < 0) texX += 32;
            shade = 0.7f;
        }
        
        // Draw Wall
        for(y = 0; y < (int)lineH; y++)
        {
            int texY = (y * 32) / (int)lineH;
            int pixel;
            int red, green, blue;
            
            if(texY < 0) texY = 0;
            if(texY >= 32) texY = 31;
            if(texX < 0) texX = 0;
            if(texX >= 32) texX = 31;

            pixel = (texY * 32 + texX) * 3;
            if(pixel >= 0 && pixel < 32*32*3 - 2)
            {
                if(wallType == 4) {
                    // Door always draws T_4 regardless of key
                    red   = (int)(T_4[pixel+0] * shade);
                    green = (int)(T_4[pixel+1] * shade);
                    blue  = (int)(T_4[pixel+2] * shade);
                } else {
                    // Regular Wall (T_1)
                    red   = (int)(T_1[pixel+0] * shade);
                    green = (int)(T_1[pixel+1] * shade);
                    blue  = (int)(T_1[pixel+2] * shade);
                }

                glPointSize(8);
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glBegin(GL_POINTS);
                glVertex2i(r*8, (int)(y + lineOff));
                glEnd();
            }
        }

        // Draw Floor (T_2)
        for(y = (int)(lineH + lineOff); y < 512; y++)
        {
            float dy = y - 256.0f;
            float floorDist;
            float wx, wy;
            int localX, localY;
            int floorTexX, floorTexY;
            int floorPixel;
            
            if(fabsf(dy) < 1e-6f) continue;
            
            float currentRa = ra;
            float caFix = cosf(pa - currentRa);
            floorDist = (mapS * 512.0f) / (dy * 2.0f * caFix);
            
            wx = px + cosf(currentRa) * floorDist;
            wy = py + sinf(currentRa) * floorDist;
            
            localX = ((int)wx) % 32;
            localY = ((int)wy) % 32;
            if(localX < 0) localX += 32;
            if(localY < 0) localY += 32;
            
            floorTexX = localX;
            floorTexY = localY;
            
            floorPixel = (floorTexY * 32 + floorTexX) * 3;
            
            if(floorPixel >= 0 && floorPixel < 32*32*3 - 2)
            {
                int red   = T_2[floorPixel+0];
                int green = T_2[floorPixel+1];
                int blue  = T_2[floorPixel+2];
                
                glColor3ub((GLubyte)red, (GLubyte)green, (GLubyte)blue);
                glPointSize(8);
                glBegin(GL_POINTS);
                glVertex2i(r*8, y);
                glEnd();
            }
        }
        // End Draw Floor

        ra += DR*0.5f;
        if(ra < 0) ra += 2*PI;
        if(ra > 2*PI) ra -= 2*PI;
    }
    
}

void drawSprite(Sprite *s, const unsigned char *tex, int texWidth, int texHeight)
{
    float spx, spy;
    float spriteAngle;
    float spriteDist;
    float spriteHeight;
    float spriteWidth;
    int spriteScreenX;
    int startX, endX, startY, endY;
    int x, y;
    
    if(!s->active) return;
    
    spx = s->x - px;
    spy = s->y - py;
    
    spriteAngle = atan2f(spy, spx) - pa;
    while(spriteAngle < -PI) spriteAngle += 2*PI;
    while(spriteAngle > PI) spriteAngle -= 2*PI;
    
    if(spriteAngle < -PI/3 || spriteAngle > PI/3) return;
    
    spriteDist = sqrtf(spx*spx + spy*spy);
    if(spriteDist < 1) return;
    
    spriteHeight = (mapS * 512.0f) / spriteDist;
    if(spriteHeight > 512) spriteHeight = 512;
    
    // Maintain square aspect ratio
    spriteWidth = spriteHeight; 
    
    spriteScreenX = (int)(512 + (spriteAngle * 512 / (DR * 30.0f)));
    
    startX = spriteScreenX - (int)(spriteWidth / 2);
    endX = spriteScreenX + (int)(spriteWidth / 2);
    startY = 256 - (int)(spriteHeight / 2);
    endY = 256 + (int)(spriteHeight / 2);
    
    if(startX < 0) startX = 0;
    if(endX > 1024) endX = 1024;
    if(startY < 0) startY = 0;
    if(endY > 512) endY = 512;
    
    // Draw key shape
    for(x = startX; x < endX; x++)
    {
        if(x < 0 || x >= 1024) continue;
        
        // Depth check: Only draw the sprite if it is closer than the wall behind it
        if(x >= 0 && x < 1024 && spriteDist >= depthBuffer[x]) continue;
        
        for(y = startY; y < endY; y++)
        {
            // Normalize coordinates to 0-1 range
            float nx = (float)(x - startX) / spriteWidth;
            float ny = (float)(y - startY) / spriteHeight;
            
            int drawPixel = 0;
            
            // Key head
            float headCenterY = 0.25f;
            float headRadius = 0.15f;
            float distToHead = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - headCenterY)*(ny - headCenterY));
            if(distToHead < headRadius) drawPixel = 1;
            
            // Key hole)
            float holeCenterY = 0.25f;
            float holeRadius = 0.06f;
            float distToHole = sqrtf((nx - 0.5f)*(nx - 0.5f) + (ny - holeCenterY)*(ny - holeCenterY));
            if(distToHole < holeRadius) drawPixel = 0;
            
            // Key shaft 
            if(nx > 0.43f && nx < 0.57f && ny > 0.35f && ny < 0.75f) drawPixel = 1;
            
            // Key teeth
            if(nx > 0.43f && nx < 0.50f && ny > 0.70f && ny < 0.78f) drawPixel = 1;
            if(nx > 0.43f && nx < 0.50f && ny > 0.82f && ny < 0.90f) drawPixel = 1;
            
            if(drawPixel)
            {
                // Color
                glColor3f(1.0f, 0.84f, 0.0f);
                glPointSize(1);
                glBegin(GL_POINTS);
                glVertex2i(x, y);
                glEnd();
            }
        }
    }
}

void updateMovement()
{
    float walkSpeed = 3.0f;
    float rotSpeed = 0.05f;
    float newX, newY;
    int moved = 0;
    
    // Only update movement if the game is actively running (not intro, not win, not start)
    if(!gameStarted || gameWon || gameIntro) return;
    
    newX = px;
    newY = py;

    if(keyStates['a']) {
        pa -= rotSpeed;
        if(pa < 0) { pa += 2*PI; }
        pdx = cosf(pa)*5.0f;
        pdy = sinf(pa)*5.0f;
        moved = 1;
    }
    if(keyStates['d']) {
        pa += rotSpeed;
        if(pa > 2*PI) { pa -= 2*PI; }
        pdx = cosf(pa)*5.0f;
        pdy = sinf(pa)*5.0f;
        moved = 1;
    }

    if(keyStates['w']) {
        newX += cosf(pa) * walkSpeed;
        newY += sinf(pa) * walkSpeed;
        moved = 1;
    }
    if(keyStates['s']) {
        newX -= cosf(pa) * walkSpeed;
        newY -= sinf(pa) * walkSpeed;
        moved = 1;
    }

    if(!checkCollision(newX, newY))
    {
        px = newX;
        py = newY;
    }
    
    if(keySprite.active)
    {
        float distToKey = dist(px, py, keySprite.x, keySprite.y);
        if(distToKey < 20)
        {
            keySprite.active = 0;
            hasKey = 1;
        }
    }

    // Check if the player passed the exit check after moving
    if(playerPassedExitCheck)
    {
        gameWon = 1; // Immediately transition to win screen
    }
    
    if(moved || gameWon) {
        glutPostRedisplay();
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if(gameWon)
    {
        drawWinScreen();
    }
    else if(!gameStarted)
    {
        drawStartScreen();
    }
    else if(gameIntro)
    {
        drawIntroScreen();
    }
    // All escape/find screens removed, fall through to main game loop
    else // Main game loop (Maze + Minimap + Key/No Key Sprite)
    {
        // Draw the background color for the game area
        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(1024, 0);
        glVertex2i(1024, 512);
        glVertex2i(0, 512);
        glEnd();
        
        drawRays3D();
        drawSprite(&keySprite, NULL, 32, 32);
        drawMinimap();
    }
    
    glutSwapBuffers();
}

void keyDown(unsigned char key, int x, int y)
{
    if(gameWon)
    {
        // Reset game state
        gameWon = 0;
        gameStarted = 0;
        gameIntro = 0;
        hasKey = 0;
        playerPassedExitCheck = 0;
        
        // Reset player position
        px = 1 * mapS + mapS/2;
        py = 1 * mapS + mapS/2;
        pa = 0;
        pdx = cosf(pa) * 5.0f;
        pdy = sinf(pa) * 5.0f;
        
        // Reset key position randomly
        float keyX, keyY;
        findRandomEmptySpot(&keyX, &keyY);
        keySprite.x = keyX;
        keySprite.y = keyY;
        keySprite.active = 1;
        
        glutPostRedisplay();
        return;
    }
    
    if(!gameStarted)
    {
        // Transition from Start Screen to Intro Screen
        gameStarted = 1;
        gameIntro = 1; // Set intro state active
        glutPostRedisplay();
    }
    else if(gameIntro)
    {
        // Transition from Intro Screen directly to Game
        gameIntro = 0; // Set intro state inactive, entering main game loop
        glutPostRedisplay();
    }
    
    keyStates[key] = 1;
}

void keyUp(unsigned char key, int x, int y)
{
    keyStates[key] = 0;
}

void timer(int value)
{
    updateMovement();
    glutTimerFunc(16, timer, 0);
}

void resize(int w, int h)
{
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 1024, 512, 0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void init()
{
    glClearColor(0.3f, 0.3f, 0.3f, 0);
    gluOrtho2D(0, 1024, 512, 0);

    px = 1 * mapS + mapS/2;
    py = 1 * mapS + mapS/2;

    pa = 0;
    pdx = cosf(pa) * 5.0f;
    pdy = sinf(pa) * 5.0f;
    
    // Use the new function to place the key randomly
    float keyX, keyY;
    findRandomEmptySpot(&keyX, &keyY);
    
    keySprite.x = keyX;
    keySprite.y = keyY;
    keySprite.active = 1;
    hasKey = 0;
    playerPassedExitCheck = 0;
}

int main(int argc, char* argv[])
{
    // Seed the random number generator
    srand(time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(1024, 512);
    
    int screenWidth = glutGet(GLUT_SCREEN_WIDTH);
    int screenHeight = glutGet(GLUT_SCREEN_HEIGHT);
    int windowX = (screenWidth - 1024) / 2;
    int windowY = (screenHeight - 512) / 2;
    
    glutInitWindowPosition(windowX, windowY);
    glutCreateWindow("RayCaster");
    init();
    glutDisplayFunc(display);
    glutKeyboardFunc(keyDown);
    glutKeyboardUpFunc(keyUp);
    glutReshapeFunc(resize);
    glutTimerFunc(0, timer, 0);
    glutMainLoop();
    return 0;
}
