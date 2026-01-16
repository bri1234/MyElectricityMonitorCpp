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

#include "ElectricityMonitor.h"

#include "Logger.h"

#include <chrono>
#include <thread>
#include <iostream>

using std::chrono::steady_clock;
using std::chrono::duration;
using std::chrono::seconds;

using namespace std;

ElectricityMonitor::ElectricityMonitor()
{

}

void ElectricityMonitor::Run(Configuration & configuration, const CancellationToken & cancellationToken)
{
    Database database(configuration.GetDatabaseFilepath(), configuration.GetInverterNumberOfChannels());
    EbzDd3 electricityMeter(configuration.GetElectricityMeterSerialPort(), GPIO_PIN_SWITCH_ELECTRICITY_METER);
    HoymilesHmDtu hmDut(configuration.GetInverterSerialNumber(), GPIO_PIN_HOYMILES_HM_DTU_CSN, GPIO_PIN_HOYMILES_HM_DTU_CE);

    hmDut.InitializeCommunication();

    for (size_t cycleCounter = 1; !cancellationToken.IsCancel(); cycleCounter++)
    {
        auto startTime = steady_clock::now();

        CollectAndStoreData(database, electricityMeter, hmDut);

        double tm = duration<double>(steady_clock::now() - startTime).count();
        double delayTime = configuration.GetDataAcquisitionPeriod() - tm;
        if (delayTime < 5.0)
            delayTime = 5.0;

        if (cycleCounter % 20 == 0)
            LOG_INFO(format("Electricity monitor is running, cycle {}", cycleCounter));

        this_thread::sleep_for(seconds((int)delayTime));
    }
}

void ElectricityMonitor::CollectAndStoreData(Database & database, EbzDd3 & electricityMeter, HoymilesHmDtu & hmDtu)
{
    EbzDd3::Readings electricityMeterReadings;

    bool success = electricityMeter.ReceiveInfo(0, electricityMeterReadings);
    if (success)
    {
        electricityMeterReadings.Print(cout);
        // database.InsertReadingsElectricityMeter(0, electricityMeterReadings);
    }

    success = electricityMeter.ReceiveInfo(1, electricityMeterReadings);
    if (success)
    {
        electricityMeterReadings.Print(cout);
        // database.InsertReadingsElectricityMeter(1, electricityMeterReadings);
    }

    (void)hmDtu;
    (void)database;
}
