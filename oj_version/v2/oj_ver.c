#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <unistd.h> // For getpid()

// Conditions
#ifdef CONSIDER_2
#define NUM_CONSIDER 2
#pragma message("NUM_CONSIDER is 2.")
#else
#define NUM_CONSIDER 1
#pragma message("NUM_CONSIDER is 1.")
#endif

#ifdef DEBUG
#define DEBUG_MODE 1
#pragma message("DEBUG is on")
#else
#define DEBUG_MODE 0
#pragma message("DEBUG is off")
#endif

typedef struct {
    void* data; //
    size_t size; //
    size_t capacity; //
    size_t element_size; //
} Vector;

typedef struct {
    int* data;
    size_t length;
} IntList;

typedef struct {
    int width;
    int height;
} Size; //

typedef struct {
    int x;
    int y;
} Pos; // Position

typedef struct {
    int label; //
    Size size; //
    Pos* occupied; //
    int occupied_count; //
} BlockRotation; //

typedef struct {
    char name; //
    BlockRotation* rotations; //
    int rotations_count; //
} Block; //

typedef struct {
    int x_offset; // x
    BlockRotation* rotation; //
    int y_offset; // y +infinity
    double assement_score; //  -infinity
} BlockStatus; //

typedef struct {
    int length; //
    double* weights; //  length+1
} AssessmentModel; //

typedef struct {
    int* awards; //  1, 2, 3, 4
    Block* available_blocks; //
    int available_blocks_count; //
} GameConfig; //

typedef struct {
    Size size; //  1
    short** grid; // 1 0  5
} Board; //

typedef struct {
    GameConfig config; //

    Board board; //
    long score; //

    Block** upcoming_blocks; //  2
    BlockStatus** available_statuses_1; //
    int available_statuses_1_count; //
    BlockStatus** available_statuses_2; //
    int available_statuses_2_count; //
} Game; //

typedef struct {
    Game* game;
    AssessmentModel* model;
} Context; //

double caculateLinearFunction(double* weights, int* features, int feature_length)
{
    double result = 0;
    for (int i = 0; i < feature_length; i++) {
        result += weights[i] * features[i];
    }
    result += weights[feature_length]; //
    return result;
}

const int k_num_features = 8; //

// ----- Block Rotation consts -----

const Block k_block_I = {
    .name = 'I',
    .rotations = (BlockRotation[]) {
        { .label = 90,
            .size = { 1, 4 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 } },
            .occupied_count = 4 },
        { .label = 0,
            .size = { 4, 1 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 3, 0 } },
            .occupied_count = 4 } },
    .rotations_count = 2
};

const Block k_block_T = {
    .name = 'T',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 1, 1 } },
            .occupied_count = 4 },
        { .label = 90,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 0, 2 } },
            .occupied_count = 4 },
        { .label = 180,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 1, 0 } },
            .occupied_count = 4 },
        { .label = 270,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 1 }, { 1, 0 }, { 1, 1 }, { 1, 2 } },
            .occupied_count = 4 } },
    .rotations_count = 4
};

const Block k_block_O = {
    .name = 'O',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 2, 2 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } },
            .occupied_count = 4 } },
    .rotations_count = 1
};

const Block k_block_J = {
    .name = 'J',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 0, 1 } },
            .occupied_count = 4 },
        { .label = 90,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 2 } },
            .occupied_count = 4 },
        { .label = 180,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 2, 0 } },
            .occupied_count = 4 },
        { .label = 270,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 1, 2 } },
            .occupied_count = 4 } },
    .rotations_count = 4
};

const Block k_block_L = {
    .name = 'L',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 2, 0 }, { 2, 1 } },
            .occupied_count = 4 },
        { .label = 90,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 1, 0 } },
            .occupied_count = 4 },
        { .label = 180,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 1 }, { 1, 1 }, { 2, 1 }, { 0, 0 } },
            .occupied_count = 4 },
        { .label = 270,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 2 }, { 1, 0 }, { 1, 1 }, { 1, 2 } },
            .occupied_count = 4 } },
    .rotations_count = 4
};

const Block k_block_S = {
    .name = 'S',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 2, 1 } },
            .occupied_count = 4 },
        { .label = 90,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 1 }, { 0, 2 }, { 1, 0 }, { 1, 1 } },
            .occupied_count = 4 } },
    .rotations_count = 2
};

const Block k_block_Z = {
    .name = 'Z',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 3, 2 },
            .occupied = (Pos[]) { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 2, 0 } },
            .occupied_count = 4 },
        { .label = 90,
            .size = { 2, 3 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 1, 1 }, { 1, 2 } },
            .occupied_count = 4 } },
    .rotations_count = 2
};

const Block* const k_blocks[] = {
    &k_block_I,
    &k_block_T,
    &k_block_O,
    &k_block_J,
    &k_block_L,
    &k_block_S,
    &k_block_Z
};

const int k_blocks_count = sizeof(k_blocks) / sizeof(k_blocks[0]);

// ----------

int findYOffset(const Board* board, BlockStatus* action);

void visualizeStep(const Game* game, const BlockStatus* action);

short** GridAlloc(Size* size)
{
    int width = size->width;
    int height = size->height;
    int buffer = 5;

    // rows
    short** grid = (short**)malloc((height + buffer) * sizeof(short*));
    if (grid == NULL) {
        fprintf(stderr, "Failed to alloc grids\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < height + buffer; i++) {
        grid[i] = (short*)malloc(width * sizeof(short));
        if (grid[i] == NULL) {
            fprintf(stderr, "Failed to alloc grid row\n");
            exit(EXIT_FAILURE);
        }
        for (int j = 0; j < width; j++) {
            grid[i][j] = 0;
        }
    }

    return grid;
}

void GridFree(short** grid, Size* size)
{
    int buffer = 5; // Match the buffer used in GridAlloc
    if (grid == NULL)
        return;

    for (int i = 0; i < size->height + buffer; i++) {
        if (grid[i] != NULL) {
            free(grid[i]);
            grid[i] = NULL;
        }
    }

    free(grid);
}

IntList* IntListAlloc(int max_size)
{
    IntList* list = (IntList*)malloc(sizeof(IntList));

    if (list == NULL) {
        fprintf(stderr, "Failed to alloc IntList\n");
        exit(EXIT_FAILURE);
    }

    list->data = (int*)malloc(max_size * sizeof(int));
    if (list->data == NULL) {
        fprintf(stderr, "Failed to alloc IntList data\n");
        exit(EXIT_FAILURE);
    }

    list->length = 0;

    return list;
}

void IntListFree(IntList* list)
{
    free(list->data);
    free(list);
}

int* IntListAt(IntList* list, int index)
{
    if (index < 0 || (size_t)index >= list->length) {
        return NULL;
    }
    return &list->data[index];
}

//  max_size
size_t IntListPush(IntList* list, int value)
{
    list->data[list->length] = value;
    list->length++;
    return list->length - 1;
}

void GameSetEnd(Game* game)
{
    game->score *= -1;
}

size_t clearFullLines(Board* board, IntList* full_lines);

int calcErodedPieceCells(BlockStatus* action, IntList* full_lines)
{
    if (full_lines->length == 0) {
        return 0;
    }
    if (action->rotation == NULL) {
        return 0;
    }

    int eliminate_bricks = 0;

    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int occupied_y = action->y_offset + pos.y;

        for (size_t j = 0; j < full_lines->length; j++) {
            if (occupied_y == full_lines->data[j]) {
                eliminate_bricks++;
                break;
            }
        }
    }

    return (int)(full_lines->length) * eliminate_bricks;
}

int calcRowTransitions(const Board* board_after_elim)
{
    int transitions = 0;
    short** grid = board_after_elim->grid;
    int height = board_after_elim->size.height;
    int width = board_after_elim->size.width;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width - 1; x++) {
            int current_filled = (grid[y][x] != 0);
            int next_filled = (grid[y][x + 1] != 0);
            if (current_filled != next_filled) {
                transitions++;
            }
        }
    }
    return transitions;
}

int calcColumnTransitions(const Board* board_after_elim)
{
    int transitions = 0;
    short** grid = board_after_elim->grid;
    int height = board_after_elim->size.height;
    int width = board_after_elim->size.width;

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height - 1; y++) {
            int current_filled = (grid[y][x] != 0);
            int next_filled = (grid[y + 1][x] != 0);
            if (current_filled != next_filled) {
                transitions++;
            }
        }
    }
    return transitions;
}

void calcHolesAndDepth(Board* board_after_elim, int* holes_count, int* total_hole_depth, int* rows_with_holes_count)
{
    *holes_count = 0;
    *total_hole_depth = 0;
    *rows_with_holes_count = 0;
    int rows_with_holes[board_after_elim->size.height + 5]; // 0
    memset(rows_with_holes, 0, sizeof(rows_with_holes));

    short** grid = board_after_elim->grid;
    int height = board_after_elim->size.height;
    int width = board_after_elim->size.width;

    for (int x = 0; x < width; x++) {
        int current_depth_contribution = 0;
        int block_encountered = 0;

        for (int y = height - 1; y >= 0; y--) {
            if (grid[y][x] == 1) {
                block_encountered = 1;
                current_depth_contribution++;
            } else if (block_encountered && y < height) { // below a block AND < logical height
                (*holes_count)++;
                rows_with_holes[y] = 1;
                *total_hole_depth += current_depth_contribution;
            }
        }
    }

    for (int i = 0; i < height; i++) {
        if (rows_with_holes[i] == 1) {
            (*rows_with_holes_count)++;
        }
    }
}

int calcBoardWells(const Board* board_after_elim)
{
    int wells_sum = 0;
    short** grid = board_after_elim->grid;
    int height = board_after_elim->size.height;
    int width = board_after_elim->size.width;

    for (int x = 0; x < width; x++) {
        int current_well_depth = 0;
        for (int y = height - 1; y >= 0; y--) { // logical height
            if (grid[y][x] == 0) {
                int left_filled = (x == 0) || (grid[y][x - 1] != 0);
                int right_filled = (x == width - 1) || (grid[y][x + 1] != 0);
                if (left_filled && right_filled) {
                    current_well_depth++;
                } else {
                    if (current_well_depth > 0) {
                        wells_sum += (current_well_depth * (current_well_depth + 1)) / 2;
                    }
                    current_well_depth = 0;
                }
            } else {
                if (current_well_depth > 0) {
                    wells_sum += (current_well_depth * (current_well_depth + 1)) / 2;
                }
                current_well_depth = 0;
            }
        }

        // bottom
        if (current_well_depth > 0) {
            wells_sum += (current_well_depth * (current_well_depth + 1)) / 2;
        }
    }
    return wells_sum;
}

Board* BoardCopy(const Board* board)
{
    Board* new_board = (Board*)malloc(sizeof(Board));
    if (new_board == NULL) {
        fprintf(stderr, "Failed to alloc new board\n");
        exit(EXIT_FAILURE);
    }

    new_board->size = board->size;
    new_board->grid = GridAlloc(&new_board->size);

    for (int i = 0; i < new_board->size.height + 5; i++) {
        // Ensure valid
        if (board->grid[i] && new_board->grid[i]) {
            memcpy(new_board->grid[i], board->grid[i], new_board->size.width * sizeof(short));
        }
    }

    return new_board;
}

IntList* getFullLines(const Board* board);

void extractFeatures(const Board* board_before_action, const BlockStatus* action_with_y_offset, Board** out_board_after_action_and_clear, int* out_game_over_flag, int* out_features)
{
    // Init
    *out_game_over_flag = 0;
    if (out_features != NULL) {
        memset(out_features, 0, k_num_features * sizeof(int));
    }
    if (out_board_after_action_and_clear != NULL) {
        *out_board_after_action_and_clear = NULL;
    }

    if (action_with_y_offset == NULL || action_with_y_offset->rotation == NULL) {
        *out_game_over_flag = 1;
        return;
    }

    Board* simulated_board = BoardCopy(board_before_action);
    if (simulated_board == NULL) {
        *out_game_over_flag = 1;
        return;
    }

    // Place
    for (int i = 0; i < action_with_y_offset->rotation->occupied_count; i++) {
        Pos pos = action_with_y_offset->rotation->occupied[i];
        int place_x = action_with_y_offset->x_offset + pos.x;
        int place_y = action_with_y_offset->y_offset + pos.y;

        // Check bounds
        if (place_x < 0 || place_x >= simulated_board->size.width || place_y < 0) {
            // not reached if y_offset valid
            *out_game_over_flag = 1;

            GridFree(simulated_board->grid, &simulated_board->size);
            free(simulated_board);
            return;
        }
        // If place_y >= simulated_board->size.height, it's in the buffer
        // not an game overdepends on whether it clears
        if (place_y < simulated_board->size.height + 5) {
            simulated_board->grid[place_y][place_x] = 1;
        } else {
            // completely out of buffer
            *out_game_over_flag = 1;
            GridFree(simulated_board->grid, &simulated_board->size);
            free(simulated_board);
            return;
        }
    }

    IntList* full_lines = getFullLines(simulated_board);

    // 1. Landing Height
    if (out_features)
        out_features[0] = action_with_y_offset->y_offset + action_with_y_offset->rotation->size.height;

    // 2. Eroded Cells
    if (out_features)
        out_features[1] = calcErodedPieceCells(action_with_y_offset, full_lines);

    // eliminate
    clearFullLines(simulated_board, full_lines);
    IntListFree(full_lines);

    // Check game over
    for (int y = simulated_board->size.height; y < simulated_board->size.height + 5; y++) {
        for (int x = 0; x < simulated_board->size.width; x++) {
            if (simulated_board->grid[y][x] != 0) {
                *out_game_over_flag = 1;
                // goto post_game_over_check; // Exit loops
                return;
            }
        }
    }
    // post_game_over_check:;

    // if (*out_game_over_flag && out_features) {
    // }

    if (out_features) {
        // 3. Row Transitions
        out_features[2] = calcRowTransitions(simulated_board);
        // 4. Column Transitions
        out_features[3] = calcColumnTransitions(simulated_board);
        // 5. Holes, 7. Hole Depth, 8. Rows with Holes
        int holes_count = 0;
        int total_hole_depth = 0;
        int rows_with_holes_count = 0;
        calcHolesAndDepth(simulated_board, &holes_count, &total_hole_depth, &rows_with_holes_count);
        out_features[4] = holes_count;
        out_features[6] = total_hole_depth;
        out_features[7] = rows_with_holes_count;

        // 6. Board Wells
        out_features[5] = calcBoardWells(simulated_board);
    }

    if (out_board_after_action_and_clear != NULL) {
        *out_board_after_action_and_clear = simulated_board;
    } else {
        GridFree(simulated_board->grid, &simulated_board->size);
        free(simulated_board);
    }
}

double assessmentTwoActions(Game* game, BlockStatus* action_1, BlockStatus* action_2, AssessmentModel* model)
{
    double score_1 = -INFINITY;
    double score_2 = -INFINITY;

    int features_1_data[k_num_features];
    int features_2_data[k_num_features];

    Board* board_after_step1 = NULL;
    int game_over_step1 = 0;

    // Check actions
    if (action_1 == NULL || action_1->rotation == NULL) {
        return -INFINITY;
    }
    if (action_2 == NULL || action_2->rotation == NULL) {
        return -INFINITY;
    }

    // --- Step 1 ---
    BlockStatus action_1_eval = *action_1;
    action_1_eval.y_offset = INT_MAX;
    int y_offset_1 = findYOffset(&game->board, &action_1_eval);

    if (y_offset_1 == -1) {
        return -INFINITY;
    }
    action_1_eval.y_offset = y_offset_1;

    extractFeatures(&game->board, &action_1_eval, &board_after_step1, &game_over_step1, features_1_data);

    if (game_over_step1 || board_after_step1 == NULL) {
        if (board_after_step1) {
            GridFree(board_after_step1->grid, &board_after_step1->size);
            free(board_after_step1);
        }
        return -INFINITY;
    }
    score_1 = caculateLinearFunction(model->weights, features_1_data, k_num_features);

    // --- Step 2 ---
    BlockStatus action_2_eval = *action_2;
    action_2_eval.y_offset = INT_MAX; // Reset
    int y_offset_2 = findYOffset(board_after_step1, &action_2_eval); // Use board_after_step1

    if (y_offset_2 == -1) {
        GridFree(board_after_step1->grid, &board_after_step1->size);
        free(board_after_step1);
        return -INFINITY;
    }
    action_2_eval.y_offset = y_offset_2;

    int game_over_step2 = 0;

    extractFeatures(board_after_step1, &action_2_eval, NULL, &game_over_step2, features_2_data);

    // board_1 no longer needed
    GridFree(board_after_step1->grid, &board_after_step1->size);
    free(board_after_step1);
    board_after_step1 = NULL;

    if (game_over_step2) {
        return -INFINITY;
    }
    score_2 = caculateLinearFunction(model->weights, features_2_data, k_num_features);

    if (score_1 <= -INFINITY || score_2 <= -INFINITY) {
        return -INFINITY;
    }

    return score_1 + score_2; // success
}

/*
判断一次放置是否有越界

:param board: 棋盘对象
:param action: 方块状态
:return: 是否越界
*/
int isOutOfIndex(const Board* board, const BlockStatus* action)
{
    if (action->rotation == NULL) {
        return 1;
    }

    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int check_x = action->x_offset + pos.x;
        int check_y = action->y_offset + pos.y;

        if (check_x < 0 || check_x >= board->size.width) {
            return 1;
        }

        if (check_y < 0 || check_y >= board->size.height) {
            return 1;
        }
    }

    return 0;
}

/*
判断假设按照 y 坐标放置，是否发生碰撞。
:param board: 棋盘对象
:param action: 方块状态
:param y_offset: 方块在棋盘上的 y 坐标
:return: 是否发生碰撞
*/
int isCollision(const Board* board, const BlockStatus* action, const int y_offset)
{
    if (isOutOfIndex(board, action)) {
        return 1;
    }

    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int check_x = action->x_offset + pos.x;
        int check_y = y_offset + pos.y;

        for (int y_itr = check_y; y_itr < board->size.height; y_itr++) {
            if (board->grid[y_itr][check_x] != 0) {
                return 1;
            }
        }
    }

    return 0;
}

/*
判断放下该方块后，是否到达死亡线。不负责判断碰撞，需要在调用前判断碰撞
到达死亡线不代表立即死亡，需要判断能否消除后，再认定为死亡

:param board: 棋盘对象
:param action: 方块状态
:param y_offset: 方块在棋盘上的 y 坐标
:return: 是否到达死亡线
*/
int isOverflow(const Board* board, const BlockStatus* action, const int y_offset)
{
    if (action->rotation == NULL) {
        return 1;
    }

    if (y_offset < 0) {
        return 1;
    }

    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int check_y = y_offset + pos.y;

        if (check_y >= board->size.height) {
            return 1;
        }
    }

    return 0;
}

/*
计算放下该方块后，y 坐标的值

:param board: 未经操作的棋盘对象
:param action: 方块状态
:return: y 坐标；若因此游戏结束，则返回 -1
*/
int findYOffset(const Board* board, BlockStatus* action)
{
    if (action->rotation == NULL) {
        action->y_offset = -1;
        return -1;
    }

    if (action->y_offset < INT_MAX) {
        return action->y_offset;
    }

    for (int y = 0;; y++) {
        action->y_offset = y;

        int collision = isCollision(board, action, y);
        int overflow = isOverflow(board, action, y);

        if (!collision && !overflow) {
            action->y_offset = y;
            return y;
        }

        if (y >= board->size.height) {
            action->y_offset = -1;
            return -1;
        }
    }

    action->y_offset = -1;
    return -1; // never reachedd
}

/*
判断一行是否满了

:param board: 棋盘对象
:param index: 该行的索引
:return: 是否满了
*/
int isLineFull(const Board* board, const int index)
{
    short* line = board->grid[index];

    for (int i = 0; i < board->size.width; i++) {
        if (line[i] == 0) {
            return 0;
        }
    }

    return 1;
}

/*
获取填满的行

:param board: 棋盘对象
:return: 填满行的 y 坐标列表
*/
IntList* getFullLines(const Board* board)
{
    IntList* full_lines = IntListAlloc(board->size.height + 5);
    int height = board->size.height;

    for (int i = 0; i < height; i++) {
        if (isLineFull(board, i)) {
            IntListPush(full_lines, i);
        }
    }

    return full_lines;
}

void ensureNoNullLine(Board* board)
{
    for (int i = 0; i < board->size.height + 5; i++) {
        if (board->grid[i] == NULL) {
            board->grid[i] = (short*)calloc(board->size.width, sizeof(short));
        }
    }
}

/*
（产生副作用）消除一次操作后的满行，并返回消除的行数
(适用于 y=0 在底部的坐标系)

:param board: 棋盘对象
:param full_lines: 满行的 y 坐标列表
:return: 消除的行数
*/
size_t clearFullLines(Board* board, IntList* full_lines)
{
    size_t num_full_lines = full_lines->length;
    if (num_full_lines == 0) {
        return 0;
    }

    int height_with_buffer = board->size.height + 5;
    int width = board->size.width;
    short** grid = board->grid;

    // for quick lookup
    int is_line_full[height_with_buffer];
    memset(is_line_full, 0, sizeof(is_line_full));
    for (size_t i = 0; i < num_full_lines; i++) {
        if (full_lines->data[i] >= 0 && full_lines->data[i] < height_with_buffer) {
            is_line_full[full_lines->data[i]] = 1;
        }
    }

    int write_y = 0; // 下一行非满行应该被复制到的位置
    for (int read_y = 0; read_y < height_with_buffer; read_y++) {
        if (!is_line_full[read_y]) {
            if (write_y != read_y) {
                memcpy(grid[write_y], grid[read_y], width * sizeof(short));
            }
            write_y++; // write pointer up
        }
    }

    // clear from write_y upwards
    for (int y = write_y; y < height_with_buffer; y++) {
        if (grid[y] != NULL) {
            memset(grid[y], 0, width * sizeof(short));
        }
    }

    return num_full_lines;
}

/*
执行动作（有实际执行的副作用）
若返回 -1，则可能是这种动作可能不合法，需要剔除该可能性

:param game: 游戏对象
:param action: 要执行的动作
:param eliminate: 是否消除满行并加分。若为 0，仅返回下落后的结果；若为 1，则执行全部操作
:return: y_offset
*/
int executeAction(Game* game, BlockStatus* action, int eliminate)
{
    // pre-check
    if (action->rotation == NULL) {
        GameSetEnd(game);
        return -1;
    }

    Board* board = &game->board;

    // find y
    int y_offset = findYOffset(board, action);
    if (y_offset == -1) {
        GameSetEnd(game);
        return -1;
    }
    action->y_offset = y_offset;

    // place
    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int place_x = action->x_offset + pos.x;
        int place_y = y_offset + pos.y;

        if (place_y >= 0 && place_y < board->size.height && place_x >= 0 && place_x < board->size.width) {
            board->grid[place_y][place_x] = 1;
        } else { // error
            GameSetEnd(game);
            return -1;
        }
    }

    if (eliminate == 0) {
        return action->y_offset;
    }

    // eliminate
    IntList* full_lines = getFullLines(board);
    size_t num_full_lines = clearFullLines(board, full_lines);

    // add score
    if (num_full_lines > 0) {
        if (num_full_lines <= 4) {
            game->score += game->config.awards[num_full_lines - 1];
        } else { // never happened
            game->score += game->config.awards[3];
        }
    }

    // check game over
    for (int y = board->size.height; y < board->size.height + 5; y++) {
        for (int x = 0; x < board->size.width; x++) {
            if (board->grid[y][x] != 0) {
                GameSetEnd(game);
                return -1;
            }
        }
    }

    // return
    action->y_offset = y_offset;
    return y_offset;
}

/*
获取所有可能的动作

:param game: 游戏对象
:param block: 下一个方块
:return: 所有可能的动作列表，以 NULL 结尾
*/
BlockStatus** getAvailableActions(Game* game, Block* block)
{
    int width = game->board.size.width;
    int actions_count = 0;

    size_t actions_capacity = (width * block->rotations_count) + 1;
    BlockStatus** actions = (BlockStatus**)malloc(sizeof(BlockStatus*) * actions_capacity);
    if (actions == NULL) {
        fprintf(stderr, "Failed to alloc actions array\n");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < actions_capacity; i++) {
        actions[i] = NULL;
    }

    for (int i = 0; i < block->rotations_count; i++) {
        BlockRotation* rotation = &block->rotations[i];
        for (int x = 0; x <= width - rotation->size.width; x++) {
            BlockStatus* action = (BlockStatus*)malloc(sizeof(BlockStatus));
            if (action == NULL) {
                fprintf(stderr, "Failed to alloc action\n");
                exit(EXIT_FAILURE);
            }

            // Init allocated action
            action->x_offset = x;
            action->rotation = rotation;
            action->y_offset = INT_MAX; // Will be calculated by findYOffset()
            action->assement_score = -INFINITY;

            action->y_offset = findYOffset(&game->board, action);
            if (action->y_offset == -1) {
                free(action); // if invalid
            } else {
                if ((size_t)actions_count >= actions_capacity - 1) {
                    fprintf(stderr, "Actions array capacity exceeded\n");
                    exit(EXIT_FAILURE);
                }
                actions[actions_count] = action;
                actions_count++;
            }
        }
    }
    actions[actions_count] = NULL;

    return actions;
}

double assessmentSingleAction(Game* game, BlockStatus* action, AssessmentModel* model)
{
    if (action->rotation == NULL) {
        return -INFINITY; // Invalid action
    }

    BlockStatus current_action_eval = *action;
    current_action_eval.y_offset = INT_MAX;

    int y_offset = findYOffset(&game->board, &current_action_eval);
    if (y_offset == -1) {
        return -INFINITY; // Invalid action
    }
    current_action_eval.y_offset = y_offset;

    int features_data[k_num_features];
    Board* board_after_action = NULL;
    int game_over = 0;

    extractFeatures(&game->board, &current_action_eval, &board_after_action, &game_over, features_data);

    if (game_over) {
        if (board_after_action) {
            GridFree(board_after_action->grid, &board_after_action->size);
            free(board_after_action);
        }
        return -INFINITY; // Invalid action
    }

    double current_score = caculateLinearFunction(model->weights, features_data, k_num_features);

    return current_score;

}

BlockStatus* findBestSingleAction(Game* game, AssessmentModel* model)
{
    double best_score = -INFINITY;
    BlockStatus* best_action = NULL;

    BlockStatus** actions = game->available_statuses_1;
    for (int i = 0; i < game->available_statuses_1_count; i++) {
        BlockStatus* action = actions[i];
        if (action->rotation == NULL) {
            continue;
        }

        BlockStatus current_action_eval = *action;
        current_action_eval.y_offset = INT_MAX;

        int y_offset = findYOffset(&game->board, &current_action_eval);
        if (y_offset == -1) {
            continue;
        }
        current_action_eval.y_offset = y_offset;

        int features_data[k_num_features];
        Board* board_after_action = NULL;
        int game_over = 0;

        extractFeatures(&game->board, &current_action_eval, &board_after_action, &game_over, features_data);

        if (game_over) {
            if (board_after_action) {
                GridFree(board_after_action->grid, &board_after_action->size);
                free(board_after_action);
            }
            continue;
        }

        double current_score = caculateLinearFunction(model->weights, features_data, k_num_features);

        if (current_score > best_score) {
            best_score = current_score;
            best_action = action;
        }

        if (board_after_action) {
            GridFree(board_after_action->grid, &board_after_action->size);
            free(board_after_action);
        }
    }

    return best_action;
}

BlockStatus* findBestActionV3(Game* game, AssessmentModel* model)
{
    double best_combined_score = -INFINITY;
    BlockStatus* best_action = NULL;

    BlockStatus** actions_1 = game->available_statuses_1;
    Block* block_2 = game->upcoming_blocks[1];

    int n = 10;
    int top_n = (int)(game->available_statuses_1_count * n / 100.0) + 1;
    double first_scores[game->available_statuses_1_count];

    for (int i = 0; i < game->available_statuses_1_count; i++) {
        first_scores[i] = assessmentSingleAction(game, game->available_statuses_1[i], model);
    }

    // 排序
    for (int i = 0; i < game->available_statuses_1_count - 1; i++) {
        for (int j = i + 1; j < game->available_statuses_1_count; j++) {
            if (first_scores[i] < first_scores[j]) {
                double temp_score = first_scores[i];
                first_scores[i] = first_scores[j];
                first_scores[j] = temp_score;

                BlockStatus* temp_action = actions_1[i];
                actions_1[i] = actions_1[j];
                actions_1[j] = temp_action;
            }
        }
    }

    // 保留
    if (top_n > game->available_statuses_1_count) {
        top_n = game->available_statuses_1_count;
    }
    BlockStatus** top_actions_1 = (BlockStatus**)malloc(sizeof(BlockStatus*) * (top_n + 1));
    for (int i = 0; i < top_n; i++) {
        top_actions_1[i] = actions_1[i];
    }
    top_actions_1[top_n] = NULL; 

    for (int i = 0; i < top_n; i++) {
        BlockStatus* action_1 = actions_1[i];
        if (action_1->rotation == NULL) {
            continue;
        }

        for (int j = 0; j < game->available_statuses_2_count; j++) {
            BlockStatus* action_2 = game->available_statuses_2[j];
            double combined_score = assessmentTwoActions(game, action_1, action_2, model);
            if (combined_score > best_combined_score) {
                best_combined_score = combined_score;
                best_action = action_1;
            }
        }
    }

    if (best_action == NULL) {
        return NULL;
    }

    

    return best_action;
}

/*
找到最佳的操作策略

:param game: 游戏对象
:param model: 评估模型
:return: 最佳的操作策略
*/
BlockStatus* findBestAction(Game* game, AssessmentModel* model)
{
    double best_combined_score = -INFINITY;
    BlockStatus* best_action = NULL;

    BlockStatus** actions_1 = game->available_statuses_1;
    Block* block_2 = game->upcoming_blocks[1];

    for (int i = 0; i < game->available_statuses_1_count; i++) {
        BlockStatus* action_1 = actions_1[i];
        if (action_1->rotation == NULL) {
            continue;
        }

        for (int j = 0; j < game->available_statuses_2_count; j++) {
            BlockStatus* action_2 = game->available_statuses_2[j];
            double combined_score = assessmentTwoActions(game, action_1, action_2, model);
            if (combined_score > best_combined_score) {
                best_combined_score = combined_score;
                best_action = action_1;
            }
        }
    }

    if (best_action == NULL) {
        return NULL;
    }

    return best_action;
}

void freeActionsArray(BlockStatus** actions, int count)
{
    if (actions == NULL) {
        return;
    }
    for (int i = 0; i < count; i++) {
        if (actions[i] != NULL) {
            free(actions[i]);
        }
    }
    free((void*)actions);
}

//  upcoming_blocks[0] upcoming_blocks
BlockStatus* runGameStep(Context* ctx, Block* next_block, int mode)
{
    Game* game = ctx->game;
    AssessmentModel* model = ctx->model;

    // update upcoming_blocks

    if (next_block == NULL) { //  next_block
        // game->upcoming_blocks[0] = game->upcoming_blocks[1];
        // game->upcoming_blocks[1] = NULL;
    } else {
        game->upcoming_blocks[0] = game->upcoming_blocks[1];
        game->upcoming_blocks[1] = next_block;
    }

    // free actions from previous step before generating new

    freeActionsArray(game->available_statuses_1, game->available_statuses_1_count);
    game->available_statuses_1 = NULL;
    game->available_statuses_1_count = 0;
    freeActionsArray(game->available_statuses_2, game->available_statuses_2_count);
    game->available_statuses_2 = NULL;
    game->available_statuses_2_count = 0;

    // if (next_block == NULL) {
    //     next_block = game->upcoming_blocks[1];
    // }

    // get available actions

    BlockStatus** actions_1 = getAvailableActions(game, game->upcoming_blocks[0]);
    game->available_statuses_1 = actions_1;
    game->available_statuses_1_count = 0;
    if (actions_1 != NULL) { // Check if  succeeded
        for (int i = 0; actions_1[i] != NULL; i++) {
            if (actions_1[i]->rotation == NULL) {
                continue;
            }
            game->available_statuses_1_count++;
        }
    }

    if (mode >= 2) {
        if (next_block == NULL) {
            next_block = game->upcoming_blocks[1];
        }
        BlockStatus** actions_2 = getAvailableActions(game, next_block);
        game->available_statuses_2 = actions_2;
        game->available_statuses_2_count = 0; // Reset count before loop
        if (actions_2 != NULL) { // Check if succeeded
            for (int i = 0; actions_2[i] != NULL; i++) {
                if (actions_2[i]->rotation == NULL)
                    continue;
                game->available_statuses_2_count++;
            }
        }
    }

    // find best action

    BlockStatus* best_action = NULL;
    if (mode == 1) {
        best_action = findBestSingleAction(game, model);
    } else if (mode == 2) {
        best_action = findBestAction(game, model);
    } else if (mode == 3) {
        best_action = findBestActionV3(game, model);
    } else {
        fprintf(stderr, "Invalid mode: %d\n", mode);
        return NULL; // Invalid mode
    }
    if (best_action == NULL) {
        GameSetEnd(game);
        return NULL;
    }

    BlockStatus best_action_copy = *best_action;

    // execute

    executeAction(game, &best_action_copy, 1);

    BlockStatus* result_action = (BlockStatus*)malloc(sizeof(BlockStatus));
    if (result_action == NULL) {
        fprintf(stderr, "Failed to alloc for result action\n");
        GameSetEnd(game);
        return NULL;
    }
    *result_action = best_action_copy;

    return result_action;
}

/*
计算线性函数的值
:param weights: 权重列表
:param features: 特征列表
:param length: 特征列表的长度
:return: 线性函数的值
*/
// double caculateLinearFunction(double* weights, int* features, int length)
// {
// }

void visualizeStep(const Game* game, const BlockStatus* action)
{
    printf("Current Score: %ld\n", game->score);
    printf("Upcoming Blocks: %c, %c\n", game->upcoming_blocks[0]->name, (game->upcoming_blocks[1] ? game->upcoming_blocks[1]->name : '-'));
    printf("Action: %ddeg at (%d, %d)\n", action->rotation->label, action->x_offset, action->y_offset);
    printf("Board:\n");
    for (int y = game->board.size.height - 1; y >= 0; y--) {
        for (int x = 0; x < game->board.size.width; x++) {
            printf("%c ", game->board.grid[y][x] ? '#' : '.');
        }
        printf("\n");
    }
}

Block* randomBlock(void)
{
    int random_index = rand() % k_blocks_count;
    return k_blocks[random_index];
}

// mode 1: single action, mode 2: double action
void runRandomTest(Context* ctx, int mode)
{
    Game* game = ctx->game;

    srand((unsigned int)time(NULL));

    game->upcoming_blocks[0] = randomBlock();
    game->upcoming_blocks[1] = randomBlock();

    BlockStatus* action_taken = NULL;

    action_taken = runGameStep(ctx, NULL, mode);
    if (action_taken) {
        visualizeStep(ctx->game, action_taken);
        free(action_taken);
        action_taken = NULL;
    }

    for (size_t i = 1; i < 1e6; i++) {
        if (ctx->game->score < 0) { // Check game over before next step
            printf("Game Over. Final Score: %ld\n", -(ctx->game->score));
            break;
        }
        action_taken = runGameStep(ctx, randomBlock(), mode);
        if (action_taken) {
            // visualizeStep(ctx->game, action_taken);
            if (i % 1000 == 0) {
                printf("Step %zu: ", i);
                visualizeStep(ctx->game, action_taken);
            }
            free(action_taken);
            action_taken = NULL;
        } else if (ctx->game->score < 0) {
            printf("Game Over during step. Final Score: %ld\n", -(ctx->game->score));
            printf("%zu steps taken.\n", i);
            printf("%ld\n", labs(ctx->game->score));
            // visualizeStep(game, action_taken);
            break;
        } else {
            fprintf(stderr, "runGameStep returned NULL without ending game.\n");
            break;
        }
    }

    // final cleanup
    freeActionsArray(ctx->game->available_statuses_1, ctx->game->available_statuses_1_count);
    ctx->game->available_statuses_1 = NULL;
    ctx->game->available_statuses_1_count = 0;
    freeActionsArray(ctx->game->available_statuses_2, ctx->game->available_statuses_2_count);
    ctx->game->available_statuses_2 = NULL;
    ctx->game->available_statuses_2_count = 0;
}

const Block* findBlock(char name)
{
    for (int i = 0; i < k_blocks_count; i++) {
        if (k_blocks[i]->name == name) {
            return k_blocks[i];
        }
    }
    // printf("No block found with name: %c\n", name);
    return NULL;
}

int degreeToNo(int degree)
{
    switch (degree) {
    case 0:
        return 0;
    case 90:
        return 1;
    case 180:
        return 2;
    case 270:
        return 3;
    default:
        fprintf(stderr, "Invalid degree: %d\n", degree);
        return -1;
    }
}

int main(int argc, char* argv[])
{
    // if (argc > 1) {
    //     // printf("Use arg: %s\n", argv[1]);
    // }

    Size grid_size = { 10, 16 };

    Block* upcoming_blocks[2] = { NULL };
    Game game = {
        .config = {
            .awards = (int[]) { 100, 300, 500, 800 },
            .available_blocks = (Block*)k_blocks,
            .available_blocks_count = k_blocks_count },
        .board = { .size = grid_size, .grid = GridAlloc(&grid_size) },
        .score = 0,
        .upcoming_blocks = upcoming_blocks,
        .available_statuses_1 = NULL,
        .available_statuses_1_count = 0,
        .available_statuses_2 = NULL,
        .available_statuses_2_count = 0
    };

    AssessmentModel assessment_model = {
        .length = 9,
        .weights = (double[]) { -14.2970, -1.6659, -9.9349, -15.6773, -17.8268, -14.1545, -1.3156, -32.9234, -0.6702 }
    };

    Context ctx = {
        .game = &game,
        .model = &assessment_model
    };

    if (!DEBUG_MODE) {
        goto oj;
    }

    // Initialize the game
    if (argc == 1) {
        runRandomTest(&ctx, 1);
    } else if (strcmp(argv[1], "single") == 0) {
        runRandomTest(&ctx, 1);
    } else if (strcmp(argv[1], "double") == 0) {
        runRandomTest(&ctx, 2);
    } else if (!DEBUG_MODE || strcmp(argv[1], "oj") == 0) {
        // {
    oj:;
        char b1 = 0, b2 = 0;
        // fflush(stdin);
        scanf("%c%c", &b1, &b2);
        // fflush(stdin);
        Block* block1 = findBlock(b1);
        Block* block2 = findBlock(b2);
        if (block1 == NULL) {
            fprintf(stderr, "Invalid block names: %c, %c\n", b1, b2);
            fflush(stdout);
            return 1;
        }

        game.upcoming_blocks[0] = block1;
        game.upcoming_blocks[1] = block2;

        // Game* game = ctx.game;

        BlockStatus* action_taken = NULL;

        action_taken = runGameStep(&ctx, NULL, NUM_CONSIDER);
        if (action_taken) {
            printf("%d %d\n%ld\n", degreeToNo(action_taken->rotation->label), action_taken->x_offset, labs(ctx.game->score));
            // visualizeStep(game, action_taken);
            fflush(stdout);
            free(action_taken);
            action_taken = NULL;
        }

        for (int i = 0; i < 1000010; i++) {
            if (!b2) {
                scanf("%c", &b1);
                while (b1 == '\n') {
                    scanf("%c", &b1);
                }

                if (b1 == 'E') {
                    break;
                }

                if (b1 == EOF) {
                    break;
                }

                const int change_to_2 = 100000;

                // if (ctx.game->score < 0 || ctx.game->score > 1000000) { // check game over before next step
                //     // printf("%ld\n", labs(ctx.game->score));
                //     // fflush(stdout);
                //     return 0;
                // }
                // printf("CURRENT Upcoming: %c, %c\n", game->upcoming_blocks[0]->name, game->upcoming_blocks[1]->name);
                action_taken = runGameStep(&ctx, findBlock(b1), i >= change_to_2 ? 2 : 3);
                // action_taken = runGameStep(&ctx, findBlock(b1), 3);
                if (action_taken) {
                    printf("%d %d\n%ld\n", degreeToNo(action_taken->rotation->label), action_taken->x_offset, labs(ctx.game->score));
                    // visualizeStep(game, action_taken);
                    fflush(stdout);
                    free(action_taken);
                    action_taken = NULL;
                } else if (ctx.game->score < 0) { // check game over after step
                    // printf("%ld\n", labs(ctx.game->score));
                    // break;
                    // return 0;
                } else {
                    fprintf(stderr, "runGameStep returned NULL without ending game.\n");
                    break;
                }

                if (b1 == 'X') {
                    break;
                }
            } else {
                b2 = 0;
            }
        }

        if (!DEBUG_MODE) {
            goto end;
        }
        // }
    } else {
        runRandomTest(&ctx, 0);
    }

end:;
    // Cleanup
    GridFree(game.board.grid, &game.board.size);
    // GridFree(game.board.grid, &game.board.size);
    freeActionsArray(game.available_statuses_1, game.available_statuses_1_count);
    freeActionsArray(game.available_statuses_2, game.available_statuses_2_count);

    return 0;
}
