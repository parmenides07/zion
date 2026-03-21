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

int newBuffer(Package* curP);
int writeBuffer(Package* curP, int input);
void savePackage(Package* curP, PackageStorage* storray, Package* grid[][60]);
void drawBuffer(char* buffer, Vector2 curPos);
int hash(int x, int y, int size);
void expandHash(HashMap* hashmap);
void insertPackage(HashMap* map, Package* package);
Package* lookupPackage(HashMap* map, Vector2 location);
void deletePackage(HashMap* map, Vector2 location);

int main(void)
{
  InitWindow(800, 450, "zion");

  int key = 0;
  Package* packageGrid[60][60];
  Vector2 lastPosition = {0.0f,0.0f};
  Package curPack = {NULL,{0.0f,0.0f},{0,0},16};
  PackageStorage storage = {NULL, 0, 16};
  Vector2 currentPosition = {0.0f, 0.0f};

  if(newBuffer(&curPack) == 0)
    exit(1);
  //need to fill that null pointer with something before it starts.
  storage.storageArray = (Package**)malloc(sizeof(Package*)*storage.capacity);

  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground((Color){247, 246, 242, 255});
    drawBuffer(curPack.buffer, currentPosition);
    key = GetCharPressed();
    if (key > 0)
    {
        currentPosition = GetMousePosition();
        if (!(currentPosition.x == lastPosition.x || currentPosition.y == lastPosition.x))
        {
                //write functiont o save current buffer
            savePackage(&curPack, &storage, packageGrid);
            free(curPack.buffer);
            curPack.capacity = 16;
            curPack.size.x = 0;
            curPack.location = currentPosition;
            if(newBuffer(&curPack) == 0)
                exit(1);
        }
        if(writeBuffer(&curPack, key)==0)
            exit(1);
        lastPosition = currentPosition;
        //also draw at bottom left of screen
    }
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
  if(curP->size.x > (curP->capacity-1))
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
  curP->buffer[curP->size.x++] = (char)input;
    //remember null character must be put at the end of string to signify it has ended.
  curP->buffer[curP->size.x] = '\0';

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

void drawBuffer(char* buffer, Vector2 curPos)
{
  DrawText(buffer, curPos.x, curPos.y, 20, ((Color){51,46,45,255}));
  DrawText(buffer, 0, GetScreenHeight()-20, 20, ((Color){51,46,45,255}));
}

int hash(int x, int y, int size)
{
  return (x * 19 + y) % size;
}

void expandHash(HashMap* hashmap)
{
  hashmap->size *= 2;
  //cant do realloc i need to do malloc then free wiht a move in between
  Package** temp = (Package**)malloc(sizeof(Package*) * hashmap->count);
  if (temp == NULL)
    exit(1);

  Package** middleman = hashmap->hashArray;
  hashmap->hashArray = temp;

  for (int i = 0; i < hashmap->count; i++)
  {
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
    int i = 0;
    do while (i != index)
    {
      if (hashmap->hashArray[i] == NULL || hashmap->hashArray[i] == (Package*)1)
        hashmap->hashArray[i] = package;
        return;
      //this wraps around the array
      i = (i+1) % hashmap->count;
    }
    expandHash(hashmap);
    insertPackage(hashmap, package);
}

Package* lookupPackage(HashMap* hashmap, Vector2 location)
{

}

void deletePackage(HashMap* hashmap, Vector2 location)
{

}
