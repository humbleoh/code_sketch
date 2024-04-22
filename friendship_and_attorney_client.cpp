#include <iostream>

template<typename T1, typename T2>
class my_attorney;

class my_restricted_friend;
class my_restricted_friend2;

class my_client
{
private:
  void func0(int)
  {
    std::cout << "func0\n";
  }

  void func1(const my_client&)
  {
    std::cout << "func1\n";
  }
  
  // 通过 attorney 限制访问私有成员的一种手段。
  friend class my_attorney<my_client, my_restricted_friend>;
  friend class my_attorney<my_client, my_restricted_friend2>;
};

template<>
class my_attorney<my_client, my_restricted_friend>
{
private:
  template<typename... Args>
  static void call_func0(my_client& client, Args&&... args)
  {
    client.func0(std::forward<Args>(args)...);
  }

  friend class my_restricted_friend;
};

template<>
class my_attorney<my_client, my_restricted_friend2>
{
private:
  template<typename... Args>
  static void call_func1(my_client& client, Args&&... args)
  {
    client.func1(std::forward<Args>(args)...);
  }

  friend class my_restricted_friend2;
};

class my_restricted_friend
{
public:
  void test_func0(my_client& t)
  {
    my_attorney<my_client, my_restricted_friend>::call_func0(t, 10);
  }
};

class my_restricted_friend2
{
public:
  void test_func1(my_client& t)
  {
    my_attorney<my_client, my_restricted_friend2>::call_func1(t, t);
  }
};

int main(int argc, char *argv[])
{
  my_client c;
  my_restricted_friend f;
  my_restricted_friend2 f2;
  f.test_func0(c);
  f2.test_func1(c);
  return 0;
}
