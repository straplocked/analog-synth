#pragma once

#include <juce_core/juce_core.h>
#include <fstream>
#include <mutex>

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
    }

    void write (const juce::String& msg)
    {
        std::lock_guard<std::mutex> lock (mtx);
        if (file.is_open())
        {
            file << juce::Time::getCurrentTime().formatted ("%H:%M:%S.") .toStdString()
                 << static_cast<int> (juce::Time::currentTimeMillis() % 1000)
                 << " | " << msg.toStdString() << std::endl;
            file.flush();
        }
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
};

#define SYNTH_LOG(msg) DebugLog::instance().write (msg)
