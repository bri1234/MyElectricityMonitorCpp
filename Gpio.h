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
#include <stdexcept>
#include <format>
#include <memory>

class Gpio
{
public:
    constexpr static const char * CHIP_PATH = "/dev/gpiochip0";
    
    enum GpioDirection
    {
        GD_INPUT,
        GD_OUTPUT
    };

    /// @brief GPIO error.
    class Error : public std::runtime_error
    {
    public:
        Error(const std::string & errorMessage) : std::runtime_error(std::format("GPIO error: {}", errorMessage)) { }
    };

    /// @brief Constructor. Initializes the GPIO library.
    /// @param applicationName The name of the application using the GPIO library.
    Gpio(const std::string & applicationName);

    Gpio(const Gpio & ) = delete;
    Gpio & operator=(const Gpio & ) = delete;

    /// @brief Destructor. Cleans up the GPIO library.
    virtual ~Gpio();

    /// @brief Configures the specified pin as input or output. Must be calles before using the pin.
    /// @param pinNumber The GPIO pin number to configure as output.
    /// @param direction Input or output direction.
    void InitializeGpioLine(int pinNumber, GpioDirection direction);

    /// @brief Sets the level of the specified pin.
    /// @param pinNumber The GPIO pin number to set.
    /// @param level The level to set (0 = low, 1 = high).
    void SetPinLevel(int pinNumber, int level);

    /// @brief Reads the level of the specified pin.
    /// @param pinNumber The GPIO pin number to read.
    /// @return The level of the pin (0 = low, 1 = high).
    int ReadPinLevel(int pinNumber);

private:
    std::string _applicationName;
    std::shared_ptr<gpiod_chip> _chip;
    int _numberOfLines;
    std::vector <std::shared_ptr<gpiod_line_request>> _gpioLines;

    /// @brief Checks if the specified pin number is valid.
    /// @param pinNumber The GPIO pin number to check.
    void AssertPinIsValid(int pinNumber);

    /// @brief Request a line for exclusive usage as output line.
    /// @param pinNumber The GPIO pin number.
    /// @param direction Input or output direction.
    /// @param consumer The application name.
    /// @return The requested line.
    std::shared_ptr<gpiod_line_request> RequestLine(unsigned int pinNumber, gpiod_line_direction direction, const std::string & consumer);
};  


