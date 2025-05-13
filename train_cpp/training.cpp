#include "training.h"
#include "game.h" // 用于 runGameForTraining
#include "models.h" // 可能需要类型定义，尽管 game.h 已包含
#include <algorithm> // 用于 std::sort, std::min_element, std::max_element
#include <chrono> // 用于计时
#include <cmath> // 用于 std::sqrt, std::pow
#include <future> // 用于 std::async, std::future
#include <iomanip> // 用于 std::fixed, std::setprecision
#include <iostream>
#include <memory> // For std::shared_ptr
#include <numeric> // 用于 std::accumulate, std::inner_product
#include <random>
#include <sstream> // 用于 ostringstream
#include <thread> // 用于 std::thread::hardware_concurrency
#include <vector>

// CEM

// Include spdlog headers
#include <spdlog/spdlog.h>
#include "spdlog/async.h" //support for async logging.
#include <spdlog/sinks/basic_file_sink.h> // For basic file logging
#include <spdlog/fmt/ostr.h> // Required for custom types like vectors if needed directly in spdlog format strings (though format_vector avoids this)
const int k_num_features = 8; // 核心特征数量
const int k_num_params = k_num_features + 1; // 权重 a0 到 an (如果像 Python 一样使用，则包含偏置/常数项)
const int k_population_size = 100; // 种群大小
const double k_elite_frac = 0.1; // 精英比例
const int k_num_iterations = 50; // 迭代次数
const int k_num_games_per_eval = 8; // 每次评估的游戏数
const double k_inital_std_dev = 5.0; // 初始标准差
const double k_std_dev_epsilon = 1e-6; // 防止 sigma 变为零

// 确定最大工作线程数 (类似于 Python 的 os.cpu_count())
const unsigned int MAX_WORKERS = 3 * std::max(1U, std::thread::hardware_concurrency());

// --- Global Logger Pointer ---
// Declare the logger pointer globally, initialize in main
std::shared_ptr<spdlog::logger> async_file = nullptr;

// 用于线程安全日志记录的辅助函数 (写入文件 using spdlog)
template <typename... Args>
void log_safe(Args&&... args)
{
    // Format the arguments into a single string
    std::ostringstream oss;
    // Use a fold expression (C++17) to stream all arguments into the oss
    (oss << ... << std::forward<Args>(args));

    // Check if logger is initialized before using
    if (async_file) {
        async_file->info(oss.str()); // Log the formatted string
        spdlog::info(oss.str()); // Also log to console
    } else {
        // Fallback or error if logger not ready (optional)
        std::cerr << "[WARN] Logger not initialized. Message: " << oss.str() << std::endl;
    }
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
    // Log parameters being evaluated
    log_safe("Task ", index, ": Starting evaluation with params: ", format_vector(params));
    auto start_time = std::chrono::high_resolution_clock::now();
    double total_score = 0;

    // Create a thread-local random number generator for game simulation if needed
    // static thread_local std::mt19937 game_rng(std::random_device{}());
    // Pass game_rng to runGameForTraining if it accepts an RNG

    for (int i = 0; i < k_num_games_per_eval; ++i) {
        // Reduced verbosity for per-game logs
        log_safe("Task ", index, ": Starting game ", i + 1, "/", k_num_games_per_eval);
        int score = runGameForTraining(params); // Assuming runGameForTraining is thread-safe or uses thread-local RNG
        total_score += score;
        log_safe("Task ", index, ": Finished game ", i + 1, "/", k_num_games_per_eval, ", Score: ", score);
    }

    double avg_score = (k_num_games_per_eval > 0) ? (total_score / k_num_games_per_eval) : 0.0;
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;

    // Log evaluation result
    log_safe("Task ", index, ": Evaluation finished in ", std::fixed, std::setprecision(2), duration.count(), "s. Avg Score: ", avg_score);
    return { index, avg_score };
}

// --- 训练函数 ---

void runTraining()
{
    log_safe("--- Starting Tetris CEM Training (C++) ---");
    log_safe("Parameters: Population Size=", k_population_size, ", Elite Fraction=", k_elite_frac,
        ", Iterations=", k_num_iterations, ", Games per Eval=", k_num_games_per_eval,
        ", Max Workers=", MAX_WORKERS, ", Initial Std Dev=", k_inital_std_dev);

    // --- 初始化 ---
    std::vector<double> initial_good_params = {
        -16.4912, 6.4811, -8.5137, -18.9269, -14.3096, -12.1746, -1.1174, -29.9476, -0.5464 // 匹配 Python 的 initial_good_params
    };
    if (initial_good_params.size() != k_num_params) {
        log_safe("ERROR: initial_good_params size mismatch. Expected ", k_num_params, ", got ", initial_good_params.size());
        return;
    }

    auto mu = initial_good_params;
    // std::vector<double> sigma(k_num_params, k_inital_std_dev);
    std::vector<double> sigma = {
        1.6127, 1.5766, 0.5512, 1.9276, 1.5725, 0.4238, 0.4413, 2.5621, 2.3811
    };
    // Log initial parameters
    log_safe("Initial mu: ", format_vector(mu));
    log_safe("Initial sigma: ", format_vector(sigma));


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
        log_safe("\n--- Iteration ", itr + 1, "/", k_num_iterations, " ---");
        auto iteration_start_time = std::chrono::high_resolution_clock::now();
        // Log parameters for the current iteration
        log_safe("Current mu: ", format_vector(mu));
        log_safe("Current sigma: ", format_vector(sigma));

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
        log_safe("Starting parallel evaluation of ", k_population_size, " parameter sets using ", MAX_WORKERS, " workers...");
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
            // Optional: Log progress less frequently if needed
            // if (completed_count % (k_population_size / 10) == 0 || completed_count == k_population_size) {
            //     log_safe("  Evaluation progress: ", completed_count, "/", k_population_size, " parameter sets evaluated.");
            // }
        }

        auto eval_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> eval_duration = eval_end_time - eval_start_time;
        log_safe("Parallel evaluation completed in ", std::fixed, std::setprecision(2), eval_duration.count(), " seconds.");

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
        double min_iteration_score = results.empty() ? 0.0 : results.back().average_score; // 排序后最后一个是最小值

        // Log iteration score statistics
        log_safe("  Iteration Scores: Avg=", std::fixed, std::setprecision(2), mean_iteration_score,
                 ", Max=", std::fixed, std::setprecision(2), max_iteration_score,
                 ", Min=", std::fixed, std::setprecision(2), min_iteration_score);
        // Log top elite score and params for reference
        if (!results.empty()) {
             log_safe("  Best Params (Score: ", std::fixed, std::setprecision(2), results[0].average_score, "): ", format_vector(population_params[results[0].index]));
        }


        // 3. 选择精英 (Selection)
        // 从排序后的结果中选择得分最高的 top k_elite_frac 百分比的参数组作为“精英”。
        // 这些精英代表了当前迭代中最好的解决方案。
        int num_elites = static_cast<int>(k_population_size * k_elite_frac);
        if (num_elites == 0 && k_population_size > 0)
            num_elites = 1; // 如果可能，确保至少有一个精英

        log_safe("Selecting top ", num_elites, " elites (", k_elite_frac * 100, "%)");

        std::vector<std::vector<double>> elite_params;
        elite_params.reserve(num_elites);
        for (int i = 0; i < num_elites && i < results.size(); ++i) {
            elite_params.push_back(population_params[results[i].index]);
            // Optional: Log selected elite params if needed (can be verbose)
            // log_safe("  Elite ", i+1, " (Score: ", results[i].average_score, "): ", format_vector(population_params[results[i].index]));
        }

        if (elite_params.empty()) {
            log_safe("WARNING: No elite parameters selected. Check scores or elite fraction. Keeping previous mu/sigma.");
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

        // Log updated parameters
        log_safe("  Updated mu: ", format_vector(mu));
        log_safe("  Updated sigma: ", format_vector(sigma));

        auto iteration_end_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> iteration_duration = iteration_end_time - iteration_start_time;
        log_safe("Iteration finished in ", std::fixed, std::setprecision(2), iteration_duration.count(), " seconds.");
    } // 结束 CEM 主循环

    // --- 训练结束 ---
    log_safe("\n--- Training Finished! ---");
    log_safe("Final Estimated Mean Parameters (mu):");
    log_safe(format_vector(mu)); // Log final parameters to file

    // Also print final result to console for convenience
    std::cout << "\n--- Training Finished! ---" << std::endl;
    std::cout << "Final Estimated Mean Parameters (mu):" << std::endl;
    std::cout << format_vector(mu) << std::endl;
    std::cout << "Check 'training_log.log' for detailed logs." << std::endl;
}

int main() {
    // --- Initialize Logging ---
    try {
        // Ensure the "logs" directory exists or handle creation failure
        // For simplicity, assuming "logs/" directory exists relative to executable path
        // Initialize the thread pool for async logging
        // Queue size: 8192, Threads: 1
        spdlog::init_thread_pool(8192, 1);

        // Create the async file logger
        async_file = spdlog::basic_logger_mt<spdlog::async_factory>("async_file_logger", "logs/async_log.log");

        // Optional: Set log level (e.g., trace, debug, info, warn, error, critical)
        async_file->set_level(spdlog::level::info);

        // Optional: Set a pattern for log messages
        // Example: [Timestamp] [Logger Name] [Log Level] Message
        async_file->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] %v");

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
        return 1;
    }
    // --- End Logging Initialization ---


    // --- Start Application Logic ---
    log_safe("--- Application Start ---"); // Log application start

    std::cout << "--- Starting Tetris Training ---" << std::endl;
    std::cout << "Logging detailed output to 'logs/async_log.log'" << std::endl; // Update path
    try {
        runTraining(); // Call the main training function
    } catch (const std::exception& e) {
        log_safe("FATAL ERROR during training: ", e.what()); // Log exception to file
        std::cerr << "An error occurred during training: " << e.what() << std::endl;
        spdlog::shutdown(); // Ensure logs are flushed on error exit
        return 1; // Indicate failure
    } 
    catch (...) {
        log_safe("FATAL UNKNOWN ERROR during training."); // Log unknown exception
        std::cerr << "An unknown error occurred during training." << std::endl;
        spdlog::shutdown(); // Ensure logs are flushed on error exit
        return 1; // Indicate failure
    }

    log_safe("--- Application End ---"); // Log application end normally
    std::cout << "--- Tetris Training Finished ---" << std::endl;

    spdlog::shutdown(); // Shutdown spdlog to flush async logs
    return 0; // Indicate success
}