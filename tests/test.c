#define main zion_main
#include "../main.c"
#undef main
#include <assert.h>
#include <sys/stat.h>

int main(void) {
  char directory[] = "/tmp/zion-tests-XXXXXX";
  assert(mkdtemp(directory));
  assert(chdir(directory) == 0);
  printf("Testing in %s\n", directory);

  Package draft = {false, 0, NULL, 16, {0, 0}, {0, 0}, -1, 0, NULL, 4};
  newBufferRelationships(&draft);
  for (int i = 0; i < 4096; i++) assert(writeBuffer(&draft, 'a'));
  assert(draft.length == 4096 && strlen(draft.buffer) == 4096);
  assert(writeBuffer(&draft, 0x00e9));
  assert(writeBuffer(&draft, 0x1f642));
  assert(draft.length == 4102);
  backspaceBuffer(&draft);
  assert(draft.length == 4098);
  backspaceBuffer(&draft);
  assert(draft.length == 4096);
  while (draft.length) backspaceBuffer(&draft);
  backspaceBuffer(&draft);
  assert(draft.buffer[0] == '\0');

  HashMapID memory = {calloc(16, sizeof(Node*)), 16, 0};
  HashMapLoc locations = {calloc(16, sizeof(Package*)), 16, 0};
  Scope scope = {calloc(16, sizeof(Package*)), 0, 16};
  ModifiedPackage modified = {calloc(16, sizeof(int)), 0, 16};
  IndexArray indexes = {0};
  assert(pullindexes(&indexes));
  assert(indexes.length == 0 && indexes.capacity >= 16);
  // An empty on-disk index must still leave room for the first note.
  FILE* file = fopen("index.zn", "wb");
  assert(file && indexFileWrite(file, &indexes));
  assert(fclose(file) == 0);
  free(indexes.indexArray);
  assert(pullindexes(&indexes));

  int nextID = 0;
  for (int i = 0; i < 512; i++) {
    draft.length = 0;
    draft.buffer[0] = '\0';
    assert(writeBuffer(&draft, 'A' + i % 26));
    draft.location = (Vector2){i - 256, -19 * (i - 256)}; // All collide.
    savePackageHandler(&draft, &locations, &memory, &nextID, &modified, &scope);
  }
  assert(nextID == 512 && scope.length == 512 && memory.length == 512);
  assert(locations.count == 512 && modified.length == 512);
  for (int i = 0; i < 512; i++) {
    Package* note = scope.viewablePackages[i];
    assert(note->id == i && note->buffer[0] == 'A' + i % 26);
    assert(lookupPackageByID(i, &memory) == note);
    assert(lookupPackageByLocation(&locations, note->location) == note);
  }
  assert(hashLoc(INT_MIN, INT_MAX, 16) >= 0);

  Package* first = scope.viewablePackages[0];
  savePackageHandler(first, &locations, &memory, &nextID, &modified, &scope);
  assert(strcmp(first->buffer, "A") == 0 && nextID == 512);
  first->relationships[first->numRelationships++] = (Relationship){CHILD, 1, {-255, 4845}};
  assert(writeBuffer(first, '\n') && writeBuffer(first, 0x00e9));
  for (int i = 1; i < 512; i += 2)
    deletePackageHandler(scope.viewablePackages[i], &locations, &memory);
  assert(locations.count == 256);
  for (int i = 0; i < 512; i++)
    assert(lookupPackageByLocation(&locations, scope.viewablePackages[i]->location) ==
           (i % 2 ? NULL : scope.viewablePackages[i]));
  // Rebuild the location table after deletions; tombstones must not be dereferenced.
  expandHashLoc(&locations);
  assert(locations.count == 256);

  // Save in reverse order to exercise insertion at the front of the sorted index.
  for (int i = 0; i < 512; i++) modified.ids[i] = 511 - i;
  assert(diskSave(&modified, &indexes, &memory));
  assert(modified.length == 0 && indexes.length == 512);
  IndexArray reopened = {0};
  assert(pullindexes(&reopened));
  for (int i = 0; i < 512; i++) {
    assert(reopened.indexArray[i].id == i);
    Package* note = buildPackageByID(&reopened, i);
    assert(note && note->id == i && note->deleted == (i % 2 != 0));
    assert(strcmp(note->buffer, scope.viewablePackages[i]->buffer) == 0);
    assert(note->location.x == i - 256);
    if (i == 0) {
      assert(note->numRelationships == 1 && note->relationships[0].ID == 1);
      for (int j = 0; j < 100; j++) assert(writeBuffer(note, 'z'));
      note->relationships[note->numRelationships++] = (Relationship){PARENT, 2, {0, 0}};
    }
    freePackage(note);
  }
  free(reopened.indexArray);

  // Failed publication must retain dirty notes and leave the old index readable.
  assert(writeBuffer(first, '!'));
  addToModified(&modified, first->id);
  assert(mkdir("index.zn.tmp", 0700) == 0);
  assert(!diskSave(&modified, &indexes, &memory));
  assert(modified.length == 1);
  assert(pullindexes(&reopened));
  Package* old = buildPackageByID(&reopened, 0);
  assert(old && strcmp(old->buffer, "A\n\xc3\xa9") == 0);
  freePackage(old);
  free(reopened.indexArray);
  assert(rmdir("index.zn.tmp") == 0);
  assert(diskSave(&modified, &indexes, &memory));
  assert(modified.length == 0);
  old = buildPackageByID(&indexes, 0);
  assert(old && strcmp(old->buffer, first->buffer) == 0);
  freePackage(old);

  // Overlapping notes must delete by identity, not accidentally remove a neighbor.
  draft.location = first->location;
  savePackageHandler(&draft, &locations, &memory, &nextID, &modified, &scope);
  Package* overlap = scope.viewablePackages[512];
  deletePackageHandler(overlap, &locations, &memory);
  assert(!first->deleted && overlap->deleted);
  assert(lookupPackageByLocation(&locations, first->location) == first);
  while (first->length) backspaceBuffer(first);
  addToModified(&modified, first->id);
  assert(diskSave(&modified, &indexes, &memory));
  old = buildPackageByID(&indexes, 0);
  assert(old && old->length == 0 && old->buffer[0] == '\0');
  assert(writeBuffer(old, 'x'));
  freePackage(old);

  // Malformed files fail cleanly instead of crashing or starting a blank canvas.
  assert(rename("index.zn", "index.zn.backup") == 0);
  reopened = (IndexArray){0};
  assert(!pullindexes(&reopened));
  assert(rename("index.zn.backup", "index.zn") == 0);
  assert(truncate("data.zn", 1) == 0);
  assert(buildPackageByID(&indexes, 0) == NULL);
  assert(unlink("data.zn") == 0);
  assert(buildPackageByID(&indexes, 0) == NULL);
  assert(truncate("index.zn", 2) == 0);
  reopened = (IndexArray){0};
  assert(!pullindexes(&reopened));
  file = fopen("index.zn", "wb");
  assert(file);
  int invalidLength = -1;
  assert(fwrite(&invalidLength, sizeof(int), 1, file) == 1);
  assert(fclose(file) == 0);
  assert(!pullindexes(&reopened));

  free(draft.buffer);
  free(draft.relationships);
  freePackageMemory(&memory);
  free(locations.hashArray);
  free(scope.viewablePackages);
  free(modified.ids);
  free(indexes.indexArray);
  assert(unlink("index.zn") == 0);
  assert(chdir("/tmp") == 0);
  assert(rmdir(directory) == 0);
  puts("All text, hashing, deletion, persistence, and failed-save checks passed.");
  return 0;
}
