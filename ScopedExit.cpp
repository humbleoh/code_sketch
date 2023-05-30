#include <concepts>
#include <functional>
#include <iostream>

#define SCOPED_EXIT(f)  SCOPED_EXIT_UNIQ((f), __LINE__)
#define SCOPED_EXIT_UNIQ(f, l)  SCOPED_EXIT_UNIQ_EXPAND(f, l)
#define SCOPED_EXIT_UNIQ_EXPAND(f, l)  ScopedExit scopedExit__##l { (f) }

template<std::invocable T>
class ScopedExit {
public:
    ScopedExit(T &&f)
        : m_f { std::forward<T>(f) }
    { }

    // https://en.cppreference.com/w/cpp/language/as_if
    ~ScopedExit() noexcept(noexcept(std::is_nothrow_invocable_v<T>))
    {
        m_f();
    }

private:
    T m_f;
};

int main(int argc, const char *argv[])
{
    int *ptrInt = new int(10);
    SCOPED_EXIT([&]() {
        std::cout << "delete ptr, *ptr=" << *ptrInt << std::endl;
        delete ptrInt;
    });
    
    std::function<void()> test2 = []() { std::cout << "test2" << std::endl; };
    SCOPED_EXIT(test2);
    
    return 0;
}

