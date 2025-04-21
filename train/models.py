from pydantic import BaseModel, SkipValidation

class Position(BaseModel):
    """
    位置类
    """

    x: int  # x 坐标
    y: int  # y 坐标
    
    def __init__(self, x=None, y=None, **kwargs):
        # 同时支持位置参数和关键字参数
        if x is not None and y is None and not kwargs and isinstance(x, (list, tuple)) and len(x) == 2:
            super().__init__(x=x[0], y=x[1])
        else:
            super().__init__(x=x, y=y, **kwargs)

class Size(BaseModel):
    """
    大小类
    """

    width: int
    height: int
    
    def __init__(self, width=None, height=None, **kwargs):
        # 同时支持位置参数和关键字参数
        if width is not None and height is None and not kwargs and isinstance(width, (list, tuple)) and len(width) == 2:
            super().__init__(width=width[0], height=width[1])
        else:
            super().__init__(width=width, height=height, **kwargs)


class BlockRoatation(BaseModel):
    """
    （用于定义枚举）方块的状态类
    """

    label: str  # 该状态的标签
    size: Size  # 该状态的大小
    occupied: list[Position]  # 占用的格子相对坐标，以左下角为原点


class BlockStatus(BaseModel):
    """
    用于枚举最优策略，方块的状态类
    """

    x_offset: int  # 方块在棋盘上的 x 坐标
    rotation: BlockRoatation
    assessment_score: float  # 评估分数


class Block(BaseModel):
    """
    （用于定义枚举）方块类型的基类
    """

    name: str  # 方块名称
    label: str  # 方块显示标签
    status_count: int  # 方块状态的数量
    rotations: list[BlockRoatation]  # 方块的可用旋转模式列表


class AssessmentModel(BaseModel):
    """
    评估模型的基类
    """

    length: int  # 参数个数
    weights: list[float]  # 权重

    from typing import Callable

    param_functions: list[Callable[["Game"], float]]  # 获取参数的函数，Game -> float
    # params: list[float]  # 模型参数

    class Config:
        arbitrary_types_allowed = True


class Strategy(BaseModel):
    """
    策略的上下文
    """

    assessment_model: AssessmentModel  # 评估模型


class GameConfig(BaseModel):
    """
    游戏的配置
    """

    awards: list[float]  # 消除 1, 2, 3, 4 行的奖励
    available_blocks: list[Block]  # 可用的方块列表


# class BoardSquare(BaseModel):
#     """
#     棋盘的一个小方块
#     """

#     block: Block  # 被占用的方块类型
#     status: BlockStatus  # 方块状态
#     x_offset: int  # 方块在棋盘上的 x 坐标
#     y_offset: int  # 方块在棋盘上的 y 坐标


class Board(BaseModel):
    """
    棋盘的上下文信息和配置
    """

    block_types: list[Block]  # 棋盘上可用的方块类型
    size: Size  # 棋盘可操作空间大小，高度为死亡判定线减 1
    squares: list[list[Block]]  # 棋盘上的小方块矩阵


class Game(BaseModel):
    """
    游戏的上下文信息
    """

    config: GameConfig  # 游戏配置

    board: Board  # 棋盘当前状态
    score: int  # 当前分数

    upcoming_blocks: list[Block]  # 即将出现的方块列表，共 2 个
    available_states: list[BlockStatus]  # 可用的方块状态列表


class Context(BaseModel):
    """
    整个程序的上下文信息
    """

    game: Game  # 游戏上下文
    strategy: Strategy  # 策略上下文
