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

int findYOffset(const Board* board, BlockStatus* action);

void visualizeStep(const Game* game, const BlockStatus* action);

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
        // Ensure that both source and destination row pointers are valid
        // GridAlloc should ensure new_board->grid[i] is allocated.
        // We assume board->grid[i] is also valid.
        if (board->grid[i] && new_board->grid[i]) {
            memcpy(new_board->grid[i], board->grid[i], new_board->size.width * sizeof(short));
        }
        // Else: If pointers could be NULL, error handling or skipping might be needed,
        // but based on GridAlloc, new_board->grid[i] should be fine.
    }

    return new_board;
}

IntList* getFullLines(const Board* board);

void extractFeatures(const Board* board_before_action, const BlockStatus* action_with_y_offset, Board** out_board_after_action_and_clear, int* out_game_over_flag, int* out_features)
{
    // Initialize outputs
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
        *out_game_over_flag = 1; // Critical allocation failure
        return;
    }

    // Place the piece on the simulated_board
    // action_with_y_offset->y_offset is assumed to be correctly calculated and validated by caller
    for (int i = 0; i < action_with_y_offset->rotation->occupied_count; i++) {
        Pos pos = action_with_y_offset->rotation->occupied[i];
        int place_x = action_with_y_offset->x_offset + pos.x;
        int place_y = action_with_y_offset->y_offset + pos.y;

        // Check bounds during placement (should be pre-validated by findYOffset and isOutOfIndex)
        // but as a safeguard, especially against y_offset issues:
        if (place_x < 0 || place_x >= simulated_board->size.width || place_y < 0) {
             // This case should ideally not be reached if y_offset is valid
            *out_game_over_flag = 1;
            // Free simulated_board before returning if it won't be passed out
            GridFree(simulated_board->grid, &simulated_board->size);
            free(simulated_board);
            return;
        }
        // If place_y >= simulated_board->size.height, it's in the buffer.
        // This is not an immediate game over; depends on whether it clears.
        if (place_y < simulated_board->size.height + 5) { // Ensure it's within allocated grid
             simulated_board->grid[place_y][place_x] = 1;
        } else {
            // Piece placed completely out of allocated buffer - severe error
            *out_game_over_flag = 1;
            GridFree(simulated_board->grid, &simulated_board->size);
            free(simulated_board);
            return;
        }
    }

    IntList* full_lines = getFullLines(simulated_board);

    // 1. Landing Height
    if (out_features) out_features[0] = action_with_y_offset->y_offset + action_with_y_offset->rotation->size.height;


    // 2. Eroded Cells
    if (out_features) out_features[1] = calcErodedPieceCells(action_with_y_offset, full_lines);

    // Execute the elimination
    clearFullLines(simulated_board, full_lines);
    IntListFree(full_lines); // Free full_lines after use

    // Check for game over: if any blocks are in the buffer zone AFTER clearing
    for (int y = simulated_board->size.height; y < simulated_board->size.height + 5; y++) {
        for (int x = 0; x < simulated_board->size.width; x++) {
            if (simulated_board->grid[y][x] != 0) {
                *out_game_over_flag = 1;
                goto post_game_over_check; // Exit loops
            }
        }
    }
post_game_over_check:;

    // If game over, features might be less relevant or could be set to indicate a bad state
    if (*out_game_over_flag && out_features) {
        // Optionally, set features to worst-case values if game over
        // For now, they'll have values from before game over was fully determined,
        // or zeros if out_features was NULL. The game_over_flag is the primary indicator.
    }

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

    // Check initial actions
    if (action_1 == NULL || action_1->rotation == NULL) {
        return -INFINITY;
    }
    if (action_2 == NULL || action_2->rotation == NULL) {
        return -INFINITY;
    }

    // --- Step 1 ---
    BlockStatus action_1_eval = *action_1; // Use a copy for y_offset calculation
    action_1_eval.y_offset = INT_MAX;      // Reset y_offset for fresh calculation
    int y_offset_1 = findYOffset(&game->board, &action_1_eval);

    if (y_offset_1 == -1) { // findYOffset itself checks for out-of-bounds/collision
        return -INFINITY; 
    }
    action_1_eval.y_offset = y_offset_1; // Set the calculated y_offset

    // No need for explicit isOutOfIndex here, findYOffset handles it via isCollision->isOutOfIndex and isOverflow

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
    BlockStatus action_2_eval = *action_2; // Use a copy
    action_2_eval.y_offset = INT_MAX;      // Reset y_offset for fresh calculation
    int y_offset_2 = findYOffset(board_after_step1, &action_2_eval); // Use board_after_step1

    if (y_offset_2 == -1) {
        GridFree(board_after_step1->grid, &board_after_step1->size);
        free(board_after_step1);
        return -INFINITY;
    }
    action_2_eval.y_offset = y_offset_2;

    int game_over_step2 = 0;
    // Pass NULL for out_board_after_action_and_clear as we don't need the board state after step 2
    extractFeatures(board_after_step1, &action_2_eval, NULL, &game_over_step2, features_2_data);
    
    // Free board_after_step1 as it's no longer needed
    GridFree(board_after_step1->grid, &board_after_step1->size);
    free(board_after_step1);
    board_after_step1 = NULL; // Good practice

    if (game_over_step2) {
        return -INFINITY;
    }
    score_2 = caculateLinearFunction(model->weights, features_2_data, k_num_features);

    // Check for invalid scores before combining (e.g. if features led to NaN or Inf)
    // game_over_step flags should primarily gate this.
    // If scores can become -INFINITY from non-game-over states, this check is useful.
    if (score_1 <= -INFINITY || score_2 <= -INFINITY) { // Check against -INFINITY which is used for bad states
        return -INFINITY;
    }
    
    return score_1 + score_2; // Return combined score
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

BlockStatus* findBestSingleAction(Game* game, AssessmentModel* model) {
    double best_score = -INFINITY;
    BlockStatus* best_action = NULL;

    BlockStatus** actions = game->available_statuses_1;
    for (int i = 0; i < game->available_statuses_1_count; i++) {
        BlockStatus* action = actions[i];
        if (action->rotation == NULL) {
            continue;
        }

        BlockStatus current_action_eval = *action; // Use a copy
        current_action_eval.y_offset = INT_MAX; // Reset y_offset for fresh calculation
        
        int y_offset = findYOffset(&game->board, &current_action_eval);
        if (y_offset == -1) {
            continue; // Invalid placement
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
        
        // Free the board created by extractFeatures if it's not NULL
        if (board_after_action) {
            GridFree(board_after_action->grid, &board_after_action->size);
            free(board_after_action);
        }
    }

    return best_action;
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
BlockStatus* runGameStep(Context* ctx, Block* next_block, int mode)
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

    if (mode == 2) {
        BlockStatus** actions_2 = getAvailableActions(game, next_block);
        game->available_statuses_2 = actions_2;
        game->available_statuses_2_count = 0; // Reset count before loop
        if (actions_2 != NULL) { // Check if getAvailableActions succeeded
            for (int i = 0; actions_2[i] != NULL; i++) {
                // Check for rotation NULL
                if (actions_2[i]->rotation == NULL)
                    continue;
                game->available_statuses_2_count++;
            }
        }
    }

    // 3. 找到最佳的操作策略

    BlockStatus* best_action = NULL;
    if (mode == 1) {
        best_action = findBestSingleAction(game, model);
    } else if (mode == 2) {
        best_action = findBestAction(game, model);
    } else {
        fprintf(stderr, "Invalid mode: %d\n", mode);
        return NULL; // Invalid mode
    }
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
        free(action_taken); // Free the returned action
        action_taken = NULL;
    }

    for (size_t i = 1; i < 1e6; i++) {
        if (ctx->game->score < 0) { // Check game over *before* next step
            printf("Game Over. Final Score: %ld\n", -(ctx->game->score)); // Print positive score
            break;
        }
        action_taken = runGameStep(ctx, randomBlock(), mode);
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
            printf("%zu steps taken.\n", i);
            printf("%ld\n", labs(ctx->game->score));
            // visualizeStep(game, action_taken);
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

const Block* findBlock(char name) {
    for (int i = 0; i < k_blocks_count; i++) {
        if (k_blocks[i]->name == name) {
            return k_blocks[i];
        }
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    if (argc > 1) {
        // printf("Use arg: %s\n", argv[1]);
    }

    Size grid_size = { 10, 15 };
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
        .weights = (double[]) { -13.9920, 4.9462, -7.8690, -17.6028, -12.8035, -11.8649, -0.8765, -31.3253, -1.8517 }
    };

    Context ctx = {
        .game = &game,
        .model = &assessment_model
    };

    // Initialize the game
    if (argc == 1) {
        runRandomTest(&ctx, 1);
    } else if (strcmp(argv[1], "single") == 0) {
        runRandomTest(&ctx, 1);
    } else if (strcmp(argv[1], "double") == 0) {
        runRandomTest(&ctx, 2);
    } else if (strcmp(argv[1], "oj") == 0){
        char b1 = 0, b2 = 0;
        // fflush(stdin);
        scanf("%c%c", &b1, &b2);
        // fflush(stdin);
        Block* block1 = findBlock(b1);
        Block* block2 = findBlock(b2);
        if (block1 == NULL || block2 == NULL) {
            fprintf(stderr, "Invalid block names: %c, %c\n", b1, b2);
            fflush(stdout);
            return 1;
        }

        game.upcoming_blocks[0] = block1;
        game.upcoming_blocks[1] = block2;

        Game* game = ctx.game;

        BlockStatus* action_taken = NULL;

        action_taken = runGameStep(&ctx, NULL, 1);
        if (action_taken) {
            printf("%d %d\n%ld\n", action_taken->x_offset, action_taken->rotation->label, labs(ctx.game->score));
            fflush(stdout);
            free(action_taken); // Free the returned action
            action_taken = NULL;
        }

        for (size_t i = 1;; i++) {
            if (!b2) {
                scanf("%c", &b1);
                while (b1 == '\n') {
                    scanf("%c", &b1);
                }
                if (ctx.game->score < 0) { // Check game over *before* next step
                    printf("%ld\n", labs(ctx.game->score)); // Print positive score
                    fflush(stdout);
                }
                // printf("CURRENT Upcoming: %c, %c\n", game->upcoming_blocks[0]->name, game->upcoming_blocks[1]->name);
                action_taken = runGameStep(&ctx, findBlock(b1), 1);
                if (action_taken) {
                    printf("%d %d\n%ld\n", action_taken->x_offset, action_taken->rotation->label, labs(ctx.game->score));
                    fflush(stdout);
                    free(action_taken); // Free the returned action
                    action_taken = NULL;
                } else if (ctx.game->score < 0) { // Check game over *after* step attempt
                    printf("%ld\n", labs(ctx.game->score));
                    break;
                } else {
                    // Handle unexpected NULL return if game didn't end
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
    } else {
        runRandomTest(&ctx, 0);
    }
    // Cleanup allocated memory before exiting
    GridFree(game.board.grid, &game.board.size); 
    // GridFree(game.board.grid, &game.board.size);
    // Free any remaining actions if runRandomTest didn't clean them up (e.g., loop break)
    freeActionsArray(game.available_statuses_1, game.available_statuses_1_count);
    freeActionsArray(game.available_statuses_2, game.available_statuses_2_count);


    return 0;
}
