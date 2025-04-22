from models import (
    AssessmentModel,
    BlockStatus,
    Context,
    Game,
    GameConfig,
    Board,
    Size,
    Block,
    FeatureExtractor,
    Strategy,
)

from constants import k_blocks

import random


def calculate_linear_function(params: list[float], vector: list[float]) -> float:
    """
    计算线性函数

    :param params: 参数列表
    :param vector: 向量列表
    :return: 线性函数值
    """
    if len(params) != len(vector):
        raise ValueError("参数列表和向量列表的长度不一致")
    return sum(p * v for p, v in zip(params, vector))


class DbtFeatureExtractor(FeatureExtractor):
    """
    DBT 特征提取器
    """

    # Dellacherie 特征
    landing_height: int = 0  # 着陆高度
    eroded_piece_cells: int = 0  # 被侵蚀的方块数量，消除行数*消除的砖块数
    row_transitions: int = 0  # 从空到满或反之的行转换方块数
    column_transitions: int = 0  # 从空到满或反之的列转换方块数
    holes: int = 0  # 空穴数量
    board_wells: int = 0  # 棋盘井数量

    # B-T 特征，最终返回值中不应有嵌套数组，而是展开为一维数组
    column_heights: list[int] = []  # 每列高度，长度为 width
    column_differences: list[int] = []  # 每列高度差绝对值，长度为 width-1
    maximum_height: int = 0  # 棋盘上最高的方块高度

    # TODO: 实现各个特征提取逻辑函数

    def extract_features(self, game: Game) -> list[float]:
        """
        提取特征

        :param game: 游戏对象
        :return: 特征向量
        """

        return []


my_assessment_model = AssessmentModel(
    length=28,  # 特征向量长度
    weights=[0.0] * 28,  # 权重初始化为 0
    feature_extractor=DbtFeatureExtractor(),  # 特征提取器实例化
)


def get_new_upcoming(ctx: Context) -> list[Block]:
    """
    获取新的即将出现的方块

    :param ctx: 传入一个游戏中的上下文对象
    :return: 新的即将出现的方块列表
    """
    # 如果游戏未开始
    if ctx.game.upcoming_blocks == []:
        # 随机选择两个方块
        block1 = random.choice(ctx.game.config.available_blocks)
        block2 = random.choice(ctx.game.config.available_blocks)
    else:
        # 如果游戏已经开始，使用当前的即将出现的方块
        block1 = ctx.game.upcoming_blocks[0]
        block2 = ctx.game.upcoming_blocks[1]
    return [block1, block2]


def _is_collision(board: Board, action: BlockStatus, y_offset: int) -> bool:
    """
    判断是否发生碰撞

    :param board: 棋盘对象
    :param action: 方块状态
    :param y_offset: 方块在棋盘上的 y 坐标
    :return: 是否发生碰撞
    """
    # 获取方块的旋转状态
    rotation = action.rotation
    occupied = rotation.occupied

    # 遍历方块的占用位置
    for pos in occupied:
        # 计算实际的 x 和 y 坐标
        x = action.x_offset + pos.x
        y = y_offset + pos.y

        # 判断是否越界或碰撞
        if x < 0 or x >= board.size.width or y < 0 or y >= board.size.height:
            return True
        if board.squares[y][x] is not None:
            return True

    return False

def _is_overflow(board: Board, action: BlockStatus, y_offset: int) -> bool:
    """
    判断放下该方块后，是否到达死亡线。不负责判断碰撞，请在调用前判断碰撞
    
    :param board: 棋盘对象
    :param action: 方块状态
    :param y_offset: 方块在棋盘上的 y 坐标
    :return: 是否到达死亡线
    """
    
    return y_offset + action.rotation.size.height >= board.size.height

def _find_y_offset(board: Board, action: BlockStatus) -> int:
    """
    找到放下该方块后，y 坐标的值
    
    :param board: 棋盘对象
    :param action: 方块状态
    :return: y 坐标；若因此游戏结束，则返回 -1
    """

    # 遍历方块的占用位置
    for y_offset in range(board.size.height):
        if not _is_collision(board, action, y_offset):
            if not _is_overflow(board, action, y_offset):
                # 如果没有碰撞，没有到达死亡线，说明可以放下
                return y_offset
            else:
                # 能放下，但是到达了死亡线
                return -1
    
    # 如果没有找到合适的 y 坐标，说明已经放不下了，返回游戏结束
    return -1


def execute_action(ctx: Context, action: BlockStatus) -> Game:
    """
    模拟执行动作（没有实际执行的副作用）
    若抛出异常，则可能是这种动作可能不合法，需要剔除该枚举的可能性

    :param ctx: 传入一个游戏中的上下文对象
    :param action: 要执行的动作
    :return: 更新后的游戏状态
    """

    # TODO: 实现动作执行逻辑
    # 创建一个临时的 Game 对象用于操作
    new_game = ctx.game.model_copy(deep=True)
    board = new_game.board

    # 判断传入的参数是否越界
    rotation = action.rotation
    width = rotation.size.width
    if (action.x_offset < width) or (action.x_offset + width > board.size.width):
        raise ValueError("x_offset 越界")

    # 判断下落后的 y 坐标，以恰好不碰撞为准
    y_offset = _find_y_offset(board, action)
    
    if y_offset == -1:
        # 游戏结束
        new_game.set_end()
        return new_game
    
    # 填充棋盘
    for pos in rotation.occupied:
        x = action.x_offset + pos.x
        y = y_offset + pos.y
        board.squares[y][x] = action.rotation.get_original_block(k_blocks)

    return new_game


def new_game() -> Game:
    """
    创建一个新的游戏对象

    :return: Game对象
    """
    return Game(
        config=GameConfig(awards=[1, 3, 5, 8], available_blocks=k_blocks),
        board=Board(size=Size(10, 14)),
        score=0,
        upcoming_blocks=[],
        available_states=[],
    )


def run_game(ctx: Context) -> None:
    """
    运行游戏

    :param ctx: 传入一个刚初始化的上下文对象
    :return: None
    """
    # 初始化游戏


if __name__ == "__main__":
    # 测试代码
    ctx = Context(
        game=new_game(), strategy=Strategy(assessment_model=my_assessment_model)
    )
    run_game(ctx)
