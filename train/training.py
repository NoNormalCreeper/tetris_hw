import numpy as np
from game import MyDbtFeatureExtractor, create_new_game, run_game, run_game_for_training
from models import AssessmentModel, Context, Strategy
from icecream import ic
import logging
import concurrent.futures # Added import
import time # Added import
import threading # Added import

from dbt import DbtFeatureExtractor


logging.basicConfig(filename='train.log', level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(threadName)s - %(message)s') # Added format

logger = logging.getLogger(__name__)


# --- 配置参数 ---
n = 8
num_params = n + 1  # 系数个数 (a0 到 an)
population_size = 100  # 每轮生成的候选参数组数量 (N)
elite_frac = 0.1     # 选择多少比例的精英 (P%)
num_iterations = 50   # 迭代轮数
num_games_per_eval = 10 # 每组参数评估时玩多少局游戏 (L)
initial_std_dev = 5.0 # 初始标准差
max_workers = 32 # Number of threads to use for evaluation (adjust as needed)

# --- 初始化分布 ---
# Define your good starting parameters here. Make sure the list has num_params elements.
initial_good_params = [-12.63, 6.60, -9.22, -19.77, -13.08, -10.49, -1.61, -24.04, 0.0] # Use the first 8 from game.py + a bias term
# Ensure the list has the correct length (num_params)
if len(initial_good_params) != num_params:
    raise ValueError(f"initial_good_params must have length {num_params}, but got {len(initial_good_params)}")

mu = np.array(initial_good_params) # Initialize mu with the good parameters

sigma = np.ones(num_params) * initial_std_dev

def run_single_game(params: list[float]):
    """
    运行一局游戏，使用给定的参数 params = [a0, ..., an] 作为评估函数系数。
    返回游戏得分。
    """
    # 这里你需要实现游戏逻辑
    # 使用 params 来评估游戏状态
    # 返回得分
    return run_game_for_training(params)

# --- 模拟游戏的函数 (你需要实现这个！) ---
def evaluate_parameters(params_tuple): # Accept a tuple (index, params)
    """
    使用给定的参数 params = [a0, ..., an] 运行 AI 玩 num_games_per_eval 局游戏，
    返回平均得分。
    """
    idx, params = params_tuple # Unpack index and params
    thread_name = threading.current_thread().name
    logger.debug(f"Task {idx}: Starting evaluation for params {params[:3]}... on {thread_name}")
    start_time = time.time()
    total_score = 0
    for i in range(num_games_per_eval):
        logger.debug(f"Task {idx}: Starting game {i+1}/{num_games_per_eval}")
        # 初始化游戏环境
        # 运行一局游戏直到结束，使用 params 作为评估函数系数
        score = run_single_game(params) # <--- 你需要实现这个函数
        # 为了示例，这里用随机数代替
        # score = np.random.rand() * 1000 # 模拟得分
        total_score += score
        logger.debug(f"Task {idx}: Finished game {i+1}/{num_games_per_eval}, score: {score}")

    avg_score = total_score / num_games_per_eval
    end_time = time.time()
    logger.debug(f"Task {idx}: Finished evaluation in {end_time - start_time:.2f}s. Avg score: {avg_score:.2f}")
    return idx, avg_score # Return index along with score

# --- CE 主循环 ---
for iteration in range(num_iterations):
    logger.info(f"Iteration {iteration+1}/{num_iterations}")

    # 1. 从当前分布采样 N 组参数
    population_params = np.random.normal(mu, sigma, size=(population_size, num_params))
    # Create tuples of (index, params) for tracking
    params_with_indices = list(enumerate(population_params))


    # 2. 并行评估每组参数
    scores = np.zeros(population_size)
    logger.info(f"Starting parallel evaluation of {population_size} parameter sets using {max_workers} workers...")
    start_eval_time = time.time()
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
        # 使用 map 来并行执行 evaluate_parameters
        # Pass the list of (index, params) tuples
        results = executor.map(evaluate_parameters, params_with_indices)

        # 将结果收集到 scores 数组中, preserving order using the returned index
        completed_count = 0
        for idx, score in results:
            scores[idx] = score
            completed_count += 1
            if completed_count % 10 == 0 or completed_count == population_size: # Log every 10 completed or at the end
                logger.info(f"  Evaluation progress: {completed_count}/{population_size} parameter sets evaluated.")


    end_eval_time = time.time()
    logger.info(f"Parallel evaluation finished in {end_eval_time - start_eval_time:.2f} seconds.")

    logger.info(f"  Average score this iteration: {np.mean(scores):.2f}")
    logger.info(f"  Max score this iteration: {np.max(scores):.2f}")


    # 3. 选择精英
    num_elites = int(population_size * elite_frac)
    # Ensure elite indices are integers if needed later, argsort returns indices
    elite_indices = np.argsort(scores)[-num_elites:].astype(int) # 分数越高越好，取最后几个
    elite_params = population_params[elite_indices]

    # 4. 更新分布参数 (均值和标准差)
    mu = np.mean(elite_params, axis=0)
    sigma = np.std(elite_params, axis=0) + 1e-6 # 加一个小值防止标准差为0

    logger.info(f"  New mu (first 3): {mu[:3]}")
    logger.info(f"  New sigma (first 3): {sigma[:3]}")


# --- 训练结束 ---
best_params = mu # 通常用最后得到的均值作为最优参数
logger.info("\nTraining finished!")
logger.info(f"Best parameters found (estimated mean): {best_params}")

# 你可以使用 best_params 来驱动你的最终 AI