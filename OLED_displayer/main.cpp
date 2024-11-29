#include "Display.hpp"

int	main(void)
{
    try
    {
        Display	dp;
        dp.putText("IP: 10.21.221.56", 0, 0);
        while (1)
        {
            dp.putImage(dp.faces[0], 16, 56, 12);
            dp.updateDisplay();
            sleep(1);
            dp.putImage(dp.faces[1], 16, 56, 12);
            dp.updateDisplay();
            sleep(1);
        }
    }
    
    catch (const Display::DisplayException& e)
    {std::cerr << e.what() << std::endl;}

}
