/*
 * AnalogSynth - Virtual analog synthesizer plugin
 * Copyright (C) 2025 AnalogSynth contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <juce_core/juce_core.h>
#include <fstream>
#include <mutex>
#include <iostream>
#include <cstdlib>

class DebugLog
{
public:
    static DebugLog& instance()
    {
        static DebugLog log;
        return log;
    }

    void open (const juce::String& path)
    {
        std::lock_guard<std::mutex> lock (mtx);
        file.open (path.toStdString(), std::ios::out | std::ios::trunc);
        if (file.is_open())
            file << "=== AnalogSynth Debug Log ===" << std::endl;

        // Auto-enable stdout if environment variable is set
        if (std::getenv ("SYNTH_DEBUG_STDOUT") != nullptr)
            writeToStdout = true;
    }

    void setStdoutEnabled (bool enabled) { writeToStdout = enabled; }

    void write (const juce::String& msg)
    {
        std::lock_guard<std::mutex> lock (mtx);

        auto timestamp = juce::Time::getCurrentTime().formatted ("%H:%M:%S.").toStdString()
                       + std::to_string (static_cast<int> (juce::Time::currentTimeMillis() % 1000));
        auto line = timestamp + " | " + msg.toStdString();

        if (file.is_open())
        {
            file << line << std::endl;
            file.flush();
        }

        if (writeToStdout)
            std::cout << line << std::endl;
    }

    void close()
    {
        std::lock_guard<std::mutex> lock (mtx);
        if (file.is_open())
            file.close();
    }

    ~DebugLog() { close(); }

private:
    DebugLog() = default;
    std::ofstream file;
    std::mutex mtx;
    bool writeToStdout = false;
};

#define SYNTH_LOG(msg)        DebugLog::instance().write (msg)
#define SYNTH_LOG_VOICE(msg)  DebugLog::instance().write (juce::String ("[VOICE] ") + msg)
#define SYNTH_LOG_PARAM(msg)  DebugLog::instance().write (juce::String ("[PARAM] ") + msg)
#define SYNTH_LOG_FILTER(msg) DebugLog::instance().write (juce::String ("[FILTER] ") + msg)
#define SYNTH_LOG_ERROR(msg)  DebugLog::instance().write (juce::String ("[ERROR] ") + msg)
