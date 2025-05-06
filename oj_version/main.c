
// #include <cstddef>
#include <limits.h>
// #include <cstddef>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> // For getpid()

typedef struct {
    void* data; // 指向实际数据的指针
    size_t size; // 当前存储的元素数量
    size_t capacity; // 当前分配的内存能容纳的元素数量
    size_t element_size; // 每个元素的大小（字节）
} Vector;

typedef struct {
    int* data;
    size_t length;
} IntList;

typedef struct {
    int width;
    int height;
} Size; // 大小

typedef struct {
    int x;
    int y;
} Pos; // Position，位置

typedef struct {
    int label; // 用于显示的方块旋转角度
    Size size; // 当前状态的方块大小
    Pos* occupied; // 占用的格子的位置的列表
    int occupied_count; // 占用的格子数量
} BlockRotation; // （用于定义枚举）方块的一个旋转状态

typedef struct {
    char name; // 用于显示的方块名称
    BlockRotation* rotations; // 旋转状态的列表
    int rotations_count; // 旋转状态的数量
} Block; // （用于定义枚举）一种方块类型

typedef struct {
    int x_offset; // x坐标
    BlockRotation* rotation; // 旋转状态
    int y_offset; // y坐标，默认为 +infinity，需要进行计算才知道
    double assement_score; // 评估分数，默认为 -infinity，需要后续进行计算
} BlockStatus; // （用于遍历评估策略）一种放置的策略

typedef struct {
    int length; // 模型参数个数
    double* weights; // 权重，长度为 length+1，最后一位是常数项
} AssessmentModel; // 评估策略的评估模型

typedef struct {
    int* awards; // 分别消除 1, 2, 3, 4 行的奖励
    Block* available_blocks; // 可用的方块列表
    int available_blocks_count; // 可用的方块数量
} GameConfig; // 游戏配置

typedef struct {
    Size size; // 游戏区域可操作空间的大小，高度为死亡判定线减 1
    short** grid; // 游戏区域的网格，1 代表占用，0 代表空闲；数组的大小高度应该比逻辑高度多 5 作为缓冲
} Board; // 游戏区域

typedef struct {
    GameConfig config; // 游戏配置

    Board board; // 游戏区域当前状态
    long score; // 当前分数，若为负数则表示游戏结束

    Block** upcoming_blocks; // 即将到来的方块列表，共 2 个
    BlockStatus** available_statuses_1; // 第一步可用的放置策略列表
    int available_statuses_1_count; // 第一步可用的放置策略数量
    BlockStatus** available_statuses_2; // 第二步可用的放置策略列表
    int available_statuses_2_count; // 第二步可用的放置策略数量
} Game; // 游戏状态

typedef struct {
    Game* game;
    AssessmentModel* model;
} Context; // 整个程序的上下文

double caculateLinearFunction(double* weights, int* features, int feature_length)
{
    double result = 0;
    for (int i = 0; i < feature_length; i++) {
        result += weights[i] * features[i];
    }
    result += weights[feature_length]; // 常数项
    return result;
}

const int k_num_features = 8; // 特征数量

// ----- Block Rotation consts -----

const Block k_block_I = {
    .name = 'I',
    .rotations = (BlockRotation[]) {
        { .label = 0,
            .size = { 1, 4 },
            .occupied = (Pos[]) { { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 } },
            .occupied_count = 4 },
        { .label = 90,
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

short** GridAlloc(Size* size)
{
    int width = size->width;
    int height = size->height;
    int buffer = 5; // buffer for the top of the board

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
            grid[i][j] = 0; // Initialize to 0
        }
    }

    return grid;
}

void GridFree(short** grid, Size* size)
{
    int buffer = 5; // Match the buffer used in GridAlloc
    if (grid == NULL) return; // Avoid freeing NULL

    // Free each row pointer up to height + buffer
    for (int i = 0; i < size->height + buffer; i++) {
        // Check if the pointer is non-NULL before freeing
        if (grid[i] != NULL) {
            free(grid[i]);
             // grid[i] = NULL; // Optional: good practice
        }
    }
    // Free the array of row pointers itself
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

// 在列表末尾添加一个元素（保证不大于 max_size），并返回该元素的索引
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
    int rows_with_holes[board_after_elim->size.height + 5]; // Initialize to 0
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
            } else if (block_encountered && y < height) { // It's a hole only if below a block AND within logical height
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
        for (int y = height - 1; y >= 0; y--) { // Iterate top-down within logical height
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
        // Add wells extending to the bottom
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
        for (int j = 0; j < new_board->size.width; j++) {
            new_board->grid[i][j] = board->grid[i][j];
        }
    }

    return new_board;
}

IntList* getFullLines(const Board* board);

int* extractFeatures(const Game* game, const BlockStatus* action)
{
    int* features = (int*)malloc((k_num_features + 5) * sizeof(int));
    if (features == NULL) {
        fprintf(stderr, "Failed to alloc features\n");
        exit(EXIT_FAILURE);
    }

    Board* board_copy = BoardCopy(&game->board); // Use Board's copy constructor ONCE

    // Place the piece on the copy
    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int place_x = action->x_offset + pos.x;
        int place_y = action->y_offset + pos.y;

        if (place_y >= 0 && place_y < board_copy->size.height && place_x >= 0 && place_x < board_copy->size.width) {
            board_copy->grid[place_y][place_x] = 1; // Place pointer to original block
        } else {
            fprintf(stderr, "Placement out of bounds during feature extraction simulation.\n");
            exit(EXIT_FAILURE);
        }
    }

    IntList* full_lines = getFullLines(board_copy);

    // 1. Landing Height
    features[0] = action->y_offset + 1;

    // 2. Eroded Cells
    features[1] = calcErodedPieceCells(action, full_lines);

    // execute the elimination
    size_t lines_cleared = clearFullLines(board_copy, full_lines);

    // 3. Row Transitions
    features[2] = calcRowTransitions(board_copy);

    // 4. Column Transitions
    features[3] = calcColumnTransitions(board_copy);

    // 5. Holes
    features[4] = 0;
    // 6. Board Wells
    features[5] = calcBoardWells(board_copy);
    // 7. Hole Depth
    features[6] = 0;
    // 8. Rows with Holes
    features[7] = 0;

    int holes_count = 0;
    int total_hole_depth = 0;
    int rows_with_holes_count = 0;
    calcHolesAndDepth(board_copy, &holes_count, &total_hole_depth, &rows_with_holes_count);
    features[4] = holes_count;
    features[6] = total_hole_depth;
    features[7] = rows_with_holes_count;

    IntListFree(full_lines);
    GridFree(board_copy->grid, &board_copy->size);
    free(board_copy);

    return features;
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

    // Create a temporary array to mark full lines for quick lookup
    int is_line_full[height_with_buffer];
    memset(is_line_full, 0, sizeof(is_line_full));
    for (size_t i = 0; i < num_full_lines; i++) {
        if (full_lines->data[i] >= 0 && full_lines->data[i] < height_with_buffer) { // Bounds check
            is_line_full[full_lines->data[i]] = 1;
        }
    }

    int write_y = 0;
    for (int read_y = 0; read_y < height_with_buffer; read_y++) {
        if (!is_line_full[read_y]) { // If the read line is NOT full
            if (write_y != read_y) {
                // Copy data from read_y row to write_y row instead of pointer assignment
                memcpy(grid[write_y], grid[read_y], width * sizeof(short));
            }
            write_y++; // Move write pointer up
        }
        // If the read line IS full, we skip it. Its contents will be overwritten
        // by a later memcpy or cleared by the memset below.
        // The memory for the row itself (grid[read_y]) will be freed by GridFree.
    }

    // Clear the lines at the top (from write_y upwards)
    for (int y = write_y; y < height_with_buffer; y++) {
        // Ensure the row pointer is valid before writing (should be unless GridAlloc failed)
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

:param block: 下一个方块
:return: 所有可能的动作列表，以 NULL 结尾
*/
BlockStatus** getAvailableActions(Game* game, Block* block)
{
    int width = game->board.size.width;
    int actions_count = 0;
    // Estimate max possible actions, adjust if needed
    size_t actions_capacity = (width * block->rotations_count) + 1;
    BlockStatus** actions = (BlockStatus**)malloc(sizeof(BlockStatus*) * actions_capacity);
    if (actions == NULL) {
        fprintf(stderr, "Failed to alloc actions array\n");
        exit(EXIT_FAILURE);
    }
    // Initialize pointers to NULL
    for (size_t i = 0; i < actions_capacity; i++) {
        actions[i] = NULL;
    }

    for (int i = 0; i < block->rotations_count; i++) {
        BlockRotation* rotation = &block->rotations[i];
        for (int x = 0; x <= width - rotation->size.width; x++) {
            // Allocate BlockStatus on the heap
            BlockStatus* action = (BlockStatus*)malloc(sizeof(BlockStatus));
            if (action == NULL) {
                fprintf(stderr, "Failed to alloc action\n");
                // Consider freeing already allocated actions before exiting
                exit(EXIT_FAILURE);
            }

            // Initialize the allocated action
            action->x_offset = x;
            action->rotation = rotation;
            action->y_offset = INT_MAX; // Will be calculated by findYOffset
            action->assement_score = -INFINITY;

            action->y_offset = findYOffset(&game->board, action);
            if (action->y_offset == -1) {
                free(action); // Free if the action is invalid
            } else {
                if ((size_t)actions_count >= actions_capacity - 1) {
                    // Optional: Resize actions array if needed (realloc)
                    fprintf(stderr, "Actions array capacity exceeded\n");
                    // Consider freeing allocated memory before exiting
                    exit(EXIT_FAILURE);
                }
                actions[actions_count] = action;
                actions_count++;
            }
        }
    }
    actions[actions_count] = NULL; // Keep the NULL terminator convention if used elsewhere

    return actions;
}

double assessmentTwoActions(Game* game, BlockStatus* action_1, BlockStatus* action_2, AssessmentModel* model)
{
    double score_1 = -INFINITY;
    double score_2 = -INFINITY;
    int* features_1 = NULL; // Initialize to NULL
    int* features_2 = NULL; // Initialize to NULL
    Board* board_1 = NULL; // Initialize to NULL
    IntList* full_lines_1 = NULL; // Initialize to NULL

    // Check initial actions
    if (action_1 == NULL || action_1->rotation == NULL) {
        return -INFINITY;
    }
    if (action_2 == NULL || action_2->rotation == NULL) {
        // This case might be valid if only looking one step ahead is allowed,
        // but based on findBestAction, it seems we always need a valid action_2.
        // If action_2 can be invalid, the logic needs adjustment.
        // Assuming for now it must be valid based on how getAvailableActions works.
        return -INFINITY;
    }

    // Calculate y_offset for action_1 based on the *current* game board
    // Make a temporary copy of action_1 to avoid modifying the original
    BlockStatus action_1_copy = *action_1;
    action_1_copy.y_offset = INT_MAX; // Reset y_offset for calculation
    int y_offset_1 = findYOffset(&game->board, &action_1_copy);
    if (y_offset_1 == -1) {
        return -INFINITY; // Invalid placement for action_1
    }
    // Use the calculated y_offset for feature extraction
    action_1_copy.y_offset = y_offset_1;

    // Check bounds *after* calculating y_offset
    if (isOutOfIndex(&game->board, &action_1_copy)) {
        return -INFINITY;
    }
    // No need to check action_2 bounds here, it's checked later on board_1

    // --- Evaluate Step 1 ---
    features_1 = extractFeatures(game, &action_1_copy); // Use the copy with correct y_offset
    if (features_1 == NULL) { // Should not happen if malloc succeeds
        goto cleanup; // Use goto for centralized cleanup on error
    }
    score_1 = caculateLinearFunction(model->weights, features_1, k_num_features); // Use k_num_features

    // --- Simulate Step 1 ---
    board_1 = BoardCopy(&game->board);
    if (board_1 == NULL) {
        goto cleanup;
    }

    // Place action_1 on board_1 using the calculated y_offset_1
    for (int i = 0; i < action_1_copy.rotation->occupied_count; i++) {
        Pos pos = action_1_copy.rotation->occupied[i];
        int place_x = action_1_copy.x_offset + pos.x;
        int place_y = y_offset_1 + pos.y; // Use y_offset_1

        // Check bounds carefully, including buffer zone
        if (place_y >= 0 && place_y < (board_1->size.height + 5) && place_x >= 0 && place_x < board_1->size.width) {
            if (place_y < board_1->size.height) { // Check if within logical height
                board_1->grid[place_y][place_x] = 1;
            } else {
                // Placed in buffer zone - indicates game over after this move
                board_1->grid[place_y][place_x] = 1; // Mark it anyway for simulation
                score_1 = -INFINITY; // Penalize heavily or mark as game over
            }
        } else {
            // Should not happen if findYOffset and isOutOfIndex work correctly
            fprintf(stderr, "Error: Placement out of bounds during assessment simulation (Step 1).\n");
            score_1 = -INFINITY;
            goto cleanup;
        }
    }

    // Clear lines on board_1
    full_lines_1 = getFullLines(board_1);
    if (full_lines_1 == NULL) {
        goto cleanup;
    }
    clearFullLines(board_1, full_lines_1);
    // Check for game over after clearing lines (blocks above logical height)
    for (int y = board_1->size.height; y < board_1->size.height + 5; y++) {
        for (int x = 0; x < board_1->size.width; x++) {
            if (board_1->grid[y][x] != 0) {
                score_1 = -INFINITY; // Game over after step 1
                goto cleanup; // No need to evaluate step 2
            }
        }
    }

    // --- Evaluate Step 2 ---
    // Calculate y_offset for action_2 based on *board_1*
    BlockStatus action_2_copy = *action_2;
    action_2_copy.y_offset = INT_MAX; // Reset y_offset for calculation
    int y_offset_2 = findYOffset(board_1, &action_2_copy); // Use board_1
    if (y_offset_2 == -1) {
        // If placing the second block is impossible, this sequence is bad
        score_2 = -INFINITY;
        goto cleanup; // Or apply a large penalty
    }
    action_2_copy.y_offset = y_offset_2; // Use the calculated y_offset

    // Check bounds for action_2 on board_1
    if (isOutOfIndex(board_1, &action_2_copy)) {
        score_2 = -INFINITY;
        goto cleanup;
    }

    // Create a temporary Game struct for step 2 feature extraction
    Game game_1_state = {
        .config = game->config, // Copy config (shallow is ok here)
        .board = *board_1, // Copy board state *after* step 1 placement and clear
        .score = game->score, // Score doesn't strictly matter for feature extraction
        .upcoming_blocks = NULL, // Not needed for extractFeatures
        .available_statuses_1 = NULL, // Not needed
        .available_statuses_1_count = 0,
        .available_statuses_2 = NULL, // Not needed
        .available_statuses_2_count = 0
    };
    features_2 = extractFeatures(&game_1_state, &action_2_copy); // Pass action_2_copy
    if (features_2 == NULL) {
        goto cleanup;
    }
    score_2 = caculateLinearFunction(model->weights, features_2, k_num_features); // Use k_num_features

cleanup:
    // Free all allocated resources within this function
    if (features_1 != NULL) {
        free(features_1);
    }
    if (features_2 != NULL) {
        free(features_2);
    }
    if (full_lines_1 != NULL) {
        IntListFree(full_lines_1);
    }
    if (board_1 != NULL) {
        GridFree(board_1->grid, &board_1->size);
        free(board_1);
    }

    // Check for invalid scores before combining
    if (score_1 <= -INFINITY || score_2 <= -INFINITY) {
        return -INFINITY;
    }

    return score_1 + score_2; // Return combined score
}

/*
找到最佳的操作策略

:param ctx: 上下文对象
:param actions: 所有可能的动作列表，以 NULL 结尾
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

void freeActionsArray(BlockStatus** actions, int count) {
    if (actions == NULL) return;
    for (int i = 0; i < count; i++) {
        if (actions[i] != NULL) {
            free(actions[i]);
        }
    }
    free((void*)actions);
}

// 放置当前的 upcoming_blocks[0]，执行游戏的一步，同时更新 upcoming_blocks
BlockStatus* runGameStep(Context* ctx, Block* next_block)
{
    Game* game = ctx->game;
    AssessmentModel* model = ctx->model;

    // Free actions from the *previous* step before generating new ones
    freeActionsArray(game->available_statuses_1, game->available_statuses_1_count);
    game->available_statuses_1 = NULL;
    game->available_statuses_1_count = 0;
    freeActionsArray(game->available_statuses_2, game->available_statuses_2_count);
    game->available_statuses_2 = NULL;
    game->available_statuses_2_count = 0;

    if (next_block == NULL) {
        next_block = game->upcoming_blocks[1];
    }

    // 获取所有可能的动作
    BlockStatus** actions_1 = getAvailableActions(game, game->upcoming_blocks[0]);
    game->available_statuses_1 = actions_1;
    game->available_statuses_1_count = 0;
    if (actions_1 != NULL) { // Check if getAvailableActions succeeded
        for (int i = 0; actions_1[i] != NULL; i++) {
            // Check for rotation NULL just in case, though getAvailableActions should filter invalid ones
            if (actions_1[i]->rotation == NULL) continue;
            game->available_statuses_1_count++;
        }
    }

    BlockStatus** actions_2 = getAvailableActions(game, next_block);
    game->available_statuses_2 = actions_2;
    game->available_statuses_2_count = 0; // Reset count before loop
    if (actions_2 != NULL) { // Check if getAvailableActions succeeded
        for (int i = 0; actions_2[i] != NULL; i++) {
             // Check for rotation NULL
            if (actions_2[i]->rotation == NULL) continue;
            game->available_statuses_2_count++;
        }
    }

    // 3. 找到最佳的操作策略

    BlockStatus* best_action = findBestAction(game, model);
    if (best_action == NULL) {
        GameSetEnd(game);
        return NULL;
    }

    BlockStatus best_action_copy = *best_action;

    // 4. 执行操作
    executeAction(game, &best_action_copy, 1);

    // 5. 更新 upcoming_blocks
    if (next_block == NULL) { // 第一次操作，不传入 next_block
        game->upcoming_blocks[0] = game->upcoming_blocks[1];
        game->upcoming_blocks[1] = NULL;
    } else {
        game->upcoming_blocks[0] = game->upcoming_blocks[1];
        game->upcoming_blocks[1] = next_block;
    }

    BlockStatus* result_action = (BlockStatus*)malloc(sizeof(BlockStatus));
    if (result_action == NULL) {
        fprintf(stderr, "Failed to allocate memory for result action\n");
        // Handle error appropriately
        GameSetEnd(game); // Or some other error state
        return NULL;
    }
    *result_action = best_action_copy; // Copy the data

    return result_action; // Caller is responsible for freeing this
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
    printf("Upcoming Blocks: %c, %c\n", game->upcoming_blocks[0]->name, game->upcoming_blocks[1]->name);
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

// Pass Context by pointer to allow modification of the seed
void runRandomTest(Context* ctx)
{
    Game* game = ctx->game;

    srand((unsigned int)time(NULL));

    game->upcoming_blocks[0] = randomBlock();
    game->upcoming_blocks[1] = randomBlock();

    BlockStatus* action_taken = NULL;

    action_taken = runGameStep(ctx, NULL);
    if (action_taken) {
        visualizeStep(ctx->game, action_taken);
        free(action_taken); // Free the returned action
        action_taken = NULL;
    }

    for (size_t i = 1; i < 1e6; i++) {
        if (ctx->game->score < 0) { // Check game over *before* next step
            printf("Game Over. Final Score: %ld\n", -(ctx->game->score)); // Print positive score
            break;
        }
        action_taken = runGameStep(ctx, randomBlock());
        if (action_taken) {
            // visualizeStep(ctx->game, action_taken);
            if (i % 1000 == 0) {
                printf("Step %zu: ", i);
                visualizeStep(ctx->game, action_taken);
            }
            free(action_taken); // Free the returned action
            action_taken = NULL;
        } else if (ctx->game->score < 0) { // Check game over *after* step attempt
            printf("Game Over during step. Final Score: %ld\n", -(ctx->game->score));
            break;
        } else {
            // Handle unexpected NULL return if game didn't end
            fprintf(stderr, "runGameStep returned NULL without ending game.\n");
            break;
        }
    }

    // Final cleanup of any remaining actions if the loop finishes naturally
    freeActionsArray(ctx->game->available_statuses_1, ctx->game->available_statuses_1_count);
    ctx->game->available_statuses_1 = NULL;
    ctx->game->available_statuses_1_count = 0;
    freeActionsArray(ctx->game->available_statuses_2, ctx->game->available_statuses_2_count);
    ctx->game->available_statuses_2 = NULL;
    ctx->game->available_statuses_2_count = 0;
}

int main()
{
    Size grid_size = { 10, 15 };
    Block* upcoming_blocks[2] = { NULL };
    Game game = {
        .config = {
            .awards = (int[]) { 0, 100, 300, 500, 800 },
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
        .weights = (double[]) { -16.8998, 6.0739, -9.0504, -19.9899, -13.3514, -11.0548, -0.6587, -30.1042, -0.8000 }
    };

    Context ctx = {
        .game = &game,
        .model = &assessment_model
    };

    // Initialize the game
    runRandomTest(&ctx);

    // Cleanup allocated memory before exiting
    GridFree(game.board.grid, &game.board.size); 
    // GridFree(game.board.grid, &game.board.size);
    // Free any remaining actions if runRandomTest didn't clean them up (e.g., loop break)
    freeActionsArray(game.available_statuses_1, game.available_statuses_1_count);
    freeActionsArray(game.available_statuses_2, game.available_statuses_2_count);


    return 0;
}
