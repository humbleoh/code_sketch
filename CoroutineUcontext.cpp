#include <functional>
#include <iostream>
#include <memory>

#include <unistd.h>
#include <ucontext.h>

void test1()
{
    ucontext_t context;
    getcontext(&context);
    std::cout << "hello world" << std::endl;
    sleep(1);
    setcontext(&context);
}

void func1(void)
{
    std::cout << "abc0" << std::endl;
    std::cout << "abc1" << std::endl;
    std::cout << "abc2" << std::endl;
}

void test2()
{
    char stack[1024*1024];
    ucontext_t child, main;

    getcontext(&child);
    child.uc_stack.ss_sp = stack;
    child.uc_stack.ss_size = sizeof(stack);
    child.uc_stack.ss_flags = 0;
    child.uc_link = &main;

    makecontext(&child, func1, 0);
    std::cout << "start" << std::endl;
    swapcontext(&main, &child);
    std::cout << "main" << std::endl;    
}

// ref: https://probablydance.com/2012/11/18/implementing-coroutines-with-ucontext/
class SimpleCoroutine {
public:
    SimpleCoroutine(size_t stackSize, std::function<void (SimpleCoroutine&)> task)
    {
        getcontext(&m_callee);
        m_task = task;
        m_stack = std::make_unique<uint8_t[]>(stackSize);
        m_callee.uc_stack.ss_sp = m_stack.get();
        m_callee.uc_stack.ss_size = stackSize;
        m_callee.uc_stack.ss_flags = 0;
        m_callee.uc_link = &m_caller;
        makecontext(&m_callee, reinterpret_cast<void (*)()>(coroutine), 1, reinterpret_cast<void *>(this));
    }

    void yield()
    {
        swapcontext(&m_callee, &m_caller);
    }

    void operator()()
    {
        if (finished) return;
        swapcontext(&m_caller, &m_callee);
    }

    operator bool() const
    {
        return finished;
    }

private:
    bool finished = false;
    ucontext_t m_caller;
    ucontext_t m_callee;
    std::unique_ptr<uint8_t[]> m_stack;
    std::function<void (SimpleCoroutine&)> m_task;

    static void coroutine(void *self)
    {
        SimpleCoroutine *c = reinterpret_cast<SimpleCoroutine *>(self);
        c->m_task(*c);
        c->finished = true;
    }
};

int main(int argc, const char* argv[])
{
    //test2();
    SimpleCoroutine co{1024*1024, [](SimpleCoroutine& self) {
        for (int i = 0; i < 100; ++i) {
            std::cout << "hello world " << i << std::endl;
            self.yield();
        }
    }};

    while (!co) { 
        std::cout << "main" << std::endl;
        co();
    }

    return 0;
}

