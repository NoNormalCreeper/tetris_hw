#ifndef VISUALIZE_H
#define VISUALIZE_H

#include "models.h" // Includes Game, Board, BlockStatus, Block, Position, Size
#include <vector>
#include <string>

// --- Color Helper Declarations ---
std::string cyan(const std::string& text);
std::string green(const std::string& text);
std::string yellow(const std::string& text);

// --- Visualization Function Declarations ---

// Visualizes the occupied cells of a block rotation.
void visualizeOccupied(const std::vector<Position>& occupied, const Size& size);

// Visualizes a single block type with all its rotations.
void visualizeBlock(const Block& block);

// Visualizes a list of block types.
void visualizeBlocks(const std::vector<const Block*>& blocks); // Takes vector of pointers

// Visualizes the game board, highlighting the potential placement of an action.
void visualizeBoard(const Board& board, const BlockStatus& action, int y_offset);

// Visualizes information about the current action.
void visualizeAction(const Game& game, const BlockStatus& action);

// Visualizes the overall game state including the board and action.
void visualizeGame(const Game& game, const BlockStatus& action, int y_offset, bool show_board = false);

// Visualizes the calculated DBT feature values.
void visualizeDbtFeature(const std::vector<int>& features);


#endif // VISUALIZE_H