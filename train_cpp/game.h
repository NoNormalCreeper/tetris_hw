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
std::vector<const Block*> getNewUpcoming(Game& game); // Modifies game's random state potentially. Returns pointers.

// Generates all possible actions (placements/rotations) for a given block.
std::vector<BlockStatus> getAllActions(const Block& block, int board_width);

// Finds the best action from a list based on the assessment model.
// Throws std::runtime_error if no valid action is found.
BlockStatus findBestAction(const Game& game, const std::vector<BlockStatus>& actions, const AssessmentModel& model);

BlockStatus findBestActionV2(const Game& game, const std::vector<BlockStatus>& actions1, const Block& block2, const AssessmentModel& model);

// Executes the chosen action, modifying the game state directly.
// Returns the y_offset where the block landed, or -1 if the action caused game over.
// Throws std::runtime_error on immediate game over (overflow).
int executeAction(Game& game, const BlockStatus& action);

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