#ifndef MODELS_H
#define MODELS_H

#include <vector>
#include <string>
#include <optional>
#include <memory> // For std::unique_ptr

// --- Forward Declarations ---
// Forward declare classes/structs that are used as pointers/references
// or where the full definition isn't needed in this header.
class Game;
class FeatureExtractor;
struct Block; // Forward declare Block for BlockRotation::getOriginalBlock

// --- Basic Structures ---
struct Position {
    int x;
    int y;
    Position(int x = 0, int y = 0);
    bool operator==(const Position& other) const;
};

struct Size {
    int width;
    int height;
    Size(int width = 0, int height = 0);
    bool operator==(const Size& other) const;
};

// --- Block Related Classes ---
class BlockRotation {
public:
    std::string label;
    Size size;
    std::vector<Position> occupied;

    BlockRotation(std::string lbl, Size sz, std::vector<Position> occ);
    bool operator==(const BlockRotation& other) const;
    // Declaration only, definition needs full Block definition
    const Block* getOriginalBlock(const std::vector<const Block*>& block_list) const;
};

struct Block {
    std::string name;
    std::string label;
    int status_count;
    std::vector<BlockRotation> rotations;

    Block(std::string n, std::string lbl, int count, std::vector<BlockRotation> rots);
};

struct BlockStatus {
    int x_offset;
    BlockRotation rotation; // Requires full BlockRotation definition
    std::optional<double> assessment_score;

    BlockStatus(int offset, BlockRotation rot, std::optional<double> score = std::nullopt);
};

// --- Strategy & AI Related Classes ---
class FeatureExtractor {
public:
    virtual ~FeatureExtractor() = default;
    virtual std::vector<int> extractFeatures(const Game& game, const BlockStatus& action) const = 0;
};

struct AssessmentModel {
    int length;
    std::vector<double> weights;
    std::unique_ptr<FeatureExtractor> feature_extractor;

    AssessmentModel(int len, std::vector<double> w, std::unique_ptr<FeatureExtractor> extractor);
    AssessmentModel(AssessmentModel&&) = default;
    AssessmentModel& operator=(AssessmentModel&&) = default;
    AssessmentModel(const AssessmentModel&) = delete;
    AssessmentModel& operator=(const AssessmentModel&) = delete;
};

struct Strategy {
    std::unique_ptr<AssessmentModel> assessment_model;

    explicit Strategy(std::unique_ptr<AssessmentModel> model);
    Strategy(Strategy&&) = default;
    Strategy& operator=(Strategy&&) = default;
    Strategy(const Strategy&) = delete;
    Strategy& operator=(const Strategy&) = delete;
};

// --- Game Configuration & State ---
struct GameConfig {
    std::vector<double> awards;
    std::vector<Block> available_blocks; // Requires full Block definition

    GameConfig(std::vector<double> awds, std::vector<Block> blocks);
};

class Board {
public:
    Size size; // Requires full Size definition
    std::vector<std::vector<const Block*>> squares; // Requires forward-declared Block

    explicit Board(Size s);
    Board(const Board& other);
    Board& operator=(const Board& other);
    Board(Board&& other) noexcept;
    Board& operator=(Board&& other) noexcept;

    bool canClearLine(int y) const;
    int getGridHeight() const;
};

class Game {
public:
    GameConfig config; // Requires full GameConfig definition
    Board board;       // Requires full Board definition
    int score;
    std::vector<Block> upcoming_blocks; // Requires full Block definition

    Game(GameConfig cfg, Board brd, int scr = 0, std::vector<Block> upcoming = {});
    Game(const Game& other);
    Game& operator=(const Game& other);
    Game(Game&& other) noexcept;
    Game& operator=(Game&& other) noexcept;

    bool isEnd() const;
    void setEnd();
};

// --- Overall Context ---
struct Context {
    Game game;         // Requires full Game definition
    Strategy strategy; // Requires full Strategy definition

    Context(Game g, Strategy strat);
    Context(Context&&) = default;
    Context& operator=(Context&&) = default;
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
};

#endif // MODELS_H