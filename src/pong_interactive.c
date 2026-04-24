// ============================================================================
//                   P01D06. PONG GAME (BONUS) — src/pong_interactive.c
//                          Интерактивный режим (ncurses)
// ----------------------------------------------------------------------------
//  Real-time Pong 80x25 на ncurses. Управление:
//      A / Z  — левая ракетка вверх / вниз
//      K / M  — правая ракетка вверх / вниз
//      Q      — выход
//  Игра до 21 очка. Все ограничения базовой части соблюдены.
//
//  Компиляция:
//      gcc -Wall -Werror -Wextra src/pong_interactive.c -lncurses -o pong_interactive
//
//  Проверка:
//      clang-format -n src/pong_interactive.c
//      cppcheck --enable=all --suppress=missingIncludeSystem src/pong_interactive.c
// ============================================================================

#include <ncurses.h>

// ----------------------------------------------------------------------------
//                              КОНСТАНТЫ
// ----------------------------------------------------------------------------
#define FIELD_WIDTH 80
#define FIELD_HEIGHT 25
#define PADDLE_SIZE 3
#define PADDLE1_X 2
#define PADDLE2_X 77
#define BALL_START_X 40
#define BALL_START_Y 12
#define PADDLE_START_Y 11
#define WIN_SCORE 21
#define TICK_MS 80

#define CHAR_BALL 'O'
#define CHAR_PADDLE '|'
#define CHAR_BORDER_H '-'
#define CHAR_BORDER_V '|'
#define CHAR_CORNER '+'
#define CHAR_EMPTY ' '

#define INPUT_NONE 0
#define INPUT_P1_UP 1
#define INPUT_P1_DN 2
#define INPUT_P2_UP 3
#define INPUT_P2_DN 4
#define INPUT_QUIT 5

// ----------------------------------------------------------------------------
//                         ОБЪЯВЛЕНИЯ ФУНКЦИЙ
// ----------------------------------------------------------------------------
void init_ncurses(void);
void cleanup_ncurses(void);

int get_cell_char(int x, int y, int ball_x, int ball_y, int p1_y, int p2_y);
void draw_frame(int ball_x, int ball_y, int p1_y, int p2_y);
void draw_score(int score1, int score2);
void draw_winner(int winner);
void render(int ball_x, int ball_y, int p1_y, int p2_y, int score1, int score2);

int read_input(void);
int update_paddle1(int input, int p1_y);
int update_paddle2(int input, int p2_y);
int next_ball_x(int ball_x, int ball_dx);
int next_ball_y(int ball_y, int ball_dy);
int bounce_dx(int ball_x, int ball_y, int ball_dx, int p1_y, int p2_y);
int bounce_dy(int ball_y, int ball_dy);
int check_goal(int ball_x);
int check_winner(int score1, int score2);
int check_quit(int input, int quit);

// ============================================================================
//                                   MAIN
// ============================================================================
int main(void) {
    int ball_x = BALL_START_X, ball_y = BALL_START_Y;
    int ball_dx = 1, ball_dy = 1;
    int p1_y = PADDLE_START_Y, p2_y = PADDLE_START_Y;
    int score1 = 0, score2 = 0;
    int winner = 0, quit = 0, goal = 0, input = 0;
    init_ncurses();
    while (winner == 0 && quit == 0) {
        input = read_input();
        while (input != INPUT_NONE) {
            p1_y = update_paddle1(input, p1_y);
            p2_y = update_paddle2(input, p2_y);
            quit = check_quit(input, quit);
            input = read_input();
        }
        ball_dx = bounce_dx(ball_x, ball_y, ball_dx, p1_y, p2_y);
        ball_dy = bounce_dy(ball_y, ball_dy);
        ball_x = next_ball_x(ball_x, ball_dx);
        ball_y = next_ball_y(ball_y, ball_dy);
        goal = check_goal(ball_x);
        if (goal == 1) {
            score1 = score1 + 1;
            ball_x = BALL_START_X;
            ball_y = BALL_START_Y;
        }
        if (goal == 2) {
            score2 = score2 + 1;
            ball_x = BALL_START_X;
            ball_y = BALL_START_Y;
        }
        winner = check_winner(score1, score2);
        render(ball_x, ball_y, p1_y, p2_y, score1, score2);
        napms(TICK_MS);
    }
    if (quit == 0) {
        draw_winner(winner);
    }
    cleanup_ncurses();
    return 0;
}

// ============================================================================
//                          ИНИЦИАЛИЗАЦИЯ NCURSES
// ============================================================================

// Переводит терминал в режим ncurses:
//   cbreak   — посимвольный ввод (без ожидания Enter);
//   noecho   — не печатать нажатые клавиши;
//   curs_set — спрятать курсор;
//   timeout  — неблокирующий getch (возвращает ERR, если ничего не нажато).
void init_ncurses(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    timeout(0);
}

// Возвращает терминал в обычный режим.
void cleanup_ncurses(void) {
    endwin();
}

// ============================================================================
//                                РЕНДЕРИНГ
// ============================================================================

// Возвращает символ, который должен стоять в клетке (x, y) поля.
// Порядок проверок: углы → горизонт/вертикаль рамок → мяч → ракетки → пусто.
int get_cell_char(int x, int y, int ball_x, int ball_y, int p1_y, int p2_y) {
    int result = CHAR_EMPTY;
    int is_corner = (x == 0 || x == FIELD_WIDTH - 1) && (y == 0 || y == FIELD_HEIGHT - 1);
    int is_h_border = (y == 0 || y == FIELD_HEIGHT - 1);
    int is_v_border = (x == 0 || x == FIELD_WIDTH - 1);
    int is_ball = (x == ball_x && y == ball_y);
    int is_p1 = (x == PADDLE1_X && y >= p1_y && y < p1_y + PADDLE_SIZE);
    int is_p2 = (x == PADDLE2_X && y >= p2_y && y < p2_y + PADDLE_SIZE);
    if (is_corner) {
        result = CHAR_CORNER;
    } else if (is_h_border) {
        result = CHAR_BORDER_H;
    } else if (is_v_border) {
        result = CHAR_BORDER_V;
    } else if (is_ball) {
        result = CHAR_BALL;
    } else if (is_p1 || is_p2) {
        result = CHAR_PADDLE;
    }
    return result;
}

// Рисует всё поле 80x25 посимвольно через mvaddch.
void draw_frame(int ball_x, int ball_y, int p1_y, int p2_y) {
    int y = 0;
    int x = 0;
    int ch = 0;
    for (y = 0; y < FIELD_HEIGHT; y = y + 1) {
        for (x = 0; x < FIELD_WIDTH; x = x + 1) {
            ch = get_cell_char(x, y, ball_x, ball_y, p1_y, p2_y);
            mvaddch(y, x, ch);
        }
    }
}

// Печатает счёт и подсказку по управлению под полем.
void draw_score(int score1, int score2) {
    mvprintw(FIELD_HEIGHT, 0, "Score: %d : %d   [A/Z - player 1, K/M - player 2, Q - quit]",
             score1, score2);
}

// Печатает поздравление победителю и ждёт нажатия любой клавиши.
void draw_winner(int winner) {
    erase();
    if (winner == 1) {
        mvprintw(12, 30, "PLAYER 1 WINS!");
    } else if (winner == 2) {
        mvprintw(12, 30, "PLAYER 2 WINS!");
    }
    mvprintw(14, 28, "Press any key to exit");
    refresh();
    timeout(-1);
    getch();
}

// Один полный кадр: стираем экран, рисуем поле, счёт, выводим на терминал.
void render(int ball_x, int ball_y, int p1_y, int p2_y, int score1, int score2) {
    erase();
    draw_frame(ball_x, ball_y, p1_y, p2_y);
    draw_score(score1, score2);
    refresh();
}

// ============================================================================
//                              ЛОГИКА ИГРЫ
// ============================================================================

// Неблокирующе читает один символ из ncurses и возвращает команду.
// Если в очереди ничего нет — getch() вернёт ERR, а мы вернём INPUT_NONE.
int read_input(void) {
    int result = INPUT_NONE;
    int c = getch();
    if (c == 'a' || c == 'A') {
        result = INPUT_P1_UP;
    } else if (c == 'z' || c == 'Z') {
        result = INPUT_P1_DN;
    } else if (c == 'k' || c == 'K') {
        result = INPUT_P2_UP;
    } else if (c == 'm' || c == 'M') {
        result = INPUT_P2_DN;
    } else if (c == 'q' || c == 'Q') {
        result = INPUT_QUIT;
    }
    return result;
}

// Двигает левую ракетку вверх/вниз, не даёт ей вылезти за рамку.
int update_paddle1(int input, int p1_y) {
    int result = p1_y;
    if (input == INPUT_P1_UP && p1_y > 1) {
        result = p1_y - 1;
    } else if (input == INPUT_P1_DN && p1_y + PADDLE_SIZE < FIELD_HEIGHT - 1) {
        result = p1_y + 1;
    }
    return result;
}

// Двигает правую ракетку. Те же ограничения, что и для левой.
int update_paddle2(int input, int p2_y) {
    int result = p2_y;
    if (input == INPUT_P2_UP && p2_y > 1) {
        result = p2_y - 1;
    } else if (input == INPUT_P2_DN && p2_y + PADDLE_SIZE < FIELD_HEIGHT - 1) {
        result = p2_y + 1;
    }
    return result;
}

// Возвращает следующее положение мяча по X.
int next_ball_x(int ball_x, int ball_dx) {
    int result = ball_x + ball_dx;
    return result;
}

// Возвращает следующее положение мяча по Y.
int next_ball_y(int ball_y, int ball_dy) {
    int result = ball_y + ball_dy;
    return result;
}

// Меняет знак ball_dx при столкновении с ракеткой.
// Проверка ДО входа в столбец ракетки — мяч не рисуется поверх ракетки.
int bounce_dx(int ball_x, int ball_y, int ball_dx, int p1_y, int p2_y) {
    int result = ball_dx;
    int hit_p1 = (ball_x == PADDLE1_X + 1) && (ball_dx == -1) && (ball_y >= p1_y) &&
                 (ball_y < p1_y + PADDLE_SIZE);
    int hit_p2 = (ball_x == PADDLE2_X - 1) && (ball_dx == 1) && (ball_y >= p2_y) &&
                 (ball_y < p2_y + PADDLE_SIZE);
    if (hit_p1 || hit_p2) {
        result = -ball_dx;
    }
    return result;
}

// Меняет знак ball_dy, если мяч у верхней или нижней границы и движется к ней.
int bounce_dy(int ball_y, int ball_dy) {
    int result = ball_dy;
    int hit_top = (ball_y == 1) && (ball_dy == -1);
    int hit_bottom = (ball_y == FIELD_HEIGHT - 2) && (ball_dy == 1);
    if (hit_top || hit_bottom) {
        result = -ball_dy;
    }
    return result;
}

// Определяет, был ли гол:
//   1 — очко игроку 1, 2 — очко игроку 2, 0 — гола нет.
int check_goal(int ball_x) {
    int result = 0;
    if (ball_x >= FIELD_WIDTH - 1) {
        result = 1;
    } else if (ball_x <= 0) {
        result = 2;
    }
    return result;
}

// Определяет победителя по счёту: 1 или 2, либо 0 если игра продолжается.
int check_winner(int score1, int score2) {
    int result = 0;
    if (score1 >= WIN_SCORE) {
        result = 1;
    } else if (score2 >= WIN_SCORE) {
        result = 2;
    }
    return result;
}

// Обновляет флаг выхода, если была команда Q.
int check_quit(int input, int quit) {
    int result = quit;
    if (input == INPUT_QUIT) {
        result = 1;
    }
    return result;
}
