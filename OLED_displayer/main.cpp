#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.updateDisplay();
    }
    
    catch (const std::exception& e)
    {std::cerr << "Exception cought" << std::endl;}

}
