#include "raylib.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

/* ---------- константы ---------- */
#define W       800
#define H       600
#define T        32        /* размер одного блока в пикселях */
#define COLS     25
#define ROWS     18
#define MAXF   7200        /* максимум кадров записи (2 мин @ 60 fps) */

/* ---------- карта уровня ---------- */
/* # - стена, ^ - шип, G - цель, P - старт игрока */
static const char MAP[] =
    "#########################"
    "#                       #"
    "#  ^G       #           #"
    "#  ###      #   ##      #"
    "#           #           #"
    "# ^    #    #           #"
    "####   #    ##    ##    #"
    "#      #      ^         #"
    "#     ###     ##    ### #"
    "#                       #"
    "#  ##    ###       ##   #"
    "#            ###        #"
    "#   ##     ^            #"
    "#         ##    ###     #"
    "#    ###                #"
    "# ^^^  P  ^^^      ^^^  #"
    "##########################"
    "#                        #";

/* ---------- состояния игры ---------- */
#define S_MENU  0   /* главное меню */
#define S_PLAY  1   /* игровой процесс */
#define S_WIN   2   /* победа */
#define S_LOSE  3   /* поражение */

static int   state, tries;  /* текущее состояние и номер попытки */
static float timer;         /* время в текущем состоянии */

/* ---------- игрок ---------- */
static float px, py;    /* позиция */
static float pvx, pvy;  /* скорость */
static int   on_ground; /* стоит ли на земле */
static int   facing;    /* направление (1 = вправо, 0 = влево) */

/* ---------- эхо (прошлая попытка) ---------- */
static float echo_x[MAXF], echo_y[MAXF];
static int   echo_len, echo_has;

/* ---------- запись текущей попытки ---------- */
static float rec_x[MAXF], rec_y[MAXF];
static int   rec_n;

/* ---------- стартовая позиция ---------- */
static float sx, sy;

/* возвращает символ карты по колонке и строке */
static char tile(int c, int r) {
    if (c<0||c>=COLS||r<0||r>=ROWS) return '#';
    return MAP[r*COLS+c];
}

/* проверяет столкновение прямоугольника с тайлом типа t */
static int hits(float x, float y, float w, float h, char t) {
    for (int r=(int)(y/T)-1; r<=(int)((y+h)/T)+1; r++)
    for (int c=(int)(x/T)-1; c<=(int)((x+w)/T)+1; c++)
        if (tile(c,r)==t)
            if (x<(c+1)*T && x+w>c*T && y<(r+1)*T && y+h>r*T)
                return 1;
    return 0;
}

/* рисует текст по центру экрана */
static void center(const char *s, int y, int sz, Color col) {
    DrawText(s,(W-MeasureText(s,sz))/2,y,sz,col);
}

/* сбрасывает игрока на стартовую позицию */
static void reset_player(void) {
    px=sx; py=sy; pvx=pvy=0; on_ground=0; facing=1;
}

/* меняет состояние игры и сбрасывает таймер */
static void go(int s) { state=s; timer=0; }

/* вызывается при смерти или победе: сохраняет запись как эхо */
static void on_death_or_win(void) {
    if (rec_n>0) {
        memcpy(echo_x, rec_x, rec_n*sizeof(float));
        memcpy(echo_y, rec_y, rec_n*sizeof(float));
        echo_len=rec_n; echo_has=1;
    }
    rec_n=0; tries++;
}

/* ---------- главный цикл (один кадр) ---------- */
static void tick(void) {
    float dt = GetFrameTime(); /* время прошлого кадра в секундах */
    if (dt>0.05f) dt=0.05f;   /* ограничение на случай зависания */
    timer+=dt;

    /* ---- обновление логики ---- */
    if (state==S_MENU) {
        if (IsKeyPressed(KEY_SPACE)) go(S_PLAY);

    } else if (state==S_PLAY) {
        /* записываем позицию каждый кадр */
        if (rec_n<MAXF) { rec_x[rec_n]=px; rec_y[rec_n]=py; rec_n++; }

        /* ввод с клавиатуры */
        float mx=0;
        if (IsKeyDown(KEY_LEFT) ||IsKeyDown(KEY_A)) { mx=-1; facing=0; }
        if (IsKeyDown(KEY_RIGHT)||IsKeyDown(KEY_D)) { mx= 1; facing=1; }
        pvx=mx*200.0f;

        /* прыжок только если стоим на земле */
        if ((IsKeyPressed(KEY_SPACE)||IsKeyPressed(KEY_W)) && on_ground)
            { pvy=-560; on_ground=0; }

        /* гравитация */
        pvy+=1300*dt;
        if (pvy>800) pvy=800;

        /* движение по X + коллизия */
        px+=pvx*dt;
        if (hits(px,py,22,26,'#')) { px-=pvx*dt; pvx=0; }

        /* движение по Y + коллизия */
        on_ground=0;
        py+=pvy*dt;
        if (hits(px,py,22,26,'#')) {
            if (pvy>0) on_ground=1; /* приземлились */
            py-=pvy*dt; pvy=0;
        }

        /* смерть: упал за карту или задел шип */
        if (py>ROWS*T+40 || hits(px+3,py+6,16,18,'^'))
            { on_death_or_win(); go(S_LOSE); return; }

        /* победа: добрался до цели */
        if (hits(px,py,22,26,'G'))
            { on_death_or_win(); go(S_WIN); }

    } else {
        /* экраны победы/поражения*/
        if (timer>0.5f && IsKeyPressed(KEY_SPACE)) { reset_player(); go(S_PLAY); }
        if (timer>0.5f && IsKeyPressed(KEY_M))
            { tries=0; echo_has=0; reset_player(); go(S_MENU); }
    }

    /* ---- отрисовка ---- */
    BeginDrawing();
    ClearBackground((Color){12,12,28,255});

    if (state==S_MENU) {
        /* анимированный призрак на фоне */
        float gx=W/2.0f-12+40*sinf(timer*0.9f);
        float gy=H/2.0f+10+8*cosf(timer*1.1f);
        DrawRectangleRounded((Rectangle){gx,gy,24,28},0.3f,4,(Color){160,190,255,60});

        /* текст меню латиница, иначе ??? на Windows */
        center("ECHO RUNNER",   H/2-80, 52, (Color){220,240,255,255});
        center("reach the goal - your echo follows", H/2-15, 17, (Color){160,180,220,180});
        if ((int)(timer*2)%2==0)
            center("SPACE - start", H/2+40, 22, (Color){120,180,255,255});
        center("A/D or arrows - move     SPACE - jump", H-40, 14, (Color){100,110,150,160});

    } else {
        /* --- рисуем тайлы --- */
        for (int r=0;r<ROWS;r++) for (int c=0;c<COLS;c++) {
            char t=tile(c,r); float rx=c*T, ry=r*T;
            if (t=='#') {
                /* стена */
                DrawRectangle(rx,ry,T,T,(Color){55,55,88,255});
                DrawRectangleLinesEx((Rectangle){rx,ry,T,T},0.5f,(Color){75,75,115,255});
            } else if (t=='^') {
                /* шипы — три треугольника */
                float tw=T/3.0f;
                for (int i=0;i<3;i++)
                    DrawTriangle(
                        (Vector2){rx+i*tw+tw/2,ry+4},
                        (Vector2){rx+i*tw,     ry+T},
                        (Vector2){rx+i*tw+tw,  ry+T},
                        (Color){210,70,70,255});
            } else if (t=='G') {
                /* цель — пульсирующий квадрат */
                float p=6+2*sinf(timer*3);
                DrawRectangleRounded((Rectangle){rx+p,ry+p,T-2*p,T-2*p},0.4f,4,(Color){255,210,60,255});
            }
        }

        /* --- рисуем эхо --- */
        if (echo_has) {
            /* берём кадр эха соответствующий текущему кадру попытки */
            int fi = rec_n < echo_len ? rec_n : echo_len-1;
            DrawRectangleRounded(
                (Rectangle){echo_x[fi], echo_y[fi], 22, 26},
                0.3f, 4, (Color){140,170,255,90});
        }

        /* игрок */
        if (state==S_PLAY) {
            DrawRectangleRounded((Rectangle){px,py,22,26},0.3f,4,(Color){220,235,255,255});
            float ex=px+(facing?15:6); /* глаз смотрит в сторону движения */
            DrawCircle(ex,py+9,2.5f,(Color){12,12,28,255});
        }

        /*интерфейс (HUD)  */
        char buf[32];
        snprintf(buf,sizeof(buf),"attempt %d", tries+1);
        DrawText(buf,12,10,16,(Color){150,160,195,170});
        if (echo_has) DrawText("echo ON",W-80,10,16,(Color){140,170,255,170});

        /* экран победы */
        if (state==S_WIN) {
            DrawRectangle(0,0,W,H,(Color){0,0,0,110});
            center("YOU ESCAPED",H/2-55,48,(Color){255,210,60,255});
            snprintf(buf,sizeof(buf),"attempts: %d",tries);
            center(buf,H/2+5,22,(Color){210,225,255,210});
            if ((int)(timer*2)%2==0)
                center("SPACE retry   M menu",H/2+55,17,(Color){120,180,255,255});
        }

        /* экран поражения */
        if (state==S_LOSE) {
            DrawRectangle(0,0,W,H,(Color){0,0,0,120});
            center("YOU FADED",H/2-55,48,(Color){210,70,70,255});
            snprintf(buf,sizeof(buf),"attempts: %d",tries);
            center(buf,H/2+5,22,(Color){210,215,235,200});
            if (timer>0.5f && (int)(timer*2)%2==0)
                center("SPACE retry   M menu",H/2+55,17,(Color){120,180,255,255});
        }
    }

    EndDrawing();
}


#if defined(PLATFORM_WEB)
#include <emscripten.h>
static void loop(void) { tick(); }
#endif

int main(void) {
    InitWindow(W, H, "Echo Runner");
    SetTargetFPS(60);

    /* P на карте это старт*/
    for (int r=0;r<ROWS;r++) for (int c=0;c<COLS;c++)
        if (tile(c,r)=='P') { sx=c*T+40; sy=r*T-26; }
    reset_player();

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(loop,0,1); 
#else
    while (!WindowShouldClose()) tick(); 
    CloseWindow();
#endif
    return 0;
}
