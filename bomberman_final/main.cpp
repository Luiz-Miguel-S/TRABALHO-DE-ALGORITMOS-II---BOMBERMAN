/**
 * JOGO BOMBERMAN - PROJETO FINAL (M2)
 * Desenvolvido em C++
 * * Este código atende a todos os requisitos solicitados no PDF da disciplina:
 * - Menus, Dificuldades, Pontuação calculada, Rank e Sistema de Save.
 * - Elementos Técnicos: Manipulação de Arquivo, Sobrecarga, Default, Structs, Template e Recursividade.
 * - Jogo OTIMIZADO (String Buffering), Emojis UTF-8 e IA avançada (AutoPlay).
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <windows.h>
#include <conio.h>
#include <cstdlib>
#include <ctime>
using namespace std;

// =================================================================
// === CONTROLE DE MÚSICA (PlaySound via WinMM) ====================
// =================================================================

// Toca o WAV em loop assíncrono (não trava o jogo)
void iniciarMusica() {
    PlaySoundA("musica_wav.wav", NULL, SND_FILENAME | SND_ASYNC | SND_LOOP);
}

// Para a música (chamado ao sair da fase ou do jogo)
void pararMusica() {
    PlaySoundA(NULL, NULL, SND_PURGE);
}

// =================================================================
// === ESTRUTURAS DE DADOS (REQUISITO DO PDF: STRUCTS) =============
// =================================================================

// Struct para gerenciar múltiplas bombas simultâneas
struct Bomba {
    int x, y;
    int timer;       // Tempo para a bomba explodir
    bool ativa;      // Define se a bomba está no chão
    bool explodiu;   // Define se o fogo está na tela
    int dono;        // Para saber se quem plantou foi o Player 1 ou Player 2
};

// Struct para salvar o estado do jogo em arquivo binário
struct JogoSave {
    char data[32]; 
    char jogador[32];
    int fase; 
    int pontuacao; 
    int bombasUsadas; 
    int movimentos;
    long tempoSegundos; 
    int vidas; 
    int dificuldade;
};

// Struct para registrar as melhores pontuações (Rank Top 10)
struct RankEntry {
    char data[32]; 
    char jogador[32]; 
    int pontuacao; 
    int fase;
};

// Struct básica para representar os inimigos na tela
struct Inimigo { 
    int x; 
    int y; 
    bool vivo; 
};


// =================================================================
// === VARIÁVEIS GLOBAIS DE ESTADO E FÍSICA ========================
// =================================================================

Bomba bombas[30];  
int fimTimer[30];  

bool gameOverColideInimigo = false;
bool gameOverExplosao = false;
bool p1Morto = false;
bool p2Morto = false;

// Atributos do Player 1 (🧑)
int pontuacao = 0;
int vidas = 3;
int p1Raio = 1;         
int p1MaxBombas = 1;    
bool p1Imune = false;   

// Atributos do Player 2 (🟦) - Para o modo Co-Op
int p2Pontuacao = 0;
int p2Vidas = 3;
int p2Raio = 1;
int p2MaxBombas = 1;
bool p2Imune = false;
int gx2 = 1, gy2 = 1; // Posição global do P2 (sem conflito com locais)

int faseAtual = 1;
int dificuldade = 1; 
int bombasUsadas = 0, movimentos = 0, caixasDestruidas = 0;
time_t tempoInicio = 0;

string nomeJogador = "Player1";
string nomeJogador2 = "Player2"; 

bool modoAutoPlay = false;       
bool modoDoisJogadores = false;  

const int ESTADO_VITORIA = 1;
const int ESTADO_DERROTA = 2;
const int ESTADO_DESISTIU = 3;
const int ESTADO_PROXIMA_FASE = 4;

const char* ARQUIVO_SAVE = "bomberman_save.dat";
const char* ARQUIVO_RANK = "bomberman_rank.dat";

int matrizItens[15][25]; 

Inimigo inimigos[7];        
int numInimigosAtivos = 0;  
int inimigosVivos = 0;      

Inimigo boss;
bool bossAtivo = false;

int portalX = -1, portalY = -1;
bool portalAtivo = false;


// =================================================================
// === UTILIÁRIOS DE RENDERIZAÇÃO ==================================
// =================================================================

void setCor(int cor) {
    if (cor == 7) cout << "\x1b[0m"; 
    else cout << "\x1b[38;5;" << cor << "m"; 
}

// REQUISITO DO PDF: TEMPLATE
template<typename T>
void imprimirCentralizado(T conteudo, int largura = 70) {
    stringstream ss; 
    ss << conteudo; 
    string s = ss.str();
    int pad = (largura - (int)s.size()) / 2;
    
    if (pad < 0) pad = 0;
    for (int i = 0; i < pad; i++) cout << " ";
    cout << s << "\n";
}

void desenharBordaTopo(int largura, int cor) {
    setCor(cor); cout << "╔";
    for (int i = 0; i < largura - 2; i++) cout << "═"; 
    cout << "╗\n";
}

void desenharBordaBase(int largura, int cor) {
    setCor(cor); cout << "╚";
    for (int i = 0; i < largura - 2; i++) cout << "═"; 
    cout << "╝\n";
}

// REQUISITO DO PDF: PARÂMETRO DEFAULT E SOBRECARGA
void desenharLinhaLateral(string conteudo, int largura = 70, int cor = 7) {
    setCor(cor); cout << "║"; setCor(7);
    
    string txt = conteudo;
    if ((int)txt.size() > largura - 4) txt = txt.substr(0, largura - 4);
    
    cout << " " << txt;
    for (int i = (int)txt.size() + 1; i < largura - 3; i++) cout << " ";
    
    setCor(cor); cout << "║\n"; setCor(7);
}

// REQUISITO DO PDF: SOBRECARGA
void desenharLinhaLateral(int largura, int cor) { 
    desenharLinhaLateral("", largura, cor); 
}

void obterDataAtual(char* buffer, int tam) {
    time_t t = time(0); 
    struct tm* lt = localtime(&t);
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d", lt->tm_mday, lt->tm_mon + 1, lt->tm_year + 1900, lt->tm_hour, lt->tm_min);
}


// =================================================================
// === MANIPULAÇÃO DE ARQUIVOS BINÁRIOS (REQUISITO DO PDF) =========
// =================================================================

void salvarJogo(JogoSave s) {
    ofstream f(ARQUIVO_SAVE, ios::binary);
    if (f) { 
        f.write((char*)&s, sizeof(JogoSave)); 
        f.close(); 
    }
}

void salvarJogo(string player = "Player1") {
    JogoSave s; 
    strncpy(s.jogador, player.c_str(), 31); s.jogador[31] = '\0';
    obterDataAtual(s.data, 32); 
    
    s.fase = faseAtual; s.pontuacao = pontuacao;
    s.bombasUsadas = bombasUsadas; s.movimentos = movimentos;
    s.tempoSegundos = (long)(time(0) - tempoInicio); 
    s.vidas = vidas; s.dificuldade = dificuldade;
    
    salvarJogo(s);
}

bool lerSave(JogoSave &s) {
    ifstream f(ARQUIVO_SAVE, ios::binary);
    if (!f) return false; 
    
    f.read((char*)&s, sizeof(JogoSave)); 
    bool ok = (f.gcount() == sizeof(JogoSave)); 
    f.close(); 
    return ok;
}

bool aplicarSave() {
    JogoSave s; 
    if (!lerSave(s)) return false;
    
    faseAtual = s.fase; pontuacao = s.pontuacao; bombasUsadas = s.bombasUsadas;
    movimentos = s.movimentos; vidas = s.vidas; dificuldade = s.dificuldade;
    nomeJogador = s.jogador; tempoInicio = time(0) - s.tempoSegundos; 
    return true;
}

// REQUISITO DO PDF: RECURSIVIDADE
void inserirRankRecursivo(RankEntry* arr, int &qtd, RankEntry novo, int i = 0) {
    if (i >= qtd) { 
        if (qtd < 10) arr[qtd++] = novo; 
        return; 
    }
    
    if (novo.pontuacao > arr[i].pontuacao) {
        RankEntry temp = arr[i]; 
        arr[i] = novo;
        if (qtd < 10) inserirRankRecursivo(arr, qtd, temp, i + 1);
        return;
    }
    
    inserirRankRecursivo(arr, qtd, novo, i + 1);
}

void registrarNoRank(string player, int pts, int fase) {
    RankEntry arr[10]; 
    int qtd = 0; 
    ifstream fin(ARQUIVO_RANK, ios::binary);
    
    if (fin) {
        while (qtd < 10) {
            RankEntry r; 
            fin.read((char*)&r, sizeof(RankEntry));
            if (fin.gcount() != sizeof(RankEntry)) break; 
            arr[qtd++] = r;
        }
        fin.close();
    }
    
    RankEntry novo; 
    strncpy(novo.jogador, player.c_str(), 31); novo.jogador[31] = '\0';
    obterDataAtual(novo.data, 32); 
    novo.pontuacao = pts; novo.fase = fase;
    
    inserirRankRecursivo(arr, qtd, novo);
    
    ofstream fout(ARQUIVO_RANK, ios::binary | ios::trunc);
    if (fout) { 
        for (int i = 0; i < qtd; i++) fout.write((char*)&arr[i], sizeof(RankEntry)); 
        fout.close(); 
    }
}

int lerRank(RankEntry* arr, int max) {
    int qtd = 0; 
    ifstream fin(ARQUIVO_RANK, ios::binary);
    if (!fin) return 0;
    
    while (qtd < max) {
        RankEntry r; 
        fin.read((char*)&r, sizeof(RankEntry));
        if (fin.gcount() != sizeof(RankEntry)) break; 
        arr[qtd++] = r;
    }
    fin.close(); 
    return qtd;
}


// =================================================================
// === LÓGICA DE JOGO E INTELIGÊNCIA ARTIFICIAL ====================
// =================================================================

void resetarItens() {
    p1Raio = 1; p1MaxBombas = 1; p1Imune = false;
    p2Raio = 1; p2MaxBombas = 1; p2Imune = false;
}

void gerarItem(int x, int y) {
    int chance = rand() % 100;
    if (chance < 15) matrizItens[x][y] = 10;      // Raio (🌟)
    else if (chance < 30) matrizItens[x][y] = 11; // Bomba Extra (🎁)
    else if (chance < 35) matrizItens[x][y] = 12; // Vida Extra (❤️)
    else if (chance < 45) matrizItens[x][y] = 13; // Imunidade (🛡️)
}

void processarColetaItens(int &px, int &py, int &pts, int &vds, int &raio, int &maxBombas, bool &imune) {
    if (matrizItens[px][py] != 0) {
        switch (matrizItens[px][py]) {
            case 10: raio++; pts += 50; break;
            case 11: if (maxBombas < 5) { maxBombas++; pts += 50; } break; // Cumulativo até 5
            case 12: vds++; pts += 100; break;
            case 13: imune = true; pts += 50; break;
        }
        matrizItens[px][py] = 0; 
    }
}

int calcularBonusFinal() {
    long tempoGasto = (long)(time(0) - tempoInicio); 
    if (tempoGasto < 1) tempoGasto = 1;
    int bonus = (1000 / (int)tempoGasto) - (movimentos / 5) - (bombasUsadas * 2);
    return (bonus > 0) ? bonus : 0;
}

bool posicaoLivreCompleta(int x, int y) {
    for (int i = 0; i < numInimigosAtivos; i++) {
        if (inimigos[i].vivo && inimigos[i].x == x && inimigos[i].y == y) return false;
    }
    if (bossAtivo && boss.vivo && boss.x == x && boss.y == y) return false;
    return true;
}

void moverInimigoIA(int &x, int &y, int m[][25], int px, int py, int chancePerseguir = 0) {
    if (rand() % 100 < chancePerseguir) { 
        int dx = (px > x) - (px < x); 
        int dy = (py > y) - (py < y);
        bool tentaXprimeiro = rand() % 2; 
        
        for (int tent = 0; tent < 2; tent++) {
            bool agoraX = (tent == 0) ? tentaXprimeiro : !tentaXprimeiro;
            if (agoraX && dx != 0) {
                int nx = x + dx; 
                if ((m[nx][y] == 0 || m[nx][y] == 6) && posicaoLivreCompleta(nx, y)) { x = nx; return; }
            } else if (!agoraX && dy != 0) {
                int ny = y + dy; 
                if ((m[x][ny] == 0 || m[x][ny] == 6) && posicaoLivreCompleta(x, ny)) { y = ny; return; }
            }
        }
    }
    
    int dir = rand() % 4;
    if (dir == 0 && (m[x - 1][y] == 0 || m[x - 1][y] == 6) && posicaoLivreCompleta(x - 1, y)) x--;
    if (dir == 1 && (m[x + 1][y] == 0 || m[x + 1][y] == 6) && posicaoLivreCompleta(x + 1, y)) x++;
    if (dir == 2 && (m[x][y - 1] == 0 || m[x][y - 1] == 6) && posicaoLivreCompleta(x, y - 1)) y--;
    if (dir == 3 && (m[x][y + 1] == 0 || m[x][y + 1] == 6) && posicaoLivreCompleta(x, y + 1)) y++;
}

void moverBoss(int &bx, int &by, int m[][25], int px, int py) {
    int dirs[5][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}, {0, 0}}; 
    
    if (rand() % 100 < 35) { 
        int dir = rand() % 4;
        int nx = bx + dirs[dir][0]; int ny = by + dirs[dir][1];
        if (nx >= 1 && nx <= 13 && ny >= 1 && ny <= 23 && (m[nx][ny] == 0 || m[nx][ny] == 6) && posicaoLivreCompleta(nx, ny)) {
            bx = nx; by = ny; return;
        }
    }

    int melhorScore = -100000; 
    int melhorDX = 0, melhorDY = 0;
    
    for (int i = 0; i < 5; i++) {
        int nx = bx + dirs[i][0]; int ny = by + dirs[i][1];
        if (nx < 1 || nx > 13 || ny < 1 || ny > 23) continue; 
        if (i != 4 && m[nx][ny] != 0 && m[nx][ny] != 6) continue; 
        if (i != 4 && !posicaoLivreCompleta(nx, ny)) continue; 

        int score = -(abs(nx - px) + abs(ny - py)) * 10; 
        
        if (m[nx][ny] == 6) score -= 5000;
        if (nx > 0 && m[nx - 1][ny] == 5) score -= 2000;
        if (nx < 14 && m[nx + 1][ny] == 5) score -= 2000;
        if (ny > 0 && m[nx][ny - 1] == 5) score -= 2000;
        if (ny < 24 && m[nx][ny + 1] == 5) score -= 2000;

        if (score > melhorScore) { melhorScore = score; melhorDX = dirs[i][0]; melhorDY = dirs[i][1]; }
    }
    bx += melhorDX; by += melhorDY;
}

// Calcula o mapa de perigo levando em conta raio real de cada bomba e paredes
void calcularMapaPerigo(bool perigo[15][25], int urgencia[15][25], int m[][25]) {
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 25; j++) { perigo[i][j] = false; urgencia[i][j] = 0; }

    int dirs4[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

    for (int b = 0; b < 30; b++) {
        if (!bombas[b].ativa) continue;

        int bx = bombas[b].x, by = bombas[b].y;
        int raio = (bombas[b].dono == 1) ? p1Raio : p2Raio;
        int timer = bombas[b].timer; // Quanto menor, mais urgente fugir

        perigo[bx][by] = true;
        if (urgencia[bx][by] < timer) urgencia[bx][by] = timer;

        for (int d = 0; d < 4; d++) {
            for (int r = 1; r <= raio; r++) {
                int nx = bx + dirs4[d][0] * r;
                int ny = by + dirs4[d][1] * r;
                if (nx < 0 || nx > 14 || ny < 0 || ny > 24) break;
                if (m[nx][ny] == 1) break; // Parede sólida bloqueia o fogo
                perigo[nx][ny] = true;
                if (urgencia[nx][ny] < timer) urgencia[nx][ny] = timer;
                if (m[nx][ny] == 4) break; // Caixa quebrável: fogo para aqui
            }
        }
    }
    // Fogo ativo (célula == 6) também é perigo imediato
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 25; j++)
            if (m[i][j] == 6) { perigo[i][j] = true; urgencia[i][j] = 0; }
}

// Verifica se existe pelo menos 1 célula segura acessível a partir de (sx,sy)
// após plantar uma bomba imaginária em (bx,by) com o raio do bot
bool temFugaSegura(int sx, int sy, int bx, int by, int m[][25]) {
    // Simula o perigo da bomba hipotética
    bool perigoSim[15][25];
    int urgSim[15][25];
    calcularMapaPerigo(perigoSim, urgSim, m);

    // Adiciona a bomba hipotética no mapa de perigo
    int dirs4[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    perigoSim[bx][by] = true;
    for (int d = 0; d < 4; d++) {
        for (int r = 1; r <= p1Raio; r++) {
            int nx = bx + dirs4[d][0] * r;
            int ny = by + dirs4[d][1] * r;
            if (nx < 0 || nx > 14 || ny < 0 || ny > 24) break;
            if (m[nx][ny] == 1) break;
            perigoSim[nx][ny] = true;
            if (m[nx][ny] == 4) break;
        }
    }

    // BFS a partir de sx,sy para encontrar célula segura acessível
    bool visitado[15][25] = {false};
    int filax[200], filay[200];
    int ini = 0, fim = 0;
    filax[fim] = sx; filay[fim] = sy; fim++;
    visitado[sx][sy] = true;

    while (ini < fim) {
        int cx = filax[ini], cy = filay[ini]; ini++;
        if (!perigoSim[cx][cy]) return true; // Achou célula segura!
        for (int d = 0; d < 4; d++) {
            int nx = cx + dirs4[d][0];
            int ny = cy + dirs4[d][1];
            if (nx < 1 || nx > 13 || ny < 1 || ny > 23) continue;
            if (visitado[nx][ny]) continue;
            if (m[nx][ny] != 0 && m[nx][ny] != 6 && m[nx][ny] != 7) continue;
            visitado[nx][ny] = true;
            if (fim < 200) { filax[fim] = nx; filay[fim] = ny; fim++; }
        }
    }
    return false; // Nenhuma saída segura encontrada
}

void jogarBot(int &x, int &y, int m[][25], bool &tentarBomba) {
    tentarBomba = false;
    int dirs[5][2] = {{0,0},{-1,0},{1,0},{0,-1},{0,1}};

    // === PASSO 1: Mapa de perigo correto (raio real de cada bomba, para nas paredes) ===
    bool perigo[15][25];
    int urgencia[15][25];
    calcularMapaPerigo(perigo, urgencia, m);

    bool emPerigo = perigo[x][y];

    // === PASSO 2: Escolha de movimento por pontuação ===
    int melhorScore = -9999999;
    int melhorDir = 0; // Parado por padrão

    for (int i = 0; i < 5; i++) {
        int nx = x + dirs[i][0];
        int ny = y + dirs[i][1];
        if (nx < 1 || nx > 13 || ny < 1 || ny > 23) continue;
        // Só pode ir para células livres (ou fogo/portal que já vão sumir)
        if (i != 0 && m[nx][ny] != 0 && m[nx][ny] != 6 && m[nx][ny] != 7) continue;

        int score = 0;

        // --- PRIORIDADE MÁXIMA: fugir de perigo ---
        if (perigo[nx][ny]) {
            // Penalidade base por entrar em perigo
            score -= 500000;
            // Penalidade extra proporcional à urgência (timer baixo = morte iminente)
            score -= (30 - urgencia[nx][ny]) * 5000;
        }

        // Bônus por sair de célula perigosa
        if (emPerigo && !perigo[nx][ny]) score += 300000;

        // Se estou em perigo e parado (i==0), penalidade enorme
        if (i == 0 && emPerigo) score -= 400000;

        // --- PRIORIDADE ALTA: não andar em cima de inimigo ---
        int distInimigo = 999;
        for (int k = 0; k < numInimigosAtivos; k++) {
            if (!inimigos[k].vivo) continue;
            int dist = abs(nx - inimigos[k].x) + abs(ny - inimigos[k].y);
            if (dist < distInimigo) distInimigo = dist;
        }
        if (bossAtivo && boss.vivo) {
            int dist = abs(nx - boss.x) + abs(ny - boss.y);
            if (dist < distInimigo) distInimigo = dist;
        }
        if (distInimigo == 0) score -= 800000; // Morte certa
        else if (distInimigo == 1) score -= 150000;
        else if (distInimigo == 2) score -= 30000;
        else if (distInimigo == 3) score -= 5000;

        // --- OBJETIVOS: portal > itens > inimigos > caixas adjacentes ---
        if (portalAtivo) {
            // Vai para o portal o mais rápido possível
            score -= (abs(nx - portalX) + abs(ny - portalY)) * 1000;
        } else {
            // Coleta itens no chão (prioridade alta)
            if (matrizItens[nx][ny] != 0) score += 8000;

            // Aproximação de inimigos para matá-los com bomba (prioridade média-alta)
            if (distInimigo > 1 && distInimigo < 8 && !emPerigo)
                score += (8 - distInimigo) * 400;

            // Ficar adjacente a caixas (prioridade baixa — só quebra se não tiver inimigo por perto)
            if (distInimigo >= 5) {
                int caixas = 0;
                if (nx > 0 && m[nx-1][ny] == 4) caixas++;
                if (nx < 14 && m[nx+1][ny] == 4) caixas++;
                if (ny > 0 && m[nx][ny-1] == 4) caixas++;
                if (ny < 24 && m[nx][ny+1] == 4) caixas++;
                score += caixas * 150; // Peso bem menor que antes (era 500)
            }
        }

        // Pequena aleatoriedade para não travar em ciclos
        score += rand() % 20;

        if (score > melhorScore) { melhorScore = score; melhorDir = i; }
    }

    x += dirs[melhorDir][0];
    y += dirs[melhorDir][1];

    // === PASSO 3: Decidir se planta bomba ===
    // Condições: não está em perigo, não está já com bomba ativa nessa célula,
    // e OBRIGATORIAMENTE tem rota de fuga após plantar
    if (!portalAtivo && !perigo[x][y]) {
        int caixas = 0;
        if (x > 0 && m[x-1][y] == 4) caixas++;
        if (x < 14 && m[x+1][y] == 4) caixas++;
        if (y > 0 && m[x][y-1] == 4) caixas++;
        if (y < 24 && m[x][y+1] == 4) caixas++;

        bool iniVizinho = false;
        for (int k = 0; k < numInimigosAtivos; k++) {
            if (inimigos[k].vivo && (abs(x - inimigos[k].x) + abs(y - inimigos[k].y) <= 2))
                iniVizinho = true;
        }
        if (bossAtivo && boss.vivo && (abs(x - boss.x) + abs(y - boss.y) <= 2))
            iniVizinho = true;

        bool temAlvo = (caixas > 0 || iniVizinho);
        // Só planta se tem alvo E tem fuga garantida (verificação via BFS)
        if (temAlvo && temFugaSegura(x, y, x, y, m)) {
            tentarBomba = true;
        }
    }
}

void plantarBomba(int x, int y, int m[][25], int maxB, int idDono) {
    int ativas = 0;
    for (int i = 0; i < 30; i++) if (bombas[i].ativa && bombas[i].dono == idDono) ativas++;
    if (ativas >= maxB) return; 

    for (int i = 0; i < 30; i++) {
        if (!bombas[i].ativa && !bombas[i].explodiu) {
            bombas[i].x = x; bombas[i].y = y; 
            // BOMBA CORRIGIDA: Estoura extremamente mais rápido agora!
            bombas[i].timer = 25; 
            bombas[i].ativa = true; bombas[i].dono = idDono;
            m[x][y] = 5;
            break;
        }
    }
}

void explosaoLogica(int bx, int by, int raio, int m[][25], int idDono) {
    m[bx][by] = 6;
    int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    
    for (int d = 0; d < 4; d++) {
        int caixasRestantes = raio; // Quantas caixas quebráveis o fogo pode atravessar
        for (int r = 1; r <= raio; r++) { 
            int nx = bx + dirs[d][0] * r;
            int ny = by + dirs[d][1] * r;
            
            if (nx < 1 || nx > 13 || ny < 1 || ny > 23 || m[nx][ny] == 1) break; 
            
            if (m[nx][ny] == 4) {  
                m[nx][ny] = 6; 
                caixasDestruidas++; 
                if (idDono == 1) pontuacao += 25; else p2Pontuacao += 25; 
                gerarItem(nx, ny); 
                caixasRestantes--;
                if (caixasRestantes <= 0) break; // Para se não tiver mais caixas permitidas
                continue; // Continua passando se ainda tiver raio
            }
            m[nx][ny] = 6;
        }
    }
}

void limparExplosao(int bx, int by, int raio, int m[][25]) {
    if (m[bx][by] == 6) m[bx][by] = 0;
    int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (int d = 0; d < 4; d++) {
        for (int r = 1; r <= raio; r++) {
            int nx = bx + dirs[d][0] * r; int ny = by + dirs[d][1] * r;
            if (nx < 1 || nx > 13 || ny < 1 || ny > 23 || m[nx][ny] == 1) break;
            if (m[nx][ny] == 6) m[nx][ny] = 0;
        }
    }
}

// =================================================================
// === DESENHO DOS MAPAS E FASES ===================================
// =================================================================

void inicializarMapa(int m[][25], int fase) {
    for (int i = 0; i < 15; i++) for (int j = 0; j < 25; j++) { m[i][j] = 0; matrizItens[i][j] = 0; }
    
    for (int i = 0; i < 15; i++) { m[i][0] = 1; m[i][24] = 1; }
    for (int j = 0; j < 25; j++) { m[0][j] = 1; m[14][j] = 1; }

    if (fase <= 2) {
        for (int i = 2; i < 14; i += 2) {
            for (int j = 2; j < 24; j += 2) m[i][j] = 1;
        }
    } 
    else {
        for (int i = 3; i < 5; i++) { for (int j = 3; j < 5; j++) m[i][j] = 1; for (int j = 20; j < 22; j++) m[i][j] = 1; }
        for (int i = 10; i < 12; i++) { for (int j = 3; j < 5; j++) m[i][j] = 1; for (int j = 20; j < 22; j++) m[i][j] = 1; }
        m[6][14] = 1; m[7][14] = 1; m[8][14] = 1;
    }
}

void gerarBlocosAleatorios(int m[][25], int spawnX, int spawnY, int spX2 = 0, int spY2 = 0, int densidade = 25) {
    for (int i = 1; i < 14; i++) {
        for (int j = 1; j < 24; j++) {
            if (m[i][j] != 0) continue; 
            if (abs(i - spawnX) + abs(j - spawnY) <= 2) continue; 
            if (modoDoisJogadores && (abs(i - spX2) + abs(j - spY2) <= 2)) continue; 
            
            if (rand() % 100 < densidade) m[i][j] = 4;
        }
    }
}

void resetarInimigos(int fase, int dif, int m[][25]) {
    numInimigosAtivos = (dif == 1) ? 3 : ((dif == 2) ? 5 : 7); 
    int pos[7][2] = {{1, 1}, {13, 1}, {1, 23}, {13, 23}, {7, 2}, {7, 22}, {13, 12}};
    
    for (int i = 0; i < numInimigosAtivos; i++) {
        inimigos[i].x = pos[i][0]; inimigos[i].y = pos[i][1];
        inimigos[i].vivo = true; m[pos[i][0]][pos[i][1]] = 0; 
    }
    
    bossAtivo = (fase == 3); // Boss aparece na fase 3 em qualquer dificuldade
    if (bossAtivo) { boss.x = 7; boss.y = 18; boss.vivo = true; m[7][18] = 0; }
    
    inimigosVivos = numInimigosAtivos + (bossAtivo ? 1 : 0);
}

// REQUISITO DO PDF: RECURSIVIDADE
void colocarPortal(int m[][25], int tentativas = 200) {
    if (tentativas <= 0) { 
        for (int i = 1; i < 14; i++) for (int j = 1; j < 24; j++)
                if (m[i][j] == 0) { m[i][j] = 7; portalX = i; portalY = j; portalAtivo = true; return; }
        return;
    }
    int i = 1 + rand() % 13; int j = 1 + rand() % 23;
    if (m[i][j] == 0 && matrizItens[i][j] == 0) { m[i][j] = 7; portalX = i; portalY = j; portalAtivo = true; return; }
    
    colocarPortal(m, tentativas - 1); 
}

// =================================================================
// === ENGINE GRÁFICA PRINCIPAL ====================================
// =================================================================

void desenharHUD() {
    setCor(15); cout << "\n"; desenharBordaTopo(70, 33);
    char buf[120];
    sprintf(buf, "Fase: %d  |  Dificuldade: %s", faseAtual, 
            dificuldade == 1 ? "FACIL" : dificuldade == 2 ? "MEDIO" : "DIFICIL");
    desenharLinhaLateral(buf, 70, 33);
    
    if (modoDoisJogadores) {
        sprintf(buf, "P1 (%s): Vds:%d Pts:%d | P2 (%s): Vds:%d Pts:%d", nomeJogador.c_str(), vidas, pontuacao, nomeJogador2.c_str(), p2Vidas, p2Pontuacao);
        desenharLinhaLateral(buf, 70, 33);
        sprintf(buf, "Movimentos:%d | Bombas:%d | Caixas:%d | Tempo:%lds", movimentos, bombasUsadas, caixasDestruidas, (long)(time(0) - tempoInicio));
        desenharLinhaLateral(buf, 70, 33);
        sprintf(buf, "P1 Itens: Raio:%d Bombas:%d %s | P2 Itens: Raio:%d Bombas:%d %s",
                p1Raio, p1MaxBombas, p1Imune ? "[Imune]" : "",
                p2Raio, p2MaxBombas, p2Imune ? "[Imune]" : "");
        desenharLinhaLateral(buf, 70, 33);
        desenharLinhaLateral("P1: WASD + B | P2: Setas + Enter | P=Salvar | F=Sair", 70, 33);
    } else if (modoAutoPlay) {
        sprintf(buf, "Tempo: %lds   Pontos: %d  |  [ MODO AUTOPLAY ]", (long)(time(0) - tempoInicio), pontuacao);
        desenharLinhaLateral(buf, 70, 33);
        sprintf(buf, "Movimentos:%d | Bombas:%d | Caixas:%d | Raio:%d", movimentos, bombasUsadas, caixasDestruidas, p1Raio);
        desenharLinhaLateral(buf, 70, 33);
        desenharLinhaLateral("Pressione F para abortar a simulacao", 70, 33);
    } else {
        sprintf(buf, "Jogador: %s | Vidas: %d | Tempo: %lds | Pts: %d", nomeJogador.c_str(), vidas, (long)(time(0) - tempoInicio), pontuacao);
        desenharLinhaLateral(buf, 70, 33);
        sprintf(buf, "Movimentos:%d | Bombas:%d | Caixas:%d | Raio:%d | MaxBombas:%d %s",
                movimentos, bombasUsadas, caixasDestruidas, p1Raio, p1MaxBombas, p1Imune ? "[IMUNE]" : "");
        desenharLinhaLateral(buf, 70, 33);
        desenharLinhaLateral("W/A/S/D=Mover | B=Bomba | P=Salvar | F=Sair", 70, 33);
    }
    
    desenharBordaBase(70, 33); setCor(7);
}

void resetarEstadoPartida() {
    for (int i = 0; i < 30; i++) { bombas[i].ativa = false; bombas[i].explodiu = false; }
    gameOverColideInimigo = false; gameOverExplosao = false; 
    vidas = 3; p2Vidas = 3; 
    p1Morto = false; p2Morto = !modoDoisJogadores;
    resetarItens();
}

void resetarPartidaCompleta() {
    resetarEstadoPartida();
    pontuacao = 0; p2Pontuacao = 0; bombasUsadas = 0; movimentos = 0; caixasDestruidas = 0; faseAtual = 1;
}


// >>> CÉREBRO DO JOGO (LOOP DA FASE) <<<
int executarFase() {
    int delayInimigo = 0, delayBot = 0, delayBoss = 0;
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE); COORD coord = {0, 0};

    int x = 7, y = 11;
    // Usa as globais gx2/gy2 para posição do P2, evitando conflito de escopo
    gx2 = 1; gy2 = 1;

    int m[15][25];
    inicializarMapa(m, faseAtual);
    
    if (modoDoisJogadores) { p1Morto = (vidas <= 0); p2Morto = (p2Vidas <= 0); } 
    else { p1Morto = false; p2Morto = true; } 

    gerarBlocosAleatorios(m, x, y, gx2, gy2);
    resetarInimigos(faseAtual, dificuldade, m);
    
    if (modoDoisJogadores) {
        for (int k = 0; k < numInimigosAtivos; k++) {
            if (inimigos[k].x == gx2 && inimigos[k].y == gy2 && inimigos[k].vivo) {
                inimigos[k].x = 13; inimigos[k].y = 23; // Reposiciona em vez de matar
            }
        }
    }

    for (int i = 0; i < 30; i++) { bombas[i].ativa = false; bombas[i].explodiu = false; }
    gameOverColideInimigo = false; gameOverExplosao = false;
    portalAtivo = false; portalX = -1; portalY = -1;

    if (tempoInicio == 0) tempoInicio = time(0);
    system("cls");
    iniciarMusica(); // Toca a música em loop durante a fase

    // === INÍCIO DO FRAME LOOP DO JOGO ===
    while (true) {
        SetConsoleCursorPosition(out, coord); 

        if (!p1Morto) processarColetaItens(x, y, pontuacao, vidas, p1Raio, p1MaxBombas, p1Imune);
        if (!p2Morto) processarColetaItens(gx2, gy2, p2Pontuacao, p2Vidas, p2Raio, p2MaxBombas, p2Imune);

        // >> SOLUÇÃO DE PERFORMANCE: OTIMIZAÇÃO POR STRING BUFFER
        // Agora construímos a tela inteira antes de usar o cout. Acabaram os lags!
        string tela = "";
        
        for (int i = 0; i < 15; i++) {
            for (int j = 0; j < 25; j++) {
                
                if (!p1Morto && i == x && j == y) { tela += "🧑"; continue; }
                if (!p2Morto && i == gx2 && j == gy2) { tela += "🟦"; continue; }
                
                bool desenhouEntidade = false;
                for (int k = 0; k < numInimigosAtivos; k++) {
                    if (inimigos[k].vivo && i == inimigos[k].x && j == inimigos[k].y) { tela += "👻"; desenhouEntidade = true; break; }
                }
                if (desenhouEntidade) continue;

                if (bossAtivo && boss.vivo && i == boss.x && j == boss.y) { tela += "🧌"; continue; }

                if (matrizItens[i][j] != 0 && m[i][j] == 0) {
                    if (matrizItens[i][j] == 10) tela += "🌟";
                    else if (matrizItens[i][j] == 11) tela += "🎁";
                    else if (matrizItens[i][j] == 12) tela += "❤️";
                    else if (matrizItens[i][j] == 13) tela += "🛡️";
                    continue;
                }

                switch (m[i][j]) {
                    case 0: tela += "  "; break;  
                    case 1: tela += "🧱"; break;
                    case 4: tela += "🕸️"; break;
                    case 5: tela += "💣"; break;
                    case 6: tela += "🔥"; break;
                    case 7: tela += "🌀"; break;
                }
            }
            tela += "\n";
        }
        cout << tela; // Imprime as 375 posições de uma vez!
        
        desenharHUD(); 

        // >> VERIFICAÇÃO DE DANO E COLISÃO
        bool p1MorreuAgora = false;
        bool p2MorreuAgora = false;

        if (!p1Morto && m[x][y] == 6 && !p1Imune) p1MorreuAgora = true; 
        if (!p2Morto && m[gx2][gy2] == 6 && !p2Imune) p2MorreuAgora = true; 
        
        for (int k = 0; k < numInimigosAtivos; k++) {
            if (!inimigos[k].vivo) continue;
            
            if (!p1Morto && x == inimigos[k].x && y == inimigos[k].y) p1MorreuAgora = true;
            if (!p2Morto && gx2 == inimigos[k].x && gy2 == inimigos[k].y) p2MorreuAgora = true;
            
            if (m[inimigos[k].x][inimigos[k].y] == 6 || m[inimigos[k].x][inimigos[k].y] == 5) {
                inimigos[k].vivo = false; pontuacao += 100; inimigosVivos--;
            }
        }
        if (bossAtivo && boss.vivo) {
            if (!p1Morto && x == boss.x && y == boss.y) p1MorreuAgora = true;
            if (!p2Morto && gx2 == boss.x && gy2 == boss.y) p2MorreuAgora = true;
            if (m[boss.x][boss.y] == 6 || m[boss.x][boss.y] == 5) { boss.vivo = false; pontuacao += 500; inimigosVivos--; }
        }

        // >> TRANSIÇÃO E VITÓRIA DA FASE
        if (inimigosVivos <= 0 && !portalAtivo) {
            colocarPortal(m); 
            SetConsoleCursorPosition(out, coord); cout << "\n";
            setCor(46); imprimirCentralizado(">>> PORTAL ABERTO! <<<", 70); setCor(7);
            Sleep(1500); system("cls");
        }
        
        if (portalAtivo && ((!p1Morto && x == portalX && y == portalY) || (!p2Morto && gx2 == portalX && gy2 == portalY))) {
            pontuacao += calcularBonusFinal();
            pararMusica(); // Para a música ao avançar de fase
            SetConsoleCursorPosition(out, coord); cout << "\n";
            setCor(13); imprimirCentralizado(">>> AVANCANDO DE FASE E LIMPANDO INVENTARIO... <<<", 70); setCor(7);
            Sleep(1800);
            return (faseAtual >= 3) ? ESTADO_VITORIA : ESTADO_PROXIMA_FASE; 
        }

        if (p1MorreuAgora) {
            vidas--;
            if (vidas > 0) {
                x = 7; y = 11; 
                if (m[x][y] == 6) m[x][y] = 0; 
                p1Raio = 1; p1MaxBombas = 1; p1Imune = false; 
            } else p1Morto = true;
        }
        if (p2MorreuAgora) {
            p2Vidas--;
            if (p2Vidas > 0) {
                gx2 = 1; gy2 = 1; 
                if (m[gx2][gy2] == 6) m[gx2][gy2] = 0; 
                p2Raio = 1; p2MaxBombas = 1; p2Imune = false; 
            } else p2Morto = true;
        }

        if (p1Morto && p2Morto) { pararMusica(); return ESTADO_DERROTA; }

        // >> TIMERS E DESTRUIÇÃO (FOGO E BOMBA)
        for (int i = 0; i < 30; i++) {
            if (bombas[i].ativa) {
                bombas[i].timer--; 
                
                if (bombas[i].timer <= 0) { 
                    m[bombas[i].x][bombas[i].y] = 0; 
                    int raioC = (bombas[i].dono == 1) ? p1Raio : p2Raio; 
                    explosaoLogica(bombas[i].x, bombas[i].y, raioC, m, bombas[i].dono);
                    bombas[i].ativa = false; 
                    fimTimer[i] = 10; // Fogo some mais rápido agora
                    bombas[i].explodiu = true;
                }
            }
            if (bombas[i].explodiu) {
                fimTimer[i]--; 
                if (fimTimer[i] <= 0) { 
                    int raioC = (bombas[i].dono == 1) ? p1Raio : p2Raio;
                    limparExplosao(bombas[i].x, bombas[i].y, raioC, m); 
                    bombas[i].explodiu = false; 
                }
            }
        }

        // >> CONTROLES
        if (modoAutoPlay) {
            delayBot++; 
            if (delayBot >= 6 && !p1Morto) { // Bot reage mais rápido ao perigo
                bool tentarBomba = false; 
                jogarBot(x, y, m, tentarBomba); movimentos++;
                if (tentarBomba) { plantarBomba(x, y, m, p1MaxBombas, 1); bombasUsadas++; }
                delayBot = 0;
            }
            if (_kbhit()) { char c = _getch(); if (c == 'f' || c == 'F') { pararMusica(); return ESTADO_DESISTIU; } }
            
        } else {
            if (_kbhit()) {
                int tecla = _getch();
                
                if (tecla == 224) { 
                    tecla = _getch();
                    if (!p2Morto) {
                        switch (tecla) {
                            case 72: if (m[gx2 - 1][gy2] == 0 || m[gx2 - 1][gy2] == 6 || m[gx2 - 1][gy2] == 7) gx2--; break; 
                            case 80: if (m[gx2 + 1][gy2] == 0 || m[gx2 + 1][gy2] == 6 || m[gx2 + 1][gy2] == 7) gx2++; break; 
                            case 75: if (m[gx2][gy2 - 1] == 0 || m[gx2][gy2 - 1] == 6 || m[gx2][gy2 - 1] == 7) gy2--; break; 
                            case 77: if (m[gx2][gy2 + 1] == 0 || m[gx2][gy2 + 1] == 6 || m[gx2][gy2 + 1] == 7) gy2++; break; 
                        }
                    }
                } else {
                    switch (tecla) { 
                        case 'w': case 'W': if (!p1Morto && (m[x - 1][y] == 0 || m[x - 1][y] == 6 || m[x - 1][y] == 7)) { x--; movimentos++; } break;
                        case 's': case 'S': if (!p1Morto && (m[x + 1][y] == 0 || m[x + 1][y] == 6 || m[x + 1][y] == 7)) { x++; movimentos++; } break;
                        case 'a': case 'A': if (!p1Morto && (m[x][y - 1] == 0 || m[x][y - 1] == 6 || m[x][y - 1] == 7)) { y--; movimentos++; } break;
                        case 'd': case 'D': if (!p1Morto && (m[x][y + 1] == 0 || m[x][y + 1] == 6 || m[x][y + 1] == 7)) { y++; movimentos++; } break;
                        case 'b': case 'B': if (!p1Morto) { plantarBomba(x, y, m, p1MaxBombas, 1); bombasUsadas++; } break;
                        case 13:  if (!p2Morto) { plantarBomba(gx2, gy2, m, p2MaxBombas, 2); } break; 
                        case 'p': case 'P': pararMusica(); salvarJogo(nomeJogador); setCor(46); imprimirCentralizado("Jogo salvo! Saindo...", 70); setCor(7); Sleep(1000); return ESTADO_DESISTIU;
                        case 'f': case 'F': pararMusica(); return ESTADO_DESISTIU;
                    }
                }
            }
        }

        // >> UPDATES INIMIGOS (Balanceado para a nova velocidade do jogo)
        delayInimigo++; delayBoss++;
        
        if (delayInimigo >= 12) { 
            int chance = (dificuldade == 2) ? 50 : ((dificuldade == 3) ? 75 : 0);
            for (int k = 0; k < numInimigosAtivos; k++) {
                if (inimigos[k].vivo) {
                    int alvoX = x; int alvoY = y;
                    if (modoDoisJogadores && !p2Morto && p1Morto) { alvoX = gx2; alvoY = gy2; } 
                    else if (modoDoisJogadores && !p2Morto && !p1Morto) { 
                        if (abs(inimigos[k].x - gx2) + abs(inimigos[k].y - gy2) < abs(inimigos[k].x - x) + abs(inimigos[k].y - y)) {
                            alvoX = gx2; alvoY = gy2;
                        }
                    }
                    moverInimigoIA(inimigos[k].x, inimigos[k].y, m, alvoX, alvoY, chance);
                }
            }
            delayInimigo = 0;
        }
        
        if (delayBoss >= 15) { 
            if (bossAtivo && boss.vivo) {
                int alvoX = x; int alvoY = y;
                if (modoDoisJogadores && !p2Morto && p1Morto) { alvoX = gx2; alvoY = gy2; }
                else if (modoDoisJogadores && !p2Morto && !p1Morto) {
                    if (abs(boss.x - gx2) + abs(boss.y - gy2) < abs(boss.x - x) + abs(boss.y - y)) { alvoX = gx2; alvoY = gy2; }
                }
                moverBoss(boss.x, boss.y, m, alvoX, alvoY);
            }
            delayBoss = 0;
        }
        
        // LIMITADOR DA ENGINE FPS: Mais rápido e fluido.
        Sleep(30); 
    }
    pararMusica();
    return ESTADO_DESISTIU;
}

// =================================================================
// === TELAS MENUS E EXIBIÇÃO ======================================
// =================================================================

void aguardarTecla() {
    setCor(244); cout << "\n  Pressione qualquer tecla para voltar..."; setCor(7);
    while (_kbhit()) _getch(); 
    _getch(); 
}

void cabecalhoTela(string titulo, int corBorda = 33) {
    system("cls"); 
    desenharBordaTopo(70, corBorda); 
    desenharLinhaLateral(70, corBorda);
    
    setCor(corBorda); cout << "║"; setCor(226);
    
    int pad = (70 - 2 - (int)titulo.size()) / 2;
    for (int i = 0; i < pad; i++) cout << " ";
    cout << titulo;
    for (int i = pad + (int)titulo.size(); i < 68; i++) cout << " ";
    
    setCor(corBorda); cout << "║\n"; 
    desenharLinhaLateral(70, corBorda); 
    desenharBordaBase(70, corBorda);
    setCor(7); cout << "\n";
}

void lerStringConsole(string &str) {
    str = "";
    while (true) {
        char c = _getch();
        if (c == 13 || c == 10) {  if (str.length() > 0) break; } 
        else if (c == 8) { 
            if (str.length() > 0) { str.pop_back(); cout << "\b \b"; }
        } else if (c >= 32 && c <= 126 && str.length() < 15) { 
            str += c; cout << c;
        }
    }
}

void configurarNomesJogadores() {
    system("cls");
    desenharBordaTopo(70, 33);
    desenharLinhaLateral("CADASTRO DE NOMES", 70, 33);
    desenharBordaBase(70, 33);
    
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = true; SetConsoleCursorInfo(out, &cursorInfo);

    cout << "\n  Digite o nome do Jogador 1 (Aperte ENTER): ";
    lerStringConsole(nomeJogador);
    
    if (modoDoisJogadores) {
        cout << "\n  Digite o nome do Jogador 2 (Aperte ENTER): ";
        lerStringConsole(nomeJogador2);
    }

    cursorInfo.bVisible = false; SetConsoleCursorInfo(out, &cursorInfo);
}

int telaGameOver(int estado) {
    system("cls");
    if (estado == ESTADO_VITORIA) { setCor(46); imprimirCentralizado("VOCE VENCEU!!!", 70); } 
    else if (estado == ESTADO_DERROTA) { setCor(196); imprimirCentralizado("FIM DE JOGO!", 70); } 
    else { setCor(244); imprimirCentralizado("PARTIDA INTERROMPIDA", 70); }
    
    setCor(7); cout << "\n"; 
    
    char buf[120];
    sprintf(buf, "Pontuacao P1 (%s): %d", nomeJogador.c_str(), pontuacao); imprimirCentralizado(buf, 70); cout << "\n";
    if (modoDoisJogadores) { sprintf(buf, "Pontuacao P2 (%s): %d", nomeJogador2.c_str(), p2Pontuacao); imprimirCentralizado(buf, 70); cout << "\n"; }

    if (estado != ESTADO_DESISTIU) {
        if (modoDoisJogadores) {
            string nomeCombinado = nomeJogador + " + " + nomeJogador2;
            int maiorPontuacao = (pontuacao > p2Pontuacao) ? pontuacao : p2Pontuacao; 
            registrarNoRank(nomeCombinado, maiorPontuacao, faseAtual);
        } else registrarNoRank(nomeJogador, pontuacao, faseAtual);
    }

    setCor(15);
    imprimirCentralizado("[1] Continuar (rejogar a fase atual)", 70);
    imprimirCentralizado("[2] Reiniciar do zero", 70);
    imprimirCentralizado("[3] Voltar ao menu", 70);
    setCor(7);

    while (true) {
        if (_kbhit()) { char c = _getch(); if (c >= '1' && c <= '3') return c - '0'; }
        Sleep(30);
    }
}

int menuPrincipal() {
    system("cls"); 
    setCor(196); cout << "\n";
    imprimirCentralizado("####    ####  ###    ###  ###   ###   ######  ######   ###    ###   ###    ###   ###    ###", 90);
    imprimirCentralizado("##  ## ##  ## ## ## ## ## ##  ##  ##  ##      ##   ##  ## ## ## ##  ##  ## ##    ## ## ##", 90);
    imprimirCentralizado("####   ##  ## ##  ###  ## ## ##   ##  ####    ######   ##  ###  ## ## ##  ## ##   ##  ## ##", 90);
    imprimirCentralizado("##  ## ##  ## ##       ## ## ##   ##  ##      ##  ##   ##       ## ##  ##  ##   ##   ####", 90);
    imprimirCentralizado("####    ####  ##       ## ##  ##  ##  ######  ##   ##  ##       ## ##  ####  ##   ##    ##", 90);
    cout << "\n\n";

    setCor(33);  imprimirCentralizado("================ MENU PRINCIPAL ================", 70); setCor(7); cout << "\n";
    setCor(46);  imprimirCentralizado("[1] Iniciar Jogo (1 Jogador)", 70);
    setCor(208); imprimirCentralizado("[2] 2 Jogadores (Local)", 70);
    setCor(33);  imprimirCentralizado("[3] Como Jogar e Itens", 70);
    setCor(33);  imprimirCentralizado("[4] Dificuldades e Pontos", 70);
    setCor(226); imprimirCentralizado("[5] Rank Top 10", 70);
    setCor(45);  imprimirCentralizado("[6] Carregar Save", 70);
    setCor(13);  imprimirCentralizado("[7] AutoPlay (O Computador Joga pra Mim)", 70); 
    setCor(196); imprimirCentralizado("[8] Sair", 70); setCor(7); cout << "\n";
    
    while (true) {
        if (_kbhit()) { char c = _getch(); if (c >= '1' && c <= '8') return c - '0'; }
        Sleep(30);
    }
}

int telaSelecionarDificuldade() {
    cabecalhoTela("SELECIONE A DIFICULDADE", 33);
    setCor(46);  imprimirCentralizado("[1] FACIL    - 3 inimigos", 70);
    setCor(226); imprimirCentralizado("[2] MEDIO    - 5 inimigos", 70);
    setCor(196); imprimirCentralizado("[3] DIFICIL  - 7 inimigos + BOSS", 70);
    setCor(7); cout << "\n"; imprimirCentralizado("Escolha (1/2/3): ", 70);
    
    while (true) {
        if (_kbhit()) { char c = _getch(); if (c >= '1' && c <= '3') return c - '0'; }
        Sleep(30);
    }
}

bool telaCarregarSave() {
    cabecalhoTela("CARREGAR SAVE", 33); JogoSave s;
    if (!lerSave(s)) { cout << "  Nenhum save encontrado no HD.\n"; aguardarTecla(); return false; }
    
    cout << "  Save de " << s.jogador << " - Fase " << s.fase << " - Pontos: " << s.pontuacao << "\n\n";
    cout << "  Carregar? (S/N): ";
    
    while (true) {
        if (_kbhit()) { 
            char c = _getch(); 
            if (c == 's' || c == 'S') { aplicarSave(); return true; } 
            if (c == 'n' || c == 'N') return false; 
        }
        Sleep(30);
    }
}

// =================================================================
// === ENTRADA PRINCIPAL DO SISTEMA ================================
// =================================================================
int main() {
    srand(time(0)); 
    SetConsoleOutputCP(CP_UTF8); 

    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = false; SetConsoleCursorInfo(out, &cursorInfo);

    while (true) {
        int op = menuPrincipal();
        
        if (op == 1 || op == 2 || op == 7) { 
            modoAutoPlay = (op == 7);
            modoDoisJogadores = (op == 2);
            
            if (modoAutoPlay) nomeJogador = "SkyNet_Bot"; 
            else configurarNomesJogadores();
            
            resetarPartidaCompleta(); 
            dificuldade = telaSelecionarDificuldade(); 
            tempoInicio = time(0);
            
            while (true) {
                int estado = executarFase(); 
                
                if (estado == ESTADO_PROXIMA_FASE) { 
                    faseAtual++; tempoInicio = time(0); resetarItens(); continue; 
                }
                if (estado == ESTADO_DESISTIU) break; 
                
                int escolha = telaGameOver(estado);
                if (escolha == 1) { resetarEstadoPartida(); tempoInicio = time(0); continue; }
                if (escolha == 2) { resetarPartidaCompleta(); dificuldade = telaSelecionarDificuldade(); tempoInicio = time(0); continue; }
                break;
            }
        }
        else if (op == 3) { 
            cabecalhoTela("COMO JOGAR E ITENS", 33); 
            cout << "    CONTROLES:\n";
            cout << "    P1: W/A/S/D (Mover) | B (Soltar Bomba) | P (Salvar Jogo) | F (Sair)\n";
            cout << "    P2: Setas (Mover) | Enter (Soltar Bomba)\n\n"; 
            cout << "    OBJETIVO: Destrua todas as caixas e inimigos, encontre o portal 🌀\n";
            cout << "    e avance de fase! Sobreviva ate vencer a Fase 3 com o Boss.\n\n";
            cout << "    ITENS ESPECIAIS (aparecem sob caixas destruidas):\n";
            cout << "    🌟 Raio+   - Aumenta raio de fogo em +1 (cumulativo)\n";
            cout << "    🎁 Bomba+  - Aumenta max de bombas simultaneas em +1 (ate 5)\n";
            cout << "    ❤️  Vida+   - Ganha 1 vida extra\n";
            cout << "    🛡️  Imune   - Sobrevive a propria explosao (reseta a cada fase)\n\n";
            cout << "    PONTUACAO:\n";
            cout << "    +100 por inimigo abatido | +500 por Boss abatido\n";
            cout << "    +25 por caixa destruida  | +50 a +100 por item coletado\n";
            cout << "    + Bonus de fase: velocidade, menos movimentos, menos bombas\n";
            aguardarTecla(); 
        }
        else if (op == 4) { 
            cabecalhoTela("DIFICULDADES E PONTUACAO", 33); 
            setCor(46);  cout << "    [FACIL]   - 3 inimigos, movimentos aleatorios\n";
            setCor(226); cout << "    [MEDIO]   - 5 inimigos, 50% de chance de perseguir o jogador\n";
            setCor(196); cout << "    [DIFICIL] - 7 inimigos, 75% de chance de perseguir + BOSS na Fase 3\n";
            setCor(7);
            cout << "\n    CALCULO DA PONTUACAO:\n";
            cout << "    +100  por inimigo normal abatido\n";
            cout << "    +500  por Boss abatido\n";
            cout << "    + 25  por caixa destruida pela bomba\n";
            cout << "    + 50  por item de raio ou imunidade coletado\n";
            cout << "    +100  por item de vida coletado\n";
            cout << "    + 50  por item de bomba extra coletado\n";
            cout << "\n    BONUS DE FIM DE FASE:\n";
            cout << "    Formula: (1000 / tempo_segundos) - (movimentos / 5) - (bombas * 2)\n";
            cout << "    Jogue rapido, com poucos movimentos e poucas bombas para bonus maximo!\n";
            cout << "    (Bonus minimo = 0, nao penaliza a pontuacao acumulada)\n";
            aguardarTecla(); 
        }
        else if (op == 5) {
            cabecalhoTela("RANK - TOP 10", 33); RankEntry arr[10]; int qtd = lerRank(arr, 10);
            if (qtd == 0) cout << "  Nenhuma partida registrada ainda.\n";
            else {
                setCor(226);
                cout << "  Pos    Pontos  Fase   Jogador              Data\n";
                cout << "  -------------------------------------------------------\n";
                setCor(7);
                for (int i = 0; i < qtd; i++) { 
                    char linha[150]; 
                    sprintf(linha, "  %3d    %6d   %3d   %-20s %s", i + 1, arr[i].pontuacao, arr[i].fase, arr[i].jogador, arr[i].data); 
                    cout << linha << "\n"; 
                }
            }
            aguardarTecla();
        }
        else if (op == 6) { 
            if (telaCarregarSave()) {
                modoAutoPlay = false; modoDoisJogadores = false;
                while (true) {
                    int estado = executarFase();
                    if (estado == ESTADO_PROXIMA_FASE) { faseAtual++; tempoInicio = time(0); resetarItens(); continue; }
                    if (estado == ESTADO_DESISTIU) break; 
                    
                    int escolha = telaGameOver(estado);
                    if (escolha == 1) { resetarEstadoPartida(); tempoInicio = time(0); continue; }
                    if (escolha == 2) { resetarPartidaCompleta(); dificuldade = telaSelecionarDificuldade(); tempoInicio = time(0); continue; }
                    break;
                }
            }
        }
        else if (op == 8) { system("cls"); setCor(196); imprimirCentralizado("Ate a proxima!", 70); setCor(7); return 0; }
    }
    return 0;
}