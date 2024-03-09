#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <random>

//сделать 2 очереди дл€ задач, сделать очередь по приоритетам,
//сделать переход из 1 очереди во вторую
// сделать возможность мнгновенно оставл€ть все активные задачи закончить работу
//сделать возможность временой остановки работы
//код в≥дпов≥дальний за додаванн€ задач в пул поток≥в, та сам +
//пул поток≥в повинн≥ знаходитис€ в р≥зних потоках виконанн€. +
//

class ThreadPool {
public:
    ThreadPool(size_t numThreads) : stop(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            threads.emplace_back([this] {
                while (true) {
                    Task task;
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        condition.wait(lock, [this] { return stop || !tasks.empty(); }); // ждет только в случае когда очередь пуста и стоп = фалсе
                        if (stop && tasks.empty()) return;
                        task = std::move(tasks.top());
                        tasks.pop();
                    }
                    task.function();
                }
                });
        }
    }

    template<class F, class... Args>
    void enqueue(std::chrono::seconds priority, F&& f, Args&&... args) {//добавить функцию/задачу в очередь
        {
            std::unique_lock<std::mutex> lock(mutex);
            tasks.emplace(priority, std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
        condition.notify_one();
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread& thread : threads) {
            thread.join();
        }
    }

private:
    std::vector<std::thread> threads;
    std::priority_queue<Task> tasks;
    std::queue<std::function<void()>> tasks_1;
    std::mutex mutex;
    std::condition_variable condition;
    bool stop;
};

struct Task {
    std::chrono::seconds priority;
    std::function<void()> function;

    // ќператор сравнени€ дл€ установки пор€дка приоритета
    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

// ѕример использовани€ пула потоков
void taskFunction(int id, int time) {
    std::cout << "Task " << id << " started" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(time));
    std::cout << "Task " << id << " finished" << std::endl;
}

void filling_queue(ThreadPool* pool, int num_work, std::mt19937& generator)
{
    std::this_thread::sleep_for(std::chrono::seconds(2));
    for (int i = 0; i < num_work; ++i) {
        std::chrono::seconds time = std::chrono::seconds(1 + generator() % 5);
        pool->enqueue(time, taskFunction, i, time);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

int main() {
    // —оздание пула потоков с 4 потоками
    ThreadPool pool(4);
    std::mt19937 generator((unsigned int)std::chrono::system_clock::now().time_since_epoch().count());
    // ƒобавление задач в пул потоков
    //std::thread mytread(filling_queue, &pool, 8, generator);
    //mytread.join();

    // ќжидание завершени€ всех задач
    std::this_thread::sleep_for(std::chrono::seconds(2));

    return 0;
}
