#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.putText("TEAM 06", 50, 8);
        dp.updateDisplay();
    }
    
    catch (const std::exception& e)
    {std::cerr << "Exception cought" << std::endl;}

}
