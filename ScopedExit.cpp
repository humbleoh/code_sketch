#include <iostream>

#define SCOPED_EXIT(f)  SCOPED_EXIT_UNIQ(f, __LINE__)
#define SCOPED_EXIT_UNIQ(f, l)  ScopedExit scopedExit__##l { f }

template<typename T>
class ScopedExit {
public:
    ScopedExit(T f)
        : m_f { f }
    { }

    ~ScopedExit() noexcept
    {
        m_f();
    }

private:
    T m_f;
};

class TestClass {
public:
    TestClass()
    {
        std::cout << "testclass ctor" << std::endl;
    }

    ~TestClass()
    {
        std::cout << "testclass dtor" << std::endl;
    }


    void Close()
    {
        std::cout << "close" << std::endl;
    }
};

int main(int argc, const char *argv[])
{
    TestClass t;
    SCOPED_EXIT([&](){ t.Close(); });
}

