//
//  main.c
//  rayCasting-c
//
//  Created by Juan Souza on 28/04/20.
//  Copyright © 2020 Juan Souza. All rights reserved.
//

#include <SDL2/SDL.h>
#include <stdio.h>
#include <limits.h>
#include "constants.h"

#include "textures.h"

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 ,1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 2, 2, 0, 3, 0, 4, 0, 5, 0, 6, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 5},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 5},
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 5},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 5, 5, 5}
};

struct Player {
    float x;
    float y;
    float width;
    float height;
    int turnDirection; // -1 for left, 1 for right
    int walkDirection; // -1 for back, 1 for front
    float rotationAngle;
    float walkSpeed;
    float turnSpeed;
} player;

struct Ray{
    float rayAngle;
    float wallHitX;
    float wallHitY;
    float distance;
    int wasHitVertical;
    int isRayFacingUp;
    int isRayFacingDown;
    int isRayFacingLeft;
    int isRayFacingRight;
    float wallHitContent;
} rays[NUM_RAYS];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;


int isGameRunning = FALSE;
int ticksLastFrame;

Uint32* colorBuffer = NULL;
SDL_Texture* colorBufferTexture;
Uint32* textures [NUM_TEXTURES];

int initializeWindow(){
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0){
        fprintf(stderr, "Error initialiazing SDL.\n");
        return FALSE;
    }
    window = SDL_CreateWindow(
                              "My Window",
                              SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED,
                              WINDOW_WIDTH,
                              WINDOW_HEIGHT,
                              SDL_WINDOW_BORDERLESS);
    
    if (!window){
        fprintf(stderr, "Error creating SDL window.\n");
        return FALSE;
    }
    
    renderer = SDL_CreateRenderer(window, -1, 0);
    if(!renderer){
        fprintf(stderr,"Error creating SDL renderer.\n");
        return FALSE;
    }
    
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    
    return TRUE;
}

void destroyWindow(){
    free(colorBuffer);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void setup(){
    player.x = WINDOW_WIDTH/2;
    player.y = WINDOW_HEIGHT/2;
    player.width = 5;
    player.height = 5;
    player.turnDirection = 0;
    player.turnSpeed = 100 * (PI/180);
    player.walkDirection = 0;
    player.rotationAngle = PI / 2;
    player.walkSpeed = 100;
    
    
    //Alocate the total amount of bytes to hold the color
    colorBuffer = (Uint32*) malloc(sizeof(Uint32) * (Uint32) WINDOW_WIDTH * (Uint32)WINDOW_HEIGHT);
    
    
    
    //create a SDL texture to display/hold the color buffer
    colorBufferTexture = SDL_CreateTexture(
                                           renderer,
                                           SDL_PIXELFORMAT_ARGB8888,
                                           SDL_TEXTUREACCESS_STREAMING,
                                           WINDOW_WIDTH,
                                           WINDOW_HEIGHT
                                           );
    
    
    //load some textures from the textures.h
    textures[0] = (Uint32*) REDBRICK_TEXTURE;
    textures[1] = (Uint32*) PURPLESTONE_TEXTURE;
    textures[2] = (Uint32*) MOSSYSTONE_TEXTURE;
    textures[3] = (Uint32*) GRAYSTONE_TEXTURE;
    textures[4] = (Uint32*) COLORSTONE_TEXTURE;
    textures[5] = (Uint32*) BLUESTONE_TEXTURE;
    textures[6] = (Uint32*) WOOD_TEXTURE;
    textures[7] = (Uint32*) EAGLE_TEXTURE;

}

int hasWallAt(float posX, float posY){
    if (posX < 0 || posX > WINDOW_WIDTH || posY < 0 || posY > WINDOW_HEIGHT){
        return TRUE;
    }
    int mapGridIndexX = floor( posX / TILE_SIZE);
    int mapGridIndexY = floor( posY / TILE_SIZE);

    return map[mapGridIndexY][mapGridIndexX] != 0;

}

void movePlayer(float deltaTime){
    player.rotationAngle += player.turnDirection * (player.turnSpeed * deltaTime);

    float moveStep = player.walkDirection * (player.walkSpeed * deltaTime);
    float newPlayerX = player.x + cos(player.rotationAngle) * moveStep;
    float newPlayerY = player.y + sin(player.rotationAngle) * moveStep;
    
   
    
    //TODO:
    //perform wall collision
    
    if(!hasWallAt(newPlayerX,newPlayerY)){
        player.x = newPlayerX;
        player.y = newPlayerY;
    }
}

void renderPlayer(){
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_Rect playerRect = {
        player.x * MINI_MAP_SCALE_FACTOR,
        player.y * MINI_MAP_SCALE_FACTOR,
        player.width * MINI_MAP_SCALE_FACTOR,
        player.height * MINI_MAP_SCALE_FACTOR
    };
    SDL_RenderFillRect(renderer, &playerRect);
    
    SDL_RenderDrawLine(renderer,
                       MINI_MAP_SCALE_FACTOR * player.x,
                       MINI_MAP_SCALE_FACTOR * player.y,
                       MINI_MAP_SCALE_FACTOR * player.x + cos(player.rotationAngle) * 40,
                       MINI_MAP_SCALE_FACTOR * player.y + sin(player.rotationAngle) * 40);
}

float normalizeAngle(float angle){
    angle = remainder(angle,TWO_PI);
    
    if(angle<0){
        angle = TWO_PI + angle;
    }
    return angle;
}

float distanceBetweenPoints(float x1,float y1,float x2,float y2){
    return sqrt((x2-x1)*(x2-x1) + (y2-y1) * (y2-y1));
}

void castRay(float rayAngle, int stripId){
    
    rayAngle = normalizeAngle(rayAngle);
    
    int isRayFacingDown = rayAngle > 0 && rayAngle < PI;
    int isRayFacingUp = !isRayFacingDown;

    int isRayFacingRight = rayAngle < PI/2 || rayAngle > 1.5 * PI;
    int isRayFacingLeft = !isRayFacingRight;
    
    float xIntersect, yIntersect;
    float xStep,yStep;
    
    ///////////////////////////////////////////
    // HORIZONTAL RAY-GRID INTERSECTION CODE
    ///////////////////////////////////////////
    
    int foundHozWallHit = FALSE;
    float horzHitX = 0;
    float horzHitY = 0;
    int horzWallContent = 0;
    
    yIntersect = floor(player.y/TILE_SIZE) * TILE_SIZE;
    yIntersect += isRayFacingDown ? TILE_SIZE : 0;
    
    xIntersect = player.x + ((yIntersect-player.y) /tan(rayAngle));
    
    yStep = TILE_SIZE;
    yStep *= isRayFacingUp ? -1 : 1;
    
    xStep = TILE_SIZE / tan(rayAngle);
    xStep *= (isRayFacingLeft && xStep > 0) ? -1 : 1;
    xStep *= (isRayFacingRight && xStep < 0) ? -1 : 1;
    
    float nextHorzTouchX = xIntersect;
    float nextHorzTouchY = yIntersect;
    
    // if(this.isRayFacingUp){
    //     nextHorzTouchY --;
    // }
    
    while(nextHorzTouchX >= 0 && nextHorzTouchX <= WINDOW_WIDTH && nextHorzTouchY >= 0 && nextHorzTouchY <= WINDOW_HEIGHT){
        
        float xToCheck = nextHorzTouchX;
        float yToCheck = nextHorzTouchY + (isRayFacingUp ? -1 : 0);
        
        if (hasWallAt(xToCheck,yToCheck)){
            foundHozWallHit = TRUE;
            horzHitX = nextHorzTouchX;
            horzHitY = nextHorzTouchY;
            horzWallContent = map[(int)floor(yToCheck/TILE_SIZE)][(int)floor(xToCheck/TILE_SIZE)];
            break;
        }else{
            nextHorzTouchX += xStep;
            nextHorzTouchY += yStep;
        }
    }
    
    ///////////////////////////////////////////
    // VERTICAL RAY-GRID INTERSECTION CODE
    ///////////////////////////////////////////
    
    int foundVerWallHit = FALSE;
    float verHitX = 0;
    float verHitY = 0;
    int verWallContent = 0;
    
    xIntersect = floor(player.x/TILE_SIZE) * TILE_SIZE;
    xIntersect += isRayFacingRight ? TILE_SIZE : 0;
    
    yIntersect = player.y + ((xIntersect-player.x) * tan(rayAngle));
    
    xStep = TILE_SIZE;
    xStep *= isRayFacingLeft ? -1 : 1;
    
    yStep = TILE_SIZE * tan(rayAngle);
    yStep *= (isRayFacingUp && yStep > 0) ? -1 : 1;
    yStep *= (isRayFacingDown && yStep < 0) ? -1 : 1;
    
    float nextVerTouchX = xIntersect;
    float nextVerTouchY = yIntersect;
    
    // if(this.isRayFacingUp){
    //     nextHorzTouchY --;
    // }
    
    while(nextVerTouchX >= 0 && nextVerTouchX <= WINDOW_WIDTH && nextVerTouchY >= 0 && nextVerTouchY <= WINDOW_HEIGHT){
        
        float xToCheck = nextVerTouchX + (isRayFacingLeft  ? -1 : 0);
        float yToCheck = nextVerTouchY;
        
        if (hasWallAt(xToCheck,yToCheck)){
            foundVerWallHit = TRUE;
            verHitX = nextVerTouchX;
            verHitY = nextVerTouchY;
            verWallContent = map[(int)floor(yToCheck/TILE_SIZE)][(int)floor(xToCheck/TILE_SIZE)];
            break;
        }else{
            nextVerTouchX += xStep;
            nextVerTouchY += yStep;
        }
    }
    
    //calculate both horizontal and vertical distance e choose the smallest one
    
    float horzHitDistance = foundHozWallHit
    ? distanceBetweenPoints(player.x,player.y,horzHitX,horzHitY)
    : INT_MAX;
    
    float vertHitDistance = foundVerWallHit
    ? distanceBetweenPoints(player.x,player.y,verHitX,verHitY)
    : INT_MAX;
    
    if(vertHitDistance < horzHitDistance) {
        rays[stripId].distance = vertHitDistance;
        rays[stripId].wallHitX = verHitX;
        rays[stripId].wallHitY = verHitY;
        rays[stripId].wallHitContent = verWallContent;
        rays[stripId].wasHitVertical = TRUE;
    }else{
        rays[stripId].distance = horzHitDistance;
        rays[stripId].wallHitX = horzHitX;
        rays[stripId].wallHitY = horzHitY;
        rays[stripId].wallHitContent = horzWallContent;
        rays[stripId].wasHitVertical = FALSE;
    }
    
    rays[stripId].rayAngle = rayAngle;
    rays[stripId].isRayFacingDown = isRayFacingDown;
    rays[stripId].isRayFacingUp = isRayFacingUp;
    rays[stripId].isRayFacingLeft = isRayFacingLeft;
    rays[stripId].isRayFacingRight = isRayFacingRight;

}



void castAllRays(){
    float rayAngle = player.rotationAngle -(FOV_ANGLE/2);
    
    for(int stripId = 0; stripId < NUM_RAYS; stripId ++){
        castRay(rayAngle, stripId);
        
        rayAngle += FOV_ANGLE / NUM_RAYS;
    }
}

void renderMap(){
    for (int i = 0; i < MAP_NUM_ROWS; i++ ){
        for(int j = 0; j < MAP_NUM_COLS; j++){
            int tileX = j * TILE_SIZE;
            int tileY = i * TILE_SIZE;
            int tileColor = map[i][j] != 0 ? 255 : 0;
            
            SDL_SetRenderDrawColor(renderer, tileColor, tileColor, tileColor, 255);
            SDL_Rect mapTileRect = {
                tileX * MINI_MAP_SCALE_FACTOR,
                tileY * MINI_MAP_SCALE_FACTOR,
                TILE_SIZE * MINI_MAP_SCALE_FACTOR,
                TILE_SIZE * MINI_MAP_SCALE_FACTOR
            };
            SDL_RenderFillRect(renderer, &mapTileRect);
        }
    }
}

void renderRays(){
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int i = 0; i < NUM_RAYS; i++){
        SDL_RenderDrawLine(
                           renderer,
                           MINI_MAP_SCALE_FACTOR * player.x,
                           MINI_MAP_SCALE_FACTOR * player.y,
                           MINI_MAP_SCALE_FACTOR * rays[i].wallHitX,
                           MINI_MAP_SCALE_FACTOR * rays[i].wallHitY);
    }
}

void processInput(){
    SDL_Event event;
    SDL_PollEvent(&event);
    
    switch(event.type){
        case SDL_QUIT:{
            isGameRunning = FALSE;
            break;
        }
        case SDL_KEYDOWN: {
            if (event.key.keysym.sym == SDLK_ESCAPE)
                isGameRunning = FALSE;
            if (event.key.keysym.sym == SDLK_UP)
                player.walkDirection = 1;
            if (event.key.keysym.sym == SDLK_DOWN)
                player.walkDirection = -1;
            if (event.key.keysym.sym == SDLK_RIGHT)
                player.turnDirection = 1;
            if (event.key.keysym.sym == SDLK_LEFT)
                player.turnDirection = -1;
            break;
        }
        case SDL_KEYUP: {
            if (event.key.keysym.sym == SDLK_UP)
                player.walkDirection = 0;
            if (event.key.keysym.sym == SDLK_DOWN)
                player.walkDirection = 0;
            if (event.key.keysym.sym == SDLK_RIGHT)
                player.turnDirection = 0;
            if (event.key.keysym.sym == SDLK_LEFT)
                player.turnDirection = 0;
            break;
        }
    }
}

void update(){
    int timeToWait = FRAME_TIME_LENGHT - (SDL_GetTicks() - ticksLastFrame);
    
    if (timeToWait > 0 && timeToWait <= FRAME_TIME_LENGHT){
        SDL_Delay(timeToWait);
    }
    
    float deltaTime = (SDL_GetTicks() - ticksLastFrame)/1000.0f;
    
    ticksLastFrame = SDL_GetTicks();
    
    movePlayer(deltaTime);
    castAllRays();
    
    
}

void generete3DProjection(){
    for (int i=0; i < NUM_RAYS;i++){
        
        float correctedDistance = rays[i].distance * cos(rays[i].rayAngle - player.rotationAngle);

        float distanceProjectionPlane = (WINDOW_WIDTH/2) / tan(FOV_ANGLE/2);
        
        int wallStripHeight = (TILE_SIZE/correctedDistance) * distanceProjectionPlane;
        
        int wallTopPixel = (WINDOW_HEIGHT/2) - (wallStripHeight/2);
        wallTopPixel = wallTopPixel < 0 ? 0 : wallTopPixel;

        int wallBottomPixel = (WINDOW_HEIGHT/2) + (wallStripHeight/2);
        wallBottomPixel = wallBottomPixel > WINDOW_HEIGHT ? WINDOW_HEIGHT : wallBottomPixel;
        
        int textureOffsetX;
    
        if (rays[i].wasHitVertical){
            textureOffsetX = (int)rays[i].wallHitY % TILE_SIZE;
        }else{
            textureOffsetX = (int)rays[i].wallHitX % TILE_SIZE;
        }
        
        //get the correnct texture id number from the map content
        int texNum = rays[i].wallHitContent - 1 ;
        
        //Cealing render
        for(int c = 0; c < wallTopPixel; c++){
            colorBuffer[(WINDOW_WIDTH * c + i)] = 0xFF5E5BA4;
        }
        
        //Walls Render
        for(int y = wallTopPixel; y < wallBottomPixel; y++){
            int distanceFromTop = (y + (wallStripHeight/2) -(WINDOW_HEIGHT / 2) );
            int textureOffsetY = distanceFromTop * ((float)TEXTURE_HEIGHT/wallStripHeight);
            //set the color of the wall based on the color of the walltexture
            Uint32 texelColor = textures[texNum][(TEXTURE_WIDTH * textureOffsetY) + textureOffsetX];
            colorBuffer[(WINDOW_WIDTH * y + i)] = texelColor;
        }
        
        //Floor Render
        for(int f = wallBottomPixel; f < WINDOW_HEIGHT; f++){
            colorBuffer[(WINDOW_WIDTH * f + i)] =  0xFFDCEDC1;
        }
        
        
        

    }
}

void clearColorBuffer(Uint32 color){
    for (int x = 0; x<WINDOW_WIDTH;x++){
        for(int y=0; y<WINDOW_HEIGHT;y++){
            colorBuffer[(WINDOW_WIDTH * y) + x ] = color;
        }
    }
}

void renderColorBuffer(){
    SDL_UpdateTexture(
                      colorBufferTexture,
                      NULL,
                      colorBuffer,
                      (int)((Uint32) WINDOW_WIDTH*sizeof(Uint32))
                      );
    SDL_RenderCopy(renderer, colorBufferTexture, NULL, NULL);
}

void render(){
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    
    generete3DProjection();
    
    renderColorBuffer();
    clearColorBuffer(0xFF000000);
    
    //display the miniMap
    renderMap();
    renderRays();
    renderPlayer();
    
    SDL_RenderPresent(renderer);
}

int main(){
    isGameRunning = initializeWindow();
    
    setup();
    
    while(isGameRunning){
        processInput();
        update();
        render();
    }
    
    destroyWindow();
    
    return 0;
}
