#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "models.h"
#include <vector>
#include <set> // For std::set

// Forward declaration
class Board;
struct BlockStatus;
struct Position;

class MyDbtFeatureExtractorCpp : public FeatureExtractor {
private:
    // Helper struct to hold calculated features temporarily
    struct Features {
        int landing_height = 0;
        int eroded_piece_cells = 0;
        int row_transitions = 0;
        int column_transitions = 0;
        int holes = 0;
        int board_wells = 0;
        int hole_depth = 0;
        int rows_with_holes = 0;
        std::vector<int> column_heights;
        std::vector<int> column_differences;
        int maximum_height = 0;
    };

    // Helper functions corresponding to Python _get_* methods
    // They now take the relevant board state(s) as const references
    // and return the calculated value directly.
    int calculateLandingHeight(int y_offset) const;
    int calculateErodedPieceCells(const Board& board_before_elim, const BlockStatus& action, int y_offset, const std::vector<int>& full_lines) const;
    int calculateRowTransitions(const Board& board_after_elim) const;
    int calculateColumnTransitions(const Board& board_after_elim) const;
    void calculateHolesAndDepth(const Board& board_after_elim, int& holes_count, int& total_hole_depth, int& rows_with_holes_count) const;
    int calculateBoardWells(const Board& board_after_elim) const;
    std::vector<int> calculateColumnHeights(const Board& board_after_elim) const;
    std::vector<int> calculateColumnDifferences(const std::vector<int>& column_heights) const;
    int calculateMaximumHeight(const std::vector<int>& column_heights) const;

    // Helper to get full lines (needed for eroded cells)
    std::vector<int> getFullLines(const Board& board) const;
    // Helper to check if a line is full
    bool isFullLine(const std::vector<const Block*>& line, int width) const;


public:
    // Override the pure virtual function from the base class
    std::vector<int> extractFeatures(const Game& game, const BlockStatus& action) const override;
};

extern int findYOffset(const Board& board, const BlockStatus& action);
extern int eliminateLines(Board& board); // Declaration, definition might be in game.cpp or here

#endif // EXTRACTOR_H