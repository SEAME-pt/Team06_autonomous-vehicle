#include "Logger.hpp"
#include "Joystick.hpp"
//#include "Timer.hpp"
//#include "VehiclePhysics.hpp"

#include <thread>
#include <mutex>
#include <chrono>

// int main()
// {
//     Canvas display;
//     if (!display.init(1, 0x3C))
//     {
//         LOG_ERROR("Failed to initialize display");
//         return 1;
//     }

//     Joystick joystick("/dev/input/js0");
//     if (!joystick.isConnected())
//     {
//         LOG_ERROR("Failed to initialize joystick");
//         return -1;
//     }

//     display.clear();
//     display.draw_text(1, 2, "Joy Pad", 1);
//     display.update();

//     std::thread display_thread(display_thread_function, &display, &joystick);

//     // Espera a thread terminar
//     display_thread.join();

//     display.cleanup();
//     return 0;
// }

int main()
{
   
    Motors car;

    if (!car.init())
    {
        LOG_ERROR("Failed to initialize Jetcar");
        return 1;
    }

    Joystick joystick("/dev/input/js0");

    if (!joystick.isConnected())
    {
        LOG_ERROR("Failed to initialize joystick");
        return -1;
    }

    auto last_update = std::chrono::steady_clock::now();
    const auto update_interval = std::chrono::milliseconds(16); // ~60fps

    double accelaration = 0;
    double turn = 0;

    while (true)
    {
        bool onClick = false;
        if (!joystick.readEvent())
        {
            break;
        }
       
        if (joystick.getButton(X_BUTTON))
        {
            // std::cout << "X_BUTTON" << std::endl;
            onClick = true;
            car.stop();
        }
        if (joystick.getButton(LEFT_BUTTON_ONE))
        {
            car.set_right_speed(100);
             std::cout << "RIGHT_BUTTON_ONE" << std::endl;
    
        }
        if (joystick.getButton(LEFT_BUTTON_TWO))
        {
             car.set_right_speed(-100);
             std::cout << "RIGHT_BUTTON 2" << std::endl;
    
        }
        
        if (joystick.getButton(RIGHT_BUTTON_ONE))
        {
            car.set_left_speed(100);
                   
        }
        if (joystick.getButton(RIGHT_BUTTON_TWO))
        {
            car.set_left_speed(-100);    
        }

        if (joystick.getButton(SELECT_BUTTON))
        {
            std::cout << "SELECT_BUTTON" << std::endl;
            break;
        }
        if (joystick.getButton(START_BUTTON))
        {
            std::cout << "START_BUTTON" << std::endl;
        }
        if (joystick.getButton(HOME_BUTTON))
        {
            std::cout << "HOME_BUTTON" << std::endl;
        }

        accelaration *= 0.99;
        turn *= 0.99;

        {
            float force = joystick.getAxis(3);
            if (force != 0)
            {
                accelaration -= (force *0.55f);
            }
            car.set_speed(static_cast<int>(accelaration * 1));

            float gear = joystick.getAxis(0);

            if (gear !=0 )
            {
                turn = (gear * 0.095f) / 0.02f;
                if (turn<=-4.5)
                {
                    turn = -4.5;
                } else
                if (turn >= 4.5)
                 {
                    turn = 4.5;
                 }
                LOG_INFO("Gear %f", turn);
            }
            car.set_steering(static_cast<int>(turn * 25));

            // auto now = std::chrono::steady_clock::now();
            // if (now - last_update >= update_interval)
            // {
            //     display.draw_text(5, 5, 1, "turn: %f", gear);
            //     display.draw_text(5, 15, 1, "accelation: %f", accelaration);
            //     display.update();
            //     last_update = now;
            // }
        }
    }
    car.stop();

    return 0;
}

// int main()
// {
//     Canvas display;

//     if (!display.init(1, 0x3C))
//     {
//         LOG_ERROR("Failed to initialize display");
//         return 1;
//     }

//     LOG_INFO("Starting display test");

//     display.clear();
//     display.draw_text(1, 2, "AB", 1);
//     display.draw_text(1, 13, "a b", 1);
//     display.draw_text(1, 25, "0 1 2", 1);

//     if (!display.update())
//     {
//         LOG_ERROR("Failed to update display");
//         return 1;
//     }

//     sleep(5);
//     display.cleanup();

//     LOG_INFO("Test completed");
//     return 0;
// }

// int main()
// {
//     Motors car;

//     if (!car.init())
//     {
//         LOG_ERROR("Failed to initialize Jetcar");
//         return 1;
//     }

//     LOG_INFO("Testing Jetcar movement...");

//     // Movimento para frente
//     car.set_speed(100);
//     usleep(2000000); // 2 segundos

//     // Curva
//     car.set_speed(-100);
//     usleep(1500000);     // 1.5 segundos

//     // Para
//     car.stop();

//     LOG_INFO("Test complete");
//     return 0;
// }

// int main()
// {

//   Motors car;

//     if (!car.init())
//     {
//         LOG_ERROR("Failed to initialize Jetcar");
//         return 1;
//     }


//     Joystick joystick("/dev/input/js0");

//     if (!joystick.isConnected())
//     {
//         LOG_ERROR("Failed to initialize joystick");
//         return 1;
//     }


//     Canvas display;

//     if (!display.init(1, 0x3C))
//     {
//         LOG_ERROR("Failed to initialize display");
//         return 1;
//     }

// VehiclePhysics physics;
// auto last_update = std::chrono::steady_clock::now();
// const auto update_interval = std::chrono::milliseconds(16); // ~60fps

//     display.clear();
//     display.draw_text(1, 2, "Team06", 1);
//     display.update();


// while (true)
// {
//     if (!joystick.readEvent())
//     {
//         break;
//     }

//     auto now = std::chrono::steady_clock::now();
//     float deltaTime = std::chrono::duration<float>(now - last_update).count();
//     LOG_INFO("%f ", deltaTime);

//     float throttle = joystick.getAxis(3); // Acelerador
//     float steering = joystick.getAxis(0); // Direção

//     physics.update(throttle, steering, deltaTime*1000.0f);


//     auto state = physics.getState();


//     car.set_speed(static_cast<int>(state.speed));
//     car.set_steering(static_cast<int>(state.steering * 10));

//     // Botões especiais
//     if (joystick.getButton(X_BUTTON))
//     {
//         physics.setBrake();
//         display.clear();
//         display.draw_text(1, 5, "BRAKE", 1);
//     }

//     if (joystick.getButton(SELECT_BUTTON))
//     {
//         std::cout << "SELECT_BUTTON" << std::endl;
//         break;
//     }

//     // Atualiza display
//     if (now - last_update >= update_interval)
//     {
//         display.clear();
//         display.draw_text(1, 5, "SPD: %.1f", state.speed);
//         display.draw_text(1, 15, "STR: %.1f", state.steering);
//         display.draw_text(1, 25, "ACC: %.1f", state.acceleration);
//         display.update();
//         last_update = now;
//     }
// }

// return 0;
// }