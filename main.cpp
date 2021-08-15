#include <iostream>
#include "my_format_cpp11.h"

class Object
{
public:
    // This is not a ToString we want. Because it return int (not string)
    int ToString()
    {
        return 1;
    }

private:
    float x = 10;
    float y = 1;
};

class Track
{
public:
    string ToString()
    {
        return "x=" + std::to_string(x) +
               ",y=" + std::to_string(y) +
               ",motion=" + motion_type;
    }

private:
    float x = 10;
    float y = 1;
    string motion_type = "Moving";
};

int main()
{
    Object o;
    Track t;
    std::cout << Format("Very Good {1:.3f} we make it { {} {0} {3} {2}", 1, 2.0, o, t) << endl;
    std::cout << Format("") << endl;
    std::cout << Format("{1:.5f}", 0, 2.0f) << endl;
}