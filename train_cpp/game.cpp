#include "game.h"
#include "constants.h" // Include k_blocks declaration
#include "extractor.h" // Include MyDbtFeatureExtractorCpp AND getBlockFromRotation declaration
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
    // No need to copy blocks for config if GameConfig doesn't require owning copies
    // Assuming GameConfig can just store awards or other simple config.
    // If GameConfig *needs* available_blocks, it should store const Block*
    GameConfig config({ 1.0, 3.0, 5.0, 8.0 }, {}); // Pass empty block list or adjust GameConfig
    Board board(Size(10, 14)); // Standard Tetris size (adjust height as needed)
    std::vector<const Block*> initial_upcoming; // Empty initial upcoming blocks (pointers)
    return Game(std::move(config), std::move(board), 0, std::move(initial_upcoming)); // Initial score 0
}

double calculateLinearFunction(const std::vector<double>& weights, const std::vector<int>& features)
{
    if (weights.size() != features.size()) {
        throw std::invalid_argument("Weights and features vectors must have the same size.");
    }
    // Use std::inner_product for efficient calculation
    return std::inner_product(weights.begin(), weights.end(), features.begin(), 0.0);
}

// Returns pointers to the next upcoming blocks
std::vector<const Block*> getNewUpcoming(Game& game)
{
    // Assuming game.config doesn't need available_blocks or it stores pointers
    if (k_blocks.empty()) { // Check global k_blocks directly
        throw std::runtime_error("No available blocks defined (k_blocks).");
    }

    const Block* block2_ptr = getRandomBlock(); // Get pointer to random block

    if (game.upcoming_blocks.empty()) {
        // Game start: Get two random blocks
        const Block* block1_ptr = getRandomBlock();
        return { block1_ptr, block2_ptr }; // Return vector of pointers
    } else {
        // Game in progress: Shift blocks and get one new random block
        // Ensure upcoming_blocks has at least two elements if we access [1]
        if (game.upcoming_blocks.size() < 2) {
             throw std::logic_error("Upcoming blocks has less than 2 elements during getNewUpcoming.");
        }
        return { game.upcoming_blocks[1], block2_ptr }; // Return vector of pointers
    }
}

std::vector<BlockStatus> getAllActions(const Block& block, int board_width)
{
    std::vector<BlockStatus> actions;
    actions.reserve(block.rotations.size() * board_width); // Pre-allocate estimate
    for (const auto& rotation : block.rotations) {
        int max_x_offset = board_width - rotation.size.width;
        for (int x = 0; x <= max_x_offset; ++x) {
            // Pass pointer to the rotation
            actions.emplace_back(x, &rotation); // assessment_score defaults to nullopt
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

    std::optional<BlockStatus> best_action_opt; // Use optional to handle no valid actions found initially
    double best_score = -std::numeric_limits<double>::infinity();

    // Use const& in the loop
    for (const auto& current_action : actions) {
        if (!current_action.rotation) continue; // Skip if rotation pointer is null

        double current_score = -std::numeric_limits<double>::infinity();
        std::optional<double> score_opt = std::nullopt; // To store the calculated score

        try {
            std::vector<int> features = model.feature_extractor->extractFeatures(game, current_action);
            size_t len_to_use = std::min(static_cast<size_t>(model.length), features.size());
            len_to_use = std::min(len_to_use, model.weights.size());

            if (len_to_use > 0) {
                 current_score = std::inner_product(model.weights.begin(), model.weights.begin() + len_to_use,
                                                   features.begin(), 0.0);
            } else {
                 current_score = 0.0;
            }

            score_opt = current_score; // Store the valid score

        } catch (const std::runtime_error& e) {
            // Feature extraction failed (e.g., invalid placement)
            current_score = -std::numeric_limits<double>::infinity();
            // score_opt remains nullopt
        }

        if (current_score > best_score) {
            best_score = current_score;
            best_action_opt = current_action; // Copy the action here as it's the new best
            if (best_action_opt) { // Check if copy succeeded (should always)
                 best_action_opt->assessment_score = score_opt; // Store the score in the best action copy
            }
        } else if (!best_action_opt.has_value() && score_opt.has_value()) {
             // Initialize best_action_opt with the first valid action found
             best_score = current_score;
             best_action_opt = current_action;
             best_action_opt->assessment_score = score_opt;
        }
    }

    if (!best_action_opt) {
        // This means *every* action resulted in an exception from extractFeatures or had null rotation
        throw std::runtime_error("No valid actions found - game likely over.");
    }

    return best_action_opt.value();
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
    std::optional<BlockStatus> best_action1_opt;

    // Use const& in the loop
    for (const auto& action1 : actions1) {
        if (!action1.rotation) continue; // Skip null rotation

        double score1 = -std::numeric_limits<double>::infinity();
        double score2 = -std::numeric_limits<double>::infinity();
        double current_combined_score = -std::numeric_limits<double>::infinity();
        std::optional<double> score1_opt = std::nullopt;

        try {
            // 1. Evaluate the first action (action1) in the current game state
            std::vector<int> features1 = model.feature_extractor->extractFeatures(game, action1);
            size_t len_to_use1 = std::min(static_cast<size_t>(model.length), features1.size());
            len_to_use1 = std::min(len_to_use1, model.weights.size());

            if (len_to_use1 > 0) {
                 score1 = std::inner_product(model.weights.begin(), model.weights.begin() + len_to_use1,
                                            features1.begin(), 0.0);
            } else {
                 score1 = 0.0;
            }
            score1_opt = score1;

            // 2. Simulate placing action1 on a *copy of the board*
            Board board1 = game.board; // Copy only the board
            int y_offset1 = findYOffset(game.board, action1); // Find offset on original board

            if (y_offset1 == -1) {
                // Action1 itself causes overflow/collision, treat as invalid path
                score2 = -std::numeric_limits<double>::infinity();
                current_combined_score = score1 + score2; // Will be low
                goto compare_scores_v2; // Use goto for efficiency here
            }

            // Get the original block pointer for action1
            const Block* block_to_place1 = getBlockFromRotation(action1.rotation);
             if (!block_to_place1) {
                 throw std::runtime_error("Could not determine block type for action1 in V2 sim.");
             }

            // Place block on the copied board
            for (const auto& pos : action1.rotation->occupied) { // Use ->
                int place_x = action1.x_offset + pos.x;
                int place_y = y_offset1 + pos.y;
                if (place_y >= 0 && place_y < board1.getGridHeight() && place_x >= 0 && place_x < board1.size.width) {
                    board1.squares[place_y][place_x] = block_to_place1; // Place pointer
                } else {
                     // This indicates a logic error if findYOffset/isOverflow worked correctly
                     throw std::logic_error("Placement out of bounds during V2 simulation.");
                }
            }

            // Eliminate lines on the copied board
            eliminateLines(board1); // Modifies board1

            // Check for game over *after* placing action1 and clearing lines
            bool game_over_after_action1 = false;
             for (int y = game.board.size.height; y < board1.getGridHeight(); ++y) { // Check buffer zone
                 for (int x = 0; x < board1.size.width; ++x) {
                     if (board1.squares[y][x] != nullptr) {
                         game_over_after_action1 = true;
                         break;
                     }
                 }
                 if (game_over_after_action1) break;
             }

            if (!game_over_after_action1) {
                // Create a temporary Game object using the *moved* board1.
                // GameConfig copy should be cheap or Game needs a constructor taking const GameConfig&.
                // Assume GameConfig copy is acceptable for now.
                 std::vector<const Block*> next_upcoming_sim = { &block2, getRandomBlock() }; // Simplified next state
                 Game game1_sim(game.config, std::move(board1), game.score, std::move(next_upcoming_sim));
                 // Note: score isn't updated, config is copied, board1 is moved.

                auto actions2 = getAllActions(block2, game.board.size.width); // Use original board width
                if (!actions2.empty()) {
                    try {
                        // Call findBestAction with the simulated game state
                        BlockStatus best_action2 = findBestAction(game1_sim, actions2, model);
                        score2 = best_action2.assessment_score.value_or(-std::numeric_limits<double>::infinity());
                    } catch (const std::runtime_error& e) {
                         // If findBestAction throws (e.g., no valid moves for block2), score2 remains -inf
                         score2 = -std::numeric_limits<double>::infinity();
                    }
                } else {
                    // No possible actions for the second block
                    score2 = -std::numeric_limits<double>::infinity();
                }
            } else {
                // Game ended immediately after action1
                score2 = -std::numeric_limits<double>::infinity();
            }

            // 3. Combine scores
            current_combined_score = score1 + score2;

        } catch (const std::runtime_error& e) {
            // Catch errors during feature extraction for action1 itself
            score1 = -std::numeric_limits<double>::infinity();
            score2 = -std::numeric_limits<double>::infinity();
            current_combined_score = -std::numeric_limits<double>::infinity();
            score1_opt = std::nullopt;
        }

    compare_scores_v2: // Label for goto jump
        // 4. Update the best action found so far
        if (current_combined_score > best_combined_score) {
            best_combined_score = current_combined_score;
            best_action1_opt = action1; // Copy action1
            if (best_action1_opt) {
                 best_action1_opt->assessment_score = score1_opt; // Store score1 (score of the first step)
            }
        } else if (!best_action1_opt.has_value() && score1_opt.has_value()) {
            // Handle initialization: if no best action is set yet, take the first valid one encountered.
            best_combined_score = current_combined_score; // Might still be -inf
            best_action1_opt = action1; // Copy action1
            if (best_action1_opt) {
                 best_action1_opt->assessment_score = score1_opt;
            }
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

// Modifies game state directly, returns y_offset
int executeAction(Game& game, const BlockStatus& action)
{
    if (!action.rotation) {
         throw std::runtime_error("Game Over: executeAction called with null rotation.");
    }
    Board& board = game.board; // Get a reference to the game's board

    // 2. Find landing position using the current board state
    int y_offset = findYOffset(board, action); // Use current board

    if (y_offset == -1) {
        // Action immediately results in game over (overflow or no valid placement)
        game.setEnd();
        throw std::runtime_error("Game Over: Action causes overflow or invalid placement.");
    }

    // 3. Place the block directly on the game's board
    // Get the original block pointer
    const Block* block_to_place = getBlockFromRotation(action.rotation);
     if (!block_to_place) {
         throw std::runtime_error("Could not determine block type during executeAction.");
     }

    for (const auto& pos : action.rotation->occupied) { // Use ->
        int place_x = action.x_offset + pos.x;
        int place_y = y_offset + pos.y;
        // Bounds check
        if (place_y >= 0 && place_y < board.getGridHeight() && place_x >= 0 && place_x < board.size.width) {
            board.squares[place_y][place_x] = block_to_place; // Place pointer
        } else {
            // This indicates a logic error, potentially in findYOffset or action generation
            game.setEnd(); // Set game over state
            throw std::logic_error("Placement out of bounds during executeAction.");
        }
    }

    // 4. Eliminate lines and update score on the game's board
    int eliminated_lines = eliminateLines(board); // Modifies the game's board

    if (eliminated_lines > 0) {
        if (eliminated_lines <= static_cast<int>(game.config.awards.size())) {
            game.score += static_cast<int>(game.config.awards[eliminated_lines - 1] * 100);
        } else if (!game.config.awards.empty()) {
            game.score += static_cast<int>(game.config.awards.back() * 100 * eliminated_lines);
        }
    }

     // Check for game over AFTER placement and line clearing (block above ceiling)
     for (int y = game.board.size.height; y < board.getGridHeight(); ++y) { // Check buffer zone
         for (int x = 0; x < board.size.width; ++x) {
             if (board.squares[y][x] != nullptr) {
                 game.setEnd();
                 // Return offset even if game ended here, main loop checks isEnd()
                 return y_offset;
             }
         }
     }

    // 5. Update upcoming blocks in the game state (now returns pointers)
    game.upcoming_blocks = getNewUpcoming(game);

    // 6. Return the y_offset
    return y_offset;
}

int runGame(Context& ctx)
{
    try {
        while (!ctx.game.isEnd()) {
            // 1. Get current block and generate actions
            if (ctx.game.upcoming_blocks.empty()) {
                ctx.game.upcoming_blocks = getNewUpcoming(ctx.game);
                 if (ctx.game.upcoming_blocks.empty()) { // Check if still empty (error case)
                     throw std::runtime_error("Failed to get initial upcoming blocks.");
                 }
            }
            // upcoming_blocks now holds pointers
            const Block* current_block_ptr = ctx.game.upcoming_blocks[0];
             if (!current_block_ptr) {
                 throw std::runtime_error("Current upcoming block is null.");
             }
            const Block& current_block = *current_block_ptr; // Dereference pointer

            std::vector<BlockStatus> actions = getAllActions(current_block, ctx.game.board.size.width);

            if (actions.empty()) {
                ctx.game.setEnd(); // No actions possible
                break;
            }

            // 2. Find best action
            if (!ctx.strategy.assessment_model) {
                throw std::runtime_error("Context strategy has no assessment model.");
            }
             if (ctx.game.upcoming_blocks.size() < 2 || !ctx.game.upcoming_blocks[1]) {
                  throw std::runtime_error("Next upcoming block is missing or null for V2.");
             }
            const Block& next_block = *ctx.game.upcoming_blocks[1]; // Dereference pointer

            BlockStatus best_action = findBestActionV2(ctx.game, actions, next_block, *ctx.strategy.assessment_model);

            // 3. Execute best action (modifies ctx.game directly)
            executeAction(ctx.game, best_action); // Modifies ctx.game

            // Loop continues until game.isEnd() is true or an exception occurs
        }
    } catch (const std::runtime_error& e) {
        // Catch exceptions from findBestAction (no valid moves) or executeAction (overflow)
        // These indicate a game over condition.
        // Ensure game score is negative.
        if (!ctx.game.isEnd()) {
            ctx.game.setEnd();
        }
        // Optionally log: std::cerr << "Game ended with error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        // Catch other potential standard exceptions
        if (!ctx.game.isEnd()) {
            ctx.game.setEnd();
        }
        // Optionally log: std::cerr << "Game ended with unexpected error: " << e.what() << std::endl;
    }

    return std::abs(ctx.game.score); // Return absolute score
}

int runGameForTraining(const std::vector<double>& weights)
{
    auto feature_extractor = std::make_unique<MyDbtFeatureExtractorCpp>();
    int model_length = 8;
    // Ensure weights vector is copied for the model, or model takes const& if lifetime allows
    std::vector<double> weights_copy = weights;
    model_length = std::min(static_cast<int>(weights_copy.size()), model_length);
    // Ensure weights_copy has at least model_length elements if needed by AssessmentModel constructor
    weights_copy.resize(model_length, 0.0); // Or handle length mismatch appropriately

    auto assessment_model = std::make_unique<AssessmentModel>(model_length, std::move(weights_copy), std::move(feature_extractor));
    Strategy strategy(std::move(assessment_model));
    Context ctx(createNewGame(), std::move(strategy)); // createNewGame is updated

    return runGame(ctx); // runGame is updated
}