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

#include "Configuration.h"
#include "CancellationToken.h"
#include "Database.h"
#include "EbzDd3.h"
#include "HoymilesHmDtu.h"

constexpr const int GPIO_PIN_SWITCH_ELECTRICITY_METER = 17;
constexpr const int GPIO_PIN_HOYMILES_HM_DTU_CSN = 0;
constexpr const int GPIO_PIN_HOYMILES_HM_DTU_CE = 24;


/// @brief The main program logic for monitoring.
class ElectricityMonitor
{
public:

    /// @brief Constructor.
    ElectricityMonitor();

    ElectricityMonitor(const ElectricityMonitor &) = delete;
    ElectricityMonitor & operator=(const ElectricityMonitor &) = delete;
    
    /// @brief The main loop.
    /// @param configuration The configuration.
    /// @param cancellationToken Token to cancel the main loop.
    void Run(Configuration & configuration, const CancellationToken & cancellationToken);

private:

    /// @brief Collect and stores the electricity and inverter data.
    /// @param database The database to store the data.
    /// @param electricityMeter The electricity meter to collect data.
    /// @param hmDtu The hoymiles inverter to collect data.
    void CollectAndStoreData(Database & database, EbzDd3 & electricityMeter, HoymilesHmDtu & hmDtu);

};

