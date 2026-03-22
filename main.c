#define _POSIX_C_SOURCE 200809L
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOMBSTONE (Package*)1

typedef struct Package {
  char* buffer;
  // Actual world location
  Vector2 location;
  Vector2 size;
  int length;
  int capacity;
} Package;

typedef struct PackageStorage {
  Package** storageArray;
  int length;
  int capacity;
} PackageStorage;

typedef struct HashMap {
  Package** hashArray;
  int size;
  int count;
} HashMap;

/*
#define TEXTBOX ((Color){255,49,46,255})
#define BACKGROUND ((Color){27,31,118,255})
#define WORDS ((Color){249,224,217,255})
#define FONTIWANT "Heming.ttf"
*/

#define TEXTBOX ((Color){255,49,46,255})
#define BACKGROUND ((Color){27,31,118,255})
#define WORDS ((Color){249,224,217,255})
#define FONTIWANT "JBM.ttf"

Font myFont;

int newBuffer(Package* curP);
int writeBuffer(Package* curP, int input);
void savePackage(Package* curP, PackageStorage* storray, Package* grid[][60]);
void drawBuffer(char* buffer, Vector2 curPos, int cellSize);
int hash(int x, int y, int size);
void expandHash(HashMap* hashmap);
//cameraloc refers to the location or offset and cellsize is set by zooming
void daRenderer(PackageStorage* pacStor, Vector2 cameraLoc, int cellSize);
void insertPackage(HashMap* map, Package* package);
Package* lookupPackage(HashMap* map, Vector2 location);
void deletePackage(HashMap* map, Vector2 location);

int main(void)
{
  InitWindow(1200, 1200, "zion");
  myFont = LoadFontEx(FONTIWANT, 128, NULL, 0);
  SetTextureFilter(myFont.texture, TEXTURE_FILTER_POINT);

  int key = 0;
  Package* packageGrid[60][60];
  //Vector2 lastPosition = {0.0f,0.0f};
  Package curPack = {NULL,{0.0f,0.0f},{0,0}, 0, 16};
  PackageStorage storage = {NULL, 0, 16};
  Vector2 currentPosition = {0.0f, 0.0f};
  Vector2 cameraLocation = {0.0f, 0.0f};
  int cellsize = 20;

  if(newBuffer(&curPack) == 0)
    exit(1);
  //need to fill that null pointer with something before it starts.
  storage.storageArray = (Package**)malloc(sizeof(Package*)*storage.capacity);

  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground(BACKGROUND);

    //DO THIS FIRST GNG VVVVV


    //rendering is VERY SLOW AT HIGH STORED PACKAGES so maybe be nuanced with cals, if new package added just print that package, if mouse zooming or panning then call da renderer, otherwise DO NOT
    //and then always just call drawbuffer since its very low cost and neccesary


    daRenderer(&storage, cameraLocation, cellsize);
    drawBuffer(curPack.buffer, currentPosition, cellsize);
    key = GetCharPressed();

    if (key > 0)
    {
        //currentPosition.x = GetMousePosition().x / cellsize;
        //currentPosition.y = GetMousePosition().y / cellsize;
        //if (!(currentPosition.x == lastPosition.x || currentPosition.y == lastPosition.y))
        if(writeBuffer(&curPack, key)==0)
            exit(1);
        //lastPosition = currentPosition;
        //also draw at bottom left of screen
    }
    currentPosition.x = GetMousePosition().x / cellsize;
    currentPosition.y = GetMousePosition().y / cellsize;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
            //write functiont o save current buffer
        curPack.location = currentPosition;
        savePackage(&curPack, &storage, packageGrid);
        printf("storage length: %d\n", storage.length);
        free(curPack.buffer);
        curPack.capacity = 16;
        curPack.size.x = 0.0;
        curPack.size.y = 0.0;
        curPack.length = 0;
        if(newBuffer(&curPack) == 0)
            exit(1);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE))
    {
      cameraLocation.x -= GetMouseDelta().x / cellsize;
      cameraLocation.y -= GetMouseDelta().y / cellsize;
    }
    cellsize += GetMouseWheelMove() * 3;
    if(cellsize < 5)
      cellsize = 5;
    if(cellsize > 100)
      cellsize = 100;
    key = 0;
    EndDrawing();
   }

        CloseWindow();

        return 0;
    }

int newBuffer(Package* curP)
{
  curP->buffer = (char*)malloc(sizeof(char) * curP->capacity);

  if (curP->buffer == NULL)
  {
    printf("yo");
    return 0;
  }
  //writing [] signifies that we are trying to get the value so its like the same as doing *bufferLoc[]
  curP->buffer[0] = '\0';

  return 1;
}

int writeBuffer(Package* curP, int input)
{
  if(curP->length > (curP->capacity-1))
  {
    curP->capacity *= 2;
    char *temp = (char*)realloc(curP->buffer, curP->capacity);

    if(temp == NULL)
    {
        return 0;
    }


    curP->buffer = temp;
    //expand array to make room for hte next character
  }

    //note this will incriment length AFTER this line runs
  curP->buffer[curP->length++] = (char)input;
    //remember null character must be put at the end of string to signify it has ended.
  curP->buffer[curP->length] = '\0';

  return 1;
}

void savePackage(Package* curP, PackageStorage* storray, Package* grid[][60])
{
    Package* savePac = (Package*)malloc(sizeof(Package));

  if(savePac == NULL)
  {
    exit(1);
  }

  memcpy(savePac, curP, sizeof(Package));
  savePac->buffer = strdup(curP->buffer);

  if(storray->length > (storray->capacity - 1))
  {
    storray->capacity *= 2;
    Package** temp = (Package**)realloc(storray->storageArray, sizeof(Package*)*storray->capacity);

    if(temp == NULL)
    {
      exit(1);
    }

    storray->storageArray = temp;
  }
  storray->storageArray[storray->length] = savePac;
  storray->length++;

  //I need to fix mouse click location adn mapping but lets do this first
  for (int i = curP->location.x; i < (curP->location.x + curP->size.x); i++){
    for (int j = curP->location.y; j < (curP->location.y + curP->size.y); j++){
      //impliment expansion of these with realloc and malloc as it gets bigger but first for packageStorage array
      grid[i][j] =  savePac;
    }
  }
}

void drawBuffer(char* buffer, Vector2 curPos, int cellSize)
{
  //happy accident the whole kepeing the buffer drawn eaxctly where cursor is every second until they release is fire
  Vector2 mousePos = GetMousePosition();
  DrawRectangle(mousePos.x, mousePos.y - cellSize/15, MeasureTextEx(myFont, buffer, cellSize, 1).x, cellSize, TEXTBOX);
  DrawTextEx(myFont, buffer, mousePos, cellSize, 1, WORDS);

  Vector2 bottomPos = {0.0f, GetScreenHeight() - cellSize};
  DrawRectangle(bottomPos.x, bottomPos.y - cellSize/15, MeasureTextEx(myFont, buffer, cellSize, 1).x, cellSize, TEXTBOX);
  DrawTextEx(myFont, buffer, bottomPos, cellSize, 1, WORDS);
}

void daRenderer(PackageStorage* pacStor, Vector2 cameraLoc, int cellSize)
{
  Vector2 screenRelPos;
  for (int i = 0; i < pacStor->length; i++)
  {
    printf("rendering package: %s at %f %f\n", pacStor->storageArray[i]->buffer, cameraLoc.x, cameraLoc.y);
    screenRelPos.x = (pacStor->storageArray[i]->location.x - cameraLoc.x) * cellSize;
    screenRelPos.y = (pacStor->storageArray[i]->location.y - cameraLoc.y) * cellSize;

    //righgt edge greater than 0?, left edge grearter than horizontal max?
    if (screenRelPos.x + MeasureText(pacStor->storageArray[i]->buffer, cellSize) >= 0 && screenRelPos.x <= GetScreenWidth())
    {
      if (screenRelPos.y >= 0 && screenRelPos.y <= GetScreenHeight())
      {
        DrawRectangle(screenRelPos.x, screenRelPos.y - cellSize/15, MeasureTextEx(myFont, pacStor->storageArray[i]->buffer, cellSize, 1).x, cellSize, TEXTBOX);
        DrawTextEx(myFont, pacStor->storageArray[i]->buffer, screenRelPos, cellSize, 1, WORDS);
      }
    }
  }
}


int hash(int x, int y, int size)
{
  return (x * 19 + y) % size;
}

void expandHash(HashMap* hashmap)
{
  hashmap->size *= 2;
  //cant do realloc i need to do malloc then free wiht a move in between
  Package** temp = (Package**)malloc(sizeof(Package*) * hashmap->size);
  if (temp == NULL)
    exit(1);

  Package** middleman = hashmap->hashArray;
  hashmap->hashArray = temp;

  //use size since we want to go through the entire array(if we going through all we have to check null or tombstone to make sure we dont pull a nonexistant package)
  for (int i = 0; i < hashmap->size/2; i++)
  {
    if(!(middleman[i] == NULL || middleman[i] == TOMBSTONE))
      insertPackage(hashmap, middleman[i]);
  }
  free(middleman);
}


void insertPackage(HashMap* hashmap, Package* package)
{
    if (hashmap->count/0.7 >= hashmap->size)
    {
      expandHash(hashmap);
    }

    int index = hash(package->location.x, package->location.y, hashmap->size);
    //do while executes at least once
    int i = index;
    do
    {
      if (hashmap->hashArray[i] == NULL || hashmap->hashArray[i] == TOMBSTONE)
      {
        hashmap->hashArray[i] = package;
        hashmap->count++;
        return;
      }
      //this wraps around the array. notice how we use size since we want the full size not count
      i = (i+1) % hashmap->size;
    } while(i != index);
    expandHash(hashmap);
    insertPackage(hashmap, package);
}

Package* lookupPackage(HashMap* hashmap, Vector2 location)
{

    int index = hash(location.x, location.y, hashmap->size);
    //do while executes at least once
    int i = index;
    do
    {
      if(hashmap->hashArray[i] == NULL)
        return NULL;
      //need to check location to preven SEGFAULTING
      else if ((hashmap->hashArray[i]->location.x == location.x) && (hashmap->hashArray[i]->location.y == location.y))
        return hashmap->hashArray[i];
      //this wraps around the array. notice how we use size since we want the full size not count
      i = (i+1) % hashmap->size;
    } while(i != index);

    return NULL;
}

void deletePackage(HashMap* hashmap, Vector2 location)
{
  int index = hash(location.x, location.y, hashmap->size);
  //DOuu WHILE EXECUTESu AT least once
  int i = index;
  do
  {
    if(hashmap->hashArray[i] == NULL)
      return;
    else if ((hashmap->hashArray[i]->location.x == location.x) && (hashmap->hashArray[i]->location.y == location.y))
    {
      hashmap->hashArray[i] = (Package*)1;
      hashmap->count--;
      return;
    }
    //this wraps around the array. notice how we use size since we want the full size not count
    i = (i+1) % hashmap->size;
    } while(i != index);
}
