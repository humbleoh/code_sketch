#include <functional>
#include <iostream>
#include <memory>
#include <deque>

#include <unistd.h>
#include <ucontext.h>

class CoroTask;

class CoroContext {
public:
    ucontext_t &GetCallerContext()
    {
        return m_caller;
    }
    
    void Resume(CoroTask *pCoroTask)
    {
        m_readyTasks.push_back(pCoroTask);
    }
    
    void Schedule();
    
private:
    ucontext_t m_caller;
    std::deque<CoroTask *> m_readyTasks; //!! make thread-safe
};

class CoroTask {
public:
    explicit CoroTask(CoroContext &context, std::size_t ssize, std::function<void (CoroTask &)> task)
        : m_context { context }
        , m_task { task }
        , m_ssize { ssize }
        , m_stack { new uint8_t[m_ssize] }
    {
        getcontext(&m_callee);
        m_task = task;
        m_callee.uc_stack.ss_sp = m_stack.get();
        m_callee.uc_stack.ss_size = m_ssize;
        m_callee.uc_stack.ss_flags = 0;
        m_callee.uc_link = &m_context.GetCallerContext();
        // On architectures where int and pointer types are the same size (e.g., x86-32, where both types are 32 bits),
        // you may be able to get away with passing pointers as arguments to makecontext() following argc. However,
        // doing this is not guaranteed to be portable, is undefined according to the standards, and won't work on
        // architectures where pointers are larger than ints. Nevertheless, starting with version 2.8, glibc makes some
        // changes to makecontext(), to permit this on some 64-bit architectures (e.g., x86-64). 
        makecontext(&m_callee, reinterpret_cast<void (*)()>(RawTask), 1, reinterpret_cast<void *>(this));
    }
    
    void Yield()
    {
        swapcontext(&m_callee, &m_context.GetCallerContext());
    }
    
    void Resume()
    {
        if (done)
            return;
        swapcontext(&m_context.GetCallerContext(), &m_callee);
    }
    
    operator bool()
    {
        return done;
    }
    
    CoroTask() = delete;
    CoroTask(const CoroTask &) = delete;
    CoroTask &operator=(const CoroTask &) = delete;
    CoroTask(CoroTask &&) = default;
    CoroTask &operator=(CoroTask &&) = default;
    
private:
    static void RawTask(void *arg)
    {
        auto pCoroTask = reinterpret_cast<CoroTask *>(arg);
        pCoroTask->m_task(*pCoroTask);
        pCoroTask->done = true;
    }
    
private:
    CoroContext &m_context;
    ucontext_t m_callee;
    std::function<void (CoroTask &)> m_task;
    std::size_t m_ssize;
    std::unique_ptr<uint8_t[]> m_stack;
    bool done = false;
};

void CoroContext::Schedule()
{
    while (!m_readyTasks.empty()) {
        m_readyTasks.front()->Resume();
        m_readyTasks.pop_front();
    }
}

int main(int argc, const char* argv[])
{
    CoroContext context;
    CoroTask co{ context, 1024*1024, [](CoroTask& self) {
        for (int i = 0; i < 100; ++i) {
            std::cout << "hello world " << i << std::endl;
            self.Yield();
        }
    }};

    while (!co) { 
        std::cout << "main" << std::endl;
        co.Resume();
    }

    return 0;
}

