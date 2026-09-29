// Galaga-style game (requires raylib: https://www.raylib.com/)
// Put your MP3 beside the executable and name it "ship_destroyed.mp3".
// Example build: g++ GalagaGame.cpp -std=c++17 -lraylib -lopengl32 -lgdi32 -lwinmm
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct Bullet {
	Vector2 position;
	float speed;
	bool enemy;
};

struct Enemy {
	Vector2 position;
	bool alive = true;
};

static bool Hit(Rectangle a, Rectangle b) { return CheckCollisionRecs(a, b); }

int main() {
	constexpr int screenWidth = 960;
	constexpr int screenHeight = 720;
	InitWindow(screenWidth, screenHeight, "Galaga Style");
	InitAudioDevice();
	SetTargetFPS(60);

	std::vector<Sound> destroyedSounds;
	for (int voice = 0; voice < 8; ++voice) {
		Sound sound = LoadSound("ship_destroyed.mp3");
		if (IsSoundValid(sound)) destroyedSounds.push_back(sound);
	}
	const bool hasDestroyedSound = !destroyedSounds.empty();
	std::size_t destroyedSoundIndex = 0;
	auto playDestroyedSound = [&]() {
		if (!hasDestroyedSound) return;
		PlaySound(destroyedSounds[destroyedSoundIndex]);
		destroyedSoundIndex = (destroyedSoundIndex + 1) % destroyedSounds.size();
	};
	Vector2 player{screenWidth / 2.0f, screenHeight - 70.0f};
	std::vector<Bullet> bullets;
	std::vector<Enemy> enemies;
	int score = 0, lives = 3, wave = 1;
	float shootTimer = 0.0f, enemyShootTimer = 0.0f, formationOffset = 0.0f;

	auto createWave = [&]() {
		enemies.clear();
		for (int row = 0; row < 4; ++row)
			for (int col = 0; col < 9; ++col)
				enemies.push_back({{140.0f + col * 85.0f, 95.0f + row * 58.0f}, true});
		formationOffset = 0.0f;
	};
	auto restart = [&]() {
		score = 0; lives = 3; wave = 1; bullets.clear();
		player = {screenWidth / 2.0f, screenHeight - 70.0f};
		createWave();
	};
	createWave();

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		bool gameOver = lives <= 0;
		if (gameOver && IsKeyPressed(KEY_ENTER)) restart();

		if (!gameOver) {
			float direction = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) -
							  (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
			player.x = std::clamp(player.x + direction * 420.0f * dt, 25.0f, screenWidth - 25.0f);
			shootTimer -= dt;
			if ((IsKeyDown(KEY_SPACE) || IsMouseButtonDown(MOUSE_BUTTON_LEFT)) && shootTimer <= 0.0f) {
				bullets.push_back({{player.x, player.y - 22.0f}, -610.0f, false});
				shootTimer = 0.18f;
			}

			formationOffset += 55.0f * dt;
			for (Enemy &enemy : enemies)
				if (enemy.alive) enemy.position.x += std::sin(formationOffset * 0.025f + enemy.position.y) * 18.0f * dt;

			enemyShootTimer -= dt;
			if (enemyShootTimer <= 0.0f) {
				std::vector<Enemy *> shooters;
				for (Enemy &enemy : enemies) if (enemy.alive) shooters.push_back(&enemy);
				if (!shooters.empty()) {
					Enemy *shooter = shooters[GetRandomValue(0, (int)shooters.size() - 1)];
					bullets.push_back({{shooter->position.x, shooter->position.y + 18.0f}, 270.0f, true});
				}
				enemyShootTimer = 0.65f;
			}

			for (Bullet &bullet : bullets) bullet.position.y += bullet.speed * dt;
			Rectangle playerBox{player.x - 22, player.y - 16, 44, 32};
			for (Bullet &bullet : bullets) {
				if (!bullet.enemy) {
					for (Enemy &enemy : enemies) {
						Rectangle enemyBox{enemy.position.x - 21, enemy.position.y - 15, 42, 30};
						if (enemy.alive && Hit({bullet.position.x - 3, bullet.position.y - 8, 6, 16}, enemyBox)) {
							enemy.alive = false; bullet.position.y = -100.0f; score += 100;
							playDestroyedSound();
							break;
						}
					}
				} else if (Hit({bullet.position.x - 3, bullet.position.y - 8, 6, 16}, playerBox)) {
					bullet.position.y = screenHeight + 100.0f; --lives;
					playDestroyedSound();
				}
			}
			bullets.erase(std::remove_if(bullets.begin(), bullets.end(), [](const Bullet &b) {
				return b.position.y < -30 || b.position.y > screenHeight + 30;
			}), bullets.end());

			if (std::none_of(enemies.begin(), enemies.end(), [](const Enemy &e) { return e.alive; })) {
				++wave; bullets.clear(); createWave();
			}
		}

		BeginDrawing();
		ClearBackground({5, 8, 24, 255});
		for (int i = 0; i < 90; ++i) DrawPixel((i * 79) % screenWidth, (i * 137) % screenHeight, Fade(WHITE, 0.45f));
		for (const Enemy &enemy : enemies) if (enemy.alive) {
			Color color = ((int)(enemy.position.y / 58) % 2) ? ORANGE : SKYBLUE;
			DrawTriangle({enemy.position.x, enemy.position.y - 17}, {enemy.position.x - 23, enemy.position.y + 15}, {enemy.position.x + 23, enemy.position.y + 15}, color);
			DrawCircleV({enemy.position.x, enemy.position.y + 2}, 7, RAYWHITE);
		}
		for (const Bullet &bullet : bullets)
			DrawRectangle((int)bullet.position.x - 2, (int)bullet.position.y - 7, 4, 14, bullet.enemy ? RED : YELLOW);
		if (lives > 0) {
			DrawTriangle({player.x, player.y - 22}, {player.x - 23, player.y + 17}, {player.x + 23, player.y + 17}, LIME);
			DrawRectangle((int)player.x - 5, (int)player.y, 10, 20, GREEN);
		}
		DrawText(TextFormat("SCORE %06i", score), 24, 20, 24, RAYWHITE);
		DrawText(TextFormat("LIVES %i   WAVE %i", std::max(lives, 0), wave), 690, 20, 24, RAYWHITE);
		if (!hasDestroyedSound) DrawText("Add ship_destroyed.mp3 beside the executable", 235, 685, 16, YELLOW);
		if (gameOver) {
			DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.62f));
			DrawText("GAME OVER", 350, 300, 48, RED);
			DrawText("Press ENTER to restart", 330, 365, 24, RAYWHITE);
		}
		EndDrawing();
	}
	for (Sound sound : destroyedSounds) UnloadSound(sound);
	CloseAudioDevice();
	CloseWindow();
	return 0;
}
