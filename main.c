#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>

typedef struct Package {
	char* buffer;
	Vector2 location;
	int capacity, length;
} Package;

int newBuffer(Package* curP);
int writeBuffer(Package* curP, int input);
void drawBuffer(char* buffer, Vector2 curPos);

int main(void)
{
	InitWindow(800, 450, "zion");
	
	int key = 0;
    	Vector2 lastPosition = {0.0f,0.0f};
	struct Package curPack = {NULL,{0.0f,0.0f},16,0};
	Vector2 currentPosition = {0.0f, 0.0f};

	if(newBuffer(&curPack) == 0)
		exit(1);
    	
	while (!WindowShouldClose())
    	{
		BeginDrawing();
		drawBuffer(curPack.buffer, currentPosition);
		key = GetCharPressed(); 
		if (key > 0)
		{
			currentPosition = GetMousePosition();
			if (!(currentPosition.x == lastPosition.x || currentPosition.y == lastPosition.x))
			{
				//write functiont o save current buffer
				free(curPack.buffer);
				curPack.capacity = 16;
				curPack.length = 0;
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

	if (curP->buffer == NULL) {
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

void drawBuffer(char* buffer, Vector2 curPos)
{
	DrawText(buffer, curPos.x, curPos.y, 20, RED);
	DrawText(buffer, 0, GetScreenHeight()-20, 20, RED);
}
