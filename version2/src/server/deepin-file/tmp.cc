#include <vector>
#include <iostream>

int main(void)
{
    std::vector<int> x;
    x.reserve(1);
    std::cout << x.size() << std::endl;
    std::cout << x.capacity() << std::endl;
    x.reserve(11);
    std::cout << x.size() << std::endl;
    std::cout << x.capacity() << std::endl;
    x.reserve(111);
    std::cout << x.size() << std::endl;
    std::cout << x.capacity() << std::endl;
}
