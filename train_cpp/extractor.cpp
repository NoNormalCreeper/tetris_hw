#include "extractor.h"
#include "constants.h" // Required for getOriginalBlock
#include "models.h"
#include <algorithm> // For std::max_element, std::all_of, std::copy, std::fill
#include <cmath> // For std::abs
#include <numeric> // For std::accumulate
#include <set>
#include <stdexcept>
#include <vector>

// --- Helper function declarations (from game.py, needed here) ---
// These operate on const Board& and don't modify state.
// We might move these to a shared utility header later if needed elsewhere.
bool isCollision(const Board& board, const BlockStatus& action, int y_offset);
bool isOverflow(const Board& board, const BlockStatus& action, int y_offset);
int findYOffset(const Board& board, const BlockStatus& action);
// We also need a non-const version of eliminateLines for the temporary board
int eliminateLines(Board& board); // Declaration, definition might be in game.cpp or here
// We need the Block context for feature extraction now
// const Block* getBlockFromRotation(const BlockRotation* rotation); // Helper declaration

// --- MyDbtFeatureExtractorCpp Implementation ---

int MyDbtFeatureExtractorCpp::calculateLandingHeight(int y_offset) const
{
    // Landing height is y_offset + 1 (since y=0 is the bottom row)
    return y_offset + 1;
}

// Make static as it doesn't depend on instance state
/* static */ bool MyDbtFeatureExtractorCpp::isFullLine(const std::vector<const Block*>& line, int width) const /* const removed */
{
    // Check only the logical width of the board
    if (line.size() < width)
    return std::all_of(line.begin(), line.begin() + width, [](const Block* b) { return b != nullptr; });
}

// Make static as it doesn't depend on instance state and calls static isFullLine
/* static */ std::vector<int> MyDbtFeatureExtractorCpp::getFullLines(const Board& board) const /* const removed */
{
    std::vector<int> full_lines;
    // Iterate only up to the logical height of the board
    for (int y = 0; y < board.size.height; ++y) {
        // Call the static version
        if (MyDbtFeatureExtractorCpp::isFullLine(board.squares[y], board.size.width)) {
            full_lines.push_back(y);
        }
    }
    return full_lines;
}

int MyDbtFeatureExtractorCpp::calculateErodedPieceCells(const Board& board_before_elim, const BlockStatus& action, int y_offset, const std::vector<int>& full_lines) const
{
    if (full_lines.empty()) {
        return 0;
    }
    if (!action.rotation) return 0; // Safety check

    // int block_height = action.rotation->size.height; // block_height not used
    int eliminate_bricks = 0;

    for (const auto& pos : action.rotation->occupied) { // Use ->
        int occupied_y = pos.y + y_offset;
        // Check if this cell's y-coordinate is one of the full lines
        for (int line_y : full_lines) {
            if (occupied_y == line_y) {
                eliminate_bricks++;
                break; // Count each occupied cell at most once per full line it's in
            }
        }
    }

    return full_lines.size() * eliminate_bricks;
}

int MyDbtFeatureExtractorCpp::calculateRowTransitions(const Board& board_after_elim) const
{
    int transitions = 0;
    const auto& squares = board_after_elim.squares;
    int height = board_after_elim.size.height; // Logical height
    int width = board_after_elim.size.width; // Logical width

    for (int y = 0; y < height; ++y) {
        // --- REMOVED WALL CHECK ---
        // // Add transitions for left wall (empty to block)
        // if (squares[y][0] != nullptr) {
        //     transitions++;
        // }
        // --- END REMOVED WALL CHECK ---

        // Check transitions between adjacent cells within the row
        for (int x = 0; x < width - 1; ++x) {
            bool current_filled = (squares[y][x] != nullptr);
            bool next_filled = (squares[y][x + 1] != nullptr);
            if (current_filled != next_filled) {
                transitions++;
            }
        }
        // --- REMOVED WALL CHECK ---
        // // Add transitions for right wall (block to empty)
        // if (squares[y][width - 1] != nullptr) {
        //     transitions++;
        // }
        // --- END REMOVED WALL CHECK ---
    }
    return transitions;
}

// ... existing includes ...

int MyDbtFeatureExtractorCpp::calculateColumnTransitions(const Board& board_after_elim) const
{
    int transitions = 0;
    const auto& squares = board_after_elim.squares;
    int height = board_after_elim.size.height; // Logical height
    int width = board_after_elim.size.width; // Logical width

    for (int x = 0; x < width; ++x) {
        // --- REMOVED WALL CHECK ---
        // // Add transitions for bottom wall (empty to block) - assuming y=0 is bottom
        // if (squares[0][x] != nullptr) {
        //      transitions++;
        // }
        // --- END REMOVED WALL CHECK ---

        // Check transitions between adjacent cells within the column
        for (int y = 0; y < height - 1; ++y) {
            bool current_filled = (squares[y][x] != nullptr);
            bool next_filled = (squares[y + 1][x] != nullptr);
            if (current_filled != next_filled) {
                transitions++;
            }
        }
        // --- REMOVED WALL CHECK ---
        // // Add transitions for top boundary (block to empty) - Check highest logical row
        //  if (height > 0 && squares[height - 1][x] != nullptr) {
        //      // Check if cell *above* logical height is empty (implicitly true)
        //      transitions++;
        //  }
        // --- END REMOVED WALL CHECK ---
    }
    return transitions;
}

// ... rest of the file ...

void MyDbtFeatureExtractorCpp::calculateHolesAndDepth(const Board& board_after_elim, int& holes_count, int& total_hole_depth, int& rows_with_holes_count) const
{
    holes_count = 0;
    total_hole_depth = 0;
    std::set<int> rows_with_holes_set;
    const auto& squares = board_after_elim.squares;
    int height = board_after_elim.size.height; // Logical height
    int width = board_after_elim.size.width; // Logical width
    int grid_height = board_after_elim.getGridHeight(); // Actual grid height including buffer

    for (int x = 0; x < width; ++x) {
        bool block_encountered = false;
        int current_depth_contribution = 0; // Blocks above current cell in this column scan

        // Iterate top-down through the *entire* grid height, including buffer
        for (int y = grid_height - 2; y >= 0; --y) {
            // Only consider cells within the logical board height for hole *detection*
            // but consider blocks *above* the logical height for depth calculation.
            if (squares[y][x] != nullptr) {
                block_encountered = true;
                current_depth_contribution++;
            } else if (block_encountered && y < height) { // It's a hole only if below a block AND within logical height
                holes_count++;
                rows_with_holes_set.insert(y);
                // Depth is blocks strictly above the hole
                total_hole_depth += (current_depth_contribution);
            }
        }
    }
    rows_with_holes_count = rows_with_holes_set.size();
}

int MyDbtFeatureExtractorCpp::calculateBoardWells(const Board& board_after_elim) const
{
    int wells_sum = 0;
    const auto& squares = board_after_elim.squares;
    int height = board_after_elim.size.height; // Logical height
    int width = board_after_elim.size.width; // Logical width

    for (int x = 0; x < width; ++x) {
        int current_well_depth = 0;
        for (int y = height - 1; y >= 0; --y) { // Iterate top-down within logical height
            if (squares[y][x] == nullptr) {
                bool left_filled = (x == 0) || (squares[y][x - 1] != nullptr);
                bool right_filled = (x == width - 1) || (squares[y][x + 1] != nullptr);
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

// ... existing includes ...

std::vector<int> MyDbtFeatureExtractorCpp::calculateColumnHeights(const Board& board_after_elim) const
{
    int width = board_after_elim.size.width;
    int height = board_after_elim.size.height; // Logical height
    std::vector<int> heights(width, 0);
    const auto& squares = board_after_elim.squares;

    for (int x = 0; x < width; ++x) {
        // Iterate top-down within logical height (from y=height-1 down to 0)
        for (int y = height - 1; y >= 0; --y) {
            if (squares[y][x] != nullptr) {
                // Python calculates height as distance from top + 1? No, it's board.size.height - row
                // If top block is at y=13 (height=14), python height = 14-13 = 1.
                // If top block is at y=0 (height=14), python height = 14-0 = 14.
                // Let's match the Python logic: height = logical_height - y_index
                // Wait, Python iterates 0 to height. If block at row 0, height = 14-0=14. If block at row 13, height=14-13=1.
                // C++ iterates height-1 down to 0. If block at y=13, height = 13+1 = 14. If block at y=0, height = 0+1 = 1.
                // The Python logic seems reversed or calculates empty space above.
                // Let's stick to the C++ logic (y+1) as it represents the actual height from the bottom (row 0).
                // The previous analysis of Python was incorrect. Python iterates top-down (0 to height-1).
                // If block found at row `r`, height = `board.size.height - r`.
                // Example: height=14. Block at row 0 -> 14-0=14. Block at row 13 -> 14-13=1.
                // Let's implement the Python logic correctly.
                heights[x] = height - y; // Match Python's calculation
                break; // Found highest block in column
            }
        }
        // If loop finishes, height remains 0 (correct for empty column)
    }
    return heights;
}

// ... rest of the file ...

std::vector<int> MyDbtFeatureExtractorCpp::calculateColumnDifferences(const std::vector<int>& column_heights) const
{
    if (column_heights.size() < 2) {
        return {};
    }
    std::vector<int> differences;
    differences.reserve(column_heights.size() - 1);
    for (size_t i = 0; i < column_heights.size() - 1; ++i) {
        differences.push_back(std::abs(column_heights[i] - column_heights[i + 1]));
    }
    return differences;
}

int MyDbtFeatureExtractorCpp::calculateMaximumHeight(const std::vector<int>& column_heights) const
{
    if (column_heights.empty()) {
        return 0;
    }
    return *std::max_element(column_heights.begin(), column_heights.end());
}

void ensureNoNullLine(Board& board) {
    auto& squares = board.squares;
    for (auto& line : squares) {
        if (line.empty()) {
            line = std::vector<const Block*>(squares[0].size(), nullptr);
        }
    }
}

std::vector<int> MyDbtFeatureExtractorCpp::extractFeatures(const Game& game, const BlockStatus& action) const
{
    if (!action.rotation) {
         throw std::runtime_error("Invalid action: rotation pointer is null.");
    }
    // --- 1. Find Placement Offset ---
    int y_offset = findYOffset(game.board, action);
    if (y_offset == -1) {
        throw std::runtime_error("Invalid action: Cannot place block or causes game over.");
    }

    // --- 2. Simulate Placement & Elimination on ONE Copy ---
    Board board_copy = game.board; // Use Board's copy constructor ONCE

    // Get the original block pointer - needed for placing on board
    const Block* block_to_place = getBlockFromRotation(action.rotation);
    if (!block_to_place) {
        throw std::runtime_error("Could not determine block type during feature extraction.");
    }

    // Place the piece on the copy
    for (const auto& pos : action.rotation->occupied) { // Use ->
        int place_x = action.x_offset + pos.x;
        int place_y = y_offset + pos.y;
        if (place_y >= 0 && place_y < board_copy.getGridHeight() && place_x >= 0 && place_x < board_copy.size.width) {
            board_copy.squares[place_y][place_x] = block_to_place; // Place pointer to original block
        } else {
            throw std::logic_error("Placement out of bounds during feature extraction simulation.");
        }
    }

    // --- 3. Calculate Pre-Elimination Features ---
    Features f;
    // Need full lines *before* elimination for eroded_piece_cells
    std::vector<int> full_lines = MyDbtFeatureExtractorCpp::getFullLines(board_copy); // Assuming static is okay

    f.landing_height = calculateLandingHeight(y_offset);
    f.eroded_piece_cells = calculateErodedPieceCells(board_copy, action, y_offset, full_lines); // Use the copied board

    // --- 4. Eliminate Lines on the Copy ---
    int eliminated_count = eliminateLines(board_copy); // Modifies board_copy

    // --- 5. Calculate Post-Elimination Features ---
    // Use the modified board_copy for these features
    f.row_transitions = calculateRowTransitions(board_copy);
    f.column_transitions = calculateColumnTransitions(board_copy);
    calculateHolesAndDepth(board_copy, f.holes, f.hole_depth, f.rows_with_holes);
    f.board_wells = calculateBoardWells(board_copy);
    f.column_heights = calculateColumnHeights(board_copy);
    f.column_differences = calculateColumnDifferences(f.column_heights);
    f.maximum_height = calculateMaximumHeight(f.column_heights);

    // --- 6. Assemble Feature Vector ---
    // (Assemble vector as before)
    std::vector<int> feature_vector;
    feature_vector.reserve(8); // Reserve for core features initially

    feature_vector.push_back(f.landing_height);
    feature_vector.push_back(f.eroded_piece_cells);
    feature_vector.push_back(f.row_transitions);
    feature_vector.push_back(f.column_transitions);
    feature_vector.push_back(f.holes);
    feature_vector.push_back(f.board_wells);
    feature_vector.push_back(f.hole_depth);
    feature_vector.push_back(f.rows_with_holes);

    // Optional: Add extended features if needed by the model length
    // ...

    return feature_vector;
}


// --- Definition for getBlockFromRotation ---
// Needs access to the global block definitions (e.g., ALL_BLOCKS from constants.h)
const Block* getBlockFromRotation(const BlockRotation* rotation) {
    if (!rotation) return nullptr;
    // Iterate through all defined block types in ALL_BLOCKS (defined in constants.h/cpp)
    for (const auto& block : k_blocks) {
        // Iterate through the rotations of the current block type
        for (const auto& block_rotation : block->rotations) {
            // Compare the address of the input rotation pointer
            // with the address of the rotation within the global list.
            if (&block_rotation == rotation) {
                return block; // Found the parent block
            }
        }
    }
    // This indicates an error or a rotation pointer that doesn't belong
    // to any block in ALL_BLOCKS.
    // Consider throwing an exception or returning nullptr based on expected usage.
     // throw std::runtime_error("getBlockFromRotation: Rotation not found in ALL_BLOCKS.");
    return nullptr; // Rotation not found
}


// --- Definition for eliminateLines (if not defined in game.cpp) ---
// This function modifies the board state.
int eliminateLines(Board& board)
{
    std::vector<int> full_lines;
    int width = board.size.width;
    int height = board.size.height; // Logical height

    // Find full lines within logical height
    for (int y = 0; y < height; ++y) {
        bool line_full = true;
        for (int x = 0; x < width; ++x) {
            if (board.squares[y][x] == nullptr) {
                line_full = false;
                break;
            }
        }
        if (line_full) {
            full_lines.push_back(y);
        }
    }

    int num_full_lines = full_lines.size();
    if (num_full_lines == 0) {
        return 0;
    }

    // Use two pointers (read/write) - modify in place
    int write_y = 0;
    // Iterate through all rows in the grid (including buffer potentially)
    for (int read_y = 0; read_y < board.getGridHeight(); ++read_y) {
        bool is_full = false;
        // Check if read_y is one of the full lines we identified
        for (int full_y : full_lines) {
            if (read_y == full_y) {
                is_full = true;
                break;
            }
        }

        if (!is_full) {
            if (write_y != read_y) {
                // Move row down
                board.squares[write_y] = std::move(board.squares[read_y]);
            }
            write_y++;
        }
        // If it is a full line, we just skip it (don't increment write_y)
    }

    // Clear the lines at the top (up to grid height)
    for (int y = write_y; y < board.getGridHeight(); ++y) {
        std::fill(board.squares[y].begin(), board.squares[y].end(), nullptr);
    }

    ensureNoNullLine(board); // Ensure no null lines remain

    return num_full_lines;
}

// --- Definitions for helper functions (isCollision, isOverflow, findYOffset) ---
// These are needed by the extractor and potentially game logic.
// Place them here or in game.cpp. Let's put them here for now.

bool isCollision(const Board& board, const BlockStatus& action, int y_offset)
{
    if (!action.rotation) return true; // Treat null rotation as collision
    // const auto& rotation = action.rotation; // No longer needed
    for (const auto& pos : action.rotation->occupied) { // Use ->
        int check_x = action.x_offset + pos.x;
        int check_y = y_offset + pos.y;

        // Check bounds (against logical board size)
        if (check_x < 0 || check_x >= board.size.width || check_y < 0) {
            return true; // Collision with walls or floor
        }

        // Check collision with existing blocks within the grid
        // Ensure check_y is within the bounds of the squares vector
        if (check_y < board.getGridHeight() && board.squares[check_y][check_x] != nullptr) {
             return true; // Collision with existing block at the target position
        }

        // OPTIMIZATION?: The check below seems redundant if findYOffset works correctly.
        // findYOffset should find the lowest non-colliding position.
        // If we are checking a position `y_offset`, we only need to know if THAT position collides.
        // The original Python code might have done this differently. Let's remove this inner loop.
        for (auto y = check_y + 1; y < board.getGridHeight(); ++y) {
            if (board.squares[y][check_x] != nullptr) {
                return true; // Collision with existing block below the target position
            }
        }
    }
    return false; // No collision detected for this y_offset
}

// OPTIMIZED isOverflow
bool isOverflow(const Board& board, const BlockStatus& action, int y_offset)
{
    if (!action.rotation) return true; // Treat null rotation as overflow

    // Check if any part of the block *at this y_offset* is at or above the logical height
    for (const auto& pos : action.rotation->occupied) { // Use ->
        int check_y = y_offset + pos.y;
        if (check_y >= board.size.height) { // Check against logical height
            return true; // Overflow detected
        }
    }
    // Also check if y_offset itself is negative (shouldn't happen with findYOffset logic, but good safety check)
    if (y_offset < 0) {
        return true;
    }

    return false; // No overflow detected at this y_offset
}


// Corrected findYOffset implementation (using optimized isCollision and isOverflow)
int findYOffset(const Board& board, const BlockStatus& action)
{
    if (!action.rotation) return -1; // Cannot place null rotation

    // Start checking from y_offset = 0 upwards.
    for (int y = 0; ; ++y) {
        // Check for collision at the potential next position (y-1)
        // Or check collision at current position y and find the first non-colliding one?
        // Let's try finding the lowest position `y` such that placing the block there *collides*
        // or overflows. The correct landing position is then `y-1`.

        bool collides_at_y = isCollision(board, action, y);
        bool overflows_at_y = isOverflow(board, action, y); // Check overflow based on y

        if (!collides_at_y && !overflows_at_y) {
            return y;
            // return landing_y; // Valid landing position found.
        }

        // If we check beyond a reasonable height without collision/overflow, something is wrong.
        // Add a safeguard. Grid height should be sufficient.
        if (y >= board.getGridHeight()) {
             // This implies the block can fall indefinitely without collision/overflow? Error.
             // Or maybe it landed exactly at the bottom without collision?
             // Let's reconsider the loop logic.

             // Alternative logic: Iterate downwards from top? No, that's less efficient.
             // Iterate upwards, find the *first* position `y` that is *invalid* (collision or overflow).
             // The landing position is `y-1`.

             // If the loop reaches here, it means no collision/overflow was found up to grid height.
             // This shouldn't happen if the board has a floor or existing blocks.
             // Let's assume an error or an edge case not handled.
             return -1; // Indicate failure to find a position.
        }
    }
    // Should be unreachable
    return -1;
}