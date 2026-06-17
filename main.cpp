#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <algorithm>

// Janela
static const int SCR_W = 1280;
static const int SCR_H = 768;

// Dimensões de exibição das tiles (source 128x64 -> 80x40)
static const float TILE_W = 80.0f;
static const float TILE_H = 40.0f;
static const float TILE_HW = TILE_W * 0.5f;
static const float TILE_HH = TILE_H * 0.5f;

// Origem do mapa isométrico em coordenadas de tela
static const float ORIGIN_X = SCR_W * 0.5f;
static const float ORIGIN_Y = SCR_H - 60.0f;

// Dimensões do mapa (15x15)
static const int MAP_ROWS = 15;
static const int MAP_COLS = 15;

// Tiles
#define TILE_DARK 0 // grama escura – caminhável (personagem não pisou)
#define TILE_LIGHT 1 // grama iluminada – caminhável (personagem já pisou)
#define TILE_CORR 2 // corrupção – caminhável (mata o personagem)
#define TILE_THORN 3 // espinhos – não caminhável
#define TILE_ALTAR 4 // altar – caminhável (início e fim do jogo)

// Tileset: 5 tiles -> cada UV = 1/5 = 0.2
static const int TILESET_N = 5;
static const float TILE_UV_W = 1.0f / TILESET_N;

// Spritesheet personagem: 8 frames -> cada UV = 1/8 = 0.125
static const int SPRITE_N = 8;
static const float SPRITE_UV_W = 1.0f / SPRITE_N;

// Tamanho de exibição do personagem
static const float CHAR_W = 36.0f;
static const float CHAR_H = 36.0f;

// Estados de jogo
#define STATE_PLAYING 0
#define STATE_WIN 1
#define STATE_LOSE 2

// Dados do jogo
struct GameState {
    int grid[MAP_ROWS][MAP_COLS]; // índice de tile atual
    bool runeAt[MAP_ROWS][MAP_COLS]; // checa se há runa a ser coletada na tile
    int playerRow, playerCol;
    int altarExitRow, altarExitCol;
    int runeCount;
    int totalRunes;
    bool portalOpen;
    int state;
};
static GameState gGame;

// Temporizadores
static float gMoveTimer = 0.0f;
static const float MOVE_CD = 0.18f;  // intervalo mínimo entre passos

static float gAnimTimer = 0.0f;
static const float ANIM_SPEED = 0.1f; // duração de cada frame de animação
static int gAnimFrame = 0;

// Recursos OpenGL
static GLuint gSpriteProgram; // shader p/ sprites com textura
static GLuint gColorProgram; // shader p/ HUD e overlay
static GLuint gVAO, gVBO, gEBO;
static GLuint gTexTileset, gTexPortal, gTexRuna, gTexSpirit;

// Leitura de arquivo p/ shader
static std::string readFile(const char* path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "Erro ao abrir arquivo: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Compilação de shader
static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        std::cerr << "Erro no shader: " << log << "\n";
    }
    return s;
}

// Criação de programa p/ shaders
static GLuint createProgram(const char* vsPath, const char* fsPath) {
    std::string vsStr = readFile(vsPath);
    std::string fsStr = readFile(fsPath);
    GLuint prog = glCreateProgram();
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsStr.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsStr.c_str());
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// Carregar texturas
static GLuint loadTexture(const char* path) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    int w, h, ch;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &w, &h, &ch, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return tex;
}

// Modelo de retângulo unitário (0,0 a 1,1)
static void setupQuad() {
    float verts[] = {
        0.0f, 0.0f, 0.0f, 0.0f, // inferior-esquerdo
        1.0f, 0.0f, 1.0f, 0.0f, // inferior-direito
        1.0f, 1.0f, 1.0f, 1.0f, // superior-direito
        0.0f, 1.0f, 0.0f, 1.0f, // superior-esquerdo
    };
    unsigned int idx[] = { 0, 1, 2, 0, 2, 3 };

    glGenVertexArrays(1, &gVAO);
    glGenBuffers(1, &gVBO);
    glGenBuffers(1, &gEBO);

    glBindVertexArray(gVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

// Matriz modelo para o retângulo unitário (0,0 a 1,1) -> retângulo de destino (x0,y0 a x1,y1)
static glm::mat4 makeModel(float x0, float y0, float x1, float y1) {
    glm::mat4 m(1.0f);
    m = glm::translate(m, glm::vec3(x0, y0, 0.0f));
    m = glm::scale(m, glm::vec3(x1 - x0, y1 - y0, 1.0f));
    return m;
}

// Projeção isométrica: grid (row, col) -> coordenadas de tela
static void isoToScreen(int row, int col, float& cx, float& yTop) {
    cx = (col - row) * TILE_HW + ORIGIN_X;
    yTop = ORIGIN_Y - (col + row) * TILE_HH;
}

// Desenha sprite em coordenadas da tela
static void drawSprite(GLuint tex, float x0, float y0, float x1, float y1, float uvOx, float uvOy, float uvSx, float uvSy)
{
    glUseProgram(gSpriteProgram);
    glm::mat4 m = makeModel(x0, y0, x1, y1);
    glUniformMatrix4fv(glGetUniformLocation(gSpriteProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform2f(glGetUniformLocation(gSpriteProgram, "uUVOffset"), uvOx, uvOy);
    glUniform2f(glGetUniformLocation(gSpriteProgram, "uUVSize"),   uvSx, uvSy);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    glBindVertexArray(gVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

// Desenha HUD e overlay
static void drawRect(float x0, float y0, float x1, float y1, float r, float g, float b, float a)
{
    glUseProgram(gColorProgram);
    glm::mat4 m = makeModel(x0, y0, x1, y1);
    glUniformMatrix4fv(glGetUniformLocation(gColorProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(m));
    glUniform4f(glGetUniformLocation(gColorProgram, "uColor"), r, g, b, a);
    glBindVertexArray(gVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
}

// Carrega mapa do arquivo map.txt
static bool loadMap(const char* path) {
    std::ifstream f(path);

    // Lê e descarta nomenclatura dos assets (exemplo: tileset.png 5 128 64)
    { std::string s; int a, b, c; f >> s >> a >> b >> c; } // tileset
    { std::string s; int a, b; f >> s >> a >> b; } // portal
    { std::string s; int a, b; f >> s >> a >> b; } // runa
    { std::string s; int a, b, c; f >> s >> a >> b >> c; } // spirit

    // Dimensões do mapa
    int rows, cols;
    f >> rows >> cols;

    // Reinicia estado do jogo
    memset(gGame.grid, 0, sizeof(gGame.grid));
    memset(gGame.runeAt, 0, sizeof(gGame.runeAt));
    gGame.runeCount = 0;
    gGame.totalRunes = 0;
    gGame.portalOpen = false;

    // Lê a grade de tiles
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < cols; c++)
            f >> gGame.grid[r][c];

    // Lê posição de objetos
    std::string token;
    int row, col;
    while (f >> token >> row >> col) {
        if (token == "player") {
            gGame.playerRow = row;
            gGame.playerCol = col;
        } else if (token == "altar_exit") {
            gGame.altarExitRow = row;
            gGame.altarExitCol = col;
        } else if (token == "runa") {
            gGame.runeAt[row][col] = true;
            gGame.totalRunes++;
        }
    }

    gGame.state = STATE_PLAYING;
    return true;
}

// Reinicia o jogo
static void resetGame() {
    gMoveTimer = 0.0f;
    gAnimFrame = 0;
    gAnimTimer = 0.0f;
    loadMap("map.txt");
}

// Processa input p/ reset, fechar e movimentação de personagem
static void processInput(GLFWwindow* win, float dt) {
    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) // ESC = fechar
        glfwSetWindowShouldClose(win, true);

    if (glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS) { // R = reiniciar
        resetGame();
        return;
    }

    if (gGame.state != STATE_PLAYING) return;

    gMoveTimer += dt;
    if (gMoveTimer < MOVE_CD) return;

    int drow = 0, dcol = 0;
    bool anyKey = false;

    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)
    { drow--; dcol--; anyKey = true; }

    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)
    { drow++; dcol++; anyKey = true; }

    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)
    { drow++; dcol--; anyKey = true; }

    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS)
    { drow--; dcol++; anyKey = true; }

    if (!anyKey) return;

    // Clamp para (-1, +1) (combinações de teclas dão as 4 direções norte, sul, leste e oeste)
    drow = std::max(-1, std::min(1, drow));
    dcol = std::max(-1, std::min(1, dcol));
    if (drow == 0 && dcol == 0) return;

    int nr = gGame.playerRow + drow;
    int nc = gGame.playerCol + dcol;

    // Limites do mapa
    if (nr < 0 || nr >= MAP_ROWS || nc < 0 || nc >= MAP_COLS) return;

    // Tile não caminhável (espinhos)
    if (gGame.grid[nr][nc] == TILE_THORN) return;

    // Movimenta o personagem
    gGame.playerRow = nr;
    gGame.playerCol = nc;
    gMoveTimer = 0.0f;

    int& tile = gGame.grid[gGame.playerRow][gGame.playerCol];

    // Grama escura -> grama iluminada ao pisar
    if (tile == TILE_DARK)
        tile = TILE_LIGHT;

    // Coleta runa se houver na tile
    if (gGame.runeAt[gGame.playerRow][gGame.playerCol]) {
        gGame.runeAt[gGame.playerRow][gGame.playerCol] = false;
        gGame.runeCount++;
        if (gGame.runeCount >= gGame.totalRunes)
            gGame.portalOpen = true;
    }

    // Perde se pisar na corrupção
    if (tile == TILE_CORR) {
        gGame.state = STATE_LOSE;
        return;
    }

    // Jogador vence ao chegar no altar de saída com o portal aberto
    if (gGame.playerRow == gGame.altarExitRow &&
        gGame.playerCol == gGame.altarExitCol &&
        gGame.portalOpen)
    {
        gGame.state = STATE_WIN;
    }
}

// Renderiza tile, portal e runas
static void renderTile(int row, int col) {
    float cx, yTop;
    isoToScreen(row, col, cx, yTop);

    float x0 = cx - TILE_HW;
    float y0 = yTop - TILE_H;
    float x1 = cx + TILE_HW;
    float y1 = yTop;

    // Tile
    int ti = gGame.grid[row][col];
    drawSprite(gTexTileset, x0, y0, x1, y1, ti * TILE_UV_W, 0.0f, TILE_UV_W, 1.0f);

    // Portal
    if (row == gGame.altarExitRow &&
        col == gGame.altarExitCol &&
        gGame.portalOpen)
    {
        drawSprite(gTexPortal, x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f);
    }

    // Runa
    if (gGame.runeAt[row][col]) {
        drawSprite(gTexRuna, x0, y0, x1, y1, 0.0f, 0.0f, 1.0f, 1.0f);
    }
}

// Renderiza personagem no centro da tile
static void renderPlayer() {
    float cx, yTop;
    isoToScreen(gGame.playerRow, gGame.playerCol, cx, yTop);

    // Centro do losango
    float charX0 = cx - CHAR_W * 0.5f;
    float charY0 = yTop - TILE_HH;
    float charX1 = charX0 + CHAR_W;
    float charY1 = charY0 + CHAR_H;

    float uvOx = gAnimFrame * SPRITE_UV_W;
    drawSprite(gTexSpirit, charX0, charY0, charX1, charY1, uvOx, 0.0f, SPRITE_UV_W, 1.0f);
}

// Renderiza HUD e overlay
static void renderHUD() {
    // Faixa escura no topo p/ pontuação
    drawRect(0.0f, SCR_H - 36.0f, (float)SCR_W, (float)SCR_H, 0.0f, 0.0f, 0.0f, 0.55f);

    // Contador de runas (dourado = coletado, escuro = faltando)
    const float SQ = 18.0f, GAP = 4.0f;
    float totalW = gGame.totalRunes * (SQ + GAP) - GAP;
    float startX = ((float)SCR_W - totalW) * 0.5f;
    float sqY0 = (float)SCR_H - 27.0f;
    float sqY1 = sqY0 + SQ;

    for (int i = 0; i < gGame.totalRunes; i++) {
        float sx = startX + i * (SQ + GAP);
        if (i < gGame.runeCount)
            drawRect(sx, sqY0, sx + SQ, sqY1, 0.95f, 0.80f, 0.10f, 1.0f); // ouro
        else
            drawRect(sx, sqY0, sx + SQ, sqY1, 0.18f, 0.18f, 0.28f, 1.0f); // escuro
    }

    // Overlay de vitória (verde) ou derrota (vermelho)
    if (gGame.state == STATE_WIN)
        drawRect(0.0f, 0.0f, (float)SCR_W, (float)SCR_H, 0.05f, 0.55f, 0.10f, 0.55f);
    else if (gGame.state == STATE_LOSE)
        drawRect(0.0f, 0.0f, (float)SCR_W, (float)SCR_H, 0.60f, 0.02f, 0.02f, 0.55f);
}

// Main
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(SCR_W, SCR_H, "Jardim das Runas", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glViewport(0, 0, SCR_W, SCR_H);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Shaders
    gSpriteProgram = createProgram("_sprite_vs.glsl", "_sprite_fs.glsl");
    gColorProgram = createProgram("_color_vs.glsl", "_color_fs.glsl");

    // Projeção ortográfica
    glm::mat4 proj = glm::ortho(0.0f, (float)SCR_W, 0.0f, (float)SCR_H, -1.0f, 1.0f);

    glUseProgram(gSpriteProgram);
    glUniformMatrix4fv(glGetUniformLocation(gSpriteProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(glGetUniformLocation(gSpriteProgram, "uTexture"), 0);

    glUseProgram(gColorProgram);
    glUniformMatrix4fv(glGetUniformLocation(gColorProgram, "uProjection"), 1, GL_FALSE, glm::value_ptr(proj));

    // Setup do modelo de retângulo unitário compartilhado
    setupQuad();

    // Texturas
    gTexTileset = loadTexture("assets/tileset.png");
    gTexPortal = loadTexture("assets/portal.png");
    gTexRuna = loadTexture("assets/runa.png");
    gTexSpirit = loadTexture("assets/spirit.png");

    // Estado inicial
    resetGame();

    double prevTime = glfwGetTime();

    // Loop principal
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - prevTime);
        prevTime = now;

        glfwPollEvents();

        // Animação do personagem
        gAnimTimer += dt;
        if (gAnimTimer >= ANIM_SPEED) {
            gAnimTimer -= ANIM_SPEED;
            gAnimFrame = (gAnimFrame + 1) % SPRITE_N;
        }

        // Processa inputs de movimentação e comandos
        processInput(window, dt);

        // Renderização
        glClearColor(0.06f, 0.06f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenha o mapa tile a tile
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++)
                renderTile(r, c);

        // Personagem sempre por cima dos tiles
        renderPlayer();

        // HUD e overlay
        renderHUD();

        glfwSwapBuffers(window);
    }

    // Limpeza
    glDeleteTextures(1, &gTexTileset);
    glDeleteTextures(1, &gTexPortal);
    glDeleteTextures(1, &gTexRuna);
    glDeleteTextures(1, &gTexSpirit);
    glDeleteVertexArrays(1, &gVAO);
    glDeleteBuffers(1, &gVBO);
    glDeleteBuffers(1, &gEBO);
    glDeleteProgram(gSpriteProgram);
    glDeleteProgram(gColorProgram);
    glfwTerminate();
    return 0;
}
