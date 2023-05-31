#include <concepts>
#include <functional>
#include <iostream>

#define SCOPED_EXIT(f)  SCOPED_EXIT_UNIQ(f, __LINE__)
#define SCOPED_EXIT_UNIQ(f, l)  SCOPED_EXIT_UNIQ_EXPAND(f, l)
#define SCOPED_EXIT_UNIQ_EXPAND(f, l)  ScopedExit CONCAT_NAME(scopedExit, l) { f }
#define CONCAT_NAME(n1, n2)  n1 ## n2

template<std::invocable Callable>
class ScopedExit {
public:
    ScopedExit(Callable f)
        : m_f { std::move(f) }
    { }

    // https://en.cppreference.com/w/cpp/language/as_if
    ~ScopedExit() noexcept(noexcept(std::is_nothrow_invocable_v<Callable>))
    {
        m_f();
    }

private:
    Callable m_f;
};

void test3()
{
    std::cout << "test3" << std::endl;
}

int main(int argc, const char *argv[])
{
    int *ptrInt = new int(10);
    SCOPED_EXIT([&]() {
        std::cout << "delete ptr, *ptr=" << *ptrInt << std::endl;
        delete ptrInt;
    });
    
    std::function<void()> test2 = []() { std::cout << "test2" << std::endl; };
    SCOPED_EXIT(test2);
    
    SCOPED_EXIT(test3);
    
    return 0;
}

