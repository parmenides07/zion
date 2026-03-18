#include "raylib.h"

char* newBuffer(int capacity);
char* writeBuffer(char **buffer, int input, int length);
void drawBuffer(char **buffer, Vector2 curPos);

int main(void)
{
	InitWindow(800, 450, "zion");
	
	int key = 0;
	global int length = 0;
    	Vector2 lastPosition = {0.0f,0.0f};
	int capacity = 16;
    	while (!WindowShouldClose())
    	{	
		key = GetCharPressed(); 
		if (key > 0)
		{
			Vector2 currentPosition = GetMousePosition();
			if (currentPosition != lastPosition)
				//write functiont o save current buffer
				free(bufferLoc);
				capcity = 16;
				length = 0;
				bufferLoc = newBuffer(capacity); 
			bufferLoc = writeBuffer(bufferLoc, key, length);
			drawBuffer(bufferLoc, currentPosition);
		}
		key = 0;
	}	

    	CloseWindow();

    	return 0;
	}

char* newBuffer(int capacity)
{
	char *bufferLoc = (char*)malloc(sizeof(char) * capacity);

	if (bufferLoc == NULL) {
		return NULL;
	}
	//writing [] signifies that we are trying to get the value so its like the same as doing *bufferLoc[]
	bufferLoc[0] = '\0';

	return bufferLoc;
}

char* writeBuffer(char **buffer, int input, int length)
{

	if(length > (capacity-1)) 
	{
		capacity *= 2;
		char *temp = (char*)realloc(*buffer, capacity);
		
		if(temp == NULL)
		{
			free(temp);
			return NULL; 
		}

		free(*buffer);

		*buffer = temp; 
		//expand array to make room for hte next character
	}
	
	//note this will incriment length AFTER this line runs
	buffer[length++] = (char)input;
	//remember null character must be put at the end of string to signify it has ended. 
	buffer[length] = '\0';

	return buffer; 
}
