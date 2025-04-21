from models import BlockStatus, Context, Game, GameConfig, Board, Size, Block

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

class FeatureExtractor:
    """
    特征提取器
    """
    
    # Dellacherie 特征
    landing_height: int = 0  # 着陆高度
    eroded_piece_cells: int = 0  # 被侵蚀的方块数量，消除行数*消除的砖块数
    row_transitions: int = 0  # 从空到满或反之的行转换方块数
    column_transitions: int = 0  # 从空到满或反之的列转换方块数
    holes: int = 0  # 空穴数量
    board_wells: int = 0  # 棋盘井数量
    
    # B-T 特征
    column_heights: list[int] = []  # 每列高度，长度为 width
    column_differences: list[int] = []  # 每列高度差绝对值，长度为 width-1
    maximum_height: int = 0  # 棋盘上最高的方块高度

    def __init__(self, params: list[float]):
        self.params = params
        
    # TODO: 实现各个特征提取逻辑函数

    def extract_features(self, game: Game) -> list[float]:
        """
        提取特征

        :param game: 游戏对象
        :return: 特征向量
        """
        
        return []

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



def execute_action(ctx: Context, action: BlockStatus) -> Game:
    """
    模拟执行动作（没有实际执行的副作用）

    :param ctx: 传入一个游戏中的上下文对象
    :param action: 要执行的动作
    :return: 更新后的游戏状态
    """
    
    # TODO: 实现动作执行逻辑
    
    return ctx.game



def run_game(ctx: Context) -> None:
    """
    运行游戏

    :param ctx: 传入一个刚初始化的上下文对象
    :return: None
    """
    # 初始化游戏
    ctx.game = Game(
        config=GameConfig(awards=[1, 3, 5, 8], available_blocks=k_blocks),
        board=Board(block_types=k_blocks, size=Size(10, 14), squares=[]),
        score=0,
        upcoming_blocks=[],
        available_states=[],
    )
