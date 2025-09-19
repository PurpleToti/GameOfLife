#include "raylib.h"

#include <stdlib.h>
#include <math.h>
#include <omp.h>

#pragma region Cellular Automata

#define CELLULAR_AUTOMATA_WIDTH 1000
#define CELLULAR_AUTOMATA_HEIGHT 1000
#define CELLULAR_AUTOMATA_NUMBER_CELLS (CELLULAR_AUTOMATA_WIDTH * CELLULAR_AUTOMATA_HEIGHT)

#define CELLULAR_AUTOMATA_NUM_NEIGHBORS 8

#define DEAD_CELL 0
#define ALIVE_CELL 1

#define BITS_PER_INT (sizeof(int) * 8)
#define CELLULAR_AUTOMATA_SIZE (CELLULAR_AUTOMATA_NUMBER_CELLS + BITS_PER_INT - 1) / BITS_PER_INT

const int NEIGHBORS_OFFSET[8][2] = { {-1, 0}, {1, 0}, {0, -1}, {0, 1}, {-1, 1}, {1, -1}, {-1, -1}, {1, 1} };
const Color CELL_STATE_COLOR[2] = { {0, 0, 0, 255}, {255, 255, 255, 255} };
const unsigned int CELL_STATE_SWAP[2] = { ALIVE_CELL, DEAD_CELL };

unsigned int cells_buffers[2][CELLULAR_AUTOMATA_SIZE];
int current_buffer = 0;

#define ACTIVE_CELLS cells_buffers[current_buffer]
#define FUTURE_CELLS cells_buffers[current_buffer ^ 1]

inline void swap_buffer() {
	current_buffer ^= 1;
}

static inline int get_cell_index(int x, int y) {
	x = (x % CELLULAR_AUTOMATA_WIDTH + CELLULAR_AUTOMATA_WIDTH) % CELLULAR_AUTOMATA_WIDTH;
	y = (y % CELLULAR_AUTOMATA_HEIGHT + CELLULAR_AUTOMATA_HEIGHT) % CELLULAR_AUTOMATA_HEIGHT;
	return y * CELLULAR_AUTOMATA_WIDTH + x;
}

static inline int get_bit_index(int cell_index) {
	return cell_index / BITS_PER_INT;
}

static inline int get_bit_position(int cell_index) {
	return cell_index % BITS_PER_INT;
}

static inline int get_bit_mask(int cell_index) {
	return 1 << (get_bit_position(cell_index));
}

static inline int get_cell_state(int cell_index) {
	return (ACTIVE_CELLS[get_bit_index(cell_index)] & get_bit_mask(cell_index)) != 0;
}

static inline void set_cell_alive(int cell_index) {
	ACTIVE_CELLS[get_bit_index(cell_index)] |= get_bit_mask(cell_index);
}

static inline void set_cell_dead(int cell_index) {
	ACTIVE_CELLS[get_bit_index(cell_index)] &= ~get_bit_mask(cell_index);
}

static inline void set_cell_state(int cell_index, int alive) {
	unsigned int mask = get_bit_mask(cell_index);
	unsigned int* cell = &ACTIVE_CELLS[get_bit_index(cell_index)];
	*cell = (*cell & ~mask) | (-alive & mask);
}

static inline void set_future_cell_state(int cell_index, int alive) {
	unsigned int mask = get_bit_mask(cell_index);
	unsigned int* cell = &FUTURE_CELLS[get_bit_index(cell_index)];
	*cell = (*cell & ~mask) | (-alive & mask);
}

void cellular_automata_initalize() {
	for (int i = 0; i < CELLULAR_AUTOMATA_SIZE; i++) {
		ACTIVE_CELLS[i] = DEAD_CELL;
	}
}

#pragma region Neighbors wrapping optimisations

static inline int wrap_x(int x) {
	if (x < 0) return x + CELLULAR_AUTOMATA_WIDTH;
	if (x >= CELLULAR_AUTOMATA_WIDTH) return x - CELLULAR_AUTOMATA_WIDTH;
	return x;
}

static inline int wrap_y(int y) {
	if (y < 0) return y + CELLULAR_AUTOMATA_HEIGHT;
	if (y >= CELLULAR_AUTOMATA_HEIGHT) return y - CELLULAR_AUTOMATA_HEIGHT;
	return y;
}

static inline int get_cell_index_fast(int x, int y) {
	return wrap_y(y) * CELLULAR_AUTOMATA_WIDTH + wrap_x(x);
}

#pragma endregion

int count_alive_neighboring_cells(unsigned int x, unsigned int y) {
	int sum = 0;
	for (int i = 0; i < CELLULAR_AUTOMATA_NUM_NEIGHBORS; i++) {
		sum += get_cell_state(get_cell_index_fast(x + NEIGHBORS_OFFSET[i][0], y + NEIGHBORS_OFFSET[i][1]));
	}
	return sum;
}


void update_cell(unsigned int x, unsigned int y) {
	int idx = get_cell_index(x, y);
	int S = count_alive_neighboring_cells(x, y);
	int E = get_cell_state(idx);
	set_future_cell_state(idx, (S == 3) || (E == 1 && S == 2));
}

void cellular_automata_update() {
	for (unsigned int y = 0; y < CELLULAR_AUTOMATA_HEIGHT; y++) {
		for (unsigned int x = 0; x < CELLULAR_AUTOMATA_WIDTH; x++) {
			update_cell(x, y);
		}
	}

	swap_buffer();
}

#define CELL_SIZE 10
#define DISPLAYED_CELLS_WIDTH 100
#define DISPLAYED_CELLS_HEIGHT 100

#define SCREEN_WIDTH (CELL_SIZE * DISPLAYED_CELLS_WIDTH)
#define SCREEN_HEIGHT (CELL_SIZE * DISPLAYED_CELLS_HEIGHT)

// Some easy parallelization with omp
void cellular_automata_draw() {
	#pragma omp parallel for
	for (unsigned int y = 0; y < DISPLAYED_CELLS_HEIGHT; y++) {
		for (unsigned int x = 0; x < DISPLAYED_CELLS_WIDTH; x++) {
			DrawRectangle(
				x * CELL_SIZE,
				y * CELL_SIZE,
				CELL_SIZE,
				CELL_SIZE,
				CELL_STATE_COLOR[get_cell_state(get_cell_index(x, y))]
			);
		}
	}
}

#pragma endregion

// Setups


void setup_oscillator() {
	set_cell_alive(get_cell_index(4, 5));
	set_cell_alive(get_cell_index(5, 5));
	set_cell_alive(get_cell_index(6, 5));
}

void setup_ship() {
	set_cell_alive(get_cell_index(11, 10));
	set_cell_alive(get_cell_index(12, 11));
	set_cell_alive(get_cell_index(10, 12));
	set_cell_alive(get_cell_index(11, 12));
	set_cell_alive(get_cell_index(12, 12));
}


// Main Loop

int main ()
{
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
	InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Cellular Automata");

	cellular_automata_initalize();
	
	setup_oscillator();
	setup_ship();
	
	while (!WindowShouldClose())
	{
		BeginDrawing();
		ClearBackground(BLACK);


		cellular_automata_update();
		cellular_automata_draw();
		// _sleep(250);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
