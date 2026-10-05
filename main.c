#define _POSIX_C_SOURCE 200809L
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <unistd.h>

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

#define TEXTBOX    ((Color){207, 204, 194, 255}) // #CFCCC2
#define BACKGROUND ((Color){52, 57, 60, 255})    // #34393C
#define WORDS      ((Color){58, 68, 68, 255})    // #3A4444
#define WRITINGBG BACKGROUND
#define SELECTION WORDS
#define FONTIWANT "SpaceMono-Regular.ttf"
#define MAXSELECTIONS 64


/*IDPool newIDPool();
int acquireID(IDPool* pool);
void releaseID(IDPool* pool, int id); */
void newBufferRelationships(Package* curP);
int writeBuffer(Package* curP, int input);
void backspaceBuffer(Package* package);
void drawBuffer(char* buffer, Vector2 curPos, int cellSize);
void drawSelected(char* buffer, Vector2 curPos, int cellSize);
Rectangle textBounds(const char* buffer, Vector2 position, int cellSize);
Package* packageAtMouse(Scope* scope, Vector2 camera, int cellSize);
void addToScope(Scope* scope, Package* package);
Package* lookupPackageByLocation(HashMapLoc* locmap, Vector2 location);
Package* lookupPackageByID(int id, HashMapID* idmap);
Package* buildPackageByID(IndexArray* indexArrayStruct, int id);
void savePackageMemory(Package** packages, int numPackages, HashMapID* idmap);
void savePackageHandler(Package* curP, HashMapLoc* locmap, HashMapID* packageMemory, int* nextID, ModifiedPackage* modPack, Scope* scope);
void savePackageLocationMap(HashMapLoc* locmap, Package* package);
void addToModified(ModifiedPackage* modPack, int id);
void deletePackageByLocation(HashMapLoc* locmap, Package* package);
void deletePackageByID(int id, HashMapID* memory);
void deletePackageHandler(Package* package, HashMapLoc* locmap, HashMapID* memory);
void freePackage(Package* package);
void freePackageMemory(HashMapID* memory);
void daRenderer(Scope* packages, Vector2 cameraLoc, int cellSize);
int hashLoc(int x, int y, int capacity);
void expandHashLoc(HashMapLoc* locmap);
int hashID(int id, int capacity);
void expandHashID(HashMapID* idmap);
bool pullindexes(IndexArray* indexArrayStruct);
bool diskSave(ModifiedPackage* modPack, IndexArray* indexArrayStruct, HashMapID* idmap);
int getIDIndexForModPackages(IndexArray* indexArrayStruct, int id);
bool packageFileWrite(FILE* file, Package* package);
bool indexFileWrite(FILE* file, IndexArray* indexArr);

Font myFont;

int main(void) {
  Package babyPack = {false, 0, NULL, 16, {0, 0}, {0, 0}, -1, 0, NULL, 4};
  newBufferRelationships(&babyPack);
  HashMapID packageMemory = {calloc(16, sizeof(Node*)), 16, 0};
  ModifiedPackage modPack = {calloc(16, sizeof(int)), 0, 16};
  Scope scope = {calloc(16, sizeof(Package*)), 0, 16};
  HashMapLoc hashmaploc = {calloc(16, sizeof(Package*)), 16, 0};
  IndexArray indexes = {0};
  if (!packageMemory.buckets || !modPack.ids || !scope.viewablePackages || !hashmaploc.hashArray)
    exit(1);

  int nextID = 0;
  int exitCode = 0;
  if (!pullindexes(&indexes)) {
    perror("Could not load index.zn (existing files were left unchanged)");
    exitCode = 1;
    goto cleanup;
  }
  for (int i = 0; i < indexes.length; i++) {
    Package* package = buildPackageByID(&indexes, indexes.indexArray[i].id);
    if (!package) {
      perror("Could not load data.zn (existing files were left unchanged)");
      exitCode = 1;
      goto cleanup;
    }
    savePackageMemory(&package, 1, &packageMemory);
    addToScope(&scope, package);
    if (!package->deleted) savePackageLocationMap(&hashmaploc, package);
    if (package->id >= nextID) nextID = package->id + 1;
  }

  SetConfigFlags(FLAG_WINDOW_RESIZABLE);
  InitWindow(1200, 1200, "zion");
  SetTargetFPS(60);
  SetExitKey(KEY_NULL);
  myFont = LoadFontEx(TextFormat("%s%s", GetApplicationDirectory(), FONTIWANT), 128, NULL, 0);
  SetTextureFilter(myFont.texture, TEXTURE_FILTER_BILINEAR);

  Vector2 cameraLocation = {0, 0};
  const int cellsize = 30;
  Package* selected[MAXSELECTIONS] = {NULL};
  int selectedCount = 0;
  bool writingBuffer = false;
  bool saveFailed = false;
  double lastSave = GetTime();

  for (;;) {
    bool closing = WindowShouldClose();
    bool control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
    bool saveRequested = control && IsKeyPressed(KEY_S);
    Vector2 mouse = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
      cameraLocation.x -= GetMouseDelta().x / cellsize;
      cameraLocation.y -= GetMouseDelta().y / cellsize;
    }
    Vector2 clickPos = {floorf(mouse.x / cellsize + cameraLocation.x),
                        floorf(mouse.y / cellsize + cameraLocation.y)};
    Vector2 previewPos = {(clickPos.x - cameraLocation.x) * cellsize,
                          (clickPos.y - cameraLocation.y) * cellsize};

    if (IsKeyPressed(KEY_ESCAPE)) {
      writingBuffer = false;
      babyPack.length = 0;
      babyPack.buffer[0] = '\0';
      selectedCount = 0;
    }

    // Drain the input queue so fast typing does not lose characters.
    int key;
    while ((key = GetCharPressed()) > 0) {
      if (control) continue;
      if (writingBuffer) {
        if (!writeBuffer(&babyPack, key)) exit(1);
      } else {
        for (int i = 0; i < selectedCount; i++) {
          if (!writeBuffer(selected[i], key)) exit(1);
          addToModified(&modPack, selected[i]->id);
        }
      }
    }
    if (IsKeyPressed(KEY_ENTER)) {
      if (writingBuffer) {
        if (!writeBuffer(&babyPack, '\n')) exit(1);
      } else {
        for (int i = 0; i < selectedCount; i++) {
          if (!writeBuffer(selected[i], '\n')) exit(1);
          addToModified(&modPack, selected[i]->id);
        }
      }
    }
    if (IsKeyPressed(KEY_BACKSPACE) || IsKeyPressedRepeat(KEY_BACKSPACE)) {
      if (writingBuffer) backspaceBuffer(&babyPack);
      else {
        for (int i = 0; i < selectedCount; i++) {
          backspaceBuffer(selected[i]);
          addToModified(&modPack, selected[i]->id);
        }
      }
    }
    if (!writingBuffer && IsKeyPressed(KEY_DELETE)) {
      for (int i = 0; i < selectedCount; i++) {
        deletePackageHandler(selected[i], &hashmaploc, &packageMemory);
        addToModified(&modPack, selected[i]->id);
      }
      selectedCount = 0;
    }

    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && mouse.y >= 44;
    if (writingBuffer && (clicked || saveRequested || closing)) {
      if (babyPack.length > 0) {
        babyPack.location = clickPos;
        savePackageHandler(&babyPack, &hashmaploc, &packageMemory, &nextID, &modPack, &scope);
      }
      babyPack.length = 0;
      babyPack.buffer[0] = '\0';
      writingBuffer = false;
    } else if (clicked) {
      Package* hit = packageAtMouse(&scope, cameraLocation, cellsize);
      if (!hit) {
        if (selectedCount > 0) selectedCount = 0;
        else writingBuffer = true;
      } else {
        bool shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
        if (!shift) selectedCount = 0;
        bool alreadySelected = false;
        for (int i = 0; i < selectedCount; i++)
          if (selected[i] == hit) alreadySelected = true;
        if (!alreadySelected && selectedCount < MAXSELECTIONS)
          selected[selectedCount++] = hit;
      }
    }

    if (closing || saveRequested ||
        (modPack.length > 0 && GetTime() - lastSave >= (saveFailed ? 5.0 : 1.0))) {
      saveFailed = !diskSave(&modPack, &indexes, &packageMemory);
      lastSave = GetTime();
      if (saveFailed) perror("Could not save canvas; changes are still in memory");
      if (closing && !saveFailed) break;
    }

    BeginDrawing();
    ClearBackground(writingBuffer ? WRITINGBG : BACKGROUND);
    daRenderer(&scope, cameraLocation, cellsize);
    for (int i = 0; i < selectedCount; i++) {
      Vector2 position = {(selected[i]->location.x - cameraLocation.x) * cellsize,
                          (selected[i]->location.y - cameraLocation.y) * cellsize};
      drawSelected(selected[i]->buffer, position, cellsize);
    }
    if (writingBuffer) drawBuffer(babyPack.buffer, previewPos, cellsize);
    DrawRectangle(0, 0, GetScreenWidth(), 44, TEXTBOX);
    DrawLine(0, 43, GetScreenWidth(), 43, Fade(WORDS, 0.16f));
    DrawTextEx(myFont, "Click blank space: write, then click to place | Click text: edit | Shift+click: select more", (Vector2){8, 5}, 16, 0, WORDS);
    DrawTextEx(myFont, "Middle drag: pan | Enter: new line | Backspace: erase | Delete: remove | Esc: cancel | Ctrl+S: save", (Vector2){8, 24}, 16, 0, WORDS);
    if (saveFailed) {
      DrawRectangle(0, 44, GetScreenWidth(), 26, WORDS);
      DrawTextEx(myFont, "Save failed. Changes are still open. Check folder permissions or disk space, then press Ctrl+S.", (Vector2){8, 49}, 16, 0, TEXTBOX);
    }
    EndDrawing();
  }

  UnloadFont(myFont);
  CloseWindow();
cleanup:
  free(babyPack.buffer);
  free(babyPack.relationships);
  freePackageMemory(&packageMemory);
  free(modPack.ids);
  free(scope.viewablePackages);
  free(hashmaploc.hashArray);
  free(indexes.indexArray);
  return exitCode;
}

/*
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
}*/

//Buffer Functions---------------------------------------------------------------------------------------------
void newBufferRelationships(Package* curP) {
  curP->buffer = (char*)malloc(sizeof(char) * curP->capacity);
  if (curP->buffer == NULL)
    exit(1);
  curP->buffer[0] = '\0';

  curP->relationships = (Relationship*)malloc(sizeof(Relationship) * curP->capacRelationships);
  if (curP->relationships == NULL) //its chill to set it directly like this because its empty we dont care if we lose it also its going to exit program anyway we don gaf
    exit(1);
}

int writeBuffer(Package* curP, int input) {
  int bytes = 0;
  const char* encoded = CodepointToUTF8(input, &bytes);
  if (curP->length > INT_MAX - bytes - 1) return 0;
  int needed = curP->length + bytes + 1;
  if (needed > curP->capacity) {
    int capacity = curP->capacity <= INT_MAX / 2 ? curP->capacity * 2 : needed;
    if (capacity < needed) capacity = needed;
    char* temp = realloc(curP->buffer, capacity);
    if (!temp) return 0;
    curP->buffer = temp;
    curP->capacity = capacity;
  }
  memcpy(curP->buffer + curP->length, encoded, bytes);
  curP->length += bytes;
  curP->buffer[curP->length] = '\0';
  return 1;
}

void backspaceBuffer(Package* package) {
  if (package->length == 0) return;
  do {
    package->length--;
  } while (package->length > 0 && ((unsigned char)package->buffer[package->length] & 0xc0) == 0x80);
  package->buffer[package->length] = '\0';
}

Rectangle textBounds(const char* buffer, Vector2 position, int cellSize) {
  Vector2 measure = MeasureTextEx(myFont, buffer, cellSize, 1);
  return (Rectangle){position.x, position.y - cellSize / 15,
                     fmaxf(measure.x, cellSize / 2.0f), fmaxf(measure.y, cellSize)};
}

void drawBuffer(char* buffer, Vector2 curPos, int cellSize) {
  DrawRectangleRec(textBounds(buffer, curPos, cellSize), TEXTBOX);
  DrawTextEx(myFont, buffer, curPos, cellSize, 1, WORDS);
  Vector2 bottomPos = {0, GetScreenHeight() - textBounds(buffer, curPos, cellSize).height};
  DrawRectangleRec(textBounds(buffer, bottomPos, cellSize), TEXTBOX);
  DrawTextEx(myFont, buffer, bottomPos, cellSize, 1, WORDS);
}

void drawSelected(char* buffer, Vector2 curPos, int cellSize) {
  DrawRectangleLinesEx(textBounds(buffer, curPos, cellSize), 2, SELECTION);
}

Package* packageAtMouse(Scope* scope, Vector2 camera, int cellSize) {
  // Search back to front so overlapping notes select the one drawn on top.
  for (int i = scope->length - 1; i >= 0; i--) {
    Package* package = scope->viewablePackages[i];
    if (!package || package->deleted) continue;
    Vector2 position = {(package->location.x - camera.x) * cellSize,
                        (package->location.y - camera.y) * cellSize};
    if (CheckCollisionPointRec(GetMousePosition(), textBounds(package->buffer, position, cellSize)))
      return package;
  }
  return NULL;
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
      else if (locmap->hashArray[i] != TOMBSTONE && !locmap->hashArray[i]->deleted &&
               (locmap->hashArray[i]->location.x == location.x) && (locmap->hashArray[i]->location.y == location.y))
        return locmap->hashArray[i];
      //this wraps around the array. notice how we use size since we want the full size not count
      i = (i+1) % locmap->size;
    } while(i != index);

    return NULL;
}

Package* lookupPackageByID(int id, HashMapID* idmap) {
  int index = hashID(id, idmap->capacity);

  Node* currNode = idmap->buckets[index];
  while (currNode != NULL) {
    if (currNode->package->id == id) {
      return currNode->package;
    }
    currNode = currNode->next;
  }
  return NULL; //the procedure would be if the package doesnt exist in the memory which is what this checks. then the caller should then call the buiuldpackagebyid
}

Package* buildPackageByID(IndexArray* indexArrayStruct, int id) {
  int index = getIDIndexForModPackages(indexArrayStruct, id);
  if (index < 0) { errno = EINVAL; return NULL; }
  FILE* data = fopen("data.zn", "rb");
  if (!data) return NULL;
  Package* package = calloc(1, sizeof(Package));
  if (!package) { fclose(data); return NULL; }
  unsigned char deleted;
  int storedCapacity;
  if (fseek(data, 0, SEEK_END) != 0) goto invalid;
  long fileSize = ftell(data);
  long offset = indexArrayStruct->indexArray[index].fileOffset;
  if (fileSize < 0 || offset < 0 || offset >= fileSize || fseek(data, offset, SEEK_SET) != 0)
    goto invalid;
  if (fread(&deleted, sizeof(bool), 1, data) != 1 || deleted > 1 ||
      fread(&package->length, sizeof(int), 1, data) != 1 ||
      package->length < 0 || package->length == INT_MAX || package->length > fileSize - ftell(data))
    goto invalid;
  package->deleted = deleted;
  package->capacity = package->length + 1;
  if (package->capacity < 16) package->capacity = 16;
  package->buffer = malloc(package->capacity);
  if (!package->buffer) goto invalid;
  if (fread(package->buffer, 1, package->length, data) != (size_t)package->length ||
      fread(&storedCapacity, sizeof(int), 1, data) != 1 ||
      fread(&package->location, sizeof(Vector2), 1, data) != 1 ||
      fread(&package->size, sizeof(Vector2), 1, data) != 1 ||
      fread(&package->id, sizeof(int), 1, data) != 1 || package->id != id ||
      fread(&package->numRelationships, sizeof(int), 1, data) != 1 ||
      package->numRelationships < 0 ||
      package->numRelationships > (fileSize - ftell(data)) / (long)sizeof(Relationship) ||
      !isfinite(package->location.x) || !isfinite(package->location.y) ||
      fabs((double)package->location.x) > INT_MAX || fabs((double)package->location.y) > INT_MAX)
    goto invalid;
  package->buffer[package->length] = '\0';
  if (memchr(package->buffer, '\0', package->length)) goto invalid;
  package->capacRelationships = package->numRelationships > 4 ? package->numRelationships : 4;
  package->relationships = malloc(sizeof(Relationship) * package->capacRelationships);
  if (!package->relationships) goto invalid;
  if (fread(package->relationships, sizeof(Relationship), package->numRelationships, data) != (size_t)package->numRelationships ||
      fread(&storedCapacity, sizeof(int), 1, data) != 1)
    goto invalid;
  fclose(data);
  return package;
invalid:
  fclose(data);
  freePackage(package);
  errno = EINVAL;
  return NULL;
}

//Package Storage Functions---------------------------------------------------------------------------------------------------------
void savePackageMemory(Package** packages, int numPackages, HashMapID* idmap) {
  int index;
  for(int i = 0; i < numPackages; i++) {
    if ((idmap->length + 1) / (float)idmap->capacity > 0.7)
      expandHashID(idmap);
    index = hashID(packages[i]->id, idmap->capacity);
    if(lookupPackageByID(packages[i]->id, idmap) != NULL) continue;

    Node* newNode = malloc(sizeof(Node));
    if (!newNode) exit(1);
    newNode->package = packages[i];
    newNode->next = idmap->buckets[index]; // point to current head
    idmap->buckets[index] = newNode;       // new node becomes head
    idmap->length++;
  }
}

void savePackageLocationMap(HashMapLoc* locmap, Package* package) {
    if ((locmap->count + 1)/(float)locmap->size > 0.7) {
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
    savePackageLocationMap(locmap, package);
}

void addToScope(Scope* scope, Package* package) {
  if (scope->length >= scope->capacity) {
    int capacity = scope->capacity * 2;
    Package** temp = realloc(scope->viewablePackages, capacity * sizeof(Package*));
    if (!temp) exit(1);
    scope->viewablePackages = temp;
    scope->capacity = capacity;
  }
  scope->viewablePackages[scope->length++] = package;
}

void savePackageHandler(Package* curP, HashMapLoc* locmap, HashMapID* packageMemory, int* nextID, ModifiedPackage* modPack, Scope* scope) {
  // Selected packages are edited in place: copying here would free their own buffer.
  Package* existing = curP->id < 0 ? NULL : lookupPackageByID(curP->id, packageMemory);
  if (existing) {
    addToModified(modPack, existing->id);
    return;
  }
  if (*nextID == INT_MAX) {
    fprintf(stderr, "No more note IDs available\n");
    exit(1);
  }
  Package* saved = malloc(sizeof(Package));
  if (!saved) exit(1);
  *saved = *curP;
  saved->buffer = malloc(curP->capacity);
  saved->relationships = malloc(sizeof(Relationship) * curP->capacRelationships);
  if (!saved->buffer || !saved->relationships) exit(1);
  memcpy(saved->buffer, curP->buffer, curP->length + 1);
  memcpy(saved->relationships, curP->relationships, sizeof(Relationship) * curP->numRelationships);
  saved->id = (*nextID)++;
  savePackageMemory(&saved, 1, packageMemory);
  savePackageLocationMap(locmap, saved);
  addToModified(modPack, saved->id);
  addToScope(scope, saved);
}

void addToModified(ModifiedPackage* modPack, int id) {
  for (int i = 0; i < modPack->length; i++)
    if (modPack->ids[i] == id) return;
  if (modPack->length >= modPack ->capacity) {
    modPack->capacity *= 2;
    int* temp = realloc(modPack->ids, sizeof(int) * modPack->capacity);
    if (!temp) exit(1);
    modPack->ids = temp;
  }
  modPack->ids[modPack->length++] = id;
}
//Package Modification Functions-------------------------------------------------------------------------------------
void deletePackageByLocation(HashMapLoc* locmap, Package* package) {
  int index = hashLoc(package->location.x, package->location.y, locmap->size);
  int i = index;
  do {
    if (!locmap->hashArray[i]) return;
    if (locmap->hashArray[i] == package) {
      package->deleted = true;
      locmap->hashArray[i] = TOMBSTONE;
      locmap->count--;
      return;
    }
    i = (i + 1) % locmap->size;
  } while (i != index);
}

void deletePackageByID(int id, HashMapID* memory) {
  // Keep deleted records in memory until their tombstone has been saved.
  Package* package = lookupPackageByID(id, memory);
  if (package) package->deleted = true;
}

void deletePackageHandler(Package* package, HashMapLoc* locmap, HashMapID* memory) {
  deletePackageByID(package->id, memory);
  deletePackageByLocation(locmap, package);
}

void freePackage(Package* package) {
  if (!package) return;
  free(package->buffer);
  free(package->relationships);
  free(package);
}

void freePackageMemory(HashMapID* memory) {
  for (int i = 0; i < memory->capacity; i++) {
    Node* node = memory->buckets[i];
    while (node) {
      Node* next = node->next;
      freePackage(node->package);
      free(node);
      node = next;
    }
  }
  free(memory->buckets);
}
//Renderer Functions---------------------------------------------------------------------------------------------
void daRenderer(Scope* packages, Vector2 cameraLoc, int cellSize) {
  for (int i = 0; i < packages->length; i++) {
    Package* package = packages->viewablePackages[i];
    if (!package || package->deleted) continue;
    Vector2 position = {(package->location.x - cameraLoc.x) * cellSize,
                        (package->location.y - cameraLoc.y) * cellSize};
    Rectangle bounds = textBounds(package->buffer, position, cellSize);
    if (CheckCollisionRecs(bounds, (Rectangle){0, 0, GetScreenWidth(), GetScreenHeight()})) {
      DrawRectangleRec(bounds, TEXTBOX);
      DrawTextEx(myFont, package->buffer, position, cellSize, 1, WORDS);
    }
  }
}

//Hash Functions---------------------------------------------------------------------------------------------
int hashLoc(int x, int y, int capacity) {
  return ((uint32_t)x * 19u + (uint32_t)y) % (uint32_t)capacity;
}

void expandHashLoc(HashMapLoc* locmap) {
  locmap->size *= 2;
  //cant do realloc i need to do malloc then free wiht a move in between
  Package** temp = (Package**)calloc(locmap->size, sizeof(Package*));
  if (temp == NULL)
    exit(1);

  Package** middleman = locmap->hashArray;
  locmap->hashArray = temp;
  locmap->count = 0;

  //use size since we want to go through the entire array(if we going through all we have to check null or tombstone to make sure we dont pull a nonexistant package)
  for (int i = 0; i < locmap->size/2; i++) {
    if(!(middleman[i] == NULL || middleman[i] == TOMBSTONE))
      savePackageLocationMap(locmap, middleman[i]);
  }
  free(middleman);
}

int hashID(int id, int capacity) {
  return (uint32_t)id % (uint32_t)capacity;
}

void expandHashID(HashMapID* idmap) {
  idmap->capacity *= 2;

  Node** newBuckets = (Node**)calloc(idmap-> capacity, sizeof(Node*));
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
bool pullindexes(IndexArray* indexArrayStruct) {
  FILE* index = fopen("index.zn", "rb");
  if (!index && errno != ENOENT) return false;
  if (!index) {
    FILE* data = fopen("data.zn", "rb");
    if (!data && errno != ENOENT) return false;
    if (data) {
      bool hasData = fgetc(data) != EOF || ferror(data);
      fclose(data);
      if (hasData) { errno = EINVAL; return false; }
    }
  }
  int length = 0;
  if (index) {
    if (fseek(index, 0, SEEK_END) != 0) goto invalid;
    long size = ftell(index);
    if (size < (long)sizeof(int) || fseek(index, 0, SEEK_SET) != 0 ||
        fread(&length, sizeof(int), 1, index) != 1 || length < 0 ||
        length > (size - (long)sizeof(int)) / (long)sizeof(IndexEntry))
      goto invalid;
  }
  int capacity = length > 16 ? length : 16;
  IndexEntry* entries = calloc(capacity, sizeof(IndexEntry));
  if (!entries) { if (index) fclose(index); return false; }
  if (index) {
    if (fread(entries, sizeof(IndexEntry), length, index) != (size_t)length) {
      free(entries);
      goto invalid;
    }
    for (int i = 0; i < length; i++) {
      if (entries[i].id < 0 || entries[i].id == INT_MAX || entries[i].fileOffset < 0 ||
          (i > 0 && entries[i - 1].id >= entries[i].id)) {
        free(entries);
        goto invalid;
      }
    }
    fclose(index);
  }
  *indexArrayStruct = (IndexArray){length, entries, capacity};
  return true;
invalid:
  fclose(index);
  errno = EINVAL;
  return false;
}

bool diskSave(ModifiedPackage* modPack, IndexArray* indexes, HashMapID* idmap) {
  if (modPack->length == 0) return true;
  FILE* data = fopen("data.zn", "ab");
  if (!data) return false;
  for (int i = 0; i < modPack->length; i++) {
    Package* package = lookupPackageByID(modPack->ids[i], idmap);
    if (!package) continue;
    if (fseek(data, 0, SEEK_END) != 0) { fclose(data); return false; }
    long offset = ftell(data);
    if (offset < 0 || !packageFileWrite(data, package)) { fclose(data); return false; }
    int index = getIDIndexForModPackages(indexes, package->id);
    if (index < 0) {
      index = -index - 1;
      if (indexes->length >= indexes->capacity) {
        int capacity = indexes->capacity ? indexes->capacity * 2 : 16;
        IndexEntry* temp = realloc(indexes->indexArray, sizeof(IndexEntry) * capacity);
        if (!temp) { fclose(data); return false; }
        indexes->indexArray = temp;
        indexes->capacity = capacity;
      }
      memmove(indexes->indexArray + index + 1, indexes->indexArray + index,
              sizeof(IndexEntry) * (indexes->length - index));
      indexes->length++;
    }
    indexes->indexArray[index] = (IndexEntry){package->id, offset};
  }
  // Records are append-only. Publish their offsets only after the data is flushed.
  bool saved = fflush(data) == 0 && fsync(fileno(data)) == 0;
  if (fclose(data) != 0) saved = false;
  if (!saved) return false;
  FILE* index = fopen("index.zn.tmp", "wb");
  if (!index) return false;
  saved = indexFileWrite(index, indexes) && fflush(index) == 0 && fsync(fileno(index)) == 0;
  if (fclose(index) != 0) saved = false;
  if (!saved || rename("index.zn.tmp", "index.zn") != 0) return false;
  modPack->length = 0;
  return true;
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

bool packageFileWrite(FILE* file, Package* package) {
  return fwrite(&package->deleted, sizeof(bool), 1, file) == 1 &&
    fwrite(&package->length, sizeof(int), 1, file) == 1 &&
    fwrite(package->buffer, 1, package->length, file) == (size_t)package->length &&
    fwrite(&package->capacity, sizeof(int), 1, file) == 1 &&
    fwrite(&package->location, sizeof(Vector2), 1, file) == 1 &&
    fwrite(&package->size, sizeof(Vector2), 1, file) == 1 &&
    fwrite(&package->id, sizeof(int), 1, file) == 1 &&
    fwrite(&package->numRelationships, sizeof(int), 1, file) == 1 &&
    fwrite(package->relationships, sizeof(Relationship), package->numRelationships, file) == (size_t)package->numRelationships &&
    fwrite(&package->capacRelationships, sizeof(int), 1, file) == 1;
}

bool indexFileWrite(FILE* file, IndexArray* indexArr) {
  return fwrite(&indexArr->length, sizeof(int), 1, file) == 1 &&
    fwrite(indexArr->indexArray, sizeof(IndexEntry), indexArr->length, file) == (size_t)indexArr->length &&
    fwrite(&indexArr->capacity, sizeof(int), 1, file) == 1;
}
