#include "game.h"
#include "constants.h" // Include k_blocks declaration
#include "extractor.h" // Include MyDbtFeatureExtractorCpp
#include "models.h"
#include <algorithm> // For std::max_element
#include <limits> // For std::numeric_limits
#include <memory> // For std::make_unique
#include <numeric> // For std::inner_product
#include <random> // For random number generation
#include <stdexcept>
#include <utility> // For std::pair
#include <vector>

// --- Random Number Generation ---
// Use a static generator for reproducibility if needed, or thread-local for multi-threading.
// For simplicity, a global static generator here.
static std::mt19937 rng(std::random_device {}());

// Helper to get a random block
const Block* getRandomBlock()
{
    if (k_blocks.empty()) {
        throw std::runtime_error("k_blocks is empty. Cannot select random block.");
    }
    std::uniform_int_distribution<size_t> dist(0, k_blocks.size() - 1);
    return k_blocks[dist(rng)];
}

// --- Function Implementations ---

Game createNewGame()
{
    // Ensure k_blocks is usable (defined in constants.cpp)
    std::vector<Block> available_blocks_copy;
    available_blocks_copy.reserve(k_blocks.size());
    for (const Block* block_ptr : k_blocks) {
        if (block_ptr) {
            available_blocks_copy.push_back(*block_ptr); // Copy the blocks
        }
    }

    GameConfig config({ 1.0, 3.0, 5.0, 8.0 }, std::move(available_blocks_copy)); // Awards and available blocks
    Board board(Size(10, 14)); // Standard Tetris size (adjust height as needed)
    return Game(std::move(config), std::move(board), 0, {}); // Initial score 0, empty upcoming
}

double calculateLinearFunction(const std::vector<double>& weights, const std::vector<int>& features)
{
    if (weights.size() != features.size()) {
        throw std::invalid_argument("Weights and features vectors must have the same size.");
    }
    // Use std::inner_product for efficient calculation
    return std::inner_product(weights.begin(), weights.end(), features.begin(), 0.0);
}

std::vector<Block> getNewUpcoming(Game& game)
{
    if (game.config.available_blocks.empty()) {
        throw std::runtime_error("No available blocks in game config.");
    }

    if (game.upcoming_blocks.empty()) {
        // Game start: Get two random blocks
        const Block* block1_ptr = getRandomBlock();
        const Block* block2_ptr = getRandomBlock();
        return { *block1_ptr, *block2_ptr }; // Return copies
    } else {
        // Game in progress: Shift blocks and get one new random block
        const Block* block2_ptr = getRandomBlock();
        return { game.upcoming_blocks[1], *block2_ptr }; // Return copies
    }
}

std::vector<BlockStatus> getAllActions(const Block& block, int board_width)
{
    std::vector<BlockStatus> actions;
    for (const auto& rotation : block.rotations) {
        int max_x_offset = board_width - rotation.size.width;
        for (int x = 0; x <= max_x_offset; ++x) {
            // Create BlockStatus with the current rotation (copied)
            actions.emplace_back(x, rotation); // assessment_score defaults to nullopt
        }
    }
    return actions;
}

BlockStatus findBestAction(const Game& game, const std::vector<BlockStatus>& actions, const AssessmentModel& model)
{
    if (actions.empty()) {
        throw std::runtime_error("No actions provided to findBestAction.");
    }
    if (!model.feature_extractor) {
        throw std::runtime_error("AssessmentModel has no feature extractor.");
    }

    BlockStatus best_action = actions[0]; // Temporary best
    double best_score = -std::numeric_limits<double>::infinity();
    bool found_valid_action = false;

    for (const auto& current_action_const : actions) {
        BlockStatus current_action = current_action_const; // Make a mutable copy
        double current_score = -std::numeric_limits<double>::infinity();
        try {
            std::vector<int> features = model.feature_extractor->extractFeatures(game, current_action);
            // 只取前 8 个特征
            features.resize(8); // Ensure we only use the first 8 features

            // Ensure weights vector is long enough
            if (model.weights.size() < features.size()) {
                throw std::runtime_error("Model weights vector is shorter than feature vector.");
            }
            // Only use the number of weights specified by model.length (or min of lengths)
            size_t len_to_use = std::min(static_cast<size_t>(model.length), features.size());
            len_to_use = std::min(len_to_use, model.weights.size());

            // Create temporary vectors of the correct size for calculation
            std::vector<double> weights_subset(model.weights.begin(), model.weights.begin() + len_to_use);
            std::vector<int> features_subset(features.begin(), features.begin() + len_to_use);

            current_score = calculateLinearFunction(weights_subset, features_subset);
            current_action.assessment_score = current_score; // Store score in the mutable copy
            found_valid_action = true; // Mark that we found at least one valid action

        } catch (const std::runtime_error& e) {
            // Feature extraction failed (e.g., invalid placement)
            // Keep score as -infinity, assessment_score remains nullopt or previous value
            current_score = -std::numeric_limits<double>::infinity();
            current_action.assessment_score = std::nullopt; // Explicitly mark as not scored
        }

        if (current_score > best_score) {
            best_score = current_score;
            best_action = current_action; // Store the copy with the score
        }
    }

    if (!found_valid_action) {
        // This means *every* action resulted in an exception from extractFeatures
        // This implies a game over state where no piece can be placed.
        throw std::runtime_error("No valid actions found - game likely over.");
    }

    return best_action;
}

BlockStatus findBestActionV2(const Game& game, const std::vector<BlockStatus>& actions1, const Block& block2, const AssessmentModel& model)
{
    if (actions1.empty()) {
        throw std::runtime_error("No actions1 provided to findBestActionV2.");
    }
    if (!model.feature_extractor) {
        throw std::runtime_error("AssessmentModel has no feature extractor for findBestActionV2.");
    }

    double best_combined_score = -std::numeric_limits<double>::infinity();
    // Initialize best_action1 carefully. Find the first valid action if possible.
    std::optional<BlockStatus> best_action1_opt;

    for (const auto& action1_const : actions1) {
        BlockStatus action1 = action1_const; // Mutable copy
        double score1 = -std::numeric_limits<double>::infinity();
        double score2 = -std::numeric_limits<double>::infinity();
        double current_combined_score = -std::numeric_limits<double>::infinity();

        try {
            // 1. Evaluate the first action (action1) in the current game state
            std::vector<int> features1 = model.feature_extractor->extractFeatures(game, action1);
            // Use the same feature/weight length logic as findBestAction
            features1.resize(std::min(features1.size(), static_cast<size_t>(model.length))); // Ensure correct feature count
            if (model.weights.size() < features1.size()) {
                // This check might be redundant if model.length is used correctly, but safe
                throw std::runtime_error("Model weights vector shorter than feature vector for action1.");
            }
            size_t len_to_use1 = std::min(static_cast<size_t>(model.length), features1.size());
            len_to_use1 = std::min(len_to_use1, model.weights.size());
            std::vector<double> weights_subset1(model.weights.begin(), model.weights.begin() + len_to_use1);
            std::vector<int> features_subset1(features1.begin(), features1.begin() + len_to_use1);
            score1 = calculateLinearFunction(weights_subset1, features_subset1);
            action1.assessment_score = score1; // Store score in the action copy

            // 2. Simulate placing action1 and evaluate the second step
            auto game1 = game; // Copy the game state for simulation
            try {
                game1 = executeAction(game, action1).first; // Simulate the first move
            } catch (const std::runtime_error& sim_error) {
                // If executeAction throws (e.g., game over), this action1 is invalid.
                // score1 remains valid, but score2 will be -inf.
                // Log or handle simulation failure if needed.
                // std::cerr << "Simulation failed for action1: " << sim_error.what() << std::endl;
                score2 = -std::numeric_limits<double>::infinity(); // Penalize heavily
                current_combined_score = score1 + score2; // Combine scores
                // Continue to the comparison below, this path will likely not be chosen.
                goto compare_scores; // Use goto for clarity in this specific try-catch structure
            }

            // If simulation succeeded, proceed to evaluate the second block
            if (!game1.isEnd()) { // Only evaluate if the game didn't end after action1
                auto actions2 = getAllActions(block2, game.board.size.width);
                if (!actions2.empty()) {
                    // Find the best action2 for the second block in the simulated state game1
                    BlockStatus best_action2 = findBestAction(game1, actions2, model);
                    // Get the score associated with the best second action
                    score2 = best_action2.assessment_score.value_or(-std::numeric_limits<double>::infinity());
                } else {
                    // Should not happen if block2 is valid, but handle defensively
                    score2 = -std::numeric_limits<double>::infinity();
                }
            } else {
                // Game ended immediately after action1
                score2 = -std::numeric_limits<double>::infinity(); // Or potentially a large penalty
            }

            // 3. Combine scores (simple addition for now)
            current_combined_score = score1 + score2;

        } catch (const std::runtime_error& e) {
            // Catch errors during feature extraction for action1 itself
            // This action is invalid.
            // std::cerr << "Feature extraction failed for action1: " << e.what() << std::endl;
            score1 = -std::numeric_limits<double>::infinity();
            score2 = -std::numeric_limits<double>::infinity();
            current_combined_score = -std::numeric_limits<double>::infinity();
            action1.assessment_score = std::nullopt;
        }

    compare_scores: // Label for goto jump
        // 4. Update the best action found so far
        if (current_combined_score > best_combined_score) {
            best_combined_score = current_combined_score;
            best_action1_opt = action1; // Store the action1 that led to this best combined score
        } else if (!best_action1_opt.has_value() && action1.assessment_score.has_value()) {
            // Handle initialization: if no best action is set yet, take the first valid one encountered.
            // This ensures we return *something* if all combined scores are -inf but some action1 are valid.
            best_combined_score = current_combined_score; // Might still be -inf
            best_action1_opt = action1;
        }
    }

    if (!best_action1_opt) {
        // This means *no* action1 could even be evaluated (e.g., all failed feature extraction
        // or all simulations failed immediately). This implies a likely game over state initially.
        // Fallback: Return the first action, or throw, depending on desired behavior.
        // Throwing is safer as it indicates an unplayable state.
        throw std::runtime_error("No valid action sequence found in findBestActionV2 - game likely over.");
        // return actions1[0]; // Less safe fallback
    }

    // Store the best *combined* score in the chosen action1 for potential debugging/logging
    best_action1_opt->assessment_score = best_combined_score;
    return best_action1_opt.value();
}

// Forward declare eliminateLines if its definition is in extractor.cpp
// int eliminateLines(Board& board);

std::pair<Game, int> executeAction(const Game& current_game, const BlockStatus& action)
{
    // 1. Create a deep copy of the game to modify
    Game next_game_state = current_game; // Use Game's copy constructor
    Board& board = next_game_state.board; // Get a reference to the board *in the copy*

    // 2. Find landing position using the *original* board state (read-only)
    // Use the findYOffset from extractor.cpp (make sure it's accessible or redefine here)
    int y_offset = findYOffset(current_game.board, action); // Use original board for finding offset

    if (y_offset == -1) {
        // Action immediately results in game over (overflow)
        next_game_state.setEnd();
        // Throw exception to signal immediate game over from this action
        throw std::runtime_error("Game Over: Action causes overflow.");
        // return {next_game_state, -1}; // Or return the ended state
    }

    // 3. Place the block on the *copied* board
    const Block* block_to_place = action.rotation.getOriginalBlock(k_blocks);
    for (const auto& pos : action.rotation.occupied) {
        int place_x = action.x_offset + pos.x;
        int place_y = y_offset + pos.y;
        // Bounds check (redundant if findYOffset is correct, but safe)
        if (place_y >= 0 && place_y < board.getGridHeight() && place_x >= 0 && place_x < board.size.width) {
            board.squares[place_y][place_x] = block_to_place;
        } else {
            throw std::logic_error("Placement out of bounds during executeAction.");
        }
    }

    // 4. Eliminate lines and update score on the *copied* board
    int eliminated_lines = eliminateLines(board); // Modifies the copied board

    if (eliminated_lines > 0) {
        if (eliminated_lines <= static_cast<int>(next_game_state.config.awards.size())) {
            // Awards are 0-indexed, eliminated_lines is 1-based count
            next_game_state.score += static_cast<int>(next_game_state.config.awards[eliminated_lines - 1] * 100);
        } else if (!next_game_state.config.awards.empty()) {
            // Award for max defined lines if more are cleared
            next_game_state.score += static_cast<int>(next_game_state.config.awards.back() * 100 * eliminated_lines); // Or some other logic
        }
    }

    // 5. Update upcoming blocks in the *copied* game state
    next_game_state.upcoming_blocks = getNewUpcoming(next_game_state);

    // 6. Return the new game state and the y_offset
    return { next_game_state, y_offset };
}

int runGame(Context& ctx)
{
    try {
        while (!ctx.game.isEnd()) {
            // 1. Get current block and generate actions
            if (ctx.game.upcoming_blocks.empty()) {
                ctx.game.upcoming_blocks = getNewUpcoming(ctx.game);
            }
            const Block& current_block = ctx.game.upcoming_blocks[0];
            std::vector<BlockStatus> actions = getAllActions(current_block, ctx.game.board.size.width);

            if (actions.empty()) { // Should not happen if blocks are defined
                ctx.game.setEnd();
                break;
            }

            // 2. Find best action
            // Ensure the strategy and model exist
            if (!ctx.strategy.assessment_model) {
                throw std::runtime_error("Context strategy has no assessment model.");
            }
            // BlockStatus best_action = findBestAction(ctx.game, actions, *ctx.strategy.assessment_model);
            auto best_action = findBestActionV2(ctx.game, actions, ctx.game.upcoming_blocks[1], *ctx.strategy.assessment_model);

            // 3. Execute best action (updates a copy)
            auto result = executeAction(ctx.game, best_action);
            ctx.game = std::move(result.first); // Move the new game state into the context

            // Optional: Add loop counter, visualization calls, etc.
        }
    } catch (const std::runtime_error& e) {
        // Catch exceptions from findBestAction (no valid moves) or executeAction (overflow)
        // These indicate a game over condition.
        // Ensure game score is negative.
        if (!ctx.game.isEnd()) {
            ctx.game.setEnd();
        }
        // Optionally log the error: std::cerr << "Game ended with error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Catch other potential standard exceptions
        if (!ctx.game.isEnd()) {
            ctx.game.setEnd();
        }
        // Optionally log the error: std::cerr << "Game ended with unexpected error: " << e.what() << std::endl;
    }

    return std::abs(ctx.game.score); // Return absolute score
}

int runGameForTraining(const std::vector<double>& weights)
{
    // Create a default assessment model for this run
    // Assuming MyDbtFeatureExtractorCpp is the desired extractor
    auto feature_extractor = std::make_unique<MyDbtFeatureExtractorCpp>();
    // Determine length based on weights or a fixed value (e.g., 8 for the core features)
    int model_length = 8; // Or weights.size(), or a fixed constant
    model_length = std::min(static_cast<int>(weights.size()), model_length); // Ensure length doesn't exceed weights

    auto assessment_model = std::make_unique<AssessmentModel>(model_length, weights, std::move(feature_extractor));
    Strategy strategy(std::move(assessment_model));
    Context ctx(createNewGame(), std::move(strategy));

    return runGame(ctx); // Run the game using the main loop
}