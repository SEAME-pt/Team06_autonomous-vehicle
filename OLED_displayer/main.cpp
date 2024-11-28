#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.putText("TEAM 06", 120, 8);
        dp.updateDisplay();
        dp.invert();
    }
    
    catch (const std::exception& e)
    {std::cerr << "Exception cought" << std::endl;}

}
