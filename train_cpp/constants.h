#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "models.h" // Include Block definition
#include <vector>

// Declare the blocks defined in constants.cpp
extern const Block k_block_I;
extern const Block k_block_T;
extern const Block k_block_O;
extern const Block k_block_J;
extern const Block k_block_L;
extern const Block k_block_S;
extern const Block k_block_Z;

// Declare the vector of block pointers (or copies, depending on definition)
// Using pointers to avoid copying the const Block objects
extern const std::vector<const Block*> k_blocks;

#endif // CONSTANTS_H