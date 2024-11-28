#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.setPixel(0, 0);
        dp.setPixel(127, 0);
        dp.setPixel(0, 31);
        dp.setPixel(127, 31);
    }
    
    catch (const std::exception& e)
    {std::cerr << "Exception cought" << std::endl;}

}
