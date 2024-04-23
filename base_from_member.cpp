#include <iostream>

class my_base
{
public:
  my_base(int param)
  {
    std::cout << "param: " << param << "\n";
  }

  virtual ~my_base() = default;
};

template<typename T, int id = 0>
struct my_member_from_base
{
  // 这个成员本身应该是在子类中的，但由于这个成员
  // 对象需要被用来客制初始化父类，这个成员对象的
  // 构造必须先于父类对象。所以，member-from-base
  // 把成员变为私有父类，使之可以较早地构造。
  T member;

  template<typename... Ts>
  my_member_from_base(Ts&&... ts)
    : member(ts...)
  {
  }
};

class my_derived
  : private my_member_from_base<int>
  , public my_base
{
  using member0_t = my_member_from_base<int>;

public:
  my_derived(int param)
    : member0_t(param)
    , my_base(member0_t::member)
  {
  }
};

int main(int argc, char *argv[])
{
  my_derived a(100u);
  return 0;
}
