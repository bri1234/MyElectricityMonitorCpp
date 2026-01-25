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

#include <iostream>
#include <chrono>
#include <thread>
#include <sys/resource.h>

#include "Logger.h"
#include "Database.h"
#include "UnixUtils.h"
#include "ElectricityMonitor.h"
#include "Configuration.h"
#include "CancellationToken.h"

using namespace std;

constexpr int RESTART_DELAY = 30;

/// @brief Changes the process priority.
/// @param newPriority The new priority. (lower value means higher priority)
void ChangeProcessPriority(int newPriority)
{
    int result = setpriority(PRIO_PROCESS, 0, newPriority);
    if (result != 0)
    {
        LOG_WARN(format("ChangeProcessPriority: setpriority failed with error code {}", errno));
    }
}

/// @brief The main entry point of the program.
/// @param argc The number of command line arguments.
/// @param argv The command line arguments.
/// @return The exit code.
int main(int argc, char **argv)
{
    string configurationFile;
    if (argc > 1)
        configurationFile = argv[1];
    else
        configurationFile = "configuration.json";

    try
    {
        // string logFile = Utils::CreateUnixLogFilepath("MyElectricityMonitor");
        // Logger::Instance().OpenLogFile(logFile);

        Logger::Instance().SetOutputStream(cout);

        LOG_INFO("********************************");
        LOG_INFO("*** PROGRAM STARTET          ***");
        LOG_INFO("********************************");

        try
        {
            // elevate process priority if possible
            ChangeProcessPriority(-10);

            Configuration configuration;
            configuration.Load(configurationFile);

            int retryCount = 0;
            CancellationToken cancellationToken;

            while (true)
            {
                auto startTime = chrono::system_clock::now();

                try
                {
                    ElectricityMonitor electricityMonitor;

                    LOG_INFO("Start electricity monitor");
                    electricityMonitor.Run(configuration, cancellationToken);
                }
                catch(const exception& e)
                {
                    LOG_ERROR(e);
                }
                LOG_INFO("Electricity monitor stopped");

                auto endTime = chrono::system_clock::now();
                int elapsedTimeMinutes = chrono::duration_cast<chrono::minutes>(endTime - startTime).count();
                
                if (elapsedTimeMinutes < 10)
                {
                    retryCount++;
                    if (retryCount > 3)
                        break;
                }
                else
                {
                    retryCount = 0;
                }

                LOG_INFO(format("try to restart in {} seconds", RESTART_DELAY));

                this_thread::sleep_for(chrono::seconds(RESTART_DELAY));
            }
        }
        catch(const exception & exc)
        {
            LOG_ERROR(exc);
        }

        LOG_INFO("*** PROGRAM STOPPED ***");
    }
    catch(const exception & exc)
    {
        cerr << exc.what() << endl;
    }

    return 0;
}
