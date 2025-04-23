from models import (
    AssessmentModel,
    BlockStatus,
    Context,
    Game,
    GameConfig,
    Board,
    Position,
    Size,
    Block,
    FeatureExtractor,
    Strategy,
)
from dbt import DbtFeatureExtractor
import visualize

from constants import k_blocks

import random


def create_new_game() -> Game:
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


def calculate_linear_function(params: list[float | int], vector: list[float | int]) -> float:
    """
    计算线性函数

    :param params: 参数列表
    :param vector: 向量列表
    :return: 线性函数值
    """
    if len(params) != len(vector):
        raise ValueError("参数列表和向量列表的长度不一致")
    return sum(p * v for p, v in zip(params, vector))


class MyDbtFeatureExtractor(DbtFeatureExtractor):
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
    
    # 论文作者引入的新的两个特征值
    hole_depth: int = 0
    rows_with_holes: int = 0
    
    new_game: Game = create_new_game()  # 新的游戏对象

    def _init_new_game(self, game: Game, action: BlockStatus) -> None:
        """
        （仅有副作用）拷贝一个新的游戏对象，执行 action 后存储，用于后续操作

        :param game: 执行过操作的游戏对象
        :param action: 执行的操作
        :return: None
        """
        self.new_game = game.model_copy(deep=True)
        self.new_game = execute_action(self.new_game, action)
        
    def _get_landing_height(self, action: BlockStatus) -> int:
        self.landing_height = _find_y_offset(self.new_game.board, action)
        return self.landing_height

    def _get_eroded_piece_cells(self, action: BlockStatus) -> int:
        """
        计算被侵蚀的方块数量
        需要在调用 _get_landing_height 之后调用
        
        :param action: 方块状态
        :return: 被侵蚀的方块数量
        """
        full_lines = get_full_lines(self.new_game.board)

        # 计算贡献单元格：在刚才放置的那个方块本身包含的单元格中，有多少个是位于被消除掉的那些行里的
        contributed_to_lines = list(
            filter(
                lambda line_y: self.landing_height
                <= line_y
                < self.landing_height + action.rotation.size.height,
                full_lines,
            )
        )

        # 计算总贡献单元格数
        eliminate_bricks = 0
        for cell in action.rotation.occupied:
            # 计算该单元格在消除行中的贡献
            for line_y in contributed_to_lines:
                if cell.y + self.landing_height == line_y:
                    eliminate_bricks += 1

        self.eroded_piece_cells = len(contributed_to_lines) * eliminate_bricks

        return self.eroded_piece_cells

    def _get_row_transitions(self) -> int:
        """
        计算行转换数

        :return: 行转换数
        """
        board = self.new_game.board
        transitions = 0
        
        for row in range(board.size.height):
            for index, block in enumerate(
                board.squares[row][0 : board.size.width - 1]
            ):  # 只检查到倒数第二列
                # 检查每个方块与右边的方块是否存在转换
                if (block is None) != (board.squares[row][index + 1] is None):
                    transitions += 1
        
        self.row_transitions = transitions
        return self.row_transitions
    
    def _get_column_transitions(self) -> int:
        """
        计算列转换数

        :return: 列转换数
        """
        board = self.new_game.board
        transitions = 0
        
        for col in range(board.size.width):
            for row in range(board.size.height - 1):  # 只检查到倒数第二行
                # 检查每个方块与下边的方块是否存在转换
                if (board.squares[row][col] is None) != (
                    board.squares[row + 1][col] is None
                ):
                    transitions += 1
                    
        self.column_transitions = transitions
        return self.column_transitions
    
    def __get_hole_depth(self, hole: Position) -> int:
        """
        计算一个空穴的深度（空穴上方被占用的方块数）

        :param hole: 空穴位置
        :return: 空穴深度
        """
        board = self.new_game.board
        depth = 0
        
        # 从空穴位置向上检查
        for row in range(hole.y + 1, board.size.height):
            if board.squares[row][hole.x] is not None:
                # 找到一个被占据的方块，深度加1
                depth += 1
            # 不再有提前返回，会检查所有上方格子
        
        return depth
    
    def _get_holes(self) -> int:
        """
        计算空穴数。
        同时会计算空穴深度、含空穴的行数

        :return: 空穴数
        """
        board = self.new_game.board
        # holes = 0
        depths = []
        rows_with_holes = set()
        
        for col in range(board.size.width):
            # 从底部开始检查每一行
            for row in range(board.size.height):
                # 检查当前单元格是否为空
                if board.squares[row][col] is None:
                    # 计算空穴深度
                    depth = self.__get_hole_depth(Position(col, row))
                    if depth > 0:
                        depths.append(depth)
                        rows_with_holes.add(row)
        
        self.hole_depth = sum(depths)
        self.rows_with_holes = len(rows_with_holes)
        self.holes = len(depths)
        return self.holes
    
    def _get_board_wells(self) -> int:
        """
        计算棋盘井数

        :return: 棋盘井数
        """
        board = self.new_game.board
        depths = []
        
        wells_sum = 0

        # 遍历所有列
        for col in range(board.size.width):
            current_well_depth = 0
            
            # 从上到下检查每个单元格
            for row in range(board.size.height):
                # 检查当前单元格是否为空
                if board.squares[row][col] is None:
                    # 检查是否是井
                    is_well = False
                    
                    # 中间列：左右邻居必须都被占据
                    if 0 < col < board.size.width - 1:
                        is_well = (board.squares[row][col-1] is not None) and (board.squares[row][col+1] is not None)
                    # 最左列：右邻居必须被占据
                    elif col == 0:
                        is_well = board.squares[row][col+1] is not None
                    # 最右列：左邻居必须被占据
                    else:  # col == board.size.width - 1
                        is_well = board.squares[row][col-1] is not None
                    
                    if is_well:
                        # 这是井的一部分
                        current_well_depth += 1
                    else:
                        # 不是井，计算前一个井的惩罚值（如果有的话）
                        if current_well_depth > 0:
                            wells_sum += (current_well_depth * (current_well_depth + 1)) // 2
                            current_well_depth = 0
                else:
                    # 单元格被占据，计算前一个井的惩罚值（如果有的话）
                    if current_well_depth > 0:
                        wells_sum += (current_well_depth * (current_well_depth + 1)) // 2
                        current_well_depth = 0
            
            # 处理列底部可能剩余的井
            if current_well_depth > 0:
                wells_sum += (current_well_depth * (current_well_depth + 1)) // 2

        self.board_wells = wells_sum
        return self.board_wells
    
    def _get_column_heights(self) -> list[int]:
        """
        计算每列的高度

        :return: 每列的高度列表
        """
        board = self.new_game.board
        heights = []
        
        for col in range(board.size.width):
            height = 0
            
            # 从顶部到底部检查每个单元格
            for row in range(board.size.height):
                # 检查当前单元格是否被占据
                if board.squares[row][col] is not None:
                    height = board.size.height - row
                    break
            heights.append(height)
        
        self.column_heights = heights
        return self.column_heights
    
    def _get_column_differences(self) -> list[int]:
        """
        计算每列的高度差绝对值
        需要在调用 _get_column_heights 之后调用

        :return: 每列的高度差绝对值列表
        """
        heights = self.column_heights
        differences = []
        
        for i in range(len(heights) - 1):
            differences.append(abs(heights[i] - heights[i + 1]))
        
        self.column_differences = differences
        return self.column_differences
    
    def _get_maximum_height(self) -> int:
        """
        计算棋盘上最高的方块高度

        :return: 棋盘上最高的方块高度
        """
        heights = self.column_heights
        self.maximum_height = max(heights)
        return self.maximum_height

    def extract_features(self, game: Game, action: BlockStatus) -> list[int]:
        """
        提取特征

        :param game: 游戏对象
        :return: 特征向量
        """

        try:
            self._init_new_game(game, action)
        except ValueError:
            # 代表该状态无法进行游戏（死亡或越界），继续向上抛错误来处理，代表不应该执行该操作
            raise ValueError("游戏结束或越界")
        
        self._get_landing_height(action)
        self._get_eroded_piece_cells(action)
        self._get_row_transitions()
        self._get_column_transitions()
        self._get_holes()
        self._get_board_wells()
        self._get_column_heights()
        self._get_column_differences()
        self._get_maximum_height()
        
        # 特征向量
        feature_vector = [  # 28 维
            self.landing_height,
            self.eroded_piece_cells,
            self.row_transitions,
            self.column_transitions,
            self.holes,
            self.board_wells,
            self.hole_depth,
            self.rows_with_holes,
            *self.column_heights,
            *self.column_differences,
            self.maximum_height,
        ]
        
        # 清除所有数据便于下次使用
        self.landing_height = 0
        self.eroded_piece_cells = 0
        self.row_transitions = 0
        self.column_transitions = 0
        self.holes = 0
        self.board_wells = 0
        self.hole_depth = 0
        self.rows_with_holes = 0
        self.column_heights = []
        self.column_differences = []
        self.maximum_height = 0
        self.new_game = create_new_game()
        
        # 返回结果
        return feature_vector


my_assessment_model = AssessmentModel(
    length=28,  # 特征向量长度
    # weights=[0.0] * 28,  # 权重初始化为 0
    weights=[-12.63, 6.60, -9.22, -19.77, -13.08, -10.49, -1.61, -24.04] + [0.0] * 20,
    feature_extractor=MyDbtFeatureExtractor(),  # 特征提取器实例化
)


def get_new_upcoming(game: Game) -> list[Block]:
    """
    获取新的即将出现的方块

    :param game: 游戏对象
    :return: 新的即将出现的方块列表
    """
    # 如果游戏未开始
    if game.upcoming_blocks == []:
        # 随机选择两个方块
        block1 = random.choice(game.config.available_blocks)
        block2 = random.choice(game.config.available_blocks)
    else:
        # 如果游戏已经开始，使用当前的即将出现的方块
        block1 = game.upcoming_blocks[1]
        block2 = random.choice(game.config.available_blocks)
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

def get_full_lines(board: Board) -> list[int]:
    """
    获取填满的行

    :param board: 棋盘对象
    :return: 填满行的 y 坐标列表
    """
    
    # 遍历棋盘，找到满行
    full_lines = []
    for line in range(board.size.height):
        if all(board.squares[line][x] is not None for x in range(board.size.width)):
            full_lines.append(line)
            
    return full_lines


def _eliminate_lines(board: Board) -> int:
    """
    （产生副作用）消除以一次操作后的满行，并返回消除的行数

    :param board: 棋盘对象
    :return: 消除的行数
    """
    
    full_lines = get_full_lines(board)
    # 消除满行
    for line in full_lines:
        for x in range(board.size.width):
            board.squares[line][x] = None

    # 下移上面的行
    for line in full_lines:
        for y in range(line + 1, board.size.height):
            for x in range(board.size.width):   # 移动
                board.squares[y - 1][x] = board.squares[y][x]
                board.squares[y][x] = None

    return len(full_lines)


def execute_action(game: Game, action: BlockStatus) -> Game:
    """
    模拟执行动作（没有实际执行的副作用）
    若抛出异常，则可能是这种动作可能不合法，需要剔除该枚举的可能性

    :param ctx: 传入一个游戏中的上下文对象
    :param action: 要执行的动作
    :return: 更新后的游戏状态
    """

    # TODO: 实现动作执行逻辑
    # 创建一个临时的 Game 对象用于操作
    new_game = game.model_copy(deep=True)
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
        # 只要这里超出了死亡线，再怎么消除都没用，所以可以直接就在这结束了
        new_game.set_end()
        raise ValueError("游戏结束")
        return new_game
    
    # 填充棋盘
    for pos in rotation.occupied:
        x = action.x_offset + pos.x
        y = y_offset + pos.y
        board.squares[y][x] = action.rotation.get_original_block(k_blocks)
    
    # 消除满行
    eliminated_lines = _eliminate_lines(board)
    
    # 更新分数
    if eliminated_lines > 0:
        new_game.score += int(new_game.config.awards[eliminated_lines - 1]) * 100
        
    # 更新即将出现的方块
    new_game.upcoming_blocks = get_new_upcoming(new_game)

    return new_game


def all_actions(block: Block) -> list[BlockStatus]:
    """
    获取所有可能的动作

    :param block: 下一个方块
    :return: 所有可能的动作列表
    """
    actions = []
    for rotation in block.rotations:
        for x_offset in range(0, 10 - rotation.size.width + 1):
            actions.append(BlockStatus(x_offset=x_offset, rotation=rotation, assessment_score=None))
    return actions



def find_best_action(game: Game, actions: list[BlockStatus], weights: list[float]) -> BlockStatus:
    """
    找到最佳的操作策略

    :param game: 游戏对象
    :param actions: 所有可能的动作列表
    :return: 最佳的操作策略
    """
    feature_extractor = MyDbtFeatureExtractor()
    
    best_action = actions[0]
    best_score = float("-inf")

    for action in actions:
        # Convert the feature list to the expected type
        try:
            features = [float(x) for x in feature_extractor.extract_features(game, action)]
            action.assessment_score = calculate_linear_function(weights, features)
        except ValueError:
            # 代表该状态无法进行游戏
            action.assessment_score = float("-inf")
    
        if action.assessment_score > best_score:
            best_action = action
            best_score = action.assessment_score
    
    visualize.visualize_dbt_feature(features) # type: ignore
    return best_action

def run_game(ctx: Context) -> None:
    """
    运行游戏（自动进行）
    暂时为考虑一个方块的版本

    :param ctx: 传入一个刚初始化的上下文对象
    :return: None
    """
    # 初始化游戏
    
    while ctx.game.score >= 0:
        # 获取当前游戏状态
        game = ctx.game
        board = game.board

        # 获取下一个方块
        game.upcoming_blocks = get_new_upcoming(game)
        
        # 获取所有可能的动作
        actions = all_actions(game.upcoming_blocks[0])

        # 找到最佳的动作
        best_action = find_best_action(game, actions, ctx.strategy.assessment_model.weights)
        
        visualize.visualize_game(game, best_action)
        # visualize.visualize_dbt_feature()
        input("Press Enter to continue...")  # 暂时使用输入来控制游戏进行
        
        # 执行最佳动作
        game = execute_action(game, best_action)
        
        # 更新游戏状态
        ctx.game = game
    


if __name__ == "__main__":
    # 测试代码
    ctx = Context(
        game=create_new_game(), strategy=Strategy(assessment_model=my_assessment_model)
    )
    print("游戏开始")
    run_game(ctx)
