#ifndef GAME_H
#define GAME_H

#include "models.h"
#include <vector>
#include <utility> // For std::pair

// --- Function Declarations ---

// Creates a new game instance with default settings.
Game createNewGame();

// Calculates the linear combination of weights and features.
double calculateLinearFunction(const std::vector<double>& weights, const std::vector<int>& features);

// Gets the next two upcoming blocks based on game state and randomness.
std::vector<Block> getNewUpcoming(Game& game); // Modifies game's random state potentially

// Generates all possible actions (placements/rotations) for a given block.
std::vector<BlockStatus> getAllActions(const Block& block, int board_width);

// Finds the best action from a list based on the assessment model.
// Throws std::runtime_error if no valid action is found.
BlockStatus findBestAction(const Game& game, const std::vector<BlockStatus>& actions, const AssessmentModel& model);

// Simulates executing an action on a copy of the game state.
// Returns the resulting game state and the final y_offset of the placed block.
// Throws std::runtime_error if the action leads to game over immediately (overflow).
std::pair<Game, int> executeAction(const Game& current_game, const BlockStatus& action);

// Runs a complete game simulation using the provided context.
// Returns the final score.
int runGame(Context& ctx); // Modifies the context

// Runs a game specifically for training, taking weights directly.
// Returns the final score.
int runGameForTraining(const std::vector<double>& weights);


// --- Helper function declarations (if needed externally, otherwise keep static in game.cpp) ---
// int findYOffset(const Board& board, const BlockStatus& action); // Already in extractor.cpp
// int eliminateLines(Board& board); // Already in extractor.cpp


#endif // GAME_H