#include "constants.h"
#include "extractor.h"
#include "game.h"
#include "models.h"
#include "visualize.h" // Include the visualization header
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory> // For std::make_unique
#include <stdexcept> // For exception handling
#include <string>
#include <thread>
#include <vector>

int main(int argc, char* argv[])
{
    // --- Parse Command Line Arguments ---
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) { // Start from 1 to skip the program name
        args.push_back(argv[i]);
    }

    std::cout << "--- Tetris C++ Test ---" << std::endl;

    // --- Visualize Blocks ---
    // std::cout << "\n--- Visualizing Blocks ---" << std::endl;
    // visualizeBlocks(k_blocks);

    // --- Setup Game Context ---
    std::cout << "\n--- Setting up Game ---" << std::endl;
    Game game = createNewGame();
    game.upcoming_blocks = getNewUpcoming(game); // Get initial blocks (pointers)

    // Create a default assessment model for testing visualization
    auto feature_extractor = std::make_unique<MyDbtFeatureExtractorCpp>();
    std::vector<double> test_weights = { -19.0170, 8.3012, -7.6468, -18.8370, -14.5924, -12.0379, -1.6249, -26.9781, -1.1738 };
    // Pad with zeros if needed, or adjust length
    int model_length = 8; // Use first 8 features for this example
    // test_weights.resize(model_length, 0.0); // Ensure weights vector matches length

    auto assessment_model = std::make_unique<AssessmentModel>(model_length, test_weights, std::move(feature_extractor));
    Strategy strategy(std::move(assessment_model));
    Context ctx(std::move(game), std::move(strategy));

    std::cout << "Initial Game Created. Score: " << ctx.game.score << std::endl;
    if (!ctx.game.upcoming_blocks.empty() && ctx.game.upcoming_blocks[0]) {
        std::cout << "First upcoming block: " << ctx.game.upcoming_blocks[0]->label << std::endl; // Use ->
    }

    // --- Simulate One Step & Visualize ---
    // std::cout << "cli args: " << args << std::endl;

    std::cout << "\n--- Simulating Steps ---" << std::endl;
    auto count = -1;
    auto delay = 300; // Delay in milliseconds
    for (int i = 0; i != count; i++) {
        // delay
        if (std::find(args.begin(), args.end(), std::string("-d")) != args.end()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        }

        // Manual step mode
        if (std::find(args.begin(), args.end(), std::string("-m")) != args.end()) {
            std::cout << "Press Enter to continue..." << '\n';
            std::cin.get();
        }

        if (ctx.game.isEnd()) { // Check if game ended in the previous iteration
             std::cout << "-----------------------------------------" << std::endl;
             std::cout << "GAME OVER detected at start of step " << i + 1 << std::endl;
             std::cout << "Final Score: " << ctx.game.score << std::endl;
             std::cout << "-----------------------------------------" << std::endl;
             break;
        }

        try {
            if (ctx.game.upcoming_blocks.empty()) {
                // Attempt to get new blocks if empty (shouldn't happen with proper executeAction)
                ctx.game.upcoming_blocks = getNewUpcoming(ctx.game);
                if (ctx.game.upcoming_blocks.empty()) {
                    std::cout << "No upcoming blocks to play and cannot get new ones." << std::endl;
                    ctx.game.setEnd(); // Ensure game is marked as over
                    break; // Exit loop
                }
            }
            if (!ctx.game.upcoming_blocks[0]) {
                throw std::runtime_error("Current upcoming block is null.");
            }
            const Block& current_block = *ctx.game.upcoming_blocks[0]; // Dereference pointer

            std::vector<BlockStatus> actions = getAllActions(current_block, ctx.game.board.size.width);

            if (actions.empty()) {
                std::cout << "No possible actions for the current block." << std::endl;
                ctx.game.setEnd(); // Mark game as over
                break; // Exit loop
            }

            // Find best action
            if (ctx.game.upcoming_blocks.size() < 2 || !ctx.game.upcoming_blocks[1]) {
                throw std::runtime_error("Next upcoming block is missing or null for V2.");
            }
            const Block& next_block = *ctx.game.upcoming_blocks[1]; // Dereference pointer
            BlockStatus best_action = findBestActionV2(ctx.game, actions, next_block, *ctx.strategy.assessment_model);

            // Execute the action (modifies ctx.game directly)
            int y_offset = executeAction(ctx.game, best_action); // Returns offset, modifies ctx.game

            // Post-action checks and logging
            if (ctx.game.isEnd()) {
                // Game ended during executeAction (e.g., block placed too high after clear)
                // Logging handled within the loop break condition at the start of the next iteration
            } else {
                // Action was successful
                if (i % 1000 == 0) {
                    // Get current time
                    auto now = std::chrono::system_clock::now();
                    auto now_c = std::chrono::system_clock::to_time_t(now);
                    // Use std::localtime for local time zone
                    std::tm now_tm = *std::localtime(&now_c);
                    // Format the time (e.g., YYYY-MM-DD HH:MM:SS)
                    std::cout << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
                    // Add milliseconds
                    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
                    std::cout << '.' << std::setw(3) << std::setfill('0') << ms.count();
                    std::cout << " - "; // Separator
                    std::cout << i << " - New Score: " << ctx.game.score << std::endl;
                }

                if (std::find(args.begin(), args.end(), std::string("-v")) != args.end()) {
                    visualizeGame(ctx.game, best_action, y_offset, true); // Show board
                }
                 // Optional: Log next block
                 // if (!ctx.game.upcoming_blocks.empty() && ctx.game.upcoming_blocks[0]) {
                 //     std::cout << "Next upcoming block: " << ctx.game.upcoming_blocks[0]->label << std::endl;
                 // }
            }

        } catch (const std::runtime_error& e) {
            // Handle exceptions from findBestActionV2 or executeAction
            std::string error_msg = e.what();
            std::cout << "-----------------------------------------" << std::endl;
            std::cout << "GAME OVER: " << error_msg << std::endl;
            if (!ctx.game.isEnd()) { // Ensure game is marked as ended
                ctx.game.setEnd();
            }
            std::cout << "Final Score: " << ctx.game.score << std::endl;
            std::cout << "-----------------------------------------" << std::endl;
            break; // Exit the simulation loop
        } catch (const std::exception& e) {
            // Handle other exceptions
             std::cerr << "Unexpected error: " << e.what() << std::endl;
             if (!ctx.game.isEnd()) {
                 ctx.game.setEnd();
             }
             break; // Exit loop
        }
    }

    // Final check in case loop exited without setting score message
    if (ctx.game.isEnd()) {
         std::cout << "Simulation ended. Final Score: " << ctx.game.score << std::endl;
    }

    std::cout << "\n--- Test Finished ---" << std::endl;
    return 0;
}