#include "models.h" // Include the header file
#include <algorithm> // For std::all_of
#include <stdexcept>
#include <utility> // For std::move

// --- Basic Structures Implementation ---

Position::Position(int x_val, int y_val)
    : x(x_val)
    , y(y_val)
{
}

bool Position::operator==(const Position& other) const
{
    return x == other.x && y == other.y;
}

Size::Size(int w, int h)
    : width(w)
    , height(h)
{
}

bool Size::operator==(const Size& other) const
{
    return width == other.width && height == other.height;
}

// --- Block Related Classes Implementation ---

BlockRotation::BlockRotation(std::string lbl, Size sz, std::vector<Position> occ)
    : label(std::move(lbl))
    , size(sz)
    , occupied(std::move(occ))
{
}

bool BlockRotation::operator==(const BlockRotation& other) const
{
    return label == other.label && size == other.size && occupied == other.occupied;
}

// Definition for BlockRotation::getOriginalBlock
// Needs to be defined after Block is fully defined (which it is via include)
// REMOVED: This lookup is inefficient and unnecessary if we pass the block context.
// const Block* BlockRotation::getOriginalBlock(const std::vector<const Block*>& block_list) const {
//     // This requires searching through block_list, which is inefficient.
//     // It's better if the context (like BlockStatus or the calling function)
//     // already knows which Block this rotation belongs to.
//     // Placeholder implementation - THIS SHOULD BE AVOIDED/REPLACED.
//     for (const auto* block_ptr : block_list) {
//         if (block_ptr) {
//             for (const auto& rot : block_ptr->rotations) {
//                 if (&rot == this) { // Pointer comparison - might be fragile
//                     return block_ptr;
//                 }
//             }
//         }
//     }
//     throw std::runtime_error("Original block not found for rotation. Refactor needed.");
// }

Block::Block(std::string n, std::string lbl, int count, std::vector<BlockRotation> rots)
    : name(std::move(n))
    , label(std::move(lbl))
    , status_count(count)
    , rotations(std::move(rots))
{
}

BlockStatus::BlockStatus(int offset, const BlockRotation* rot, std::optional<double> score)
    : x_offset(offset)
    , rotation(rot) // Store the pointer
    , assessment_score(score)
{
    if (!rot) {
        throw std::invalid_argument("BlockStatus cannot be constructed with a null rotation pointer.");
    }
}

// --- Strategy & AI Related Classes Implementation ---
// FeatureExtractor is abstract, no implementation here

AssessmentModel::AssessmentModel(int len, std::vector<double> w, std::unique_ptr<FeatureExtractor> extractor)
    : length(len)
    , weights(std::move(w))
    , feature_extractor(std::move(extractor))
{
}

Strategy::Strategy(std::unique_ptr<AssessmentModel> model)
    : assessment_model(std::move(model))
{
}

// --- Game Configuration & State Implementation ---

GameConfig::GameConfig(std::vector<double> awds, std::vector<Block> blocks)
    : awards(std::move(awds))
    , available_blocks(std::move(blocks))
{
}

Board::Board(Size s)
    : size(s)
{
    int buffer_height = 5;
    int total_height = size.height + buffer_height;
    squares.resize(total_height, std::vector<const Block*>(size.width, nullptr));
}

// Copy constructor
Board::Board(const Board& other)
    : size(other.size)
    , squares(other.squares)
{
}

// Copy assignment operator
Board& Board::operator=(const Board& other)
{
    if (this != &other) {
        size = other.size;
        squares = other.squares;
    }
    return *this;
}

// Move constructor
Board::Board(Board&& other) noexcept
    : size(other.size)
    , squares(std::move(other.squares))
{
}

// Move assignment operator
Board& Board::operator=(Board&& other) noexcept
{
    if (this != &other) {
        size = other.size;
        squares = std::move(other.squares);
    }
    return *this;
}

bool Board::canClearLine(int y) const
{
    if (y < 0 || y >= static_cast<int>(squares.size())) { // Use static_cast for comparison
        return false;
    }
    // Check only within the logical board width
    return std::all_of(squares[y].begin(), squares[y].begin() + size.width, // Check only up to size.width
        [](const Block* block_ptr) { return block_ptr != nullptr; });
}

int Board::getGridHeight() const
{
    return static_cast<int>(squares.size()); // Use static_cast
}

// Game Implementation
Game::Game(GameConfig cfg, Board b, int s, std::vector<const Block*> upcoming)
    : config(std::move(cfg)) // Use move for config
    , board(std::move(b)) // Use move for board
    , score(s)
    , upcoming_blocks(std::move(upcoming)) // Use move for upcoming_blocks
    , game_over(false)
{
}

// Copy Constructor
Game::Game(const Game& other)
    : config(other.config) // GameConfig copy constructor should handle its members
    , board(other.board) // Board copy constructor should handle its members
    , score(other.score)
    , upcoming_blocks(other.upcoming_blocks) // Copy the vector of pointers
    , game_over(other.game_over)
{
}

// Move Constructor
Game::Game(Game&& other) noexcept
    : config(std::move(other.config))
    , board(std::move(other.board))
    , score(other.score)
    , upcoming_blocks(std::move(other.upcoming_blocks))
    , game_over(other.game_over)
{
    // Reset other state if necessary (score, game_over are simple types)
    other.score = 0;
    other.game_over = false;
}

// Copy Assignment Operator
Game& Game::operator=(const Game& other)
{
    if (this != &other) {
        config = other.config;
        board = other.board;
        score = other.score;
        upcoming_blocks = other.upcoming_blocks;
        game_over = other.game_over;
    }
    return *this;
}

// Move Assignment Operator
Game& Game::operator=(Game&& other) noexcept
{
    if (this != &other) {
        config = std::move(other.config);
        board = std::move(other.board);
        score = other.score;
        upcoming_blocks = std::move(other.upcoming_blocks);
        game_over = other.game_over;

        // Reset other state if necessary
        other.score = 0;
        other.game_over = false;
    }
    return *this;
}


bool Game::isEnd() const
{
    return game_over;
}

void Game::setEnd()
{
    game_over = true;
    // Consider setting a negative score or specific state if needed
}

// --- Overall Context Implementation ---

Context::Context(Game g, Strategy strat)
    : game(std::move(g))
    , strategy(std::move(strat))
{
}

// Move constructor/assignment are defaulted
// Copy constructor/assignment are deleted