import numpy as np

# --- 配置参数 ---
n = 20
num_params = n + 1  # 系数个数 (a0 到 an)
population_size = 100  # 每轮生成的候选参数组数量 (N)
elite_frac = 0.1     # 选择多少比例的精英 (P%)
num_iterations = 50   # 迭代轮数
num_games_per_eval = 10 # 每组参数评估时玩多少局游戏 (L)
initial_std_dev = 5.0 # 初始标准差

# --- 初始化分布 ---
mu = np.zeros(num_params)
sigma = np.ones(num_params) * initial_std_dev

def run_single_game(params: list[float]):
    """
    运行一局游戏，使用给定的参数 params = [a0, ..., an] 作为评估函数系数。
    返回游戏得分。
    """
    # 这里你需要实现游戏逻辑
    # 使用 params 来评估游戏状态
    # 返回得分
    pass  # TODO: 实现游戏逻辑

# --- 模拟游戏的函数 (你需要实现这个！) ---
def evaluate_parameters(params: list[float]):
    """
    使用给定的参数 params = [a0, ..., an] 运行 AI 玩 num_games_per_eval 局游戏，
    返回平均得分。
    """
    total_score = 0
    for _ in range(num_games_per_eval):
        # 初始化游戏环境
        # 运行一局游戏直到结束，使用 params 作为评估函数系数
        score = run_single_game(params) # <--- 你需要实现这个函数
        # 为了示例，这里用随机数代替
        # score = np.random.rand() * 1000 # 模拟得分
        # total_score += score
    return total_score / num_games_per_eval

# --- CE 主循环 ---
for iteration in range(num_iterations):
    print(f"Iteration {iteration+1}/{num_iterations}")

    # 1. 从当前分布采样 N 组参数
    population_params = np.random.normal(mu, sigma, size=(population_size, num_params))

    # 2. 评估每组参数
    scores = np.array([evaluate_parameters(params) for params in population_params])
    print(f"  Average score this iteration: {np.mean(scores):.2f}")
    print(f"  Max score this iteration: {np.max(scores):.2f}")


    # 3. 选择精英
    num_elites = int(population_size * elite_frac)
    elite_indices = np.argsort(scores)[-num_elites:] # 分数越高越好，取最后几个
    elite_params = population_params[elite_indices]

    # 4. 更新分布参数 (均值和标准差)
    mu = np.mean(elite_params, axis=0)
    sigma = np.std(elite_params, axis=0) + 1e-6 # 加一个小值防止标准差为0

    print(f"  New mu (first 3): {mu[:3]}")
    print(f"  New sigma (first 3): {sigma[:3]}")


# --- 训练结束 ---
best_params = mu # 通常用最后得到的均值作为最优参数
print("\nTraining finished!")
print(f"Best parameters found (estimated mean): {best_params}")

# 你可以使用 best_params 来驱动你的最终 AI