#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <SDL2/SDL.h>
#include "constants.h"
#include "textures.h"

const int map[MAP_NUM_ROWS][MAP_NUM_COLS] = {
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
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
	{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 5, 5, 5, 5, 5, 5}};

struct Player {
	float x;
	float y;
	float width;
	float height;
	int turnDirection; // -1 left, +1 right
	int walkDirection; // -1 backwards, +1 foward
	float rotationAngle;
	float walkSpeed;
	float turnSpeed;
} player;

struct Ray {
	float rayAngle;
	float wallCollisionX;
	float wallCollisionY;
	float distance;
	bool wasCollisionVertical;
	int wallCollisionContent;
} rays[NUM_RAYS];

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
bool isGameRunning = false;
int ticksLastFrame;

uint32_t *colorBuffer = NULL;
SDL_Texture *colorBufferTexture;

bool initializeWindow() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		fprintf(stderr, "Error initializing SDL.\n");
		return false;
	}
	SDL_DisplayMode display_mode;
	SDL_GetCurrentDisplayMode(0, &display_mode);
	int fullScreenWidth = display_mode.w;
	int fullScreenHeight = display_mode.h;
	window = SDL_CreateWindow(
		NULL,
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		fullScreenWidth,
		fullScreenHeight,
		SDL_WINDOW_BORDERLESS);
	if (!window) {
		fprintf(stderr, "Error creating SDL window.\n");
		return false;
	}

	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		fprintf(stderr, "Error creating SDL renderer.\n");
		return false;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
	return true;
}

void destroyWindow() {
	freeWallTextures();
	free(colorBuffer);
	SDL_DestroyTexture(colorBufferTexture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void setup() {
	player.x = WINDOW_WIDTH / 2;
	player.y = WINDOW_HEIGHT / 2;
	player.width = 1;
	player.height = 1;
	player.turnDirection = 0;
	player.walkDirection = 0;
	player.rotationAngle = PI / 2;
	player.walkSpeed = 100;
	player.turnSpeed = 45 * (PI / 180);

	// allocate the total amount of bytes in memory to hold the colorbuffer
	colorBuffer = (uint32_t *)malloc(sizeof(uint32_t) * WINDOW_WIDTH * WINDOW_HEIGHT);

	// create an SDL_Texture to display the colorbuffer
	colorBufferTexture = SDL_CreateTexture(
		renderer,
		SDL_PIXELFORMAT_RGBA32,
		SDL_TEXTUREACCESS_STREAMING,
		WINDOW_WIDTH,
		WINDOW_HEIGHT);

	// Calls uPNG to decode  PNG files and load them onto the wallTextures array
	loadWallTextures();
}

bool mapHasWallAt(float x, float y) {
	if (x < 0 || x >= MAP_NUM_COLS * TILE_SIZE || y < 0 || y >= MAP_NUM_ROWS * TILE_SIZE) {
		return true;
	}

	int mapGridIndexX = floor(x / TILE_SIZE);
	int mapGridIndexY = floor(y / TILE_SIZE);
	return map[mapGridIndexY][mapGridIndexX] != 0;
}

void movePlayer(float deltaTime) {
	player.rotationAngle += player.turnDirection * player.turnSpeed * deltaTime;
	float moveStep = player.walkDirection * player.walkSpeed * deltaTime;
	float newPlayerX = player.x + cos(player.rotationAngle) * moveStep;
	float newPlayerY = player.y + sin(player.rotationAngle) * moveStep;

	if (!mapHasWallAt(newPlayerX, newPlayerY)) {
		player.x = newPlayerX;
		player.y = newPlayerY;
	}
}

void renderPlayer() {
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Rect playerRect = {
		player.x * MINIMAP_SCALE_FACTOR,
		player.y * MINIMAP_SCALE_FACTOR,
		player.width * MINIMAP_SCALE_FACTOR,
		player.height * MINIMAP_SCALE_FACTOR};
	SDL_RenderFillRect(renderer, &playerRect);

	SDL_RenderDrawLine(
		renderer,
		MINIMAP_SCALE_FACTOR * player.x,
		MINIMAP_SCALE_FACTOR * player.y,
		MINIMAP_SCALE_FACTOR * player.x + cos(player.rotationAngle) * 40,
		MINIMAP_SCALE_FACTOR * player.y + sin(player.rotationAngle) * 40);
}

float normalizeAngle(float angle) {
	angle = remainder(angle, TWO_PI);
	if (angle < 0) {
		angle = TWO_PI + angle;
	}
	return angle;
}

float distanceBetweenPoints(float x1, float y1, float x2, float y2) {
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

void castRay(float rayAngle, int stripId) {
	rayAngle = normalizeAngle(rayAngle);

	int isRayFacingDown = rayAngle > 0 && rayAngle < PI;
	int isRayFacingUp = !isRayFacingDown;

	int isRayFacingRight = rayAngle < 0.5 * PI || rayAngle > 1.5 * PI;
	int isRayFacingLeft = !isRayFacingRight;

	float xintercept, yintercept;
	float xstep, ystep;

	///////////////////////////////////////////
	// HORIZONTAL RAY-GRID INTERSECTION CODE
	///////////////////////////////////////////
	bool foundHorzWallCollision = false;
	float horzWallCollisionX = 0;
	float horzWallCollisionY = 0;
	int horzWallContent = 0;

	//  y-coordinate of the closest horizontal grid intersection
	yintercept = floor(player.y / TILE_SIZE) * TILE_SIZE;
	yintercept += isRayFacingDown ? TILE_SIZE : 0;

	// x-coordinate of the closest horizontal grid intersection
	xintercept = player.x + (yintercept - player.y) / tan(rayAngle);

	// Calculates the increment xstep and ystep
	ystep = TILE_SIZE;
	ystep *= isRayFacingUp ? -1 : 1;

	xstep = TILE_SIZE / tan(rayAngle);
	xstep *= (isRayFacingLeft && xstep > 0) ? -1 : 1;
	xstep *= (isRayFacingRight && xstep < 0) ? -1 : 1;

	float nextHorzTouchX = xintercept;
	float nextHorzTouchY = yintercept;

	// Increments xstep and ystep until a wall is hit
	while (nextHorzTouchX >= 0 && nextHorzTouchX <= MAP_NUM_COLS * TILE_SIZE && nextHorzTouchY >= 0 && nextHorzTouchY <= MAP_NUM_ROWS * TILE_SIZE) {
		float xToCheck = nextHorzTouchX;
		float yToCheck = nextHorzTouchY + (isRayFacingUp ? -1 : 0);

		if (mapHasWallAt(xToCheck, yToCheck)) {
			// finding wall collisions
			horzWallCollisionX = nextHorzTouchX;
			horzWallCollisionY = nextHorzTouchY;
			horzWallContent = map[(int)floor(yToCheck / TILE_SIZE)][(int)floor(xToCheck / TILE_SIZE)];
			foundHorzWallCollision = true;
			break;
		}
		else {
			nextHorzTouchX += xstep;
			nextHorzTouchY += ystep;
		}
	}

	///////////////////////////////////////////
	// VERTICAL RAY-GRID INTERSECTION CODE
	///////////////////////////////////////////
	bool foundVertWallCollision = false;
	float vertWallCollisionX = 0;
	float vertWallCollisionY = 0;
	int vertWallContent = 0;

	// x-coordinate of the closest horizontal grid intersection
	xintercept = floor(player.x / TILE_SIZE) * TILE_SIZE;
	xintercept += isRayFacingRight ? TILE_SIZE : 0;

	// y-coordinate of the closest horizontal grid intersection
	yintercept = player.y + (xintercept - player.x) * tan(rayAngle);

	// Computes the increment xstep and ystep
	xstep = TILE_SIZE;
	xstep *= isRayFacingLeft ? -1 : 1;

	ystep = TILE_SIZE * tan(rayAngle);
	ystep *= (isRayFacingUp && ystep > 0) ? -1 : 1;
	ystep *= (isRayFacingDown && ystep < 0) ? -1 : 1;

	float nextVertTouchX = xintercept;
	float nextVertTouchY = yintercept;

	// Increments xstep and ystep until a wall is hit
	while (nextVertTouchX >= 0 && nextVertTouchX <= MAP_NUM_COLS * TILE_SIZE && nextVertTouchY >= 0 && nextVertTouchY <= MAP_NUM_ROWS * TILE_SIZE) {
		float xToCheck = nextVertTouchX + (isRayFacingLeft ? -1 : 0);
		float yToCheck = nextVertTouchY;

		if (mapHasWallAt(xToCheck, yToCheck)) {
			// hit a wall
			vertWallCollisionX = nextVertTouchX;
			vertWallCollisionY = nextVertTouchY;
			vertWallContent = map[(int)floor(yToCheck / TILE_SIZE)][(int)floor(xToCheck / TILE_SIZE)];
			foundVertWallCollision = true;
			break;
		} else {
			nextVertTouchX += xstep;
			nextVertTouchY += ystep;
		}
	}

	// Calculate both horizontal and vertical hit distances and choose the smallest one
	float horzCollisionDistance = foundHorzWallCollision
								? distanceBetweenPoints(player.x, player.y, horzWallCollisionX, horzWallCollisionY)
								: FLT_MAX;
	float vertCollisionDistance = foundVertWallCollision
								? distanceBetweenPoints(player.x, player.y, vertWallCollisionX, vertWallCollisionY)
								: FLT_MAX;

	if (vertCollisionDistance < horzCollisionDistance) {
		rays[stripId].distance = vertCollisionDistance;
		rays[stripId].wallCollisionX = vertWallCollisionX;
		rays[stripId].wallCollisionY = vertWallCollisionY;
		rays[stripId].wallCollisionContent = vertWallContent;
		rays[stripId].wasCollisionVertical = true;
		rays[stripId].rayAngle = rayAngle;
	} else {
		rays[stripId].distance = horzCollisionDistance;
		rays[stripId].wallCollisionX = horzWallCollisionX;
		rays[stripId].wallCollisionY = horzWallCollisionY;
		rays[stripId].wallCollisionContent = horzWallContent;
		rays[stripId].wasCollisionVertical = false;
		rays[stripId].rayAngle = rayAngle;
	}
}

void castAllRays() {
	for (int col = 0; col < NUM_RAYS; col++) {
		float rayAngle = player.rotationAngle + atan((col - NUM_RAYS / 2) / DIST_PROJ_PLANE);
		castRay(rayAngle, col);
	}
}

void renderMap() {
	for (int i = 0; i < MAP_NUM_ROWS; i++) {
		for (int j = 0; j < MAP_NUM_COLS; j++) {
			int tileX = j * TILE_SIZE;
			int tileY = i * TILE_SIZE;
			int tileColor = map[i][j] != 0 ? 255 : 0;

			SDL_SetRenderDrawColor(renderer, tileColor, tileColor, tileColor, 255);
			SDL_Rect mapTileRect = {
				tileX * MINIMAP_SCALE_FACTOR,
				tileY * MINIMAP_SCALE_FACTOR,
				TILE_SIZE * MINIMAP_SCALE_FACTOR,
				TILE_SIZE * MINIMAP_SCALE_FACTOR};
			SDL_RenderFillRect(renderer, &mapTileRect);
		}
	}
}

void renderRays() {
	SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
	for (int i = 0; i < NUM_RAYS; i++) {
		SDL_RenderDrawLine(
			renderer,
			MINIMAP_SCALE_FACTOR * player.x,
			MINIMAP_SCALE_FACTOR * player.y,
			MINIMAP_SCALE_FACTOR * rays[i].wallCollisionX,
			MINIMAP_SCALE_FACTOR * rays[i].wallCollisionY);
	}
}

void processInput() {
	SDL_Event event;
	SDL_PollEvent(&event);
	switch (event.type) {
	case SDL_QUIT: {
		isGameRunning = false;
		break;
		}
	case SDL_KEYDOWN: {
		if (event.key.keysym.sym == SDLK_ESCAPE)
			isGameRunning = false;
		if (event.key.keysym.sym == SDLK_UP)
			player.walkDirection = +1;
		if (event.key.keysym.sym == SDLK_DOWN)
			player.walkDirection = -1;
		if (event.key.keysym.sym == SDLK_RIGHT)
			player.turnDirection = +1;
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

void update() {
	int timeToWait = FRAME_TIME_LENGTH - (SDL_GetTicks() - ticksLastFrame);
	if (timeToWait > 0 && timeToWait <= FRAME_TIME_LENGTH) {
		SDL_Delay(timeToWait);
	}

	float deltaTime = (SDL_GetTicks() - ticksLastFrame) / 1000.0f;
	ticksLastFrame = SDL_GetTicks();
	movePlayer(deltaTime);
	castAllRays();
}

void generate3DProjection() {
	for (int x = 0; x < NUM_RAYS; x++) {
		float perpDistance = rays[x].distance * cos(rays[x].rayAngle - player.rotationAngle);
		float projectedWallHeight = (TILE_SIZE / perpDistance) * DIST_PROJ_PLANE;

		int wallStripHeight = (int)projectedWallHeight;

		int wallTopPixel = (WINDOW_HEIGHT / 2) - (wallStripHeight / 2);
		wallTopPixel = wallTopPixel < 0 ? 0 : wallTopPixel;

		int wallBottomPixel = (WINDOW_HEIGHT / 2) + (wallStripHeight / 2);
		wallBottomPixel = wallBottomPixel > WINDOW_HEIGHT ? WINDOW_HEIGHT : wallBottomPixel;

		// set the color of the ceiling
		for (int y = 0; y < wallTopPixel; y++)
			colorBuffer[(WINDOW_WIDTH * y) + x] = 0xFF444444;

		int textureOffsetX;
		if (rays[x].wasCollisionVertical)
			textureOffsetX = (int)rays[x].wallCollisionY % TILE_SIZE;
		else
			textureOffsetX = (int)rays[x].wallCollisionX % TILE_SIZE;

		// get the correct texture id number from the map content
		int texNum = rays[x].wallCollisionContent - 1;

		int texture_width = wallTextures[texNum].width;
		int texture_height = wallTextures[texNum].height;

		// render the wall from wallTopPixel to wallBottomPixel
		for (int y = wallTopPixel; y < wallBottomPixel; y++) {
			int distanceFromTop = y + (wallStripHeight / 2) - (WINDOW_HEIGHT / 2);
			int textureOffsetY = distanceFromTop * ((float)texture_height / wallStripHeight);

			// set the color of the wall based on the color from the texture
			uint32_t texelColor = wallTextures[texNum].texture_buffer[(texture_width * textureOffsetY) + textureOffsetX];
			colorBuffer[(WINDOW_WIDTH * y) + x] = texelColor;
		}

		// set the floor color
		for (int y = wallBottomPixel; y < WINDOW_HEIGHT; y++)
			colorBuffer[(WINDOW_WIDTH * y) + x] = 0xFF282828;
	}
}

void clearColorBuffer(uint32_t color) {
	for (int i = 0; i < WINDOW_WIDTH * WINDOW_HEIGHT; i++)
		colorBuffer[i] = color;
}

void renderColorBuffer() {
	SDL_UpdateTexture(
		colorBufferTexture,
		NULL,
		colorBuffer,
		(int)(WINDOW_WIDTH * sizeof(uint32_t)));
	SDL_RenderCopy(renderer, colorBufferTexture, NULL, NULL);
}

void render() {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	generate3DProjection();

	renderColorBuffer();
	clearColorBuffer(0xFF000000);

	renderMap();
	renderRays();
	renderPlayer();

	SDL_RenderPresent(renderer);
}

int main() {
	isGameRunning = initializeWindow();

	setup();

	while (isGameRunning) {
		processInput();
		update();
		render();
	}
	destroyWindow();
	return 0;
}
