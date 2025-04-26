#include "visualize.h"
#include "constants.h" // For k_blocks in visualizeBoard
#include "models.h"
#include <iomanip> // For potential formatting
#include <iostream>
#include <string>
#include <vector>

// --- Color Helper Implementations ---
// ANSI escape codes for colors
const std::string RESET_COLOR = "\033[0m";
const std::string CYAN_COLOR = "\033[96m";
const std::string GREEN_COLOR = "\033[92m";
const std::string YELLOW_COLOR = "\033[93m";

std::string cyan(const std::string& text)
{
    return CYAN_COLOR + text + RESET_COLOR;
}

std::string green(const std::string& text)
{
    return GREEN_COLOR + text + RESET_COLOR;
}

std::string yellow(const std::string& text)
{
    return YELLOW_COLOR + text + RESET_COLOR;
}

// --- Visualization Function Implementations ---

void visualizeOccupied(const std::vector<Position>& occupied, const Size& size)
{
    if (size.width <= 0 || size.height <= 0)
        return; // Avoid invalid sizes

    std::vector<std::vector<char>> grid(size.height, std::vector<char>(size.width, ' '));

    for (const auto& pos : occupied) {
        // Check bounds before accessing grid
        if (pos.x >= 0 && pos.x < size.width && pos.y >= 0 && pos.y < size.height) {
            // Adjust y-coordinate to flip the grid vertically for printing
            grid[size.height - 1 - pos.y][pos.x] = 'X';
        }
    }

    for (const auto& row : grid) {
        std::cout << cyan("  ");
        for (size_t i = 0; i < row.size(); ++i) {
            std::cout << row[i] << (i == row.size() - 1 ? "" : " ");
        }
        std::cout << std::endl;
    }
}

void visualizeBlock(const Block& block)
{
    std::cout << "Block Name: " << block.name << std::endl;
    std::cout << "Block Label: " << block.label << std::endl;
    std::cout << "Status Count: " << block.status_count << std::endl;
    std::cout << "Rotations:" << std::endl;
    for (const auto& rotation : block.rotations) {
        std::cout << "  Rotation Label: " << rotation.label << std::endl;
        std::cout << "  Size: " << rotation.size.width << " x " << rotation.size.height << std::endl;
        std::cout << "  Occupied Visualization:" << std::endl;
        visualizeOccupied(rotation.occupied, rotation.size);
    }
}

void visualizeBlocks(const std::vector<const Block*>& blocks)
{
    for (const auto* block_ptr : blocks) {
        if (block_ptr) {
            visualizeBlock(*block_ptr);
            std::cout << std::string(20, '-') << std::endl
                      << std::endl;
        }
    }
}

void visualizeBoard(const Board& board, const BlockStatus& action, int y_offset)
{
    // Create a set of positions for the falling block for quick lookup
    std::vector<Position> falling_block_pos;
    falling_block_pos.reserve(action.rotation.occupied.size());
    for (const auto& pos : action.rotation.occupied) {
        falling_block_pos.emplace_back(action.x_offset + pos.x, y_offset + pos.y);
    }

    auto is_falling_block = [&](int x, int y) {
        for (const auto& p : falling_block_pos) {
            if (p.x == x && p.y == y)
                return true;
        }
        return false;
    };

    // Board borders
    std::string horizontal_border = cyan("+" + std::string(board.size.width * 2 - 1, '-') + "+");

    std::cout << horizontal_border << std::endl;

    // Print board rows top-down (from logical height - 1 down to 0)
    for (int y = board.size.height - 1; y >= 0; --y) {
        std::cout << cyan("|"); // Left border
        bool line_can_clear = board.canClearLine(y);

        for (int x = 0; x < board.size.width; ++x) {
            std::string cell_content = " "; // Default empty
            std::string cell_color = ""; // Default no color

            if (is_falling_block(x, y)) {
                cell_content = "X";
                cell_color = GREEN_COLOR;
            } else if (y < board.getGridHeight() && board.squares[y][x] != nullptr) {
                cell_content = board.squares[y][x]->label; // Use block label
                // Optionally add color based on block type later
            }

            // Apply color
            std::string output_cell = cell_content;
            if (!cell_color.empty()) {
                output_cell = cell_color + cell_content + RESET_COLOR;
            }
            // Apply yellow if the line can be cleared (overrides other colors for the whole line)
            if (line_can_clear) {
                output_cell = YELLOW_COLOR + cell_content + RESET_COLOR;
            }

            std::cout << (x == 0 ? "" : " ") << output_cell;
        }
        std::cout << " " << cyan("|") << std::endl; // Right border
    }

    std::cout << horizontal_border << std::endl;

    // Print bottom coordinates
    std::cout << "  ";
    for (int x = 0; x < board.size.width; ++x) {
        std::cout << x << (x == board.size.width - 1 ? "" : " ");
    }
    std::cout << std::endl;
}

void visualizeAction(const Game& game, const BlockStatus& action)
{
    std::cout << "Current Score: " << game.score << std::endl;
    if (!game.upcoming_blocks.empty()) {
        std::cout << "Current Block: " << game.upcoming_blocks[0].label << std::endl;
    } else {
        std::cout << "Current Block: (None)" << std::endl;
    }
    std::cout << "Best Action: degree=" << action.rotation.label
              << ", x=" << action.x_offset;
    if (action.assessment_score.has_value()) {
        std::cout << ", score=" << action.assessment_score.value();
    } else {
        std::cout << ", score=(N/A)";
    }
    std::cout << std::endl;
    // visualizeOccupied(action.rotation.occupied, action.rotation.size); // Optional: visualize the shape itself
}

void visualizeGame(const Game& game, const BlockStatus& action, int y_offset, bool show_board)
{
    std::cout << "Current Action:" << std::endl;
    visualizeAction(game, action);
    if (show_board) {
        std::cout << "Board State:" << std::endl;
        // Need y_offset to show placement correctly
        visualizeBoard(game.board, action, y_offset);
        std::cout << "Upcoming Blocks: ";
        for (const auto& block : game.upcoming_blocks) {
            std::cout << block.label << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::string(20, '-') << std::endl;
}

void visualizeDbtFeature(const std::vector<int>& features)
{
    std::cout << "DBT Features:" << std::endl;
    if (features.size() < 8) {
        std::cout << "  (Insufficient features provided)" << std::endl;
        return;
    }
    std::vector<std::string> feature_texts = {
        "Landing Height: " + std::to_string(features[0]),
        "Eroded Cells: " + std::to_string(features[1]),
        "Row Transitions: " + std::to_string(features[2]),
        "Col Transitions: " + std::to_string(features[3]),
        "Holes: " + std::to_string(features[4]),
        "Board Wells: " + std::to_string(features[5]),
        "Hole Depth: " + std::to_string(features[6]),
        "Rows w/ Holes: " + std::to_string(features[7])
    };

    // Assuming the rest are column heights, differences, max height
    int board_width = 10; // Assuming standard width
    int col_heights_start = 8;
    int col_diffs_start = col_heights_start + board_width;
    int max_height_idx = col_diffs_start + (board_width - 1);

    std::cout << "  ";
    for (size_t i = 0; i < feature_texts.size(); ++i) {
        std::cout << feature_texts[i] << (i == feature_texts.size() - 1 ? "" : " | ");
    }
    std::cout << std::endl;

    if (features.size() > col_heights_start) {
        std::cout << "  Column Heights: ";
        for (int i = col_heights_start; i < col_diffs_start && i < features.size(); ++i) {
            std::cout << features[i] << " ";
        }
        std::cout << std::endl;
    }
    if (features.size() > col_diffs_start) {
        std::cout << "  Column Diffs: ";
        for (int i = col_diffs_start; i < max_height_idx && i < features.size(); ++i) {
            std::cout << features[i] << " ";
        }
        std::cout << std::endl;
    }
    if (features.size() > max_height_idx) {
        std::cout << "  Max Height: " << features[max_height_idx] << std::endl;
    }
}