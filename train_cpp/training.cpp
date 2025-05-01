#include "training.h"
#include "game.h" // 用于 runGameForTraining
#include "models.h" // 可能需要类型定义，尽管 game.h 已包含
#include <algorithm> // 用于 std::sort, std::min_element, std::max_element
#include <chrono> // 用于计时
#include <cmath> // 用于 std::sqrt, std::pow
#include <future> // 用于 std::async, std::future
#include <iomanip> // 用于 std::fixed, std::setprecision
#include <iostream>
#include <mutex> // 用于 std::mutex, std::lock_guard
#include <numeric> // 用于 std::accumulate, std::inner_product
#include <random>
#include <thread> // 用于 std::thread::hardware_concurrency
#include <vector>

// --- 配置参数 (匹配 Python 版本) ---
const int k_num_features = 8; // 核心特征数量
const int k_num_params = k_num_features + 1; // 权重 a0 到 an (如果像 Python 一样使用，则包含偏置/常数项)
const int k_population_size = 100; // 种群大小
const double k_elite_frac = 0.1; // 精英比例
const int k_num_iterations = 50; // 迭代次数
const int k_num_games_per_eval = 10; // 每次评估的游戏数
const double k_inital_std_dev = 5.0; // 初始标准差
const double k_std_dev_epsilon = 1e-6; // 防止 sigma 变为零

// 确定最大工作线程数 (类似于 Python 的 os.cpu_count())
const unsigned int MAX_WORKERS = std::max(1U, std::thread::hardware_concurrency());

// --- 用于线程安全日志记录的全局互斥锁 ---
std::mutex cout_mutex;

// 用于线程安全日志记录的辅助函数
template <typename... Args>
void log_safe(Args&&... args)
{
    std::lock_guard<std::mutex> lock(cout_mutex);
    (std::cout << ... << std::forward<Args>(args)) << std::endl;
}

// Helper function to format a vector<double> for printing
std::string format_vector(const std::vector<double>& vec) {
    std::ostringstream oss;
    oss << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        // Use the same precision as the final output for consistency
        oss << std::fixed << std::setprecision(4) << vec[i];
        if (i < vec.size() - 1) {
            oss << ", ";
        }
    }
    oss << "]";
    return oss.str();
}


// --- 数值计算辅助函数 ---

// 计算向量的平均值
double calculateMean(const std::vector<double>& v)
{
    if (v.empty())
        return 0.0;
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// 计算向量的标准差
double calculateStdDev(const std::vector<double>& v, double mean)
{
    if (v.size() < 2)
        return 0.0; // 0 或 1 个元素的标准差为 0
    double sq_sum = std::inner_product(v.begin(), v.end(), v.begin(), 0.0,
        std::plus<>(), [mean](double a, double b) {
            return (a - mean) * (b - mean);
        });
    // 使用总体标准差 (除以 N)，与 numpy 默认行为一致
    return std::sqrt(sq_sum / v.size());
}

// --- 评估函数 ---

// 用于保存 evaluate_parameters 结果的结构体
struct EvalResult {
    int index;
    double average_score;
};

// 通过运行多个游戏来评估单组参数的函数
// 接收索引用于日志记录
EvalResult evaluate_parameters(int index, const std::vector<double>& params)
{
    log_safe("任务 ", index, ": 开始评估...");
    auto start_time = std::chrono::high_resolution_clock::now();
    double total_score = 0;

    for (int i = 0; i < k_num_games_per_eval; ++i) {
        // log_safe("任务 ", index, ": 开始游戏 ", i + 1, "/", NUM_GAMES_PER_EVAL); // 详细日志
        int score = runGameForTraining(params); // 假设 runGameForTraining 如果使用全局 RNG 是线程安全的
                                                // 注意: game.cpp 中的 runGameForTraining 使用静态 RNG，这不是线程安全的。
                                                // 为了实现真正的并行执行，需要解决这个问题。
                                                // 目前我们继续进行，但要注意潜在的 RNG 问题。
        total_score += score;
        // log_safe("任务 ", index, ": 完成游戏 ", i + 1, "/", NUM_GAMES_PER_EVAL, ", 得分: ", score); // 详细日志
    }

    double avg_score = (k_num_games_per_eval > 0) ? (total_score / k_num_games_per_eval) : 0.0;
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    log_safe("任务 ", index, ": 评估完成于 ", std::fixed, std::setprecision(2), duration.count(), "s. 平均得分: ", avg_score);
    return { index, avg_score };
}

// --- 训练函数 ---

void runTraining()
{
    log_safe("--- 开始 Tetris CEM 训练 (C++) ---");
    log_safe("参数: 种群大小=", k_population_size, ", 精英比例=", k_elite_frac,
        ", 迭代次数=", k_num_iterations, ", 每次评估的游戏数=", k_num_games_per_eval,
        ", 工作线程数=", MAX_WORKERS);

    // --- 初始化 ---
    std::vector<double> initial_good_params = {
        -12.63, 6.60, -9.22, -19.77, -13.08, -10.49, -1.61, -24.04, 0.0 // 匹配 Python 的 initial_good_params
    };
    if (initial_good_params.size() != k_num_params) {
        log_safe("错误: initial_good_params 大小不匹配。预期 ", k_num_params, ", 实际 ", initial_good_params.size());
        return;
    }

    auto mu = initial_good_params;
    std::vector<double> sigma(k_num_params, k_inital_std_dev);

    // --- 随机数生成器设置 ---
    // 主线程采样使用独立的生成器。
    // 注意: evaluate_parameters 理想情况下需要线程局部随机数生成器。
    std::random_device rd;
    std::mt19937 main_rng(rd());

    // --- CEM 主循环 ---
    // CEM (Cross-Entropy Method) 是一种基于优化的方法，
    // 通过迭代地从参数分布中采样，评估样本，选择表现最好的样本（精英），
    // 并使用这些精英来更新参数分布（均值和标准差），逐步逼近最优参数。
    for (int itr = 0; itr < k_num_iterations; ++itr) {
        log_safe("\n迭代 ", itr + 1, "/", k_num_iterations);
        auto iteration_start_time = std::chrono::high_resolution_clock::now();

        // 1. 采样种群 (Sampling)
        // 从当前的参数分布（由均值 mu 和标准差 sigma 定义的正态分布）中
        // 随机抽取 k_population_size 组参数。每一组参数代表一个潜在的解决方案。
        std::vector<std::vector<double>> population_params(k_population_size, std::vector<double>(k_num_params));
        for (int i = 0; i < k_population_size; ++i) {
            for (int j = 0; j < k_num_params; ++j) {
                std::normal_distribution<double> dist(mu[j], sigma[j]);
                population_params[i][j] = dist(main_rng);
            }
        }

        // 2. 并行评估 (Evaluation)
        // 使用多个工作线程并行地评估种群中的每一组参数。
        // 对于每组参数，运行 k_num_games_per_eval 次游戏，计算平均得分作为其性能指标。
        log_safe("开始并行评估 ", k_population_size, " 组参数，使用 ", MAX_WORKERS, " 个工作线程...");
        auto eval_start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::future<EvalResult>> futures;
        futures.reserve(k_population_size);

        // 启动异步任务来评估每个参数集
        for (int i = 0; i < k_population_size; ++i) {
            // 异步启动任务
            futures.push_back(std::async(std::launch::async, evaluate_parameters, i, population_params[i]));
        }

        // 收集评估结果
        // 等待所有并行评估任务完成，并收集它们的平均得分。
        std::vector<EvalResult> results;
        results.reserve(k_population_size);
        int completed_count = 0;
        for (auto& fut : futures) {
            results.push_back(fut.get()); // 阻塞直到 future 就绪
            completed_count++;
            if (completed_count % 10 == 0 || completed_count == k_population_size) {
                log_safe("  评估进度: ", completed_count, "/", k_population_size, " 组参数已评估。");
            }
        }

        auto eval_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> eval_duration = eval_end_time - eval_start_time;
        log_safe("并行评估完成于 ", std::fixed, std::setprecision(2), eval_duration.count(), " 秒。");

        // 按得分降序排序结果
        // 将评估结果按照平均得分从高到低排序，以便选出表现最好的参数组。
        std::sort(results.begin(), results.end(), [](const EvalResult& a, const EvalResult& b) {
            return a.average_score > b.average_score; // 分数越高越好
        });

        double total_iteration_score = 0;
        for (const auto& res : results) {
            total_iteration_score += res.average_score;
        }
        double mean_iteration_score = results.empty() ? 0.0 : total_iteration_score / results.size();
        double max_iteration_score = results.empty() ? 0.0 : results[0].average_score; // 排序后第一个是最大值

        log_safe("  本次迭代平均得分: ", std::fixed, std::setprecision(2), mean_iteration_score);
        log_safe("  本次迭代最高得分: ", std::fixed, std::setprecision(2), max_iteration_score);

        // 3. 选择精英 (Selection)
        // 从排序后的结果中选择得分最高的 top k_elite_frac 百分比的参数组作为“精英”。
        // 这些精英代表了当前迭代中最好的解决方案。
        int num_elites = static_cast<int>(k_population_size * k_elite_frac);
        if (num_elites == 0 && k_population_size > 0)
            num_elites = 1; // 如果可能，确保至少有一个精英

        std::vector<std::vector<double>> elite_params;
        elite_params.reserve(num_elites);
        for (int i = 0; i < num_elites && i < results.size(); ++i) {
            elite_params.push_back(population_params[results[i].index]);
        }

        if (elite_params.empty()) {
            log_safe("警告: 未选择精英参数。请检查得分或精英比例。");
            // 可选：保留旧的 mu/sigma 或重置 sigma？暂时保留旧的。
            continue;
        }

        // 4. 更新分布 (Update)
        // 使用选出的精英参数组来更新下一次迭代的参数分布。
        // 新的均值 (mu) 是精英参数各维度的平均值。
        // 新的标准差 (sigma) 是精英参数各维度的标准差（加上一个小的 epsilon 防止其变为零）。
        // 这个步骤使得下一次采样的参数更可能接近于当前找到的较优解区域。
        std::vector<double> next_mu(k_num_params, 0.0);
        std::vector<double> next_sigma(k_num_params, 0.0);

        // 计算新的均值 (mu)
        for (int j = 0; j < k_num_params; ++j) {
            double sum = 0.0;
            std::vector<double> param_values_j;
            param_values_j.reserve(elite_params.size());
            for (const auto& elite : elite_params) {
                param_values_j.push_back(elite[j]);
            }
            next_mu[j] = calculateMean(param_values_j);
        }

        // 计算新的标准差 (sigma)
        for (int j = 0; j < k_num_params; ++j) {
            std::vector<double> param_values_j;
            param_values_j.reserve(elite_params.size());
            for (const auto& elite : elite_params) {
                param_values_j.push_back(elite[j]);
            }
            next_sigma[j] = calculateStdDev(param_values_j, next_mu[j]) + k_std_dev_epsilon; // 添加 epsilon
        }

        mu = next_mu;
        sigma = next_sigma;

        log_safe("  新 mu ：", format_vector(mu));
        log_safe("  新 sigma：", format_vector(sigma));

        auto iteration_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> iteration_duration = iteration_end_time - iteration_start_time;
        log_safe("迭代完成于 ", std::fixed, std::setprecision(2), iteration_duration.count(), " 秒。");
    } // 结束 CEM 主循环

    // --- 训练结束 ---
    log_safe("\n--- 训练结束! ---");
    log_safe("找到的最佳参数 (估计均值):");
    std::cout << "[";
    for (size_t i = 0; i < mu.size(); ++i) {
        std::cout << std::fixed << std::setprecision(4) << mu[i] << (i == mu.size() - 1 ? "" : ", ");
    }
    std::cout << "]" << std::endl;
}