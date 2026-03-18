#include "raylib.h"

struct Package {
	char* buffer;
	Vector2 location;
	int capacity, length;
};:

Package newBuffer(Package curP);
Package writeBuffer(Package curP, int input);
void drawBuffer(char *buffer, Vector2 curPos);

int main(void)
{
	InitWindow(800, 450, "zion");
	
	int key = 0;
    	Vector2 lastPosition = {0.0f,0.0f};
	struct Package curPack;
    	while (!WindowShouldClose())
    	{	
		key = GetCharPressed(); 
		if (key > 0)
		{
			Vector2 currentPosition = GetMousePosition();
			if (currentPosition != lastPosition)
				//write functiont o save current buffer
				free(curPack.buffer);
				//so i gotta fix this its trying to free nonexistant value idk	
				curPack.capcity = 16;
				curPack.length = 0;
				curPack = newBuffer(curPack); 
			curPack = writeBuffer(curPack, key);
			drawBuffer(curpack.buffer, currentPosition);
			//also draw at bottom left of screen 
		}
		key = 0;
	}	

    	CloseWindow();

    	return 0;
	}

Package newBuffer(Package curP)
{
	curP.buffer = (char*)malloc(sizeof(char) * curP.capacity);

	if (curP.buffer == NULL) {
		return NULL;
	}
	//writing [] signifies that we are trying to get the value so its like the same as doing *bufferLoc[]
	curP.buffer[0] = '\0';

	return curP;
}

Package writeBuffer(Package curP, int input)
{

	if(curP.length > (curP.capacity-1)) 
	{
		curP.capacity *= 2;
		char *temp = (char*)realloc(*curP.buffer, curP.capacity);
		
		if(temp == NULL)
		{
			free(temp);
			return NULL; 
		}

		free(curP.buffer);

		curP.buffer = temp; 
		//expand array to make room for hte next character
	}
	
	//note this will incriment length AFTER this line runs
	curP.buffer[length++] = (char)input;
	//remember null character must be put at the end of string to signify it has ended. 
	curP.buffer[length] = '\0';

	return curP; 
}
