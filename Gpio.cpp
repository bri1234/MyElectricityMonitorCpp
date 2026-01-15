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
, _numberOfLines(0)
{
    _chip = shared_ptr<gpiod_chip>(gpiod_chip_open(CHIP_PATH), gpiod_chip_close);
    if (!_chip)
    {
        throw Error("Failed to open GPIO chip: " + string(CHIP_PATH));
    }

    shared_ptr<gpiod_chip_info> chipInfo(gpiod_chip_get_info(_chip.get()), gpiod_chip_info_free);
    _numberOfLines = gpiod_chip_info_get_num_lines(chipInfo.get());

    _gpioLines.resize(_numberOfLines, nullptr);
}

Gpio::~Gpio()
{
    _gpioLines.clear();
    _chip.reset();
}

std::shared_ptr<gpiod_line_request> Gpio::RequestLine(unsigned int pinNumber, gpiod_line_direction direction, const std::string & consumer)
{
	shared_ptr<gpiod_line_settings> settings(gpiod_line_settings_new(), gpiod_line_settings_free);
	if (!settings)
        throw Error(format("gpiod_line_settings_new() failed for pin {}", pinNumber));

	gpiod_line_settings_set_direction(settings.get(), direction);

    if (direction == GPIOD_LINE_DIRECTION_OUTPUT)
	    gpiod_line_settings_set_output_value(settings.get(), GPIOD_LINE_VALUE_INACTIVE);

    shared_ptr <gpiod_line_config> lineConfig(gpiod_line_config_new(), gpiod_line_config_free);
	if (!lineConfig)
        throw Error(format("gpiod_line_config_new() failed for pin {}", pinNumber));

	if (gpiod_line_config_add_line_settings(lineConfig.get(), &pinNumber, 1, settings.get()))
        throw Error(format("gpiod_line_config_add_line_settings() faield for pin {}", pinNumber));

    shared_ptr <gpiod_request_config> requestConfig(gpiod_request_config_new(), gpiod_request_config_free);
    if (!requestConfig)
        throw Error(format("gpiod_request_config_new() failed for pin {}", pinNumber));

    gpiod_request_config_set_consumer(requestConfig.get(), consumer.c_str());

	shared_ptr <gpiod_line_request> request(gpiod_chip_request_lines(_chip.get(), requestConfig.get(), lineConfig.get()),
        gpiod_line_request_release);

    if (!request)
        throw Error(format("gpiod_chip_request_lines() failed for pin {}", pinNumber));

    return request;
}

void Gpio::InitializeGpioLine(int pinNumber, GpioDirection direction)
{
    AssertPinIsValid(pinNumber);

    if (_gpioLines.at(pinNumber))
        _gpioLines[pinNumber].reset();

    _gpioLines[pinNumber] = RequestLine(pinNumber, direction == GD_OUTPUT ? GPIOD_LINE_DIRECTION_OUTPUT : GPIOD_LINE_DIRECTION_INPUT, _applicationName);
}

void Gpio::SetPinLevel(int pinNumber, int level)
{
    auto gpioLine = _gpioLines.at(pinNumber);
    if (!gpioLine)
        throw Error(format("SetPinLevel() failed, pin {} is not initialized", pinNumber));

    int result = gpiod_line_request_set_value(gpioLine.get(), pinNumber, level ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);
    if (result < 0)
        throw Error(format("SetPinLevel() failed to set GPIO line {} to level {}", pinNumber, level));
}

int Gpio::ReadPinLevel(int pinNumber)
{
    auto gpioLine = _gpioLines.at(pinNumber);
    if (!gpioLine)
        throw Error(format("ReadPinLevel() failed, pin {} is not initialized", pinNumber));

    auto result = gpiod_line_request_get_value(gpioLine.get(), pinNumber);
    if (result < 0)
        throw Error(format("ReadPinLevel() failed to read GPIO line {}", pinNumber));
    
    return result == GPIOD_LINE_VALUE_ACTIVE ? 1 : 0;
}

void Gpio::AssertPinIsValid(int pin)
{
    if ((pin < 0) || (pin >= _numberOfLines))
    {
        throw Error(format("GPIO pin number {} out of range 0 ... {}", pin, _numberOfLines));
    }
}

