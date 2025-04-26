#include "models.h" // Include the header with definitions
#include "constants.h" // Include the header file
#include <vector>

// Define the constant blocks using the constructors from models.h/models.cpp

const Block k_block_I(
    "I", "I", 2,
    {
        BlockRotation("0", Size(4, 1), {Position(0, 0), Position(1, 0), Position(2, 0), Position(3, 0)}),
        BlockRotation("90", Size(1, 4), {Position(0, 0), Position(0, 1), Position(0, 2), Position(0, 3)})
    }
);

const Block k_block_T(
    "T", "T", 4,
    {
        BlockRotation("0", Size(3, 2), {Position(0, 0), Position(1, 0), Position(2, 0), Position(1, 1)}),
        BlockRotation("90", Size(2, 3), {Position(0, 0), Position(0, 1), Position(1, 1), Position(0, 2)}),
        BlockRotation("180", Size(3, 2), {Position(0, 1), Position(1, 1), Position(2, 1), Position(1, 0)}),
        BlockRotation("270", Size(2, 3), {Position(0, 1), Position(1, 0), Position(1, 1), Position(1, 2)})
    }
);

const Block k_block_O(
    "O", "O", 1,
    {
        BlockRotation("0", Size(2, 2), {Position(0, 0), Position(1, 0), Position(0, 1), Position(1, 1)})
    }
);

const Block k_block_J(
    "J", "J", 4,
    {
        BlockRotation("0", Size(3, 2), {Position(0, 0), Position(1, 0), Position(2, 0), Position(0, 1)}),
        BlockRotation("90", Size(2, 3), {Position(0, 0), Position(0, 1), Position(0, 2), Position(1, 2)}),
        BlockRotation("180", Size(3, 2), {Position(0, 1), Position(1, 1), Position(2, 1), Position(2, 0)}),
        BlockRotation("270", Size(2, 3), {Position(0, 0), Position(1, 0), Position(1, 1), Position(1, 2)})
    }
);

const Block k_block_L(
    "L", "L", 4,
    {
        BlockRotation("0", Size(3, 2), {Position(0, 0), Position(1, 0), Position(2, 0), Position(2, 1)}),
        BlockRotation("90", Size(2, 3), {Position(0, 0), Position(0, 1), Position(0, 2), Position(1, 0)}),
        BlockRotation("180", Size(3, 2), {Position(0, 1), Position(1, 1), Position(2, 1), Position(0, 0)}),
        BlockRotation("270", Size(2, 3), {Position(0, 2), Position(1, 0), Position(1, 1), Position(1, 2)})
    }
);

const Block k_block_S(
    "S", "S", 2,
    {
        BlockRotation("0", Size(3, 2), {Position(0, 0), Position(1, 0), Position(1, 1), Position(2, 1)}),
        BlockRotation("90", Size(2, 3), {Position(0, 1), Position(0, 2), Position(1, 0), Position(1, 1)})
    }
);

const Block k_block_Z(
    "Z", "Z", 2,
    {
        BlockRotation("0", Size(3, 2), {Position(0, 1), Position(1, 1), Position(1, 0), Position(2, 0)}),
        BlockRotation("90", Size(2, 3), {Position(0, 0), Position(0, 1), Position(1, 1), Position(1, 2)})
    }
);

// Define the vector containing copies of the constant blocks
const std::vector<const Block*> k_blocks = {
    &k_block_I,
    &k_block_T,
    &k_block_O,
    &k_block_J,
    &k_block_L,
    &k_block_S,
    &k_block_Z
};
