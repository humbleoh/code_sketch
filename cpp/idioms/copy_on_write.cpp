#include <iostream>
#include <memory>
#include <string>
#include <concepts>
#include <type_traits>
#include <mutex>

template<std::copyable T>
class CopyOnWritePtr {
    // 无法直接约束CopyOnWritePtr类模板为某个concept，只有在实例化时方可约束。
    //static_assert(std::copyable<CopyOnWritePtr<T>>);
    //static_assert(std::copyable<std::shared_ptr<T>>);
public:
    /** */
    CopyOnWritePtr() = default;
    /** */
    CopyOnWritePtr(const CopyOnWritePtr &robj) = default;
    /** */
    template<typename... Ts>
    CopyOnWritePtr(Ts... ts); // 注意：universal absorber when a ctor of certain form is not declared.
    /** */
    CopyOnWritePtr &operator=(const CopyOnWritePtr &robj) = default;
    /** */
    ~CopyOnWritePtr() = default;
    
    /** */
    T *GetMut();
    /** */
    const T *GetImmut() const;
    
private:
    /** */
    void detach();

private:
    std::shared_ptr<T> data_;
};

template<std::copyable T>
template<typename... Ts>
CopyOnWritePtr<T>::CopyOnWritePtr(Ts... ts)
{
    data_ = std::make_shared<T>(ts...);
}

template<std::copyable T>
T *CopyOnWritePtr<T>::GetMut()
{
    detach();
    return data_.get();
}

template<std::copyable T>
const T *CopyOnWritePtr<T>::GetImmut() const
{
    return data_.get();
}

template<std::copyable T>
void CopyOnWritePtr<T>::detach()
{
    if (data_ && data_.use_count() > 1) {
        data_ = std::make_shared<T>(*data_);
    }
}

int main(int argc, const char *argv[])
{
    //CopyOnWritePtr<std::string> t0 = "hello world";
    CopyOnWritePtr<std::string> t0;
    //const CopyOnWritePtr<std::string> t1 = t0;
    std::copyable auto t1 = t0; // 在这里约束CopyOnWritePtr<std::string>为std::copy对象。
    //t0.GetImmut()->at(0);
    //std::cout << "t0:" << t0.GetImmut() << " " << *t0.GetImmut() << std::endl;
    //std::cout << "t1:" << t1.GetImmut() << " " << *t1.GetImmut() << std::endl;
    std::cout << "t0:" << t0.GetImmut() << std::endl;
    std::cout << "t1:" << t1.GetImmut()  << std::endl;
    
    //t0.GetMut()->at(0) = 'a';
    //std::cout << "t0:" << t0.GetMut() << " " << *t0.GetMut() << std::endl;
    //t1.GetMut()->at(0) = 'v';
    //std::cout << "t1:" << t1.GetMut() << " " << *t1.GetMut() << std::endl;
    //std::cout << "t1:" << t1.GetImmut() << " " << *t1.GetImmut() << std::endl;
    std::cout << "t0:" << t0.GetMut() << std::endl;
    std::cout << "t1:" << t1.GetImmut() << std::endl;
    
    CopyOnWritePtr<std::string> t2;
    //CopyOnWritePtr<std::mutex> t3;
    return 0;
}
