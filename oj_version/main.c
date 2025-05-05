
// #include <cstddef>
#include <limits.h>
// #include <cstddef>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    short** grid = (short**)calloc(height + buffer, sizeof(short*));
    if (grid == NULL) {
        fprintf(stderr, "Failed to alloc grids\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < height; i++) {
        grid[i] = (short*)calloc(width, sizeof(short));
        if (grid[i] == NULL) {
            fprintf(stderr, "Failed to alloc grids\n");
            exit(EXIT_FAILURE);
        }
    }

    return grid;
}

void GridFree(short** grid, Size* size)
{
    for (int i = 0; i < size->height; i++) {
        free(grid[i]);
    }
    free((void*)grid);
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
    holes_count = 0;
    total_hole_depth = 0;
    rows_with_holes_count = 0;
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

IntList* getFullLines(const Board* board);

int* extractFeatures(const Game* game, const BlockStatus* action)
{
    int* features = (int*)malloc((k_num_features + 5) * sizeof(int));
    if (features == NULL) {
        fprintf(stderr, "Failed to alloc features\n");
        exit(EXIT_FAILURE);
    }

    Board board_copy = game->board; // Use Board's copy constructor ONCE

    // Place the piece on the copy
    for (int i = 0; i < action->rotation->occupied_count; i++) {
        Pos pos = action->rotation->occupied[i];
        int place_x = action->x_offset + pos.x;
        int place_y = action->y_offset + pos.y;

        if (place_y >= 0 && place_y < board_copy.size.height && place_x >= 0 && place_x < board_copy.size.width) {
            board_copy.grid[place_y][place_x] = 1; // Place pointer to original block
        } else {
            fprintf(stderr, "Placement out of bounds during feature extraction simulation.\n");
            exit(EXIT_FAILURE);
        }
    }

    IntList* full_lines = IntListAlloc(board_copy.size.height);
    full_lines = getFullLines(&board_copy);

    // 1. Landing Height
    features[0] = action->y_offset + 1;

    // 2. Eroded Cells
    features[1] = calcErodedPieceCells(action, full_lines);

    // execute the elimination
    clearFullLines(&board_copy, full_lines);

    // 3. Row Transitions
    features[2] = calcRowTransitions(&board_copy);

    // 4. Column Transitions
    features[3] = calcColumnTransitions(&board_copy);

    // 5. Holes
    features[4] = 0;
    // 6. Board Wells
    features[5] = 0;
    // 7. Hole Depth
    features[6] = 0;
    // 8. Rows with Holes
    features[7] = 0;

    calcHolesAndDepth(&board_copy, &features[4], &features[6], &features[7]);

    IntListFree(full_lines);

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

        for (int y_itr = check_y + 1; y_itr < board->size.height; y_itr++) {
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

    // 双指针 (r/w)
    int write_y = 0;

    for (int read_y = 0; read_y < board->size.height + 5; read_y++) {
        int is_full = 0;

        for (size_t i = 0; i < num_full_lines; i++) {
            if (read_y == full_lines->data[i]) {
                is_full = 1;
                break;
            }
        }

        if (!is_full) {
            if (write_y != read_y) {
                // 向下移动
                board->grid[write_y] = board->grid[read_y];
            }
            write_y++;
        }
    }

    // clear top lines
    for (int y = write_y; y < board->size.height + 5; y++) {
        memset(board->grid[y], 0, sizeof(short) * board->size.width);
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
        return y_offset;
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
    size_t actions_size = (width * block->rotations_count) + 1;
    BlockStatus** actions = (BlockStatus**)malloc(sizeof(BlockStatus*) * actions_size);
    for (size_t i = 0; i < actions_size; i++) {
        actions[i] = NULL;
    }

    for (int i = 0; i < block->rotations_count; i++) {
        BlockRotation* rotation = &block->rotations[i];
        for (int x = 0; x <= width - rotation->size.width; x++) {
            BlockStatus* action = (BlockStatus*)malloc(sizeof(BlockStatus));
            action = &(BlockStatus) {
                .x_offset = x,
                .rotation = rotation,
                .y_offset = INT_MAX,
                .assement_score = -INFINITY
            };
            action->y_offset = findYOffset(&game->board, action);
            if (action->y_offset == -1) {
                free(action);
            } else {
                actions[actions_count] = action;
                actions_count++;
            }
        }
    }

    return actions;
}

double assessmentTwoActions(Game* game, BlockStatus* action_1, BlockStatus* action_2, AssessmentModel* model)
{
    double score_1 = -INFINITY;
    double score_2 = -INFINITY;

    // 判断
    if (action_1->rotation == NULL) {
        return -INFINITY;
    }
    if (action_2->rotation == NULL) {
        return -INFINITY;
    }

    if (isOutOfIndex(&game->board, action_1)) {
        return -INFINITY;
    }

    if (isOutOfIndex(&game->board, action_2)) {
        return -INFINITY;
    }

    // 评估第一步
    int* features_1 = extractFeatures(game, action_1);
    score_1 = caculateLinearFunction(model->weights, features_1, model->length - 1);
    if (score_1 == -INFINITY) {
        return -INFINITY;
    }

    Board board_1 = game->board; // Copy only the board
    int y_offset_1 = findYOffset(&game->board, action_1); // Find offset on original board
    if (y_offset_1 == -1) {
        return -INFINITY;
    }

    // 第一步放置
    for (int i = 0; i < action_1->rotation->occupied_count; i++) {
        Pos pos = action_1->rotation->occupied[i];
        int place_x = action_1->x_offset + pos.x;
        int place_y = y_offset_1 + pos.y;

        if (place_y >= 0 && place_y < board_1.size.height && place_x >= 0 && place_x < board_1.size.width) {
            board_1.grid[place_y][place_x] = 1;
        } else { // error
            return -INFINITY;
        }
    }

    clearFullLines(&board_1, getFullLines(&board_1));

    // check game over
    for (int y = board_1.size.height; y < board_1.size.height + 5; y++) {
        for (int x = 0; x < board_1.size.width; x++) {
            if (board_1.grid[y][x] != 0) {
                return -INFINITY;
            }
        }
    }

    // step 2
    Game game_1 = {
        .config = game->config,
        .board = board_1,
        .upcoming_blocks = game->upcoming_blocks,
        .available_statuses_1 = game->available_statuses_1,
        .available_statuses_1_count = game->available_statuses_1_count,
        .available_statuses_2 = game->available_statuses_2,
        .available_statuses_2_count = game->available_statuses_2_count
    };
    int* features_2 = extractFeatures(&game_1, action_1);
    score_2 = caculateLinearFunction(model->weights, features_2, model->length - 1);
    if (score_2 == -INFINITY) {
        return -INFINITY;
    }

    int y_offset_2 = findYOffset(&board_1, action_2); // Find offset on original board
    if (y_offset_2 == -1) {
        return -INFINITY;
    }

    return score_1 + score_2;
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

    BlockStatus* actions_1 = game->available_statuses_1;
    Block* block_2 = game->upcoming_blocks[1];

    for (int i = 0; i < game->available_statuses_1_count; i++) {
        BlockStatus* action_1 = &actions_1[i];
        if (action_1->rotation == NULL) {
            continue;
        }

        for (int j = 0; j < game->available_statuses_2_count; j++) {
            BlockStatus* action_2 = &game->available_statuses_2[j];
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

// 放置当前的 upcoming_blocks[0]，执行游戏的一步，同时更新 upcoming_blocks
BlockStatus* runGameStep(Context* ctx, Block* next_block)
{
    Game* game = ctx->game;
    AssessmentModel* model = ctx->model;

    // 获取所有可能的动作
    BlockStatus** actions_1 = getAvailableActions(game, game->upcoming_blocks[0]);
    game->available_statuses_1 = actions_1;
    game->available_statuses_1_count = 0;
    for (int i = 0;; i++) {
        if (actions_1[i] == NULL || actions_1[i]->rotation == NULL) {
            break;
        }
        game->available_statuses_1_count++;
    }

    BlockStatus** actions_2 = getAvailableActions(game, next_block);
    game->available_statuses_2 = actions_2;
    for (int i = 0;; i++) {
        if (actions_2[i] == NULL || actions_2[i]->rotation == NULL) {
            break;
        }
        game->available_statuses_2_count++;
    }

    // 3. 找到最佳的操作策略

    BlockStatus* best_action = findBestAction(game, model);
    if (best_action == NULL) {
        GameSetEnd(game);
        return NULL;
    }

    // 4. 执行操作
    executeAction(game, best_action, 1);

    // 5. 更新 upcoming_blocks
    game->upcoming_blocks[0] = game->upcoming_blocks[1];
    game->upcoming_blocks[1] = next_block;

    return best_action;
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
    printf("Action: %c at (%d, %d)\n", action->rotation->label, action->x_offset, action->y_offset);
    printf("Board:\n");
    for (int y = 0; y < game->board.size.height; y++) {
        for (int x = 0; x < game->board.size.width; x++) {
            printf("%d ", game->board.grid[y][x]);
        }
        printf("\n");
    }
}

int main()
{
    Size grid_size = { 10, 15 };
    Game game = {
        .config = {
            .awards = (int[]) { 0, 100, 300, 500, 800 },
            .available_blocks = (Block*)k_blocks,
            .available_blocks_count = k_blocks_count },
        .board = { .size = grid_size, .grid = GridAlloc(&grid_size) },
        .upcoming_blocks = NULL,
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

    return 0;
}