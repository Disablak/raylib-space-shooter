#include "stdio.h"
#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raymath.h"


typedef struct Ship
{
	Vector2 pos;
	Vector2 vel;
	Vector2 move_dir;
	float angle;
	float angle_vel;
	float speed;
	float rot_speed;
	float speed_acc;
	float speed_max;
	Rectangle dest;
} Ship;

float LerpAngle(float startAngle, float endAngle, float t);


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 800;

Texture2D atlas;
Ship player_ship;
Rectangle ship_texture_rect = {0, 0, 32, 32};
Vector2 draw_pos = {400, 200};
Vector2 ship_origin = {16, 16};


void UpdateShipPos(Ship *ship)
{
	ship->pos = Vector2Add(ship->pos, Vector2Scale(ship->vel, GetFrameTime()));
    ship->angle += ship->angle_vel * GetFrameTime();

	if (ship->pos.x > SCREEN_WIDTH) ship->pos.x = 0;
    if (ship->pos.x < 0) ship->pos.x = SCREEN_WIDTH;
    if (ship->pos.y > SCREEN_HEIGHT) ship->pos.y = 0;
    if (ship->pos.y < 0) ship->pos.y = SCREEN_HEIGHT;
}

void ApplyThrust(Ship *ship)
{
	float thrust = ship->speed_acc * GetFrameTime();
    Vector2 force = { cosf(ship->angle * DEG2RAD) * thrust, sinf(ship->angle * DEG2RAD) * thrust };
    ship->vel = Vector2Add(ship->vel, force);
}

void GameInit()
{
	atlas = LoadTexture("atlas.png");

	player_ship.dest = (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 32, 32};
	player_ship.pos = (Vector2){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
	player_ship.angle = 0;
	player_ship.rot_speed = 100;
	player_ship.speed_acc = 200;
}

void DrawShip(Ship *ship, Texture2D texture)
{
	ship->dest.x = ship->pos.x;
	ship->dest.y = ship->pos.y;
	DrawTexturePro(texture, ship_texture_rect, ship->dest, ship_origin, ship->angle, WHITE);
}

void ListenInputs()
{
    if (IsKeyDown(KEY_UP))    ApplyThrust(&player_ship);
    if (IsKeyDown(KEY_LEFT))  player_ship.angle_vel = -player_ship.rot_speed;
	if (IsKeyDown(KEY_RIGHT)) player_ship.angle_vel = player_ship.rot_speed;
	//player_ship.angle += player_ship.rot_speed * GetFrameTime();
	//player_ship.angle -= player_ship.rot_speed * GetFrameTime();
	//player_ship.speed += player_ship.speed_acc * GetFrameTime();
    //if (IsKeyDown(KEY_DOWN))  player_ship.speed -= player_ship.speed_acc * GetFrameTime();

	if (IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_RIGHT)) player_ship.angle_vel = 0;

	//if (IsKeyDown(KEY_UP)) UpdatePlayerMoveDir();

	//player_ship.speed = Clamp(player_ship.speed, 1, player_ship.speed_max);

	UpdateShipPos(&player_ship);
}

void DrawScreen()
{
	BeginDrawing();

		ClearBackground(BLACK);

		DrawShip(&player_ship, atlas);
		
	EndDrawing();
}

void Cleanup()
{
	UnloadTexture(atlas);
}

int main ()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Space Shooter");
	SearchAndSetResourceDir("resources");
	SetTargetFPS(60);

	GameInit();
	
	while (!WindowShouldClose())
	{
		ListenInputs();
		DrawScreen();
	}

	Cleanup();

	CloseWindow();
	return 0;
}

float LerpAngle(float startAngle, float endAngle, float t) {
	float difference = fmod(endAngle - startAngle + 180, 360) - 180;
	return startAngle + difference * t;
}