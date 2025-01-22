#include "stdio.h"
#include <stdlib.h> 
#include "raylib.h"
#include "resource_dir.h"	// utility header for SearchAndSetResourceDir
#include "raymath.h"


typedef struct Ship
{
	bool is_alive;
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

typedef struct Bullet
{
	bool is_active;
	Vector2 pos;
	float angle;
	float speed;
} Bullet;


float LerpAngle(float startAngle, float endAngle, float t);


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 800;

Texture2D atlas;
Ship player_ship;
Rectangle ship_texture_rect = {0, 0, 32, 32};
Rectangle ship_effect_texture_rect = {32, 0, 32, 32};
Rectangle ship_effect_dest = {};

Vector2 draw_pos = {400, 200};
Vector2 ship_origin = {16, 16};

Vector2 black_hole_pos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
float black_hole_radius = 512;
Rectangle black_hole_texture_rect = {64, 0, 32, 32};
Rectangle black_hole_dest = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 128, 128};
Vector2 black_hole_origin = {64, 64};

const int BULLETS_LENGTH = 100;
Bullet all_bullets[BULLETS_LENGTH];
Rectangle bullet_texture_rect = {96, 0, 32, 32};
Rectangle bullet_dest = {0, 0, 32, 32};


void GameInit()
{
	atlas = LoadTexture("atlas.png");

	player_ship.is_alive = true;
	player_ship.pos = (Vector2){20, SCREEN_HEIGHT / 2};
	player_ship.dest = (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 32, 32};
	player_ship.angle = 0;
	player_ship.rot_speed = 100;
	player_ship.speed_acc = 200;

	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		all_bullets[i].is_active = false;
		all_bullets[i].angle = 0;
		all_bullets[i].speed = 1000;
	}
}

void DrawThrustEffect(Ship *ship)
{
	Vector2 effect_offset = {cosf(ship->angle * DEG2RAD), sinf(ship->angle * DEG2RAD)};
	effect_offset = Vector2Scale(effect_offset, -16);
	ship_effect_dest = (Rectangle){ship->pos.x + effect_offset.x, ship->pos.y + effect_offset.y, 32, 32};
	if (IsKeyDown(KEY_UP))
		DrawTexturePro(atlas, ship_effect_texture_rect, ship_effect_dest, ship_origin, ship->angle, WHITE);
}

void DrawShip(Ship *ship)
{
	if (ship->is_alive == false)
	{
		return;
	}

	ship->dest.x = ship->pos.x;
	ship->dest.y = ship->pos.y;

	DrawTexturePro(atlas, ship_texture_rect, ship->dest, ship_origin, ship->angle, WHITE);
	DrawThrustEffect(ship);
}

void DrawBlackHole()
{
	DrawTexturePro(atlas, black_hole_texture_rect, black_hole_dest, black_hole_origin, GetTime() * 50, WHITE);
}

void DrawBullets()
{
	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		Bullet bullet = all_bullets[i];
		if (bullet.is_active == false) 
			continue;
		
		bullet_dest.x = bullet.pos.x;
		bullet_dest.y = bullet.pos.y;
		DrawTexturePro(atlas, bullet_texture_rect, bullet_dest, ship_origin, bullet.angle, WHITE);
	}
}

void ApplyThrust(Ship *ship)
{
	float thrust = ship->speed_acc * GetFrameTime();
    Vector2 force = { cosf(ship->angle * DEG2RAD) * thrust, sinf(ship->angle * DEG2RAD) * thrust };
    ship->vel = Vector2Add(ship->vel, force);
}

void Shoot()
{
	if (player_ship.is_alive == false)
		return;

	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		if (all_bullets[i].is_active == false) 
		{
            all_bullets[i].is_active = true;
            all_bullets[i].pos = player_ship.pos;
            all_bullets[i].angle = player_ship.angle;
            break;
		}
	}
}

void ListenInputs()
{
    if (IsKeyDown(KEY_UP))    ApplyThrust(&player_ship);
    if (IsKeyDown(KEY_LEFT))  player_ship.angle_vel = -player_ship.rot_speed;
	if (IsKeyDown(KEY_RIGHT)) player_ship.angle_vel = player_ship.rot_speed;

	if (IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_RIGHT)) player_ship.angle_vel = 0;

	if (IsKeyDown(KEY_SPACE)) Shoot();
}

void UpdateBlackHoleInfluence()
{
	Vector2 direction = Vector2Subtract(black_hole_pos, player_ship.pos);
	float distance = Vector2Length(direction);
	float forceMagnitude = (5000 * 10 * 1000) / (distance * distance);
	Vector2 force = Vector2Scale(Vector2Normalize(direction), forceMagnitude);

	Vector2 acceleration = Vector2Scale(force, 1.0f / 10);
    player_ship.vel = Vector2Add(player_ship.vel, Vector2Scale(acceleration, GetFrameTime()));
    player_ship.pos = Vector2Add(player_ship.pos, Vector2Scale(player_ship.vel, GetFrameTime()));
}

void UpdateShipPos(Ship *ship)
{
	if (ship->is_alive == false)
		return;

	ship->pos = Vector2Add(ship->pos, Vector2Scale(ship->vel, GetFrameTime()));
	ship->angle += ship->angle_vel * GetFrameTime();

	if (ship->pos.x > SCREEN_WIDTH) ship->pos.x = 0;
	if (ship->pos.x < 0) ship->pos.x = SCREEN_WIDTH;
	if (ship->pos.y > SCREEN_HEIGHT) ship->pos.y = 0;
	if (ship->pos.y < 0) ship->pos.y = SCREEN_HEIGHT;
}

void DestroyPlayerShip()
{
	player_ship.is_alive = false;
}

void CheckCollision()
{
	if (player_ship.is_alive == false)
		return;

	float distance = Vector2Distance(player_ship.pos, black_hole_pos);
	if (distance < 32)
		DestroyPlayerShip();
}

void UpdateBullets()
{
	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		if (all_bullets[i].is_active == false)
			continue;
		
		Vector2 dir = {cosf(all_bullets[i].angle * DEG2RAD), sinf(all_bullets[i].angle * DEG2RAD)};
		Vector2 vel = Vector2Scale(dir, all_bullets[i].speed * GetFrameTime());
		Vector2 new_pos = Vector2Add(all_bullets[i].pos, vel);
		all_bullets[i].pos = new_pos;

		if (new_pos.x > SCREEN_WIDTH || new_pos.x < 0 || new_pos.y > SCREEN_HEIGHT || new_pos.y < 0)
			all_bullets[i].is_active = false;
	}
}

void GameLogic()
{
	UpdateBlackHoleInfluence();
	UpdateShipPos(&player_ship);
	UpdateBullets();
	CheckCollision();
}

void DrawScreen()
{
	BeginDrawing();

		ClearBackground(BLACK);

		DrawShip(&player_ship);
		DrawBlackHole();
		DrawBullets();
		
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
		GameLogic();
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