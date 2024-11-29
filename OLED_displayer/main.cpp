#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.putText("IP: 10.21.221.56", 0, 16);
        dp.updateDisplay();
    }
    
    catch (const Display::DisplayException& e)
    {std::cerr << e.what() << std::endl;}

}
