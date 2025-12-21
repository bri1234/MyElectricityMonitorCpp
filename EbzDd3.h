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

#include <vector>
#include <string>
#include <map>

class EbzDd3
{
public:
    /*
        Units for the meter readings:

        "+A"      = meter reading +A, tariff-free in kWh
        "+A T1"   = meter reading +A, tariff 1 in kWh
        "+A T2"   = meter reading +A, tariff 2 in kWh
        "-A"      = meter reading -A, tariff-free in kWh
        "P"       = Sum of instantaneous power in all phases in W
        "P L1"    = Instantaneous power phase L1 in W
        "P L2"    = Instantaneous power phase L2 in W
        "P L3"    = Instantaneous power phase L3 in W
        +A: Active energy, grid supplies to customer.
        -A: Active energy, customer supplies to grid
    */
    const std::map <std::string, std::string> Units = {
        { "+A", "kWh" },
        { "+A T1", "kWh" },
        { "+A T2", "kWh" },
        { "-A", "kWh" },
        { "P", "W" },
        { "P L1", "W" },
        { "P L2", "W" },
        { "P L3", "W" }
    };

    EbzDd3(const std::string & serialPort);

private:
    std::string _serialPort;
};

