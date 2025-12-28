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

#include "Gpio.h"

#include <stdexcept>
#include <format>

using namespace std;

Gpio::Gpio(const std::string & applicationName)
: _applicationName(applicationName)
, _chip(nullptr)
, _numberOfLines(0)
{
    _chip = gpiod_chip_open(CHIP_PATH);
    if (!_chip)
    {
        throw runtime_error("Failed to open GPIO chip: " + string(CHIP_PATH));
    }

    _numberOfLines = gpiod_chip_num_lines(_chip);

    _gpioLines.resize(_numberOfLines, nullptr);
}

Gpio::~Gpio()
{
    if (_chip)
    {
        for (auto gpioLine : _gpioLines)
        {
            if (gpioLine)
                gpiod_line_release(gpioLine);
        }

        gpiod_chip_close(_chip);
    }
}

void Gpio::InitializeGpioLine(int pin, GpioDirection direction)
{
    AssertPinIsValid(pin);

    gpiod_line * gpioLine = _gpioLines.at(pin);

    if (gpioLine == nullptr)
    {
        gpioLine = gpiod_chip_get_line(_chip, pin);
        if (gpioLine == nullptr)
            throw runtime_error(format("Failed to get GPIO line for pin {}", pin));

        _gpioLines[pin] = gpioLine;
    }

    switch (direction)
    {
    case GD_INPUT:
        if (gpiod_line_request_input(gpioLine, _applicationName.c_str()) < 0)
            throw runtime_error(format("Failed to request GPIO line {} for input", pin));
        break;
    
    case GD_OUTPUT:
        if (gpiod_line_request_output(gpioLine, _applicationName.c_str(), 0) < 0)
            throw runtime_error(format("Failed to request GPIO line {} for output", pin));
        break;
    default:
        throw runtime_error(format("Unsupported GPIO direction: {}", (int)direction));
    }
}

void Gpio::SetPinLevel(int pin, int level)
{
    auto * gpioLine = GetGpioLine(pin);

    int result = gpiod_line_set_value(gpioLine, level);
    if (result < 0)
        throw runtime_error(format("Failed to set GPIO line {} to level {}", pin, level));
}

int Gpio::ReadPinLevel(int pin)
{
    auto * gpioLine = GetGpioLine(pin);
    int level = gpiod_line_get_value(gpioLine);
    if (level < 0)
        throw runtime_error(format("Failed to read GPIO line {}", pin));

    return level;
}

void Gpio::AssertPinIsValid(int pin)
{
    if ((pin < 0) || (pin >= _numberOfLines))
    {
        throw out_of_range(format("GPIO pin number {} out of range 0 ... {}", pin, _numberOfLines));
    }
}

gpiod_line *Gpio::GetGpioLine(int pin)
{
    AssertPinIsValid(pin);

    gpiod_line * gpioLine = _gpioLines.at(pin);
    if (gpioLine == nullptr)
        throw runtime_error(format("GPIO line for pin {} is not configured", pin));

    return gpioLine;
}
