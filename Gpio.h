#pragma once

/*
Copyright (C) 2025  Torsten Brischalle
email: torsten@brischalle.de
web: http://www.aaabbb.de

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#include <gpiod.h>

#include <string>
#include <vector>

class Gpio
{
public:
    constexpr static const char * CHIP_PATH = "/dev/gpiochip0";
    
    enum GpioDirection
    {
        GD_INPUT,
        GD_OUTPUT
    };

    /// @brief Constructor. Initializes the GPIO library.
    /// @param applicationName The name of the application using the GPIO library.
    Gpio(const std::string & applicationName);

    /// @brief Destructor. Cleans up the GPIO library.
    virtual ~Gpio();

    /// @brief Configures the specified pin as input or output. Must be calles before using the pin.
    /// @param pin The GPIO pin number to configure as output.
    void InitializeGpioLine(int pin, GpioDirection direction);

    /// @brief Sets the level of the specified pin.
    /// @param pin The GPIO pin number to set.
    /// @param level The level to set (0 = low, 1 = high).
    void SetPinLevel(int pin, int level);

    /// @brief Reads the level of the specified pin.
    /// @param pin The GPIO pin number to read.
    /// @return The level of the pin (0 = low, 1 = high).
    int ReadPinLevel(int pin);

private:
    std::string _applicationName;
    gpiod_chip * _chip;
    int _numberOfLines;
    std::vector <gpiod_line *> _gpioLines;

    /// @brief Checks if the specified pin number is valid.
    /// @param pin The GPIO pin number to check.
    void AssertPinIsValid(int pin);

    /// @brief Gets the gpiod_line object for the specified pin and throws an error if the line is not configured.
    /// @param pin The GPIO pin number.
    /// @return The gpiod_line object for the specified pin.
    gpiod_line * GetGpioLine(int pin);
};  


