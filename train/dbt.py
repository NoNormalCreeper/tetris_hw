from models import FeatureExtractor


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
    
    # 论文作者引入的新的两个特征值
    hole_depth: int = 0
    rows_with_holes: int = 0