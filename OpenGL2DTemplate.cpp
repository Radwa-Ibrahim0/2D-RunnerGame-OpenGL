#include <stdlib.h>
#include <glut.h>
#include <vector>
#include <cstdlib>
#include <string>
#include <ctime>
#include <cstring>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <Windows.h>
#include <mmsystem.h>
#include <fstream>
#pragma comment(lib, "winmm.lib")
#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

enum GameState {
    GAME_RUNNING,
    GAME_OVER,
    GAME_WIN
};

enum PlayerState {
    RUNNING,
    JUMPING,
    DUCKING
};

enum ObstacleType {
    TREE,
    CLOUD
};

enum PowerUpType {
    SLOW_DOWN,
    IMMUNITY
};

struct GameObject {
    float x, y;
    float width, height;
    float speed;
    ObstacleType type;
    PowerUpType powerUpType;
};

struct BackgroundLayer {
    float x;
    float speed;
};

struct Message {
    std::string text;
    float timer;
    float y;
};

// Window dimensions
const int window_width = 800;
const int window_height = 600;

// Global variables
bool showCollisionMessage = false;
bool showScoreIncreaseMessage = false;
float collisionMessageTimer = 0.0f;
float scoreIncreaseMessageTimer = 0.0f;
const float initialGameSpeed = 0.5f;
const float minObjectSpacing = 100.0f;
GameState currentState = GAME_RUNNING;
PlayerState playerState = RUNNING;
float gameSpeed = initialGameSpeed;
const float maxGameSpeed = 2.5f;
const float gameSpeedIncrement = 0.08f;
int score = 0;
int player_lives = 5;
float remaining_time = 20.0f;
float player_x = 200;
float player_y = 150;
float playerVerticalVelocity = 0.0f;
const float jumpForce = 15.0f;
const float gravity = -0.8f;
const float duckHeight = 50;
const float minObstacleSpacing = 300.0f;
const float maxObstacleSpacing = 500.0f;
const float groundLevel = 100.0f;
const float playerHeight = 75.0f;
const float treeHeight = 60.0f;
const float cloudHeight = 120.0f;
float borderAnimationOffset = 0.0f;
const float borderAnimationSpeed = 2.0f;
float collectableAnimationOffset = 0.0f;
float powerupAnimationAngle = 0.0f;
GLuint playerTextureID;
int playerTextureWidth, playerTextureHeight;
bool isBackgroundMusicPlaying = false;
bool isSoundEffectPlaying = false;
DWORD lastPlayTime = 0;

bool isImmune = false;
float immunityTimer = 0.0f;
bool isSlowedDown = false;
float slowDownTimer = 0.0f;
const float immunityDuration = 5.0f;
const float slowDownDuration = 5.0f;
const float slowDownFactor = 0.5f;

std::vector<GameObject> obstacles;
std::vector<GameObject> collectables;
std::vector<GameObject> powerups;
std::vector<BackgroundLayer> backgroundLayers;
std::vector<Message> messages;

// Function prototypes
void display();
void reshape(int w, int h);
void timer(int);
void keyboard(unsigned char key, int x, int y);
void keyboardUp(unsigned char key, int x, int y);
void drawPlayer();
void drawObstacles();
void drawCollectables();
void drawPowerups();
void drawBackground();
void drawHUD();
void updateGameState();
void updatePlayer();
void updateObstacles();
void updateCollectables();
void updatePowerups();
void updateBackground();
void generateObstacle();
void generateCollectable(float x);
void generatePowerup(float x);
void checkCollisions();
bool checkCollision(const GameObject& obj1, const GameObject& obj2);
void initGame();
void drawCircle(float cx, float cy, float r);
void drawDiamond(float x, float y, float width, float height);
void drawCloud(float x, float y, float size);
void drawHexagon(float x, float y, float width, float height);
void drawDescriptionBox();
void playBackgroundMusic();
void playCollectSound();
void playCollisionSound();
void playWinSound();
void playLoseSound();
void drawUpperBorder();
void drawLowerBorder();
void addMessage(const std::string& text, float duration);
void drawGround();
void handleLanding();
void drawEllipse(float cx, float cy, float rx, float ry, int num_segments);
void drawCurve(float x1, float y1, float x2, float y2, float x3, float y3, int segments);
void loadBMP(const char* filename, int& width, int& height, unsigned char*& data);
void loadPlayerTexture(const char* filename);
void audioManager();
void updateAudio();
void stopBackgroundMusic();
void playImmunitySound();
void playSpeedSound();
void playSoundEffect(const char* soundFile);

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(window_width, window_height);
    glutCreateWindow("Infinite Runner Game");

    initGame();
    audioManager();
    playBackgroundMusic();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutTimerFunc(0, timer, 0);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);

    glutMainLoop();
    return 0;
}

void initGame() {
    srand(static_cast<unsigned int>(time(NULL)));

    // Initialize background layers
    backgroundLayers.push_back({ 0, 0.5f }); // Far background
    backgroundLayers.push_back({ 0, 1.0f }); // Middle background
    backgroundLayers.push_back({ 0, 2.0f }); // Near background

    loadPlayerTexture("player_image.bmp");
}

void display() {
    glClearColor(0.00f, 0.00f, 0.00f, 0.00f);
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    if (currentState == GAME_RUNNING) {
        drawBackground();
        drawLowerBorder();
        drawUpperBorder();
        drawPlayer();
        drawObstacles();
        drawCollectables();
        drawPowerups();
        drawHUD();
        drawDescriptionBox();
        drawGround();
    }
    else if (currentState == GAME_OVER) {
        // Draw game over screen
        glColor3f(1.0f, 0.0f, 0.0f);
        glRasterPos2f(window_width / 2 - 50, window_height / 2 + 20);
        const char* gameOverText = "Game Over!";
        for (const char* c = gameOverText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        // Draw score
        glColor3f(1.0f, 1.0f, 1.0f);
        char scoreText[50];
        sprintf(scoreText, "Final Score: %d", score);
        glRasterPos2f(window_width / 2 - 70, window_height / 2 - 20);
        for (const char* c = scoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }
    else if (currentState == GAME_WIN) {
        // Draw win screen
        glColor3f(0.0f, 1.0f, 0.0f);
        glRasterPos2f(window_width / 2 - 50, window_height / 2 + 20);
        const char* winText = "You Win!";
        for (const char* c = winText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }

        // Draw score
        glColor3f(1.0f, 1.0f, 1.0f);
        char scoreText[50];
        sprintf(scoreText, "Final Score: %d", score);
        glRasterPos2f(window_width / 2 - 70, window_height / 2 - 20);
        for (const char* c = scoreText; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
        }
    }

    glutSwapBuffers();
}

void reshape(int w, int h) {
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

void timer(int) {
    updateGameState();
    glutPostRedisplay();
    audioManager();
    glutTimerFunc(1000 / 60, timer, 0);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == ' ' && playerState == RUNNING) {
        playerState = JUMPING;
        playerVerticalVelocity = jumpForce;
    }
    else if (key == 'd' && playerState == RUNNING) {
        playerState = DUCKING;
    }
}

void keyboardUp(unsigned char key, int x, int y) {
    if (key == 'd' && playerState == DUCKING) {
        playerState = RUNNING;
    }
}

void drawPlayer() {
    float characterWidth = 50;
    float characterHeight = (playerState == DUCKING) ? 33 : 75;
    float headCenterX = player_x + characterWidth / 2;
    float headCenterY = player_y + characterHeight + 12.5f;

    // Draw character body (rectangle)
    glColor3f(0.2, 0.4, 0.2); // Dark green color for the shirt/jacket
    glBegin(GL_QUADS);
    glVertex2f(player_x, player_y);
    glVertex2f(player_x, player_y + characterHeight);
    glVertex2f(player_x + characterWidth, player_y + characterHeight);
    glVertex2f(player_x + characterWidth, player_y);
    glEnd();

    // Draw the face (rounder face)
    glColor3f(0.8, 0.6, 0.4); // Tan skin tone
    drawEllipse(headCenterX, headCenterY, 18, 20, 30);

    // Draw glasses (two hollow circles connected with a curve)
    glColor3f(0.1, 0.1, 0.1); // Black color for glasses frame
    glLineWidth(1.0); // Thin frame

    // Left lens
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(headCenterX - 10 + cos(angle) * 8, headCenterY + 2 + sin(angle) * 8);
    }
    glEnd();

    // Right lens
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(headCenterX + 10 + cos(angle) * 8, headCenterY + 2 + sin(angle) * 8);
    }
    glEnd();

    // Connecting curve
    drawCurve(headCenterX - 2, headCenterY + 2, headCenterX, headCenterY + 1, headCenterX + 2, headCenterY + 2, 10);

    // Left end piece
    glBegin(GL_LINES);
    glVertex2f(headCenterX - 18, headCenterY + 2);
    glVertex2f(headCenterX - 14, headCenterY + 2);
    glEnd();

    // Right end piece
    glBegin(GL_LINES);
    glVertex2f(headCenterX + 14, headCenterY + 2);
    glVertex2f(headCenterX + 18, headCenterY + 2);
    glEnd();

    glLineWidth(1.0);


    // Draw simple eyes (black dots)
    glColor3f(0.0, 0.0, 0.0);
    glPointSize(3.0);
    glBegin(GL_POINTS);
    glVertex2f(headCenterX - 8, headCenterY + 2);
    glVertex2f(headCenterX + 8, headCenterY + 2);
    glEnd();
    glPointSize(1.0);

    // Draw eyebrows
    glColor3f(0.1, 0.1, 0.1);
    glLineWidth(2.0);
    drawCurve(headCenterX - 13, headCenterY + 8, headCenterX - 8, headCenterY + 10, headCenterX - 3, headCenterY + 8, 10);
    drawCurve(headCenterX + 3, headCenterY + 8, headCenterX + 8, headCenterY + 10, headCenterX + 13, headCenterY + 8, 10);
    glLineWidth(1.0);

    // Draw simple nose
    glColor3f(0.7, 0.5, 0.3);
    drawCurve(headCenterX - 2, headCenterY, headCenterX, headCenterY - 3, headCenterX + 2, headCenterY, 10);

    // Draw mouth (slight smile)
    glColor3f(0.5, 0.3, 0.2);
    drawCurve(headCenterX - 5, headCenterY - 10, headCenterX, headCenterY - 9, headCenterX + 5, headCenterY - 10, 15);

    //// Draw beard (adjusted to look more like a beard)
    //glColor3f(0.1, 0.1, 0.1);
    //glBegin(GL_TRIANGLE_FAN);
    //glVertex2f(headCenterX, headCenterY - 15);
    //for (int i = -90; i <= 90; i++) {
    //    float angle = i * 3.14159f / 180.0f;
    //    glVertex2f(headCenterX + cos(angle) * 18, headCenterY - 15 + sin(angle) * 15);
    //}
    //glEnd();

    // Draw mustache
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(headCenterX, headCenterY - 7);
    for (int i = 180; i <= 360; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(headCenterX + cos(angle) * 8, headCenterY - 7 + sin(angle) * 3);
    }
    glEnd();

    // Draw beanie (light gray color, moved down slightly)
    glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(headCenterX, headCenterY + 18); // Moved down by 2 units
    for (int i = 0; i <= 180; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(headCenterX + cos(angle) * 20, headCenterY + 12 + sin(angle) * 15);
    }
    glEnd();

    // Draw beanie bottom edge
    glColor3f(0.7, 0.7, 0.7);
    glLineWidth(2.0);
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= 180; i++) {
        float angle = i * 3.14159f / 180.0f;
        glVertex2f(headCenterX + cos(angle) * 20, headCenterY + 13 + sin(angle) * 2);
    }
    glEnd();
    glLineWidth(1.0);

    // Draw arms (rectangles)
    glColor3f(0.2, 0.4, 0.2);
    glBegin(GL_QUADS);
    glVertex2f(player_x - 10, player_y + characterHeight - 20);
    glVertex2f(player_x - 10, player_y + characterHeight - 40);
    glVertex2f(player_x, player_y + characterHeight - 40);
    glVertex2f(player_x, player_y + characterHeight - 20);

    glVertex2f(player_x + characterWidth, player_y + characterHeight - 20);
    glVertex2f(player_x + characterWidth, player_y + characterHeight - 40);
    glVertex2f(player_x + characterWidth + 10, player_y + characterHeight - 40);
    glVertex2f(player_x + characterWidth + 10, player_y + characterHeight - 20);
    glEnd();

    // Draw legs (rectangles)
    glColor3f(0.2, 0.2, 0.2);
    glBegin(GL_QUADS);
    float legHeight = (playerState == DUCKING) ? 25 : 50;
    glVertex2f(player_x, player_y);
    glVertex2f(player_x, player_y - legHeight);
    glVertex2f(player_x + 20, player_y - legHeight);
    glVertex2f(player_x + 20, player_y);

    glVertex2f(player_x + 30, player_y);
    glVertex2f(player_x + 30, player_y - legHeight);
    glVertex2f(player_x + 50, player_y - legHeight);
    glVertex2f(player_x + 50, player_y);
    glEnd();
}

void handleLanding() {
    for (const auto& obstacle : obstacles) {
        if (obstacle.type == TREE &&
            player_x + 50 > obstacle.x &&
            player_x < obstacle.x + obstacle.width &&
            player_y <= obstacle.y + obstacle.height) {
            // Move the player to the top of the obstacle
            player_y = obstacle.y + obstacle.height;
            player_x = obstacle.x + 20;
            playerState = RUNNING;
            playerVerticalVelocity = 0;
            return;
        }
    }
}

void drawCloud(float x, float y, float size) {
    glColor3f(0.7f, 0.7f, 0.7f);

    // Main cloud body
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 180; i++) {
        float angle = i * M_PI / 180;
        glVertex2f(x + cos(angle) * size, y + sin(angle) * (size * 0.6f));
    }
    glEnd();

    // Additional cloud puffs
    float puffSizes[] = { size * 0.7f, size * 0.6f, size * 0.5f };
    float puffOffsets[] = { -size * 0.6f, size * 0.1f, size * 0.8f };

    for (int i = 0; i < 3; i++) {
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(x + puffOffsets[i], y + puffSizes[i] * 0.3f);
        for (int j = 0; j <= 180; j++) {
            float angle = j * M_PI / 180;
            glVertex2f(x + puffOffsets[i] + cos(angle) * puffSizes[i],
                y + puffSizes[i] * 0.3f + sin(angle) * (puffSizes[i] * 0.8f));
        }
        glEnd();
    }
}

void drawObstacles() {
    for (const auto& obstacle : obstacles) {
        if (obstacle.type == TREE) {
            // Tree trunk (QUADS)
            glColor3f(0.5, 0.25, 0.0);
            glBegin(GL_QUADS);
            glVertex2f(obstacle.x + 12, obstacle.y);
            glVertex2f(obstacle.x + obstacle.width - 12, obstacle.y);
            glVertex2f(obstacle.x + obstacle.width - 12, obstacle.y + obstacle.height);
            glVertex2f(obstacle.x + 12, obstacle.y + obstacle.height);
            glEnd();

            // Tree foliage (TRIANGLE_FAN)
            glColor3f(0.0, 0.6, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            float cx = obstacle.x + obstacle.width / 2;
            float cy = obstacle.y + obstacle.height + 15;
            for (int i = 0; i <= 20; i++) {
                float angle = 2.0f * M_PI * float(i) / 20.0f;
                glVertex2f(cx + cos(angle) * 25, cy + sin(angle) * 25);
            }
            glEnd();
        }
        else {  // CLOUD
            drawCloud(obstacle.x + obstacle.width / 2, obstacle.y - 200, 30);
        }
    }
}

void drawCollectables() {
    for (const auto& collectable : collectables) {
        // Animated y-position
        float animatedY = collectable.y + sin(collectableAnimationOffset + collectable.x * 0.1) * 5;

        // Coin body (TRIANGLE_FAN)
        glColor3f(1.0, 0.84, 0.0);
        glBegin(GL_TRIANGLE_FAN);
        float cx = collectable.x + collectable.width / 2;
        float cy = animatedY + collectable.height / 2;
        for (int i = 0; i <= 30; i++) {
            float angle = 2.0f * M_PI * float(i) / 30.0f;
            glVertex2f(cx + cos(angle) * 15, cy + sin(angle) * 15);
        }
        glEnd();

        // Coin border (LINE_LOOP)
        glColor3f(0.8, 0.67, 0.0);
        glLineWidth(2.0);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i <= 30; i++) {
            float angle = 2.0f * M_PI * float(i) / 30.0f;
            glVertex2f(cx + cos(angle) * 15, cy + sin(angle) * 15);
        }
        glEnd();

        // Coin shine (LINES)
        glColor3f(1.0, 1.0, 0.6);
        glLineWidth(2.0);
        glBegin(GL_LINES);
        glVertex2f(cx - 7, cy + 7);
        glVertex2f(cx + 7, cy - 7);
        glEnd();
        glLineWidth(1.0);
    }
}

void loadBMP(const char* filename, int& width, int& height, unsigned char*& data) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    unsigned char header[54];
    file.read(reinterpret_cast<char*>(header), 54);

    width = *(int*)&header[18];
    height = *(int*)&header[22];

    int size = 3 * width * height;
    data = new unsigned char[size];

    file.read(reinterpret_cast<char*>(data), size);
    file.close();

    for (int i = 0; i < size; i += 3) {
        unsigned char tmp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = tmp;
    }
}

void loadPlayerTexture(const char* filename) {
    unsigned char* data;
    loadBMP(filename, playerTextureWidth, playerTextureHeight, data);

    glGenTextures(1, &playerTextureID);
    glBindTexture(GL_TEXTURE_2D, playerTextureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, playerTextureWidth, playerTextureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    // Use bilinear filtering
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Use texture clamping to edge
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    delete[] data;
}

void drawPowerups() {
    for (const auto& powerup : powerups) {
        // Animated rotation
        float animatedAngle = powerupAnimationAngle + powerup.x * 0.1;

        if (powerup.powerUpType == SLOW_DOWN) {
            glPushMatrix();
            glTranslatef(powerup.x + powerup.width / 2, powerup.y + powerup.height / 2, 0);
            glRotatef(animatedAngle, 0, 1, 0);

            // Clock face (TRIANGLE_FAN)
            glColor3f(0.0, 0.8, 0.8);
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i <= 30; i++) {
                float angle = 2.0f * M_PI * float(i) / 30.0f;
                glVertex2f(cos(angle) * 20, sin(angle) * 20);
            }
            glEnd();

            // Clock border (LINE_LOOP)
            glColor3f(0.0, 0.6, 0.6);
            glLineWidth(2.0);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i <= 30; i++) {
                float angle = 2.0f * M_PI * float(i) / 30.0f;
                glVertex2f(cos(angle) * 20, sin(angle) * 20);
            }
            glEnd();

            // Clock hands (LINES)
            glColor3f(0.0, 0.0, 0.0);
            glLineWidth(2.0);
            glBegin(GL_LINES);
            glVertex2f(0, 0);
            glVertex2f(cos(M_PI / 4) * 15, sin(M_PI / 4) * 15);
            glVertex2f(0, 0);
            glVertex2f(-cos(M_PI / 6) * 10, -sin(M_PI / 6) * 10);
            glEnd();

            // Clock center (TRIANGLE_FAN)
            glColor3f(0.0, 0.0, 0.0);
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i <= 30; i++) {
                float angle = 2.0f * M_PI * float(i) / 30.0f;
                glVertex2f(cos(angle) * 3, sin(angle) * 3);
            }
            glEnd();

            glLineWidth(1.0);
            glPopMatrix();
        }
        else {  // IMMUNITY
            glPushMatrix();
            glTranslatef(powerup.x + powerup.width / 2, powerup.y + powerup.height / 2, 0);
            glRotatef(animatedAngle, 0, 0, 1);

            // Shield base (TRIANGLE_FAN)
            glColor3f(0.7, 0.7, 0.7);
            glBegin(GL_TRIANGLE_FAN);
            for (int i = 0; i <= 180; i++) {
                float angle = i * M_PI / 180;
                glVertex2f(cos(angle) * 20, sin(angle) * 25);
            }
            glEnd();

            // Shield border (LINE_STRIP)
            glColor3f(0.5, 0.5, 0.5);
            glLineWidth(2.0);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 180; i++) {
                float angle = i * M_PI / 180;
                glVertex2f(cos(angle) * 20, sin(angle) * 25);
            }
            glEnd();

            // Shield emblem (TRIANGLES)
            glColor3f(0.8, 0.8, 0.8);
            glBegin(GL_TRIANGLES);
            glVertex2f(0, -15);
            glVertex2f(-10, 10);
            glVertex2f(10, 10);
            glEnd();

            // Shield inner pattern (LINE_STRIP)
            glColor3f(0.6, 0.6, 0.6);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 180; i += 20) {
                float angle = i * M_PI / 180;
                glVertex2f(cos(angle) * 15, sin(angle) * 20);
            }
            glEnd();

            glLineWidth(1.0);
            glPopMatrix();
        }
    }
}

void drawDiamond(float x, float y, float width, float height) {
    glBegin(GL_QUADS);
    glVertex2f(x, y + height / 2);
    glVertex2f(x + width / 2, y + height);
    glVertex2f(x + width, y + height / 2);
    glVertex2f(x + width / 2, y);
    glEnd();
}

void drawHexagon(float x, float y, float width, float height) {
    glBegin(GL_POLYGON);
    for (int i = 0; i < 6; i++) {
        float angle = i * M_PI / 3;
        glVertex2f(x + width / 2 + width / 2 * cos(angle),
            y + height / 2 + height / 2 * sin(angle));
    }
    glEnd();
}

void drawBackground() {
    glColor3f(0.00f, 0.00f, 0.00f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(window_width, 0);
    glVertex2f(window_width, window_height);
    glVertex2f(0, window_height);
    glEnd();
}

void drawCircle(float cx, float cy, float r) {
    int num_segments = 30;
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i <= num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = r * cosf(theta);
        float y = r * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void drawHUD() {
    char buffer[100];
    glColor3f(1.0f, 1.0f, 1.0f);

    // Draw score, time
    sprintf(buffer, "Score: %d  Time: %.1f", score, remaining_time);
    glRasterPos2f(window_width - 200, window_height - 60);
    for (char* c = buffer; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Draw messages
    float messageY = window_height - 120;
    for (const auto& msg : messages) {
        glColor4f(1.0f, 1.0f, 1.0f, msg.timer / 2.0f);  // Fade out effect
        glRasterPos2f(window_width / 2 - 50, messageY);
        for (char c : msg.text) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
        messageY -= 30;
    }

    // Draw lives as pixelated hearts
    for (int i = 0; i < player_lives; i++) {
        float x = window_width - 30 * (i + 1);
        float y = window_height - 90;

        glColor3f(1.0, 0.0, 0.0);  // Red color for hearts

        // Left top triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(x - 10, y + 10);
        glVertex2f(x - 5, y + 20);
        glVertex2f(x + 3, y + 10);
        glEnd();

        // Right top triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(x + 10, y + 10);
        glVertex2f(x + 5, y + 20);
        glVertex2f(x - 3, y + 10);
        glEnd();

        // Bottom triangle
        glBegin(GL_TRIANGLES);
        glVertex2f(x, y);
        glVertex2f(x - 10, y + 10);
        glVertex2f(x + 10, y + 10);
        glEnd();

        // Heart center (QUADS) - Added primitive
        glColor3f(0.8, 0.0, 0.0);  // Darker red for center
        glBegin(GL_QUADS);
        glVertex2f(x - 5, y + 10);
        glVertex2f(x + 5, y + 10);
        glVertex2f(x + 5, y + 15);
        glVertex2f(x - 5, y + 15);
        glEnd();
    }

}

void drawDescriptionBox() {
    float boxWidth = 240;
    float boxHeight = 180;
    float boxX = 10;
    float boxY = window_height - boxHeight - 60;  // Moved down further

    // Box background (QUADS)
    glColor4f(0.9f, 0.9f, 0.9f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(boxX, boxY);
    glVertex2f(boxX + boxWidth, boxY);
    glVertex2f(boxX + boxWidth, boxY + boxHeight);
    glVertex2f(boxX, boxY + boxHeight);
    glEnd();

    // Box border (LINE_LOOP)
    glColor3f(0.2f, 0.2f, 0.2f);
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(boxX, boxY);
    glVertex2f(boxX + boxWidth, boxY);
    glVertex2f(boxX + boxWidth, boxY + boxHeight);
    glVertex2f(boxX, boxY + boxHeight);
    glEnd();
    glLineWidth(1.0);

    // Box title
    glColor3f(0.2f, 0.2f, 0.2f);
    const char* title = "Game Guide";
    glRasterPos2f(boxX + 10, boxY + boxHeight - 20);
    for (const char* c = title; *c != '\0'; c++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);
    }

    // Box content
    glColor3f(0.2f, 0.2f, 0.2f);
    const char* descriptions[] = {
        "Controls:",
        "  - Space: Jump",
        "  - D: Duck",
        "Power-ups:",
        "  - Clock: Slow Down",
        "  - Shield: Temporary Immunity",
        "Collect gold coins to increase score!"
    };

    float y = boxY + boxHeight - 40;
    for (const char* desc : descriptions) {
        glRasterPos2f(boxX + 10, y);
        for (const char* c = desc; *c != '\0'; c++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
        }
        y -= 20;
    }
}

void updateGameState() {
    if (currentState == GAME_RUNNING) {
        // Increase game speed over time (slower increment)
        if (gameSpeed < maxGameSpeed && !isSlowedDown) {
            gameSpeed += gameSpeedIncrement * (1.0f / 60.0f);
        }

        updatePlayer();
        updateObstacles();
        updateCollectables();
        updatePowerups();
        updateBackground();
        checkCollisions();

        collectableAnimationOffset += 0.1f;
        powerupAnimationAngle += 2.0f;
        if (powerupAnimationAngle >= 360.0f) {
            powerupAnimationAngle -= 360.0f;
        }

        // Update time
        remaining_time -= 1.0f / 60.0f;
        if (remaining_time <= 0) {
            currentState = GAME_WIN;
            playWinSound();
        }

        // Update messages
        for (auto it = messages.begin(); it != messages.end();) {
            it->timer -= 1.0f / 60.0f;
            if (it->timer <= 0) {
                it = messages.erase(it);
            }
            else {
                ++it;
            }
        }

        // Update power-up timers
        if (isImmune) {
            immunityTimer -= 1.0f / 60.0f;
            if (immunityTimer <= 0) {
                isImmune = false;
                addMessage("Immunity Ended!", 2.0f);
            }
        }

        if (isSlowedDown) {
            slowDownTimer -= 1.0f / 60.0f;
            if (slowDownTimer <= 0) {
                isSlowedDown = false;
                gameSpeed /= slowDownFactor;
                addMessage("Speed Restored!", 2.0f);
            }
        }

        // Check for game over condition
        if (player_lives <= 0) {
            currentState = GAME_OVER;
            playLoseSound();
        }

        // Update border animation
        borderAnimationOffset += borderAnimationSpeed;
        if (borderAnimationOffset >= 20.0f) {
            borderAnimationOffset -= 20.0f;
        }
    }
}

void updatePlayer() {
    switch (playerState) {
    case RUNNING:
        player_y = groundLevel + 50;  // Adjusted to keep player above ground
        if (showCollisionMessage) {
            collisionMessageTimer -= 1.0f / 60.0f;
            if (collisionMessageTimer <= 0) {
                showCollisionMessage = false;
            }
        }
        break;
    case JUMPING:
        player_y += playerVerticalVelocity;
        playerVerticalVelocity += gravity;
        if (player_y <= groundLevel + 50) {  // Adjusted to match new ground level
            player_y = groundLevel + 50;
            playerState = RUNNING;
            handleLanding();
            playerVerticalVelocity = 0;
        }
        break;
    case DUCKING:
        player_y = groundLevel + 70 - duckHeight;  // Adjusted for new ground level
        break;
    }
}

void updateObstacles() {
    for (auto& obstacle : obstacles) {
        obstacle.x -= obstacle.speed * gameSpeed;
    }

    obstacles.erase(std::remove_if(obstacles.begin(), obstacles.end(),
        [](const GameObject& o) { return o.x + o.width < 0; }),
        obstacles.end());

    if (obstacles.empty() || obstacles.back().x < window_width - 300) {
        generateObstacle();
    }
}

void updateCollectables() {
    for (auto& collectable : collectables) {
        collectable.x -= collectable.speed * gameSpeed;
    }

    collectables.erase(std::remove_if(collectables.begin(), collectables.end(),
        [](const GameObject& c) { return c.x + c.width < 0; }),
        collectables.end());
}

void updatePowerups() {
    for (auto& powerup : powerups) {
        powerup.x -= powerup.speed * gameSpeed;
    }

    powerups.erase(std::remove_if(powerups.begin(), powerups.end(),
        [](const GameObject& p) { return p.x + p.width < 0; }),
        powerups.end());
}

void updateBackground() {
    for (auto& layer : backgroundLayers) {
        layer.x -= layer.speed * gameSpeed;
        if (layer.x <= -window_width) {
            layer.x = 0;
        }
    }
}

void generateObstacle() {
    float lastObjectX = obstacles.empty() ? -minObjectSpacing : obstacles.back().x;
    float collectableX = collectables.empty() ? -minObjectSpacing : collectables.back().x;
    float powerupX = powerups.empty() ? -minObjectSpacing : powerups.back().x;

    // Find the maximum x-coordinate manually
    float maxX = lastObjectX;
    if (collectableX > maxX) maxX = collectableX;
    if (powerupX > maxX) maxX = powerupX;

    float minX = maxX + minObjectSpacing;

    GameObject newObstacle;
    newObstacle.x = (static_cast<float>(window_width) > minX) ? static_cast<float>(window_width) : minX;
    newObstacle.y = groundLevel;
    newObstacle.width = 50;
    newObstacle.type = (rand() % 2 == 0) ? TREE : CLOUD;
    newObstacle.height = (newObstacle.type == TREE) ? treeHeight : cloudHeight;
    newObstacle.speed = 5;

    if (newObstacle.type == CLOUD) {
        newObstacle.y = window_height - cloudHeight - 50;
    }

    obstacles.push_back(newObstacle);

    // Generate collectable or powerup between obstacles
    float spacing = minObjectSpacing + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (maxObstacleSpacing - minObjectSpacing)));

    if (rand() % 2 == 0) {
        generateCollectable(newObstacle.x + newObstacle.width + spacing);
    }
    else {
        generatePowerup(newObstacle.x + newObstacle.width + spacing);
    }
}

void generateCollectable(float x) {
    GameObject newCollectable;
    newCollectable.x = x;
    newCollectable.y = groundLevel + playerHeight / 2 + 50;  // Adjusted for new player position
    newCollectable.width = 30;
    newCollectable.height = 30;
    newCollectable.speed = 5;
    collectables.push_back(newCollectable);
}

void generatePowerup(float x) {
    GameObject newPowerup;
    newPowerup.x = x;
    newPowerup.y = groundLevel + playerHeight / 2 + 50;  // Adjusted for new player position
    newPowerup.width = 40;
    newPowerup.height = 40;
    newPowerup.speed = 5;
    newPowerup.powerUpType = (rand() % 2 == 0) ? SLOW_DOWN : IMMUNITY;
    powerups.push_back(newPowerup);
}

void checkCollisions() {
    GameObject player = { player_x, player_y, 50, (playerState == DUCKING) ? 33.0f : playerHeight, 0 };

    for (auto it = obstacles.begin(); it != obstacles.end(); ++it) {
        GameObject adjustedObstacle = *it;
        if (adjustedObstacle.type == CLOUD) {
            adjustedObstacle.y -= 330; // Adjust cloud position for collision check
        }
        if (checkCollision(player, adjustedObstacle)) {
            if ((adjustedObstacle.type == TREE && playerState != JUMPING) ||
                (adjustedObstacle.type == CLOUD && playerState != DUCKING)) {
                if (!isImmune) {
                    player_lives--;
                    playCollisionSound();
                    addMessage("Hit an obstacle!", 2.0f);

                    // Implement rebound effect only when not immune
                    it->x += 150.0f; // Move obstacle to the right by 100 units

                    // Remove all elements to the right of this obstacle
                    float collisionX = it->x + it->width;
                    obstacles.erase(std::remove_if(it + 1, obstacles.end(),
                        [collisionX](const GameObject& o) { return o.x > collisionX; }),
                        obstacles.end());
                    collectables.erase(std::remove_if(collectables.begin(), collectables.end(),
                        [collisionX](const GameObject& c) { return c.x > collisionX; }),
                        collectables.end());
                    powerups.erase(std::remove_if(powerups.begin(), powerups.end(),
                        [collisionX](const GameObject& p) { return p.x > collisionX; }),
                        powerups.end());

                    if (player_lives <= 0) {
                        currentState = GAME_OVER;
                        playLoseSound();
                    }
                }
            }
        }
    }

    // Check collisions with collectables
    collectables.erase(std::remove_if(collectables.begin(), collectables.end(),
        [&](const GameObject& collectable) {
            if (checkCollision(player, collectable)) {
                score += 10;
                playCollectSound();
                addMessage("Score +10!", 2.0f);
                return true;
            }
            return false;
        }),
        collectables.end());

    // Check collisions with powerups
    powerups.erase(std::remove_if(powerups.begin(), powerups.end(),
        [&](const GameObject& powerup) {
            if (checkCollision(player, powerup)) {
                if (powerup.powerUpType == SLOW_DOWN) {
                    isSlowedDown = true;
                    slowDownTimer = slowDownDuration;
                    gameSpeed *= slowDownFactor;
                    playSpeedSound();
                    addMessage("Slow Down Active!", 2.0f);
                }
                else if (powerup.powerUpType == IMMUNITY) {
                    isImmune = true;
                    immunityTimer = immunityDuration;
                    playImmunitySound();
                    addMessage("Immunity Active!", 2.0f);
                }
                return true;
            }
            return false;
        }),
        powerups.end());
}

bool checkCollision(const GameObject& obj1, const GameObject& obj2) {
    return (obj1.x < obj2.x + obj2.width &&
        obj1.x + obj1.width > obj2.x &&
        obj1.y < obj2.y + obj2.height &&
        obj1.y + obj1.height > obj2.y);
}

void audioManager() {
    updateAudio();
    if (!isSoundEffectPlaying && !isBackgroundMusicPlaying && currentState == GAME_RUNNING) {
        playBackgroundMusic();
    }
}

void updateAudio() {
    if (isSoundEffectPlaying) {
        DWORD currentTime = GetTickCount();
        if (currentTime - lastPlayTime > 1200) {  // Assume sound effect is done after 1 second
            isSoundEffectPlaying = false;
            if (!isBackgroundMusicPlaying) {
                playBackgroundMusic();
            }
        }
    }
}

void playBackgroundMusic() {
    if (!isSoundEffectPlaying && !isBackgroundMusicPlaying) {
        const char* musicFile = "sounds/background-music.wav";
        int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, musicFile, -1, NULL, 0);
        wchar_t* wideMusicFile = new wchar_t[wideCharLength];
        MultiByteToWideChar(CP_UTF8, 0, musicFile, -1, wideMusicFile, wideCharLength);

        if (PlaySound(wideMusicFile, NULL, SND_ASYNC | SND_LOOP | SND_FILENAME)) {
            isBackgroundMusicPlaying = true;
        }
        else {
            // Handle error
            DWORD error = GetLastError();
            // You might want to log this error or display it to the user
        }

        delete[] wideMusicFile;
    }
}

void stopBackgroundMusic() {
    PlaySound(NULL, NULL, 0);
    isBackgroundMusicPlaying = false;
}

void playSoundEffect(const char* soundFile) {
    // Stop background music
    if (isBackgroundMusicPlaying) {
        stopBackgroundMusic();
    }

    // Convert const char* to LPCWSTR
    int wideCharLength = MultiByteToWideChar(CP_UTF8, 0, soundFile, -1, NULL, 0);
    wchar_t* wideSoundFile = new wchar_t[wideCharLength];
    MultiByteToWideChar(CP_UTF8, 0, soundFile, -1, wideSoundFile, wideCharLength);

    if (PlaySound(wideSoundFile, NULL, SND_ASYNC | SND_FILENAME)) {
        isSoundEffectPlaying = true;
        lastPlayTime = GetTickCount();
    }
    else {
        // Handle error
        DWORD error = GetLastError();
        // You might want to log this error or display it to the user
    }

    delete[] wideSoundFile;
}

void playCollectSound() {
    playSoundEffect("sounds/collect.wav");
}

void playCollisionSound() {
    playSoundEffect("sounds/collision.wav");
}

void playImmunitySound() {
    playSoundEffect("sounds/immunity.wav");
}

void playSpeedSound() {
    playSoundEffect("sounds/speed.wav");
}

void playWinSound() {
    stopBackgroundMusic();
    playSoundEffect("sounds/win.wav");
}

void playLoseSound() {
    stopBackgroundMusic();
    playSoundEffect("sounds/lose.wav");
}

void drawUpperBorder() {
    float borderHeight = 20;

    // Main border (QUADS)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0, window_height - borderHeight);
    glVertex2f(window_width, window_height - borderHeight);
    glVertex2f(window_width, window_height);
    glVertex2f(0, window_height);
    glEnd();

    // Animated pattern (TRIANGLES)
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_TRIANGLES);
    for (float x = -borderHeight; x < window_width + borderHeight; x += borderHeight) {
        float offsetX = x + borderAnimationOffset;
        glVertex2f(offsetX, window_height - borderHeight);
        glVertex2f(offsetX + borderHeight / 2, window_height);
        glVertex2f(offsetX + borderHeight, window_height - borderHeight);
    }
    glEnd();

    // Decorative dots (POINTS)
    glColor3f(0.5f, 0.5f, 0.5f);
    glPointSize(2.0);
    glBegin(GL_POINTS);
    for (float x = 0; x < window_width; x += borderHeight / 2) {
        glVertex2f(x, window_height - borderHeight / 2);
    }
    glEnd();

    // Wavy pattern (LINE_STRIP)
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINE_STRIP);
    for (float x = 0; x < window_width; x += 5) {
        float y = sin(x * 0.1) * borderHeight / 4 + borderHeight / 2;
        glVertex2f(x, y);
    }
    glEnd();
}

void drawLowerBorder() {
    float borderHeight = 20;

    // Main border (QUADS)
    glColor3f(0.3f, 0.3f, 0.3f);
    glBegin(GL_QUADS);
    glVertex2f(0, 0);
    glVertex2f(window_width, 0);
    glVertex2f(window_width, borderHeight);
    glVertex2f(0, borderHeight);
    glEnd();

    // Animated pattern (TRIANGLES)
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_TRIANGLES);
    for (float x = -borderHeight; x < window_width + borderHeight; x += borderHeight) {
        float offsetX = x + borderAnimationOffset;
        glVertex2f(offsetX, 0);
        glVertex2f(offsetX + borderHeight / 2, borderHeight);
        glVertex2f(offsetX + borderHeight, 0);
    }
    glEnd();

    // Decorative squares (QUADS)
    glColor3f(0.5f, 0.5f, 0.5f);
    glBegin(GL_QUADS);
    for (float x = 0; x < window_width; x += borderHeight * 2) {
        glVertex2f(x, 0);
        glVertex2f(x + borderHeight / 2, 0);
        glVertex2f(x + borderHeight / 2, borderHeight / 2);
        glVertex2f(x, borderHeight / 2);
    }
    glEnd();

    // Wavy pattern (LINE_STRIP)
    glColor3f(0.6f, 0.6f, 0.6f);
    glBegin(GL_LINE_STRIP);
    for (float x = 0; x < window_width; x += 5) {
        float y = sin(x * 0.1) * borderHeight / 4 + borderHeight / 2;
        glVertex2f(x, y);
    }
    glEnd();
}

void addMessage(const std::string& text, float duration) {
    messages.push_back({ text, duration, window_height - 100.0f - messages.size() * 30.0f });
}

void drawGround() {
    // Thicker, black ground
    glColor3f(0.0, 0.6, 0.0);
    glBegin(GL_QUADS);
    glVertex2f(0, 70);
    glVertex2f(window_width, 70);
    glVertex2f(window_width, groundLevel);
    glVertex2f(0, groundLevel);
    glEnd();
}

void drawEllipse(float cx, float cy, float rx, float ry, int num_segments) {
    glBegin(GL_TRIANGLE_FAN);
    for (int i = 0; i < num_segments; i++) {
        float theta = 2.0f * 3.1415926f * float(i) / float(num_segments);
        float x = rx * cosf(theta);
        float y = ry * sinf(theta);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void drawCurve(float x1, float y1, float x2, float y2, float x3, float y3, int segments) {
    glBegin(GL_LINE_STRIP);
    for (int i = 0; i <= segments; i++) {
        float t = float(i) / float(segments);
        float x = (1 - t) * (1 - t) * x1 + 2 * (1 - t) * t * x2 + t * t * x3;
        float y = (1 - t) * (1 - t) * y1 + 2 * (1 - t) * t * y2 + t * t * y3;
        glVertex2f(x, y);
    }
    glEnd();
}