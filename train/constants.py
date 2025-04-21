from models import (
    Block,
    BlockStatus,
    BlockRoatation,
    Size,
    AssessmentModel,
    Strategy,
    Position,
)

# 方块和方块状态
k_block_I = Block(
    name="I",
    label="I",
    status_count=2,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(4, 1),
            occupied=[Position(0, 0), Position(1, 0), Position(2, 0), Position(3, 0)],
        ),
        BlockRoatation(
            label="90",
            size=Size(1, 4),
            occupied=[Position(0, 0), Position(0, 1), Position(0, 2), Position(0, 3)],
        ),
    ],
)

k_block_T = Block(
    name="T",
    label="T",
    status_count=4,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(3, 2),
            occupied=[Position(0, 0), Position(1, 0), Position(2, 0), Position(1, 1)],
        ),
        BlockRoatation(
            label="90",
            size=Size(2, 3),
            occupied=[Position(0, 0), Position(0, 1), Position(1, 1), Position(0, 2)],
        ),
        BlockRoatation(
            label="180",
            size=Size(3, 2),
            occupied=[Position(0, 1), Position(1, 1), Position(2, 1), Position(1, 0)],
        ),
        BlockRoatation(
            label="270",
            size=Size(2, 3),
            occupied=[Position(0, 1), Position(1, 0), Position(1, 1), Position(1, 2)],
        ),
    ],
)

k_block_O = Block(
    name="O",
    label="O",
    status_count=1,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(2, 2),
            occupied=[Position(0, 0), Position(1, 0), Position(0, 1), Position(1, 1)],
        ),
    ],
)

k_block_J = Block(
    name="J",
    label="J",
    status_count=4,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(3, 2),
            occupied=[Position(0, 0), Position(1, 0), Position(2, 0), Position(0, 1)],
        ),
        BlockRoatation(
            label="90",
            size=Size(2, 3),
            occupied=[Position(0, 0), Position(0, 1), Position(0, 2), Position(1, 2)],
        ),
        BlockRoatation(
            label="180",
            size=Size(3, 2),
            occupied=[Position(0, 1), Position(1, 1), Position(2, 1), Position(2, 0)],
        ),
        BlockRoatation(
            label="270",
            size=Size(2, 3),
            occupied=[Position(0, 0), Position(1, 0), Position(1, 1), Position(1, 2)],
        ),
    ],
)

k_block_L = Block(
    name="L",
    label="L",
    status_count=4,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(3, 2),
            occupied=[Position(0, 0), Position(1, 0), Position(2, 0), Position(2, 1)],
        ),
        BlockRoatation(
            label="90",
            size=Size(2, 3),
            occupied=[Position(0, 0), Position(0, 1), Position(0, 2), Position(1, 0)],
        ),
        BlockRoatation(
            label="180",
            size=Size(3, 2),
            occupied=[Position(0, 1), Position(1, 1), Position(2, 1), Position(0, 0)],
        ),
        BlockRoatation(
            label="270",
            size=Size(2, 3),
            occupied=[Position(0, 2), Position(1, 0), Position(1, 1), Position(1, 2)],
        ),
    ],
)

k_block_S = Block(
    name="S",
    label="S",
    status_count=2,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(3, 2),
            occupied=[Position(0, 0), Position(1, 0), Position(1, 1), Position(2, 1)],
        ),
        BlockRoatation(
            label="90",
            size=Size(2, 3),
            occupied=[Position(0, 1), Position(0, 2), Position(1, 0), Position(1, 1)],
        ),
    ],
)

k_block_Z = Block(
    name="Z",
    label="Z",
    status_count=2,
    rotations=[
        BlockRoatation(
            label="0",
            size=Size(3, 2),
            occupied=[Position(0, 1), Position(1, 1), Position(1, 0), Position(2, 0)],
        ),
        BlockRoatation(
            label="90",
            size=Size(2, 3),
            occupied=[Position(0, 0), Position(0, 1), Position(1, 1), Position(1, 2)],
        ),
    ],
)
