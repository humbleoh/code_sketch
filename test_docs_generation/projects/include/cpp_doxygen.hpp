#include <iostream>
#include <string>

/// @brief Describe laji
class Foo {
public:
    /// @brief Say hello is nothing
    /// @param message The message to print
    void say_hello(std::string message) const;

protected:
    /// @brief Describe nothing
    void test();

private:
    /// @brief Describe private function Foo::say_hello
    /// @param[in] counter Counter to count abc
    void private_function(int);
};
