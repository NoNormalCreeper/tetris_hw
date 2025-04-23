from constants import *
from models import Board, Game, FeatureExtractor
from dbt import DbtFeatureExtractor


def cyan(text: str) -> str:
    """
    将文本变为青色
    
    :param text: 要变为青色的文本
    """
    return f"\033[96m{text}\033[0m"

def visualize_occupied(occupied: list[Position], size: Size) -> None:
    """
    可视化占用的格子，以左下角为原点
    
    :param occupied: 占用的格子列表
    :param size: 方块的大小
    :return: None
    """
    occupied_grid = [
        [" " for _ in range(size.width)]
        for _ in range(size.height)
    ]
    for pos in occupied:
        # Adjust y-coordinate to flip the grid vertically
        occupied_grid[size.height - 1 - pos.y][pos.x] = "X"
    for row in occupied_grid:
        print(cyan("  " + " ".join(row)))

def visualize_block(block: Block) -> None:
    """
    可视化方块
    
    :param block: 方块对象
    :return: None
    """
    print(f"方块名称: {block.name}")
    print(f"方块标签: {block.label}")
    print(f"方块状态数: {block.status_count}")
    print("方块状态:")
    for rotation in block.rotations:
        print(f"  旋转角度: {rotation.label}")
        print(f"  大小: {rotation.size.width} x {rotation.size.height}")
        print("  占用可视化:")
        visualize_occupied(rotation.occupied, rotation.size)


def visualize_blocks(blocks: list[Block]) -> None:
    """
    可视化多个方块
    
    :param blocks: 方块对象列表
    :return: None
    """
    for block in blocks:
        visualize_block(block)
        print("-" * 20)
        print("\n")
        

def visualize_board(board: Board) -> None:
    """
    可视化棋盘
    
    :param board: 棋盘对象
    :return: None
    """
    # 棋盘边界
    horizontal_border = cyan("+" + "-" * (board.size.width * 2 - 1) + "+")
    
    # 显示棋盘（从上到下）
    print(horizontal_border)
    for y in range(board.size.height - 1, -1, -1):  # 从上到下显示
        row = []
        for x in range(board.size.width):
            cell = board.squares[y][x]
            if cell is None:
                row.append(" ")  # 空格子
            else:
                row.append(cell.label)  # 使用方块的标签
        print(cyan("| ") + " ".join(row) + cyan(" |"))
    print(horizontal_border)
    
    # 显示底部坐标
    print("  " + " ".join(str(x) for x in range(board.size.width)))

def visualize_action(game: Game, action: BlockStatus) -> None:
    """
    可视化游戏动作
    
    :param game: 游戏对象
    :param action: 动作对象
    :return: None
    """
    print(f"当前分数: {game.score}")
    print(f"当前方块: {game.upcoming_blocks[0].label}")
    print(f"最佳操作: degree={action.rotation.label}, x={action.x_offset}, score={action.assessment_score}")
    visualize_occupied(action.rotation.occupied, action.rotation.size)

def visualize_game(game: Game, action: BlockStatus) -> None:
    """
    可视化游戏操作
    
    :param game: 游戏对象
    :param action: 动作对象
    :return: None
    """
    print("当前操作:")
    visualize_action(game, action)
    print("棋盘状态:")
    visualize_board(game.board)
    print("即将出现的方块:")
    for block in game.upcoming_blocks:
        print(f"  {block.label}")
    print("-" * 20)

def visualize_dbt_feature(features: list[int]):
    """
    可视化 DBT 特征
    
    :param feature: DBT 特征对象
    :return: None
    """
    print("DBT 特征:")
    feature_texts = [
        f"着陆高度: {features[0]}",
        f"被侵蚀的方块数量: {features[1]}",
        f"行转换方块数: {features[2]}",
        f"列转换方块数: {features[3]}",
        f"空穴数量: {features[4]}",
        f"棋盘井特征量: {features[5]}",
        f"空穴深度: {features[6]}",
        f"有空穴的行数: {features[7]}"
    ]
    print("  " + " | ".join(feature_texts))
    


if __name__ == "__main__":
    # 可视化所有方块
    visualize_blocks(
        [k_block_I, k_block_T, k_block_O, k_block_J, k_block_L, k_block_S, k_block_Z]
    )
