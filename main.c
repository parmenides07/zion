#define _POSIX_C_SOURCE 200809L
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOMBSTONE (Package*)1

typedef enum {
  CHILD,
  PARENT,
  SPOUSE,
}RelType;

typedef struct Relationship {
    RelType type;
    int ID;
    Vector2 relLocation;
}Relationship;

typedef struct Package {
  bool deleted;
  int length;
  char* buffer;
  int capacity;
  // Actual world location
  Vector2 location;
  Vector2 size;
  int id;
  int numRelationships;
  Relationship* relationships;
  int capacRelationships;
} Package;

typedef struct PackageMemory {
  Package** memoryArray;
  int length;
  int capacity;
} PackageMemory;

typedef struct ModifiedPackage {
  int* ids;
  int length;
  int capacity;
} ModifiedPackage;

typedef struct IndexEntry {
  int id;
  long fileOffset;
} IndexEntry;

typedef struct IndexArray {
  int length;
  IndexEntry* indexArray;
  int capacity;
} IndexArray;

typedef struct HashMapLoc {
  Package** hashArray;
  int size;
  int count;
} HashMapLoc;

typedef struct Node {
  Package* package; //no key needed bevayse package id is a key
  struct Node* next;
} Node;

typedef struct HashMapID {
  Node** buckets;
  int capacity;
  int length;
} HashMapID;

typedef struct IDPool {
    int* returnedIDs;
    int length;
    int capacity;
    int nextFresh;
} IDPool;

typedef struct Scope {
  Package** viewablePackages;
  int length;
  int capacity;
} Scope;

/*
#define TEXTBOX ((Color){255,49,46,255})
#define BACKGROUND ((Color){27,31,118,255})
#define WORDS ((Color){249,224,217,255})
#define FONTIWANT "Heming.ttf"
*/

#define TEXTBOX ((Color){242, 239, 233, 255})   // warm paper tone
#define BACKGROUND ((Color){70, 52, 48, 255})   // muted cocoa
#define WORDS ((Color){38, 32, 30, 255})        // deep ink
#define FONTIWANT "Heming.ttf"

IDPool newIDPool();
int acquireID(IDPool* pool);
void releaseID(IDPool* pool, int id);
int newBuffer(Package* curP);
int writeBuffer(Package* curP, int input);
void drawBuffer(char* buffer, Vector2 curPos, int cellSize) {
Package* lookupPackageByLocation(HashMapLoc* locmap, Vector2 location) {
Package* lookupPackageByID(int id, HashMapID* idmap) {
Package* buildPackageByID(IndexArray* indexArrayStruct, int id, PackageStorage* storageArrayStruct) {
void savePackageMemory(Package** packages, int numPackages, HashMapID* idmap) {
void savePackageHandler(Package* curP, PackageStorage* storray, HashMapLoc* locmap, HashMapID* idmap) {
void savePackageLocationMap(HashMapLoc* locmap, Package* package)
void saveModifiedPackage
void deletePackageByLocation(HashMapLoc* locmap, Vector2 location) {
void deletePackageByID(int ID) {
void daRenderer(PackageStorage* pacStor, Vector2 cameraLoc, int cellSize) {
int hashLoc(int x, int y, int capacity) {
void expandHashLoc(HashMapLoc* locmap) {
int hashID(int id, int capacity) {
void expandHashID(HashMapID* idmap) {
void pullindexes(IndexArray* indexArrayStruct) {
void diskSave(ModifiedPackage* modPack, IndexArray* indexArrayStruct, HashMapID* idmap) {
int getIDIndexForModPackages(IndexArray* indexArrayStruct, int id) {
void packageFileWrite(FILE* file, Package* package) {
void indexFileWrite(FILE* file, IndexArray* indexArr) {

Font myFont;

int main(void) {
  InitWindow(1200, 1200, "zion");
  myFont = LoadFontEx(FONTIWANT, 128, NULL, 0);
  SetTextureFilter(myFont.texture, TEXTURE_FILTER_POINT);

  int key = 0;
  //Vector2 lastPosition = {0.0f,0.0f};
  Package curPack = {false, NULL,{0.0f,0.0f},{0,0}, 0, 16, 0, NULL};
  PackageStorage storage = {NULL, 0, 16};
  Vector2 currentPosition = {0.0f, 0.0f};
  Vector2 cameraLocation = {0.0f, 0.0f};
  IDPool idPool = newIDPool();
  int cellsize = 20;

  if(newBuffer(&curPack) == 0)
    exit(1);
  //need to fill that null pointer with something before it starts.
  storage.storageArray = (Package**)malloc(sizeof(Package*)*storage.capacity);



// RENAMAE PACKAGESTORAGE TO PACKAGE MEMORY all working packages. MAKE IT A HASHMAP BY INDEX



  HashMapLoc hashmaploc = {NULL, 16, 0}
  hashmaploc.hashArray = (Package**)calloc(hashmaploc.size, sizeof(Package*));

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BACKGROUND);
    DrawFPS(0,10);
    daRenderer(&storage, cameraLocation, cellsize);
    drawBuffer(curPack.buffer, currentPosition, cellsize);
    key = GetCharPressed();

    if (key > 0) {
        if(writeBuffer(&curPack, key)==0)
            exit(1);
    }
    currentPosition.x = GetMousePosition().x / cellsize;
    currentPosition.y = GetMousePosition().y / cellsize;
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            //write functiont o save current buffer
        curPack.location = currentPosition;
        savePackage(&curPack, &storage, &hashmap, &idPool);
        free(curPack.buffer);
        curPack.capacity = 16;
        curPack.size.x = 0.0;
        curPack.size.y = 0.0;
        curPack.length = 0;
        if(newBuffer(&curPack) == 0)
            exit(1);
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
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

//ID Functions---------------------------------------------------------------------------------------------
IDPool newIDPool() {
    IDPool pool;
    pool.returnedIDs = (int*)malloc(sizeof(int) * 16);
    pool.length = 0;
    pool.capacity = 16;
    pool.nextFresh = 0;
    return pool;
}

int acquireID(IDPool* pool) {
    if (pool->length > 0)
        return pool->returnedIDs[--pool->length];

    return pool->nextFresh++;
}

void releaseID(IDPool* pool, int id) {
    if (pool->length >= pool->capacity) {
        pool->capacity *= 2;
        int* temp = (int*)realloc(pool->returnedIDs, sizeof(int) * pool->capacity);
        if (temp == NULL)
            exit(1);
        pool->returnedIDs = temp;
    }
    pool->returnedIDs[pool->length++] = id;
}

//Buffer Functions---------------------------------------------------------------------------------------------
int newBuffer(Package* curP) {
  curP->buffer = (char*)malloc(sizeof(char) * curP->capacity);

  if (curP->buffer == NULL) {
    printf("yo");
    return 0;
  }
  //writing [] signifies that we are trying to get the value so its like the same as doing *bufferLoc[]
  curP->buffer[0] = '\0';

  return 1;
}

int writeBuffer(Package* curP, int input) {
  if(curP->length > (curP->capacity-1)) {
    curP->capacity *= 2;
    char *temp = (char*)realloc(curP->buffer, curP->capacity);

    if(temp == NULL) {
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

void drawBuffer(char* buffer, Vector2 curPos, int cellSize) {
  //happy accident the whole kepeing the buffer drawn eaxctly where cursor is every second until they release is fire
  Vector2 mousePos = GetMousePosition();
  DrawRectangle(mousePos.x, mousePos.y - cellSize/15, MeasureTextEx(myFont, buffer, cellSize, 1).x, cellSize, TEXTBOX);
  DrawTextEx(myFont, buffer, mousePos, cellSize, 1, WORDS);

  Vector2 bottomPos = {0.0f, GetScreenHeight() - cellSize};
  DrawRectangle(bottomPos.x, bottomPos.y - cellSize/15, MeasureTextEx(myFont, buffer, cellSize, 1).x, cellSize, TEXTBOX);
  DrawTextEx(myFont, buffer, bottomPos, cellSize, 1, WORDS);
}

//Package Search-----------------------------------------------------------------------------------------------
Package* lookupPackageByLocation(HashMapLoc* locmap, Vector2 location) {
    int index = hashLoc(location.x, location.y, locmap->size);
    //do while executes at least once
    int i = index;
    do {
      if(locmap->hashArray[i] == NULL)
        return NULL;
      //need to check location to preven SEGFAULTING
      else if ((locmap->hashArray[i]->location.x == location.x) && (locmap->hashArray[i]->location.y == location.y))
        return locmap->hashArray[i];
      //this wraps around the array. notice how we use size since we want the full size not count
      i = (i+1) % locmap->size;
    } while(i != index);

    return NULL;
}

Package* lookupPackageByID(int id, HashMapID* idmap) {
  index = hashID(id, idmap->capacity);

  Node* currNode = idmap->buckets[index];
  while (currNode != NULL) {
    if (currNode->package->id == id) {
      return currNode->package;
    }
    currNode = currNode->next;
  }
  return NULL;
}

Package* buildPackageByID(IndexArray* indexArrayStruct, int id) {
  FILE* data = fopen("data.zn", "rb");

  int index = getIDIndexForModPackages(indexArrayStruct, id);
  if (index < 0)
    return NULL; //essnetially if there is no package with that id found return null
  int fileOffset = indexArrayStruct->indexArray[index].fileOffset;
  fseek(data, fileOffset, SEEK_SET); //essentialyl start at begnning and then go down the amount of file offset

  Package* package = (Package*)malloc(sizeof(Package));
  if (package == NULL)
    exit(1);
  fread(&package->deleted, sizeof(bool), 1, data);

  fread(&package->length, sizeof(int), 1, data);
  char* bufferAll = (char*)malloc(sizeof(char)*package->length);
  if (bufferAll == NULL)
    exit(1);
  package->buffer = bufferAll;
  fread(package->buffer, sizeof(char)* package->length, 1, data); //NOTE NO &, it is already a pointer
  fread(&package->capacity, sizeof(int), 1, data);

  fread(&package->location, sizeof(Vector2), 1, data);
  fread(&package->size, sizeof(Vector2), 1, data);
  fread(&package->id, sizeof(int), 1, data);

  fread(&package->numRelationships, sizeof(int), 1, data);
  Relationship* relArrayAll = (Relationship*)malloc(sizeof(Relationship)*package->numRelationships);//array of relationshios (NOT POINTER TO ARRAY which would be **Relationship)
  //we can do sizeofrelationship times number because every relationship struct is the same size there is no variable lengths in it
  if (relArrayAll == NULL)
    exit(1);
  package->relationships = relArrayAll;
  fread(package->relationships, sizeof(Relationship) * package->numRelationships, 1, data); //no & same reason as above
  fread(&package->capacRelationships, sizeof(int), 1, data);

  return package;
}

//Package Storage Functions---------------------------------------------------------------------------------------------------------
void savePackageMemory(Package** packages, int numPackages, HashMapID* idmap) {
  if (idmap->length /(float)idmap->capacity > 0.7) //if exceed load factor just make it bigger. also integer division truncation
    expandHashID(idmap);

  int index;
  for(int i = 0; i < numPackages; i++) {
    index = hashID(packages[i]->id, idmap->capacity);
    if(lookupPackageByID(packages[i]->id, idmap) == NULL) continue;

    Node* newNode = malloc(sizeof(Node));
    newNode->package = packages[i];
    newNode->next = idmap->buckets[index]; // point to current head
    idmap->buckets[index] = newNode;       // new node becomes head
  }
}

void savePackageHandler(Package* curP, PackageStorage* storray, HashMapLoc* locmap, HashMapID* idmap, IDPool* pool) {
    Package* savePac = (Package*)malloc(sizeof(Package));
    if (savePac == NULL)
        exit(1);

    memcpy(savePac, curP, sizeof(Package));
    savePac->buffer = strdup(curP->buffer); // why strdup i thought we copied curp?
    savePac->id = acquireID(pool);

    Package** packagePass = [package];
    savePackageMemory(packagePass, 1, idmap);

    savePackageLocationMap(locmap, savePac);
}

void savePackageLocationMap(HashMapLoc* locmap, Package* package) {
    if (locmap->count/0.7 >= locmap->size) {
      expandHashLoc(locmap);
    }

    int index = hashLoc(package->location.x, package->location.y, locmap->size);
    //do while executes at least once
    int i = index;
    do {
      if (locmap->hashArray[i] == NULL || locmap->hashArray[i] == TOMBSTONE) {
        locmap->hashArray[i] = package;
        locmap->count++;
        return;
      }
      //this wraps around the array. notice how we use size since we want the full size not count
      i = (i+1) % locmap->size;
    } while(i != index);
    expandHashLoc(locmap);
}

//for the below rather than storing pointer to package could we just use the straight up id, that would be better no?
//we dont even need to really sort this by id either, Only add it if that id already dosent exist in the array.
void saveModifiedPackage
//Package Modification Functions-------------------------------------------------------------------------------------
void deletePackageByLocation(HashMapLoc* locmap, Vector2 location) {
  int index = hashLoc(location.x, location.y, locmap->size);
  //DOuu WHILE EXECUTESu AT least once
  int i = index;
  do {
    if(locmap->hashArray[i] == NULL)
      return;
    else if ((locmap->hashArray[i]->location.x == location.x) && (locmap->hashArray[i]->location.y == location.y)) {
      locmap->hashArray[i] = (Package*)1;
      locmap->count--;
      return;
    }
    //this wraps around the array. notice how we use size since we want the full size not count
    i = (i+1) % locmap->size;
    } while(i != index);
}

void deletePackageByID(int ID) {
  shoudl essentially be a lookup by ID and get pointer to package
  and then add that package pointer to modified packages. with bool deleted. HAVE THIS BE HANDLED IN the disksaving functions
}


//Renderer Functions---------------------------------------------------------------------------------------------
void daRenderer(Scope* packages, Vector2 cameraLoc, int cellSize) {
  Vector2 screenRelPos;
  for (int i = 0; i < packages->length; i++) {
    //printf("rendering package: %s at %f %f\n", pacStor->storageArray[i]->buffer, cameraLoc.x, cameraLoc.y);
    if (packages->viewablePackages[i] == NULL)  // add this
        continue;

    screenRelPos.x = (packages->viewablePackages[i]->location.x - cameraLoc.x) * cellSize;
    screenRelPos.y = (packages->viewablePackages[i]->location.y - cameraLoc.y) * cellSize;

    //righgt edge greater than 0?, left edge grearter than horizontal max?
    if (screenRelPos.x + MeasureText(packages->viewablePackages[i]->buffer, cellSize) >= 0 && screenRelPos.x <= GetScreenWidth()) {
      if (screenRelPos.y >= 0 && screenRelPos.y <= GetScreenHeight()) {
        DrawRectangle(screenRelPos.x, screenRelPos.y - cellSize/15, MeasureTextEx(myFont, packages->viewablePackages[i]->buffer, cellSize, 1).x, cellSize, TEXTBOX);
        DrawTextEx(myFont, packages->viewablePackages[i]->buffer, screenRelPos, cellSize, 1, WORDS);
      }
    }
  }
}

//Hash Functions---------------------------------------------------------------------------------------------
int hashLoc(int x, int y, int capacity) {
  return (x * 19 + y) % capacity;
}

void expandHashLoc(HashMapLoc* locmap) {
  locmap->size *= 2;
  //cant do realloc i need to do malloc then free wiht a move in between
  Package** temp = (Package**)malloc(sizeof(Package*) * locmap->size);
  if (temp == NULL)
    exit(1);

  Package** middleman = locmap->hashArray;
  locmap->hashArray = temp;

  //use size since we want to go through the entire array(if we going through all we have to check null or tombstone to make sure we dont pull a nonexistant package)
  for (int i = 0; i < locmap->size/2; i++) {
    if(!(middleman[i] == NULL || middleman[i] == TOMBSTONE))
      savePackageLocationMap(locmap, middleman[i])
  }
  free(middleman);
}

int hashID(int id, int capacity) {
  return id % capacity;
}

void expandHashID(HashMapID* idmap) {
  idmap->capacity *= 2;

  Node** newBuckets = malloc(sizeof(Node*) * idmap->capacity);
  if (newBuckets == NULL)
    exit(1);
  // very cool logic so we have for loop to iterate through every bucket and then while loop to go till it hits end
  for (int i = 0; i < idmap->capacity/2; i++) {
    Node* currNode = idmap->buckets[i];
    while (currNode != NULL) {
      Node* nextNode = currNode->next;
      int newIndex = hashID(currNode->package->id, idmap->capacity);

      currNode->next = newBuckets[newIndex];
      newBuckets[newIndex] = currNode;

      currNode = nextNode;
    }
  }
  free(idmap->buckets);
  idmap->buckets = newBuckets;
}

//Data Functions---------------------------------------------------------------------------------------------
void pullindexes(IndexArray* indexArrayStruct) {
  FILE* index = fopen("index.zn", "rb");
  if (index == NULL) {
    indexArrayStruct->length = 0;
    indexArrayStruct->indexArray = malloc(sizeof(IndexEntry) * 16);
    indexArrayStruct->capacity = 16;
    return;
  }
  else {
    if (fread(&indexArrayStruct->length, sizeof(int), 1, index) != 1) {
        exit(-1);
    }
    indexArrayStruct->indexArray = (IndexEntry*)(malloc(sizeof(IndexEntry) * indexArrayStruct->length));
    //fread(destination, sizeOfEachElement, numberOfElements, file);
    fread(indexArrayStruct->indexArray, sizeof(IndexEntry), indexArrayStruct->length, index);
    fclose(index);

    indexArrayStruct->capacity = indexArrayStruct->length;
  }
}

void diskSave(ModifiedPackage* modPack, IndexArray* indexArrayStruct, HashMapID* idmap) {
  int fileIndex;
  FILE* dataPtr;
  long newFileOffset;
  FILE* data = fopen("data.zn", "ab+");
  int deleted = 1;
  int insertIndex;
  for (int i = 0; i < modPack->length; i++) {
    fseek(data, 0, SEEK_END);
    newFileOffset = ftell(data);

    //handle deleted pakcages that were deleted during that session meaning their deleted flag would already be flipped to 1.



    packageFileWrite(dataPtr, lookupPackageByID(modPack->ids[i], idmap));
    int modPackageIndexInID = getIDIndexForModPackages(indexArrayStruct, modPack->ids[i]);
    if (modPackageIndexInID >= 0) {
      fseek(data, indexArrayStruct->indexArray[modPackageIndexInID].fileOffset, SEEK_SET);
      fwrite(&deleted, sizeof(int), 1, data);
      indexArrayStruct->indexArray[modPackageIndexInID].fileOffset = newFileOffset;
    }
    else {
      insertIndex = -modPackageIndexInID-1;
      if (indexArrayStruct->length + 1 > indexArrayStruct->capacity) {
        indexArrayStruct->capacity *= 2;
        IndexEntry* temp = (IndexEntry*)realloc(indexArrayStruct->indexArray, sizeof(IndexEntry*) * indexArrayStruct->capacity);
        if (temp == NULL)
          exit(-1);
        indexArrayStruct->indexArray = temp;
      }
      for(int j = indexArrayStruct->length-1; j > insertIndex; j--) {
        indexArrayStruct->indexArray[j] = indexArrayStruct->indexArray[j-1];
      } // we just moved everything forward one to make space for that one at insertIndex
      IndexEntry newEntry = {modPack->ids[i], newFileOffset};
      indexArrayStruct->indexArray[insertIndex] = newEntry;
    }
  }
  fclose(data);
  FILE* index = fopen("index.zn", "wb");
  indexFileWrite(index, indexArrayStruct);
  fclose(index);
}

int getIDIndexForModPackages(IndexArray* indexArrayStruct, int id) {
  int low = 0;
  int high = indexArrayStruct->length - 1;
  int mid;

  while (low <= high) {
    mid = (low + high) / 2;

    if (indexArrayStruct->indexArray[mid].id == id)
        return mid;
    else if (indexArrayStruct->indexArray[mid].id < id)
        low = mid + 1;
    else
        high = mid - 1;
  }
  return -(low+1);
}

void packageFileWrite(FILE* file, Package* package) {
  fwrite(&package->deleted, sizeof(bool), 1, file);
  fwrite(&package->length, sizeof(int), 1, file);
  fwrite(package->buffer, sizeof(char)*package->length, 1, file); //NOTE NO &, it is already a pointer
  fwrite(&package->capacity, sizeof(int), 1, file);
  fwrite(&package->location, sizeof(Vector2), 1, file);
  fwrite(&package->size, sizeof(Vector2), 1, file);
  fwrite(&package->id, sizeof(int), 1, file);
  fwrite(&package->numRelationships, sizeof(int), 1, file);
  //same reason we had to write out packages, we also have to individually write out relationships struct
  fwrite(package->relationships, sizeof(Relationship) * package->numRelationships, 1, file); //no & same reason as above
  fwrite(&package->capacRelationships, sizeof(int), 1, file);
}
//oh no wait do i have to dereference relationship struct and write out those individual components????!!!

void indexFileWrite(FILE* file, IndexArray* indexArr) {
  fwrite(&indexArr->length, sizeof(int), 1, file); // & needed we need address to pass as pointer
  fwrite(indexArr->indexArray, sizeof(IndexEntry) * indexArr->length, 1, file); //already a pointer no & needed
  fwrite(&indexArr->capacity, sizeof(int), 1, file); // same as length
}

