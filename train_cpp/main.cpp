#include "constants.h"
#include "extractor.h"
#include "game.h"
#include "models.h"
#include "visualize.h" // Include the visualization header
#include <iostream>
#include <memory> // For std::make_unique
#include <stdexcept> // For exception handling
#include <vector>

int main()
{
    std::cout << "--- Tetris C++ Test ---" << std::endl;

    // --- Visualize Blocks ---
    // std::cout << "\n--- Visualizing Blocks ---" << std::endl;
    // visualizeBlocks(k_blocks);

    // --- Setup Game Context ---
    std::cout << "\n--- Setting up Game ---" << std::endl;
    Game game = createNewGame();
    game.upcoming_blocks = getNewUpcoming(game); // Get initial blocks

    // Create a default assessment model for testing visualization
    auto feature_extractor = std::make_unique<MyDbtFeatureExtractorCpp>();
    std::vector<double> test_weights = { -12.63, 6.60, -9.22, -19.77, -13.08, -10.49, -1.61, -24.04 };
    // Pad with zeros if needed, or adjust length
    int model_length = 8; // Use first 8 features for this example
    // test_weights.resize(model_length, 0.0); // Ensure weights vector matches length

    auto assessment_model = std::make_unique<AssessmentModel>(model_length, test_weights, std::move(feature_extractor));
    Strategy strategy(std::move(assessment_model));
    Context ctx(std::move(game), std::move(strategy));

    std::cout << "Initial Game Created. Score: " << ctx.game.score << std::endl;
    if (!ctx.game.upcoming_blocks.empty()) {
        std::cout << "First upcoming block: " << ctx.game.upcoming_blocks[0].label << std::endl;
    }

    // --- Simulate One Step & Visualize ---
    std::cout << "\n--- Simulating One Step ---" << std::endl;
    try {
        if (ctx.game.upcoming_blocks.empty()) {
            std::cout << "No upcoming blocks to play." << std::endl;
            return 1;
        }
        const Block& current_block = ctx.game.upcoming_blocks[0];
        std::vector<BlockStatus> actions = getAllActions(current_block, ctx.game.board.size.width);

        if (actions.empty()) {
            std::cout << "No possible actions for the current block." << std::endl;
            return 1;
        }

        // Find best action (using the model in the context)
        BlockStatus best_action = findBestAction(ctx.game, actions, *ctx.strategy.assessment_model);

        // Find landing position for visualization
        int y_offset = findYOffset(ctx.game.board, best_action);

        if (y_offset != -1) {
            // Visualize the game state *before* executing the action
            visualizeGame(ctx.game, best_action, y_offset, true); // Show board

            // Visualize features for the best action
            std::vector<int> features = ctx.strategy.assessment_model->feature_extractor->extractFeatures(ctx.game, best_action);
            visualizeDbtFeature(features);

            // Execute the action (creates a new game state)
            auto result = executeAction(ctx.game, best_action);
            ctx.game = std::move(result.first); // Update game state in context

            std::cout << "\n--- After Executing Action ---" << std::endl;
            std::cout << "New Score: " << ctx.game.score << std::endl;
            if (!ctx.game.upcoming_blocks.empty()) {
                std::cout << "Next upcoming block: " << ctx.game.upcoming_blocks[0].label << std::endl;
            }
            // Visualize the board *after* the action (without highlighting placement)
            // We need a dummy action or modify visualizeBoard to handle this
            // For simplicity, just print score again.
            std::cout << "Board state updated." << std::endl;

        } else {
            std::cout << "Best action leads to game over immediately (overflow)." << std::endl;
            visualizeAction(ctx.game, best_action); // Show the problematic action
        }

    } catch (const std::runtime_error& e) {
        std::cerr << "Error during simulation step: " << e.what() << std::endl;
        // Visualize the state that caused the error if possible
        if (!ctx.game.upcoming_blocks.empty()) {
            const Block& current_block = ctx.game.upcoming_blocks[0];
            std::vector<BlockStatus> actions = getAllActions(current_block, ctx.game.board.size.width);
            if (!actions.empty()) {
                // Try visualizing the first action as a placeholder
                int y_offset = findYOffset(ctx.game.board, actions[0]);
                visualizeGame(ctx.game, actions[0], y_offset, true);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }

    // --- Optional: Run a short game ---
    // std::cout << "\n--- Running a Short Game (e.g., 10 steps) ---" << std::endl;
    // int final_score = runGame(ctx); // This will modify ctx.game further
    // std::cout << "Game finished with score: " << final_score << std::endl;

    std::cout << "\n--- Test Finished ---" << std::endl;
    return 0;
}