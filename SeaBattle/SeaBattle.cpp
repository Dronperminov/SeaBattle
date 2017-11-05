#include "stdafx.h"
#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int red = FOREGROUND_RED;
const int green = FOREGROUND_GREEN;
const int blue = FOREGROUND_BLUE;

const int black = 0;
const int yellow = red + green;
const int cyan = green + blue;
const int white = red + green + blue;

const int menu_color = yellow; // цвет меню
const int menu_active_item_color = white + BACKGROUND_RED + BACKGROUND_GREEN + FOREGROUND_INTENSITY; // цвет активного пункта меню
const int menu_items_color = white; // цвет пунктов меню
const int menu_text_color = white; // цвет текста в меню

const int info_color = green; // цвет информации
const int scores_color = red; // цвет информации об очках

const int user_won_color = green;
const int pc_won_color = red;

const int ship_color = yellow; // цвет кораблей
const int target_color = red; // цвет цели
const int boom_color = red; // цвет взрыва
const int died_color = black; // цвет убитого корабля
const int fail_color = cyan; // цвет промаха

const int w = 80; // ширина игровой области
const int h = 15; // высота игровой области

const int x0 = 3; // смещение от левого края
const int y0 = 3; // смещение сверху
const int margin = 14; // смещение правого поля
const int text_x = 27; // смещение правого текста

const int start_state = 1; // начальное положение кораблей (1 - горизонтальное, 0 - вертикальное)

const char wall = '#'; // символ стены для меню
const char empty = ' '; // символ пустой клетки на поле
const char ship = '#'; // символ корабля
const char target = 'x'; // символ цели
const char boom = '@'; // символ попадания (взрыва)
const char fail = '-'; // символ промаха

const char top_field_char = '_'; // символ верхней рамки поля
const char bottom_field_char = '~'; // символ нижней рамки поля
const char left_field_char = '|'; // символ левой рамки поля
const char right_field_char = '|'; // символ правой рамки поля

const int key_R = 82;
const int key_S = 83;
const int key_X = 88;

struct pointT {
	char c;
	int color;
};

struct shipsT {
	int x; // координата Х корабля
	int y; // координата У корабля
	int state; // положение корабля на поле (вертикально (0) / горизонтально (1))

	int length; // длина корабля
	int count; // число сбитых клеток
};

pointT field[h][w]; // основное игорове поле

pointT game_field[4][10][10]; // 4 поля игроков (2 пользовательских и 2 компьютерных)
shipsT ships[2][10];

int counters[2] = { 20, 20 }; // счётчик очков пользователя и компьютера
int ships_index[2] = { 0, 0 };

// проверка возможности расположить корабль длиной length в положении state в точку (x, y)
int can_set_ship(int x, int y, int length, int state, int n) {
	if (x < 0 || x > 9 || y < 0 || y > 9)
		return 0;

	if (!state && (y + length > 10))
		return 0;

	if (state && (x + length > 10))
		return 0;

	int res = 0;

	for (int i = 0; i < length; i++) {
		res += game_field[n][y + (!state) * i][x + state * i].c != empty; // проверка на свободную полосу для корабля

																		  // проверка на границу, длинной в корабль, в зависимости от положения state
		if (state) {
			if (y > 0)
				res += game_field[n][y - 1][x + i].c != empty;
			if (y < 9)
				res += game_field[n][y + 1][x + i].c != empty;
		}
		else {
			if (x > 0)
				res += game_field[n][y + i][x - 1].c != empty;
			if (x < 9)
				res += game_field[n][y + i][x + 1].c != empty;
		}
	}

	// проверка границ с торцов корабля
	if (state) {
		if (x > 0) {
			res += game_field[n][y][x - 1].c != empty;

			if (y > 0)
				res += game_field[n][y - 1][x - 1].c != empty;
			if (y < 9)
				res += game_field[n][y + 1][x - 1].c != empty;
		}

		if (x + length - 1 < 9) {
			res += game_field[n][y][x + length].c != empty;

			if (y > 0)
				res += game_field[n][y - 1][x + length].c != empty;
			if (y < 9)
				res += game_field[n][y + 1][x + length].c != empty;
		}
	}
	else {
		if (y > 0) {
			res += game_field[n][y - 1][x].c != empty;

			if (x > 0)
				res += game_field[n][y - 1][x - 1].c != empty;
			if (x < 9)
				res += game_field[n][y - 1][x + 1].c != empty;
		}

		if (y < 9) {
			res += game_field[n][y + length][x].c != empty;

			if (x > 0)
				res += game_field[n][y + length][x - 1].c != empty;
			if (x < 9)
				res += game_field[n][y + length][x + 1].c != empty;
		}
	}

	return !res;
}

// проверка убит ли попаданием в точку (x, y) корабль и обновление информации о текущих попаданиях
int update_ships(int x, int y, int user) {
	int ship = -1;

	for (int i = 0; i < 10 && ship == -1; i++) {
		int ship_length = ships[user][i].length;
		int ship_x = ships[user][i].x;
		int ship_y = ships[user][i].y;
		int ship_state = ships[user][i].state;

		for (int j = 0; j < ship_length && ship == -1; j++) {
			if ((ship_x + j == x && ship_y == y && ship_state) || (ship_y + j == y && ship_x == x && !ship_state)) {
				// нашли нужный корабль
				ships[user][i].count++;
				ship = i;
			}
		}
	}

	return ships[user][ship].count == ships[user][ship].length ? ship : -1;
}

// расстановка случайным образом корабля длиной length на поле
void draw_ship(int length) {
	int state = rand() % 2; // положение - вертикальное (0) или горизонтальное (1)
	int x, y;

	do {
		x = rand() % (10 - length * state);
		y = rand() % (10 - length * !state);
	} while (!can_set_ship(x, y, length, state, 2));

	ships[1][ships_index[1]].x = x;
	ships[1][ships_index[1]].y = y;
	ships[1][ships_index[1]].state = state;
	ships[1][ships_index[1]].length = length;
	ships[1][ships_index[1]].count = 0;
	ships_index[1]++;

	for (int i = 0; i < length; i++)
		game_field[2][y + (!state) * i][x + state * i].c = ship;
}

// расстановка случайным образом кораблей противника
void draw_ships() {
	ships_index[1] = 0;

	// один четырёхпалубный
	draw_ship(4);
	// два трёхпалубных
	draw_ship(3); draw_ship(3);
	// три двухпалубных
	draw_ship(2); draw_ship(2);	draw_ship(2);
	// четыре однопалубных
	draw_ship(1); draw_ship(1); draw_ship(1); draw_ship(1);
}

// отрисовка границ полей и координат левого поля
void draw_left_field() {
	for (int i = 0; i < 10; i++) {
		// горизонтальные обозначения
		field[y0 - 2][x0 + i].c = 'A' + i;
		// вертикальные обозначения
		field[y0 + i][x0 - 2].c = '0' + i;

		// рамки поля
		field[y0 - 1][x0 + i].c = top_field_char;
		field[y0 + 10][x0 + i].c = bottom_field_char;
		field[y0 + i][x0 - 1].c = left_field_char;
		field[y0 + i][x0 + 10].c = right_field_char;
	}

	for (int i = y0 - 2; i < y0 + 10 + 1; i++) {
		for (int j = x0 - 2; j < x0 + 10 + 1; j++) {
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)j, (short)i });
			printf("%c", field[i][j].c);
		}
	}
}

// отрисовка границ полей и координат правого поля
void draw_right_field() {
	for (int i = 0; i < 10; i++) {
		// горизонтальные обозначения
		field[y0 - 2][x0 + i + margin].c = 'A' + i;
		// вертикальные обозначения
		field[y0 + i][x0 + margin - 2].c = '0' + i;

		// рамки поля
		field[y0 - 1][x0 + i + margin].c = top_field_char;
		field[y0 + 10][x0 + i + margin].c = bottom_field_char;
		field[y0 + i][x0 - 1 + margin].c = left_field_char;
		field[y0 + i][x0 + 10 + margin].c = right_field_char;
	}

	for (int i = y0 - 2; i < y0 + 10 + 1; i++) {
		for (int j = x0 + margin - 2; j < x0 + 10 + margin + 1; j++) {
			SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)j, (short)i });
			printf("%c", field[i][j].c);
		}
	}
}

// отрисовка рамки меню
void draw_wall() {
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(handle, menu_color);

	SetConsoleCursorPosition(handle, { x0, (short)(y0 - 1) });
	for (int i = x0; i < w - x0; i++)
		printf("%c", wall);

	SetConsoleCursorPosition(handle, { x0, (short)(y0 + 9) });
	for (int i = x0; i < w - x0; i++)
		printf("%c", wall);

	for (int i = y0; i < y0 + 9; i++) {
		SetConsoleCursorPosition(handle, { (short)(x0), (short)i });
		printf("%c", wall);
		SetConsoleCursorPosition(handle, { (short)(w - x0 - 1), (short)i });
		printf("%c", wall);
	}
}

// отриосвка текса с параметрами цветом color в точке (x, y)
void text_xy(int x, int y, int color, const char *text, ...) {
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);

	SetConsoleTextAttribute(handle, color); // устанавливаем нужный цвет
	SetConsoleCursorPosition(handle, { (short)(x), (short)(y) }); // перемещаемся в нужные координаты

																  // обрабатываем формтаный ввод
	va_list args;
	va_start(args, text);
	vprintf(text, args);
	va_end(args);

	SetConsoleTextAttribute(handle, white); // возвращаем цвет символов по умолчанию
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { w, h }); // уводим курсор в правый нижний угол
}

// отрисовка числа очков на экран
void draw_scores() {
	text_xy(x0 + text_x, y0, scores_color, "Scores");
	text_xy(x0 + text_x, y0 + 1, scores_color, "You: %d / 20", 20 - counters[0]);
	text_xy(x0 + text_x, y0 + 2, scores_color, "PC: %d / 20", 20 - counters[1]);
}

// отриосвка корабля длиной length в положении state в точке (x, y)
void draw_ship_xy(int x, int y, int length, int state) {
	for (int i = 0; i < length; i++) {
		game_field[0][y + !state * i][x + state * i].c = ship;
		game_field[0][y + !state * i][x + state * i].color = ship_color;
	}
}

// вращение корабля длиной length в состоянии state в точке (x, y)
void rotate_ship_xy(int x, int y, int length, int *state) {
	if (can_set_ship(x, y, length, !*state, 0)) {
		*state = !*state; // меняем состояние

		for (int i = 0; i < length; i++) {
			game_field[0][y + !*state * i][x + *state * i].c = ship;
			game_field[0][y + !*state * i][x + *state * i].color = ship_color;
		}
	}
}

// удаление корабля длиной length в состоянии state из точки (x, y)
void clear_ship_xy(int x, int y, int length, int state) {
	for (int i = 0; i < length; i++) {
		game_field[0][y + !state * i][x + state * i].c = empty;
		game_field[0][y + !state * i][x + state * i].color = white;
	}
}

// обновление данных на экране (перерисовка игровых полей на экран)
void update() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			// если в левом поле есть имзенения в символе
			if (field[y0 + i][x0 + j].c != game_field[0][i][j].c) {
				text_xy(x0 + j, y0 + i, game_field[0][i][j].color | BACKGROUND_BLUE | FOREGROUND_INTENSITY, "%c", game_field[0][i][j].c);
				field[y0 + i][x0 + j].c = game_field[0][i][j].c;
				field[y0 + i][x0 + j].color = game_field[0][i][j].color;
			}

			// если в правом поле есть изменения и оно отображается
			if ((field[y0 + i][x0 + j + margin].c != game_field[1][i][j].c) && field[y0 - 2][x0 + margin].c == 'A') {

				text_xy(x0 + j + margin, y0 + i, game_field[1][i][j].color | BACKGROUND_BLUE | FOREGROUND_INTENSITY, "%c", game_field[1][i][j].c);
				field[y0 + i][x0 + j + margin].c = game_field[1][i][j].c;
				field[y0 + i][x0 + j].color = game_field[1][i][j].color;
			}
		}
	}

	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { w, h });
}

// полное обновление полей
void full_update() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			text_xy(x0 + j, y0 + i, game_field[0][i][j].color | BACKGROUND_BLUE | FOREGROUND_INTENSITY, "%c", game_field[0][i][j].c);
			field[y0 + i][x0 + j].c = game_field[0][i][j].c;
			field[y0 + i][x0 + j].color = game_field[0][i][j].color;

			text_xy(x0 + j + margin, y0 + i, game_field[1][i][j].color | BACKGROUND_BLUE | FOREGROUND_INTENSITY, "%c", game_field[1][i][j].c);
			field[y0 + i][x0 + j].c = game_field[1][i][j].c;
			field[y0 + i][x0 + j].color = game_field[1][i][j].color;
		}
	}

	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { w, h });
}

// инициализация игры
void init() {
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			field[i][j].c = '\0';
			field[i][j].color = 0;
		}
	}

	// очистка полей с игрой
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			for (int f = 0; f < 4; f++) {
				game_field[f][j][i].c = empty;
				game_field[f][i][j].color = white;
			}
		}
	}

	draw_left_field(); // отрисовка левого поля
	draw_ships(); // рандомная расстановка кораблей противника
	update(); // отрисовка на экран
}

// пользовательская расстановка кораблей
int align_ship(int length, int state) {
	int x, y;
	int flag = 0;

	for (int i = 0; i < 10 && !flag; i++)
		for (int j = 0; j < 10 && !flag; j++)
			if (can_set_ship(j, i, length, state, 0)) {
				x = j;
				y = i;

				flag = 1;
			}

	draw_ship_xy(x, y, length, state);

	update();

	int cancel = 0;

	while (!GetAsyncKeyState(VK_SPACE) && !(cancel = GetAsyncKeyState(key_X))) {
		clear_ship_xy(x, y, length, state);

		if (GetAsyncKeyState(VK_LEFT)) {
			if (can_set_ship(x - 1, y, length, state, 0))
				x--;
			else {
				int x0 = x - 2;
				while (x0 > 0 && !can_set_ship(x0, y, length, state, 0))
					x0--;

				if (x0 > 0)
					x = x0;
			}

			while (GetAsyncKeyState(VK_LEFT))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_RIGHT)) {
			if (can_set_ship(x + 1, y, length, state, 0))
				x++;
			else {
				int x0 = x + 2;
				while (x0 < 10 && !can_set_ship(x0, y, length, state, 0))
					x0++;

				if (x0 < 10)
					x = x0;
			}

			while (GetAsyncKeyState(VK_RIGHT))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_UP)) {
			if (can_set_ship(x, y - 1, length, state, 0))
				y--;
			else {
				int y0 = y - 2;
				while (y0 > 0 && !can_set_ship(x, y0, length, state, 0))
					y0--;

				if (y0 > 0)
					y = y0;
			}

			while (GetAsyncKeyState(VK_UP))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_DOWN)) {
			if (can_set_ship(x, y + 1, length, state, 0))
				y++;
			else {
				int y0 = y + 2;
				while (y0 < 10 && !can_set_ship(x, y0, length, state, 0))
					y0++;

				if (y0 < 10)
					y = y0;
			}

			while (GetAsyncKeyState(VK_DOWN))
				Sleep(20);
		}

		if (GetAsyncKeyState(key_R)) {
			rotate_ship_xy(x, y, length, &state);

			while (GetAsyncKeyState(key_R))
				Sleep(20);
		}

		draw_ship_xy(x, y, length, state);
		update();

		Sleep(100);
	}

	while (GetAsyncKeyState(VK_SPACE))
		Sleep(20);

	Sleep(100);

	// заполняем информацию о корабле
	ships[0][ships_index[0]].x = x;
	ships[0][ships_index[0]].y = y;
	ships[0][ships_index[0]].state = state;
	ships[0][ships_index[0]].length = length;
	ships[0][ships_index[0]].count = 0;
	ships_index[0]++;

	return cancel;
}

// пользовательская расстановка кораблей
int align_ships() {
	ships_index[0] = 0; // сбрасываем индекс своих кораблей

	text_xy(x0 + margin, y0 + 6, info_color, "For rotate ship press 'R' key");
	text_xy(x0 + margin, y0 + 7, info_color, "For move ship use arrow keys");
	text_xy(x0 + margin, y0 + 8, info_color, "For end with ship press 'space'");
	text_xy(x0 + margin, y0 + 9, info_color, "For cancel press 'X' key");

	while (GetAsyncKeyState(VK_SPACE))
		Sleep(100);

	if (align_ship(4, start_state))
		return 0;

	if (align_ship(3, start_state))
		return 0;
	if (align_ship(3, start_state))
		return 0;

	if (align_ship(2, start_state))
		return 0;
	if (align_ship(2, start_state))
		return 0;
	if (align_ship(2, start_state))
		return 0;

	if (align_ship(1, start_state))
		return 0;
	if (align_ship(1, start_state))
		return 0;
	if (align_ship(1, start_state))
		return 0;
	if (align_ship(1, start_state))
		return 0;

	text_xy(x0 + margin, y0, white, "Start the game? (Press 'S' for continue or 'R' for reset ships");

	int enter = 0;

	while (!(enter = GetAsyncKeyState(key_S)) && !GetAsyncKeyState(key_R))
		Sleep(50);

	while (GetAsyncKeyState(key_R))
		Sleep(100);

	// стираем инструкцию по размещению кораблей
	for (int i = y0; i < y0 + 10; i++) {
		SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), { (short)(x0 + margin), (short)i });
		for (int j = 0; j < 65; j++)
			printf(" ");
	}

	if (!enter) {
		for (int i = 0; i < 10; i++) {
			for (int j = 0; j < 10; j++) {
				game_field[0][i][j].c = empty;
				game_field[0][i][j].color = white;
			}
		}

		update();

		align_ships();
	}

	return 1;
}

void find_empty_point_right(int start_x, int start_y, int *x, int *y) {
	for (int j = start_x; j < 10; j++)
		if (game_field[1][start_y][j].c == empty) {
			*x = j;
			*y = start_y;

			return;
		}

	for (int i = start_y + 1; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			if (game_field[1][i][j].c == empty) {
				*x = j;
				*y = i;

				return;
			}
		}
	}

	for (int i = 0; i < start_y; i++) {
		for (int j = 0; j < 10; j++) {
			if (game_field[1][i][j].c == empty) {

				*x = j;
				*y = i;

				return;
			}
		}
	}
}

void find_empty_point_left(int start_x, int start_y, int *x, int *y) {
	for (int j = start_x; j >= 0; j--)
		if (game_field[1][start_y][j].c == empty) {
			*x = j;
			*y = start_y;

			return;
		}

	for (int i = start_y - 1; i >= 0; i--) {
		for (int j = 9; j >= 0; j--) {
			if (game_field[1][i][j].c == empty) {
				*x = j;
				*y = i;

				return;
			}
		}
	}

	for (int i = 9; i > start_y; i--) {
		for (int j = 9; j >= 0; j--) {
			if (game_field[1][i][j].c == empty) {

				*x = j;
				*y = i;

				return;
			}
		}
	}
}

void find_empty_point_up(int start_x, int start_y, int *x, int *y) {
	for (int i = start_y; i >= 0; i--)
		if (game_field[1][i][start_x].c == empty) {
			*x = start_x;
			*y = i;

			return;
		}

	for (int j = start_x - 1; j >= 0; j--) {
		for (int i = 9; i >= 0; i--) {
			if (game_field[1][i][j].c == empty) {
				*x = j;
				*y = i;

				return;
			}
		}
	}

	for (int j = 9; j > start_x; j--) {
		for (int i = 9; i >= 0; i--) {
			if (game_field[1][i][j].c == empty) {

				*x = j;
				*y = i;

				return;
			}
		}
	}
}

void find_empty_point_down(int start_x, int start_y, int *x, int *y) {
	for (int i = start_y; i < 10; i++)
		if (game_field[1][i][start_x].c == empty) {
			*x = start_x;
			*y = i;

			return;
		}

	for (int j = start_x + 1; j < 10; j++) {
		for (int i = 0; i < 10; i++) {
			if (game_field[1][i][j].c == empty) {
				*x = j;
				*y = i;

				return;
			}
		}
	}

	for (int j = 0; j < start_x; j++) {
		for (int i = 0; i < 10; i++) {
			if (game_field[1][i][j].c == empty) {

				*x = j;
				*y = i;

				return;
			}
		}
	}
}

void game();

int menu() {
	system("cls");

	draw_wall();

	int item = 0;

	while (!GetAsyncKeyState(VK_RETURN)) {
		if (GetAsyncKeyState(VK_DOWN)) {
			item = (item + 1) % 3;

			while (GetAsyncKeyState(VK_DOWN))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_UP)) {
			item = item > 0 ? item - 1 : 2;

			while (GetAsyncKeyState(VK_UP))
				Sleep(20);
		}

		text_xy((w - 20) / 2, y0 + 1, menu_color, "Welcome to SeaBattle");
		text_xy((w - 12) / 2, y0 + 3, item == 0 ? menu_active_item_color : menu_items_color, " Start game ");
		text_xy((w - 12) / 2, y0 + 5, item == 1 ? menu_active_item_color : menu_items_color, " About game ");
		text_xy((w - 6) / 2, y0 + 7, item == 2 ? menu_active_item_color : menu_items_color, " Exit ");

		Sleep(100);
	}

	switch (item) {
	case 0:
		game();
		return 1;

	case 1:
		system("cls");
		draw_wall();

		text_xy((w - 66) / 2, y0 + 2, menu_text_color, "SeaBattle - is a game for all people who loved sea battle game! :)");
		text_xy((w - 58) / 2, y0 + 4, menu_text_color, "For rotate ship use 'R' button, for move ship - arrow keys");
		text_xy((w - 41) / 2, y0 + 6, menu_text_color, "Developed by Dronperminov in August 2017");

		Sleep(7000);

		return menu();

	case 2:
	default:
		return 0;
	}
}

// пользовательский ход
int set_target(int *target_x, int *target_y) {
	if (counters[0] == 0)
		return 0;

	int x, y;

	find_empty_point_right(*target_x + 1, *target_y, &x, &y); // ищем первую свободную точку справа от текущих координат

															  // отрисовываем "цель" в найденной точке
	game_field[1][y][x].c = target;
	game_field[1][y][x].color = target_color;

	update();

	int cancel = 0;

	// обрабатываем перемещения цели до тех пор, пока не произойдёт выстрел или не нажмут выход
	while (!GetAsyncKeyState(VK_RETURN) && !(cancel = GetAsyncKeyState(key_X))) {
		// стираем крестик
		game_field[1][y][x].c = empty;
		game_field[1][y][x].color = white;

		if (GetAsyncKeyState(VK_LEFT)) {
			find_empty_point_left(x - 1, y, &x, &y);

			while (GetAsyncKeyState(VK_LEFT))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_RIGHT)) {
			find_empty_point_right(x + 1, y, &x, &y);

			while (GetAsyncKeyState(VK_RIGHT))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_DOWN)) {
			find_empty_point_down(x, y + 1, &x, &y);

			while (GetAsyncKeyState(VK_DOWN))
				Sleep(20);
		}

		if (GetAsyncKeyState(VK_UP)) {
			find_empty_point_up(x, y - 1, &x, &y);

			while (GetAsyncKeyState(VK_UP))
				Sleep(20);
		}

		// рисуем крестик в новых координатах
		game_field[1][y][x].c = target;
		game_field[1][y][x].color = target_color;

		update();
		Sleep(50);
	}

	if (cancel)
		return 1;

	while (GetAsyncKeyState(VK_RETURN))
		Sleep(20);

	*target_x = x;
	*target_y = y;

	// если попали в цель
	if (game_field[2][y][x].c == ship) {
		game_field[1][y][x].c = boom;
		game_field[1][y][x].color = boom_color;

		counters[0]--; // уменьшаем число несбитых клеточек

		draw_scores(); // перерисовываем очки
		update();

		int ship = update_ships(x, y, 1); // проверяем, не убит ли корабль

										  // если убит
		if (ship != -1) {
			int ship_x = ships[1][ship].x;
			int ship_y = ships[1][ship].y;
			int ship_state = ships[1][ship].state;
			int ship_length = ships[1][ship].length;

			// закрашиваем его в серый цвет
			for (int i = 0; i < ship_length; i++)
				game_field[1][ship_y + !ship_state * i][ship_x + ship_state * i].color = died_color;

			full_update();
		}

		set_target(target_x, target_y); // повторяем ход из текущих координат
	}
	else {
		// если не попали, то просто рисуем символ промаха
		game_field[1][y][x].c = fail;
		game_field[1][y][x].color = fail_color;

		update();
	}

	Sleep(100);

	return 0;
}

// компьютерный ход
void computer(int *pc_x, int *pc_y, int *was) {
	// если БОЙ окончен, то смысл идти дальше?)
	if (counters[1] * counters[0] == 0)
		return;

	int x, y; // координаты точки выстрела

			  // если было попадание
	if (*was) {
		// в приоритете бить по 4 точкам вокруг точки попадания: (x, y - 1), (x, y + 1), (x - 1, y), (x + 1, y)
		int x_p[4] = { *pc_x, *pc_x, *pc_x - 1, *pc_x + 1 };
		int y_p[4] = { *pc_y - 1, *pc_y + 1, *pc_y, *pc_y };
		int variants[4] = { -1, -1, -1, -1 };
		int count = 0;

		do {
			int variant = rand() % 4; // определяем случайную точку

									  // проверяем её наличие в списке проверенных
			int i = 0;
			while (i < count && variants[i] != variant)
				i++;

			if (i == count)
				variants[count++] = variant; // если не оказалось, то заносим в список
			else
				continue; // если оказалась, переходим к новой итерации

						  // присваиваем координатам полученную точку
			x = x_p[variant];
			y = y_p[variant];
		} while ((x < 0 || x > 9 || y < 0 || y > 10 || game_field[3][y][x].c != empty) && count < 4);

		// если точек вокруг не оказалось, то ищем рандомную точку
		if (count == 4) {
			do {
				x = rand() % 10;
				y = rand() % 10;
			} while (game_field[3][y][x].c != empty);
		}
	}
	else {
		// рандомный поиск пустой клетки на поле компютера
		do {
			x = rand() % 10;
			y = rand() % 10;
		} while (game_field[3][y][x].c != empty);
	}

	*pc_x = x;
	*pc_y = y;

	// если выстрел попал
	if (game_field[0][y][x].c == ship) {
		game_field[3][y][x].c = boom; // отмечаем на поле ПК отметку об удачном выстреле
		game_field[0][y][x].c = boom; // и на левом поле игрока
		game_field[0][y][x].color = boom_color; // окрашиваем клетку в красный

		counters[1]--; // уменьшаем счётчик несбитых кораблей
		*was = 1; // запоминаем факт попадания

		draw_scores();
		update(); // обновляем информацию на поле

		int ship = update_ships(x, y, 0);

		if (ship != -1) {
			int ship_x = ships[0][ship].x;
			int ship_y = ships[0][ship].y;
			int ship_state = ships[0][ship].state;
			int ship_length = ships[0][ship].length;

			for (int i = 0; i < ship_length; i++)
				game_field[0][ship_y + !ship_state * i][ship_x + ship_state * i].color = died_color;

			full_update();
		}

		computer(pc_x, pc_y, was); // повторяем ход компьютера с новыми параметрами
	}
	else {
		// если не попали
		game_field[3][y][x].c = fail; // отмечаем у ПК как промах
		game_field[0][y][x].c = fail; // и на левом поле игрока
		game_field[0][y][x].color = fail_color; // меняем цвет на белый

		*was = 0; // запоминаем факт промаха
	}
}

// процедура завершения игры
void game_over() {
	system("cls");

	draw_wall(); // рисуем границу

	text_xy((w - 22) / 2, y0 + 1, menu_color, "Game over | SeaBattle");

	if (counters[0] == 0)
		text_xy((w - 8) / 2, y0 + 3, user_won_color, "You won!");
	else
		text_xy((w - 7) / 2, y0 + 3, pc_won_color, "PC won!");

	text_xy((w - 45) / 2, y0 + 7, menu_text_color, "Press 'X' for go to menu or 'R' to play again");

	char buf[w];
	sprintf_s(buf, "You - %d scores, PC - %d scores", 20 - counters[0], 20 - counters[1]);
	text_xy((w - strlen(buf)) / 2, y0 + 4, menu_text_color, buf);

	int play_again = 0;

	while (!GetAsyncKeyState(key_X) && !(play_again = GetAsyncKeyState(key_R)))
		Sleep(50);

	if (play_again) {
		while (GetAsyncKeyState(key_R))
			Sleep(20);

		game();
	}
	else
		menu();
}

// основная процедура - игра
void game() {
	system("cls"); // очищаем экран

	counters[0] = counters[1] = 20;

	init(); // инициализация поля, отрисовка игрового пространства, рандомизация кораблей противника	

			// пользовательская расстановка кораблей
	if (!align_ships()) {
		menu();

		return;
	}

	while (GetAsyncKeyState(VK_RETURN))
		Sleep(500);

	draw_right_field();
	draw_scores(); // отрисовка очков

	text_xy(x0 + text_x, y0 + 7, info_color, "For move target use arrow keys");
	text_xy(x0 + text_x, y0 + 8, info_color, "For shot press 'enter'");
	text_xy(x0 + text_x, y0 + 9, info_color, "For end game and return to menu press 'X'");

	int target_x = 4, target_y = 4; // координаты цели
	int pc_x, pc_y, was = 0; // координаты цели ПК
	int quit = 0; // факт выхода из игры

	do {
		if (quit = set_target(&target_x, &target_y)) // пользовательский ход
			break;

		computer(&pc_x, &pc_y, &was); // ход компьютера
	} while (counters[0] * counters[1]);

	if (!quit)
		game_over();
	else
		menu();
}

int main() {
	system("mode con cols=100 lines=30"); // устанавливаем минимальный размер консоли
	srand(time(NULL)); // инициализируем генератор случайных чисел

	menu();

	return 0;
}