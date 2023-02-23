#include <atomic>
#include <iostream>

template<typename T>
class SimpleSharedPtr {
public:
    SimpleSharedPtr() = default;
    SimpleSharedPtr(const SimpleSharedPtr<T>& obj);
    SimpleSharedPtr(SimpleSharedPtr<T>&& obj);
    SimpleSharedPtr(T* ptr);
    ~SimpleSharedPtr();

    T *Get() const;
    void Reset(T* ptr);
    SimpleSharedPtr<T>& operator=(const SimpleSharedPtr<T>& obj);
    SimpleSharedPtr<T>& operator=(SimpleSharedPtr<T>&& obj);

private:
    class RefCounterModel {
    public:
        RefCounterModel() = default;
        RefCounterModel(T* ptr);

        template<typename... Ts>
        RefCounterModel(Ts&&... ts);

        void ShareOwnership();
        bool ReleaseOwnership();
        T* Get() const;

    private:
        std::atomic<int> m_counter { 1 };
        T *m_obj { nullptr };
    };

    mutable RefCounterModel *m_pRefCntObj { nullptr };
};

template<typename T, typename... Ts>
SimpleSharedPtr<T> MakeSharedPtr(Ts... ts)
{
    T* obj = new T(ts...);
    return SimpleSharedPtr<T>(obj);
}

/* **/

template<typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(const SimpleSharedPtr<T>& obj)
{
    m_pRefCntObj = obj.m_pRefCntObj;
    if (m_pRefCntObj) {
        m_pRefCntObj->ShareOwnership();
    }
}

template<typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(SimpleSharedPtr<T>&& obj)
{
    m_pRefCntObj = obj.m_pRefCntObj;
    obj.m_pRefCntObj = nullptr;
}

template<typename T>
SimpleSharedPtr<T>::SimpleSharedPtr(T* ptr)
{
    m_pRefCntObj = new RefCounterModel(ptr);
}

template<typename T>
SimpleSharedPtr<T>::~SimpleSharedPtr()
{
    if (m_pRefCntObj && m_pRefCntObj->ReleaseOwnership()) {
        delete m_pRefCntObj;
        m_pRefCntObj = nullptr;
        std::cout << "Release ref-cnt obj" << std::endl;
    }
}

template<typename T>
T* SimpleSharedPtr<T>::Get() const
{
    return m_pRefCntObj ? m_pRefCntObj->Get() : nullptr;
}

template<typename T>
void SimpleSharedPtr<T>::Reset(T* ptr)
{
    this->~SimpleSharedPtr();
    m_pRefCntObj = new RefCounterModel(ptr);
}


template<typename T>
SimpleSharedPtr<T>::RefCounterModel::RefCounterModel(T* ptr)
{
    m_counter = 1;
    m_obj = ptr;
}

template<typename T>
template<typename... Ts>
SimpleSharedPtr<T>::RefCounterModel::RefCounterModel(Ts&&... ts)
{
    m_counter = 1;
    m_obj = new T(ts...);
}

template<typename T>
void SimpleSharedPtr<T>::RefCounterModel::ShareOwnership()
{
    if (m_obj) {
        m_counter++;
    }
}

template<typename T>
T* SimpleSharedPtr<T>::RefCounterModel::Get() const
{
    return m_obj;
}

template<typename T>
bool SimpleSharedPtr<T>::RefCounterModel::ReleaseOwnership()
{
    if (m_obj) {
        m_counter--;
        if (m_counter == 0) {
            delete m_obj;
            return true;
        }
    }

    return false;
}

int main(int argc, const char* argv)
{
    {
    SimpleSharedPtr<int> obj1;
    SimpleSharedPtr<int> obj2{ obj1 };
    }

    {
    SimpleSharedPtr<int> obj1{ new int{99} };
    SimpleSharedPtr<int> obj2 { obj1 };
    auto t = obj2.Get();
    (*t)++;
    std::cout << "o:" << *obj1.Get() << std::endl;
    SimpleSharedPtr<int> obj3 { std::move(obj1) };
    std::cout << "p:" << obj1.Get() << " " << obj3.Get() << std::endl;
    SimpleSharedPtr<int> obj4 { (obj2) };
    }

    {
    SimpleSharedPtr<int> obj1 = MakeSharedPtr<int>(8888);
    SimpleSharedPtr<int> obj2 { obj1 };
    auto t = obj2.Get();
    (*t)++;
    std::cout << "o:" << *obj1.Get() << std::endl;
    SimpleSharedPtr<int> obj3 { std::move(obj1) };
    std::cout << "p:" << obj1.Get() << " " << obj3.Get() << std::endl;
    SimpleSharedPtr<int> obj4 { (obj2) };
    }
    return 0;
}

