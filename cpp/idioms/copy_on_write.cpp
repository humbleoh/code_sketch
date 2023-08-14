#include <iostream>
#include <memory>
#include <string>
#include <concepts>
#include <type_traits>
#include <mutex>

template<typename T>
struct TraitCopyable {
    TraitCopyable()
    {
        // 可以使用trait类模板和CRTP，对子类进行约束。
        static_assert(std::copyable<T>);
    }
};

#define DEFINE_TRAIT_CLASS(trait) \
    namespace { \
        using namespace std; \
        template<typename T> \
        struct CheckTrait##trait { \
            CheckTrait##trait() \
            { \
                static_assert(trait<T>); \
            } \
        }; \
    }
    
#define ENFORCE_TRAIT(trait, t) \
    CheckTrait##trait<t>

DEFINE_TRAIT_CLASS(copyable);

template<std::copyable T>
class CopyOnWritePtr
    //: public TraitCopyable<CopyOnWritePtr<T>> {
    : public ENFORCE_TRAIT(copyable, CopyOnWritePtr<T>) {
    // 无法在这里直接约束CopyOnWritePtr类模板为某个concept，只有在实例化时方可约束。
    // 因为CopyOnWritePtr这时是incomplete type
    //static_assert(std::copyable<CopyOnWritePtr<T>>);
    //static_assert(std::copyable<std::shared_ptr<T>>);
public:
    /** */
    CopyOnWritePtr();
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
CopyOnWritePtr<T>::CopyOnWritePtr()
    : data_{std::make_shared<T>()}
{
}

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

class MyTestClassImpl {
public:
    void SetMember1(const std::string &val)
    {
        member1_ = val;
    }
    
    const std::string &GetMember1() const
    {
        // 返回值类型可以是引用
        return member1_;
    }
    
private:
    std::string member1_;
};

class MyTestClass {
public:    
    void SetMember1(const std::string &val)
    {
        data_.GetMut()->SetMember1(val);
    }
    
    std::string GetMember1() const
    {
        // 返回值类型应该为纯值，返回成员对象的引用会造成对外不一致性，比如：caller
        // 拿到了成员对象引用，但同时它又修改了其它成员对象，这里的修改会间接detach
        // 内部的共享对象。这是，之前拿到的引用已经不属于当下所持有的共享对象。
        return data_.GetImmut()->GetMember1();
    }
private:
    CopyOnWritePtr<MyTestClassImpl> data_;
};

int main(int argc, const char *argv[])
{
    CopyOnWritePtr<std::string> t0 = "hello world";
    //CopyOnWritePtr<std::string> t0;
    const CopyOnWritePtr<std::string> t1 = t0;
    std::copyable auto t2 = t0; // 在这里约束CopyOnWritePtr<std::string>为std::copy对象。
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
    
    CopyOnWritePtr<std::string> t3;
    //CopyOnWritePtr<std::mutex> t4;
    
    MyTestClass mytest0;
    mytest0.SetMember1("hello world");
    MyTestClass mytest1 = mytest0;
    std::cout << "mytest0: " << mytest0.GetMember1() << std::endl;
    std::cout << "mytest1: " << mytest1.GetMember1() << std::endl;
    mytest0.SetMember1("good answer");
    std::cout << "mytest0: " << mytest0.GetMember1() << std::endl;
    std::cout << "mytest1: " << mytest1.GetMember1() << std::endl;
    return 0;
}
