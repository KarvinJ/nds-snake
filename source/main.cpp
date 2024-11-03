#include "starter.h"
#include <iostream>
#include <time.h>
#include <deque>
#include <math.h>
#include <fstream>

// sounds
#include <maxmod9.h>
#include "soundbank.h"
#include "soundbank_bin.h"

// fonts
#include "Cglfont.h"
#include "font_si.h"
#include "font_16x16.h"

// Texture Packer auto-generated UV coords
#include "uvcoord_font_si.h"
#include "uvcoord_font_16x16.h"

// nds resolution is 256 × 192 in both screens
#define HALF_WIDTH (SCREEN_WIDTH / 2)
#define HALF_HEIGHT (SCREEN_HEIGHT / 2)

const int CELL_SIZE = 8;
const int CELL_COUNT = 24;

const int SCREEN_WIDTH_SNAKE = CELL_SIZE * CELL_COUNT;
const int SCREEN_HEIGHT_SNAKE = CELL_SIZE * CELL_COUNT;

// This imageset would use our texture packer generated coords so it's kinda
// safe and easy to use
// FONT_SI_NUM_IMAGES is a value #defined from "uvcoord_font_si.h"
glImage FontImages[FONT_SI_NUM_IMAGES];
glImage FontBigImages[FONT_16X16_NUM_IMAGES];

// Our fonts
Cglfont Font;
Cglfont FontBig;

mm_sound_effect collisionSound;

const int WHITE = RGB15(255, 255, 255);
const int RED = RGB15(255, 0, 0);
const int GREEN = RGB15(0, 255, 0);
const int BLUE = RGB15(0, 0, 255);

bool isGamePaused;

int score;
int highScore;

typedef struct
{
	int x;
	int y;
} Vector2;

typedef struct
{
	int cellCount;
	int cellSize;
	std::deque<Vector2> body;
	Vector2 direction;
	bool shouldAddSegment;
} Snake;

Snake snake;

typedef struct
{
	int cellCount;
	int cellSize;
	Vector2 position;
	bool isDestroyed;
} Food;

Food food;

Rectangle foodBounds;

int rand_range(int min, int max)
{
	return min + rand() / (RAND_MAX / (max - min + 1) + 1);
}

Vector2 generateRandomPosition()
{
	int positionX = rand_range(0, CELL_COUNT - 1);
	int positionY = rand_range(0, CELL_COUNT - 1);

	return Vector2{positionX, positionY};
}

Vector2 vector2Add(Vector2 vector1, Vector2 vector2)
{
	Vector2 result = {vector1.x + vector2.x, vector1.y + vector2.y};

	return result;
}

int vector2Equals(Vector2 vector1, Vector2 vector2)
{
	const float EPSILON = 0.000001f;
	int result = ((fabsf(vector1.x - vector2.x)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.x), fabsf(vector2.x))))) &&
				 ((fabsf(vector1.y - vector2.y)) <= (EPSILON * fmaxf(1.0f, fmaxf(fabsf(vector1.y), fabsf(vector2.y)))));

	return result;
}

double lastUpdateTime = 0;

bool eventTriggered(int counter)
{
	lastUpdateTime += counter;

	// wait 0.2 seconds.
	if (lastUpdateTime >= 100)
	{
		lastUpdateTime = 0;

		return true;
	}

	return false;
}

void saveScore()
{
	std::string path = "high-score.txt";

	std::ofstream highScores(path);

	std::string scoreString = std::to_string(score);
	highScores << scoreString;

	highScores.close();
}

int loadHighScore()
{
	std::string highScoreText;

	std::string path = "high-score.txt";

	std::ifstream highScores(path);

	if (!highScores.is_open())
	{
		saveScore();

		std::ifstream auxHighScores(path);

		getline(auxHighScores, highScoreText);

		highScores.close();

		int highScore = stoi(highScoreText);

		return highScore;
	}

	getline(highScores, highScoreText);

	highScores.close();

	int highScore = stoi(highScoreText);

	return highScore;
}

void resetSnakePosition()
{
	// highScore = loadHighScore();

	if (score > highScore)
	{
		highScore = score;
		saveScore();
	}

	snake.body = {{6, 9}, {5, 9}, {4, 9}};
	snake.direction = {1, 0};

	score = 0;
}

bool checkCollisionWithFood(Vector2 foodPosition)
{
	if (vector2Equals(snake.body[0], foodPosition))
	{
		mmEffectEx(&collisionSound);

		snake.shouldAddSegment = true;
		return true;
	}

	return false;
}

void checkCollisionWithEdges()
{
	if (snake.body[0].x == CELL_COUNT || snake.body[0].x == -1 || snake.body[0].y == CELL_COUNT || snake.body[0].y == -1)
	{
		resetSnakePosition();
	}
}

void checkCollisionBetweenHeadAndBody()
{
	for (size_t i = 1; i < snake.body.size(); i++)
	{
		if (vector2Equals(snake.body[0], snake.body[i]))
		{
			resetSnakePosition();
		}
	}
}

int counter = 0;

void update(int keyDown)
{
	counter++;

	if (eventTriggered(counter))
	{
		counter = 0;
		if (!snake.shouldAddSegment)
		{
			snake.body.pop_back();
			snake.body.push_front(vector2Add(snake.body[0], snake.direction));
		}
		else
		{
			snake.body.push_front(vector2Add(snake.body[0], snake.direction));
			snake.shouldAddSegment = false;
		}
	}

	if (keyDown & KEY_UP && snake.direction.y != 1)
	{
		snake.direction = {0, -1};
	}

	else if (keyDown & KEY_DOWN && snake.direction.y != -1)
	{
		snake.direction = {0, 1};
	}

	else if (keyDown & KEY_LEFT && snake.direction.x != 1)
	{
		snake.direction = {-1, 0};
	}

	else if (keyDown & KEY_RIGHT && snake.direction.x != -1)
	{
		snake.direction = {1, 0};
	}

	checkCollisionWithEdges();
	checkCollisionBetweenHeadAndBody();

	food.isDestroyed = checkCollisionWithFood(food.position);

	if (food.isDestroyed)
	{
		food.position = generateRandomPosition();
		score++;
	}
}

void renderTopScreen()
{
	lcdMainOnBottom();
	vramSetBankC(VRAM_C_LCD);
	vramSetBankD(VRAM_D_SUB_SPRITE);
	REG_DISPCAPCNT = DCAP_BANK(2) | DCAP_ENABLE | DCAP_SIZE(3);

	glBegin2D();

	for (size_t i = 0; i < snake.body.size(); i++)
	{
		int positionX = snake.body[i].x;
		int positionY = snake.body[i].y;

		Rectangle bodyBounds = {(float)positionX * CELL_SIZE,(float) positionY * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE};

		drawRectangle(bodyBounds);
	}

	//(float) to avoid warning of conversion.
	foodBounds = {(float)food.position.x * CELL_SIZE, (float) food.position.y * CELL_SIZE, CELL_SIZE, CELL_SIZE, WHITE};

	drawRectangle(foodBounds);

	glLine(0, 1, SCREEN_WIDTH_SNAKE, 1, WHITE);
	glLine(0, SCREEN_HEIGHT_SNAKE - 1, SCREEN_WIDTH_SNAKE, SCREEN_HEIGHT_SNAKE - 1, WHITE);
	glLine(0, 0, 0, SCREEN_HEIGHT_SNAKE, WHITE);
	glLine(SCREEN_WIDTH_SNAKE - 1, 0, SCREEN_WIDTH_SNAKE - 1, SCREEN_HEIGHT_SNAKE, WHITE);

	glEnd2D();
}

void renderBottomScreen()
{
	lcdMainOnTop();
	vramSetBankD(VRAM_D_LCD);
	vramSetBankC(VRAM_C_SUB_BG);
	REG_DISPCAPCNT = DCAP_BANK(3) | DCAP_ENABLE | DCAP_SIZE(3);

	glBegin2D();

	glColor(RGB15(0, 31, 31));

	std::string scoreString = "SCORE: " + std::to_string(score);

	Font.Print(HALF_WIDTH + 40, 20, scoreString.c_str());

	std::string highScoreString = "HIGH: " + std::to_string(highScore);

	Font.Print(20, 20, highScoreString.c_str());

	if (isGamePaused)
	{
		Font.PrintCentered(0, HALF_HEIGHT, "GAME PAUSED");
	}

	glEnd2D();
}

int main(int argc, char *argv[])
{
	videoSetMode(MODE_5_3D);

	videoSetModeSub(MODE_5_2D);

	initSubSprites();
	mmInitDefaultMem((mm_addr)soundbank_bin);

	bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);

	glScreen2D();

	vramSetBankA(VRAM_A_TEXTURE);
	vramSetBankE(VRAM_E_TEX_PALETTE);

	Font.Load(FontImages,
			  FONT_SI_NUM_IMAGES,
			  font_si_texcoords,
			  GL_RGB256,
			  TEXTURE_SIZE_64,
			  TEXTURE_SIZE_128,
			  GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
			  256,
			  (u16 *)font_siPal,
			  (u8 *)font_siBitmap);

	FontBig.Load(FontBigImages,
				 FONT_16X16_NUM_IMAGES,
				 font_16x16_texcoords,
				 GL_RGB256,
				 TEXTURE_SIZE_64,
				 TEXTURE_SIZE_512,
				 GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T | TEXGEN_OFF | GL_TEXTURE_COLOR0_TRANSPARENT,
				 256,
				 (u16 *)font_siPal,
				 (u8 *)font_16x16Bitmap);

	mmLoadEffect(SFX_MAGIC);

	collisionSound = {
		{SFX_MAGIC},			 // id
		(int)(1.0f * (1 << 10)), // rate
		0,						 // handle
		100,					 // volume
		255,					 // panning
	};

	srand(time(NULL));

	Vector2 initialFoodPosition = generateRandomPosition();

	food = {CELL_COUNT, CELL_SIZE, initialFoodPosition, false};

	std::deque<Vector2> initialBody = {{6, 9}, {5, 9}, {4, 9}};
	Vector2 direction = {1, 0};

	snake = {CELL_COUNT, CELL_SIZE, initialBody, direction, false};

	int frame = 0;

	while (true)
	{
		frame++;

		scanKeys();

		int keyDown = keysDown();

		if (keyDown & KEY_START)
		{
			isGamePaused = !isGamePaused;
			mmEffectEx(&collisionSound);
		}

		if (!isGamePaused)
		{
			update(keyDown);
		}

		while (REG_DISPCAPCNT & DCAP_ENABLE);

		if ((frame & 1) == 0)
		{
			renderTopScreen();
		}
		else
		{
			renderBottomScreen();
		}

		glFlush(0);
		swiWaitForVBlank();
	}
}
