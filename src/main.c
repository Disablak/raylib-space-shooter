#include "stdio.h"
#include <stdlib.h> 
#include "raylib.h"
#include "resource_dir.h"
#include "raymath.h"


typedef struct Ship
{
	bool is_alive;
	bool is_player;
	bool is_thrust;
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

typedef struct Animation
{
	int cur_frame;
	int frame_count;
	Vector2 pos;
	Rectangle texture_rect;
	bool is_stopped;
} Animation;


float LerpAngle(float startAngle, float endAngle, float t);


const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 800;

//general
Texture2D atlas;
const int GAME_STATE_GAMEPLAY = 0;
const int GAME_STATE_LOOSE = 1;
const int GAME_STATE_VICTORY = 2;
int game_state = GAME_STATE_GAMEPLAY;
float time_hint = 5.0f;

//animations
int frame_counter = 0;
int frame_speed = 8;
int animation_count = 0;
Animation *animations;

//player
Ship player_ship;
Rectangle ship_texture_rect = {0, 0, 32, 32};
Rectangle ship_effect_texture_rect = {32, 0, 32, 32};
Rectangle ship_effect_dest = {};
Vector2 ship_origin = {16, 16};

//black hole
Vector2 black_hole_pos = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2};
float black_hole_radius = 512;
Rectangle black_hole_texture_rect = {64, 0, 32, 32};
Rectangle black_hole_dest = {SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 128, 128};
Vector2 black_hole_origin = {64, 64};

//bullets
const int BULLETS_LENGTH = 100;
Bullet all_bullets[BULLETS_LENGTH];
Rectangle bullet_texture_rect = {96, 0, 32, 32};
Rectangle bullet_dest = {0, 0, 32, 32};
const float BULLET_COOLDOWN_TIME = 0.3f;
float bullet_cooldown = 0;

//enemy
Ship enemy_ship;
const int ENEMY_STATE_NONE = 0;
const int ENEMY_STATE_THRUST = 1;
const int ENEMY_STATE_ROTATE = 2;
const float TIME_THRUST = 1.0f;
const float DELAY_THRUST = 0.5f;
int enemy_state = ENEMY_STATE_THRUST;
float enemy_time = -1.0f;
int enemy_rot_dir = 1;
float enemy_rot_time = 0.0f;


float FromMilisecToSec(int milisec)
{
	int seconds = milisec / 1000;
	if (milisec % 1000 > 500)
    	seconds++;
	
	return seconds;
}

float RandomRange(int milisecFrom, int milisecTo)
{
	return FromMilisecToSec(GetRandomValue(milisecFrom, milisecTo));
}

void GameReset()
{
	game_state = GAME_STATE_GAMEPLAY;
	time_hint = 5;

	player_ship.is_alive = true;
	player_ship.is_player = true;
	player_ship.pos = (Vector2){20, SCREEN_HEIGHT / 2};
	player_ship.dest = (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 32, 32};
	player_ship.vel = (Vector2){0, 0};
	player_ship.angle_vel = 0;
	player_ship.angle = 0;
	player_ship.rot_speed = 100;
	player_ship.speed_acc = 200;

	bullet_cooldown = 0.0f;

	enemy_ship.is_alive = true;
	enemy_ship.is_player = false;
	enemy_ship.pos = (Vector2){SCREEN_WIDTH - 20, SCREEN_HEIGHT / 2};
	enemy_ship.dest = (Rectangle){SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, 32, 32};
	enemy_ship.vel = (Vector2){0, 0};
	enemy_ship.angle_vel = 0;
	enemy_ship.angle = 180;
	enemy_ship.rot_speed = 100;
	enemy_ship.speed_acc = 200;

	enemy_state = ENEMY_STATE_THRUST;
	enemy_time = -1.0f;

	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		all_bullets[i].is_active = false;
		all_bullets[i].angle = 0;
		all_bullets[i].speed = 1000;
	}
}

void GameInit()
{
	atlas = LoadTexture("atlas.png");

	GameReset();
}

void DrawThrustEffect(Ship *ship)
{
	if (ship->is_thrust == false)
		return;

	Vector2 effect_offset = {cosf(ship->angle * DEG2RAD), sinf(ship->angle * DEG2RAD)};
	effect_offset = Vector2Scale(effect_offset, -16);
	ship_effect_dest = (Rectangle){ship->pos.x + effect_offset.x, ship->pos.y + effect_offset.y, 32, 32};
	
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

	DrawTexturePro(atlas, ship_texture_rect, ship->dest, ship_origin, ship->angle, ship->is_player ? BLUE : RED);
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

void DrawExplosionEffect()
{
	frame_counter++;

	if (frame_counter >= (60/frame_speed))
	{
		frame_counter = 0;

		for (size_t i = 0; i < animation_count; i++)
		{
			if (animations[i].is_stopped)
				continue;

			animations[i].cur_frame++;

			if (animations[i].cur_frame > animations[i].frame_count - 1)
				animations[i].is_stopped = true;
			
			animations[i].texture_rect.x = animations[i].cur_frame * 32;
		}
	}

	for (size_t i = 0; i < animation_count; i++)
	{
		if (animations[i].is_stopped)
			continue;
		
		DrawTextureRec(atlas, animations[i].texture_rect, animations[i].pos, WHITE);
	}
}

void SpawnExplosionEffect(Vector2 pos)
{
	animation_count++;
	animations = realloc(animations, animation_count * sizeof(Animation));

	animations[animation_count - 1].pos = Vector2Subtract(pos, (Vector2){16, 16});
	animations[animation_count - 1].texture_rect = (Rectangle){0, 32, 32, 32};
	animations[animation_count - 1].cur_frame = 0;
	animations[animation_count - 1].frame_count = 4;
	animations[animation_count - 1].is_stopped = false;
}

void GameWin()
{
	if (game_state == GAME_STATE_GAMEPLAY)
		game_state = GAME_STATE_VICTORY;
}

void GameLose()
{
	if (game_state == GAME_STATE_GAMEPLAY)
		game_state = GAME_STATE_LOOSE;
}

void DrawUI()
{
	if (game_state == GAME_STATE_GAMEPLAY)
	{
		time_hint -= GetFrameTime();
		if (time_hint <= 0)
			return;
		
		DrawText("Use arrows to controll and space to fire. To reset press R", 130, SCREEN_HEIGHT - 40, 32, WHITE);
	}
	else if (game_state == GAME_STATE_LOOSE)
	{
		DrawText("YOU LOST", 500, 600, 48, RED);
	}
	else if (game_state == GAME_STATE_VICTORY)
	{
		DrawText("YOU WIN!", 500, 600, 48, WHITE);
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

	if (bullet_cooldown > 0.0f)
		return;

	bullet_cooldown = BULLET_COOLDOWN_TIME;

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
	player_ship.is_thrust = IsKeyDown(KEY_UP);

    if (IsKeyDown(KEY_LEFT))  player_ship.angle_vel = -player_ship.rot_speed;
	if (IsKeyDown(KEY_RIGHT)) player_ship.angle_vel = player_ship.rot_speed;

	if (IsKeyReleased(KEY_LEFT) || IsKeyReleased(KEY_RIGHT)) player_ship.angle_vel = 0;

	if (IsKeyDown(KEY_SPACE)) Shoot();

	if (IsKeyReleased(KEY_R)) GameReset();
}

void UpdateBlackHoleInfluence(Ship *ship)
{
	Vector2 direction = Vector2Subtract(black_hole_pos, ship->pos);
	float distance = Vector2Length(direction);
	float forceMagnitude = (5000 * 10 * 1000) / (distance * distance);
	Vector2 force = Vector2Scale(Vector2Normalize(direction), forceMagnitude);

	Vector2 acceleration = Vector2Scale(force, 1.0f / 10);
    ship->vel = Vector2Add(ship->vel, Vector2Scale(acceleration, GetFrameTime()));
    ship->pos = Vector2Add(ship->pos, Vector2Scale(ship->vel, GetFrameTime()));
}

void UpdateShipPos(Ship *ship)
{
	if (ship->is_alive == false)
		return;

	if (ship->is_thrust)
		ApplyThrust(ship);

	ship->pos = Vector2Add(ship->pos, Vector2Scale(ship->vel, GetFrameTime()));
	ship->angle += ship->angle_vel * GetFrameTime();

	if (ship->pos.x > SCREEN_WIDTH) ship->pos.x = 0;
	if (ship->pos.x < 0) ship->pos.x = SCREEN_WIDTH;
	if (ship->pos.y > SCREEN_HEIGHT) ship->pos.y = 0;
	if (ship->pos.y < 0) ship->pos.y = SCREEN_HEIGHT;
}

void UpdateEnemyLogic(float dt)
{
	if (enemy_time > 0.0f)
	{
		enemy_time -= dt;

		if (enemy_state == ENEMY_STATE_THRUST)
		{
			enemy_ship.is_thrust = true;
		}
		if (enemy_state == ENEMY_STATE_ROTATE)
		{
			enemy_ship.is_thrust = true;
			enemy_ship.angle_vel = enemy_ship.rot_speed * enemy_rot_dir;
		}
	}

	if (enemy_time < 0.0f)
	{
		enemy_ship.is_thrust = false;

		if (enemy_state == ENEMY_STATE_NONE)
		{
			enemy_time = TIME_THRUST;
			enemy_state = ENEMY_STATE_THRUST;
		}
		else if (enemy_state == ENEMY_STATE_THRUST)
		{
			enemy_time = RandomRange(500, 1000);
			enemy_rot_dir = GetRandomValue(-1, 1);
			enemy_state = ENEMY_STATE_ROTATE;
		}
		else if (enemy_state == ENEMY_STATE_ROTATE)
		{
			enemy_ship.angle_vel = 0.0f;
			enemy_time = DELAY_THRUST;
			enemy_state = ENEMY_STATE_NONE;
		}
	}
	
	UpdateShipPos(&enemy_ship);
}

void DestroyShip(Ship *ship)
{
	ship->is_alive = false;

	SpawnExplosionEffect(ship->pos);

	if (ship->is_player)
		GameLose();
	else
		GameWin();
}

void CheckCollisionWithBlackhole()
{
	if (player_ship.is_alive == false)
		return;

	float distance = Vector2Distance(player_ship.pos, black_hole_pos);
	if (distance < 32)
	{
		DestroyShip(&player_ship);
	}
}

void CheckCollisionWithEnemy()
{
	if (player_ship.is_alive == false)
		return;
	
	if (enemy_ship.is_alive == false)
		return;
	
	float distance = Vector2Distance(player_ship.pos, enemy_ship.pos);
	if (distance < 16)
	{
		DestroyShip(&player_ship);
		DestroyShip(&enemy_ship);
	}
}

void UpdateBullets()
{
	bullet_cooldown -= GetFrameTime();

	for (size_t i = 0; i < BULLETS_LENGTH; i++)
	{
		if (all_bullets[i].is_active == false)
			continue;
		
		Vector2 dir = {cosf(all_bullets[i].angle * DEG2RAD), sinf(all_bullets[i].angle * DEG2RAD)};
		Vector2 vel = Vector2Scale(dir, all_bullets[i].speed * GetFrameTime());
		Vector2 new_pos = Vector2Add(all_bullets[i].pos, vel);
		all_bullets[i].pos = new_pos;

		if (new_pos.x > SCREEN_WIDTH || new_pos.x < 0 || new_pos.y > SCREEN_HEIGHT || new_pos.y < 0)
		{
			all_bullets[i].is_active = false;
			continue;
		}
		
		float distanceToBlackHole = Vector2Distance(new_pos, black_hole_pos);
		if (distanceToBlackHole < 16)
		{
			all_bullets[i].is_active = false;
			continue;
		}

		if (enemy_ship.is_alive == false)
			continue;

		if (Vector2Distance(new_pos, enemy_ship.pos) < 12)
		{
			DestroyShip(&enemy_ship);
			all_bullets[i].is_active = false;
		}
	}
}

void GameLogic()
{
	UpdateBlackHoleInfluence(&player_ship);

	UpdateShipPos(&player_ship);
	UpdateEnemyLogic(GetFrameTime());
 
	UpdateBullets();
	CheckCollisionWithBlackhole();
	CheckCollisionWithEnemy();
}

void DrawScreen()
{
	BeginDrawing();

		ClearBackground(BLACK);

		DrawShip(&player_ship);
		DrawShip(&enemy_ship);
		DrawBlackHole();
		DrawBullets();
		DrawExplosionEffect();

		DrawUI();
		
	EndDrawing();
}

void Cleanup()
{
	UnloadTexture(atlas);
	free(animations);
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