#ifndef LITEHTML_PERF_H

#include <unordered_map>
#include <chrono>
#include <string_view>
#include <iostream>

namespace litehtml
{

    class perf
    {
        std::unordered_map<std::string_view, std::chrono::microseconds> m_counters;

        bool m_started = false;

      public:
        void print_all()
        {
            std::cout << "=============Performance counters:" << std::endl;
            for(const auto& [name, duration] : m_counters)
            {
                std::cout << name << ": " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()
                          << " ms" << std::endl;
            }
            std::cout << "**********************************" << std::endl << std::endl;
        }

        void add_counter(const std::string_view name, const std::chrono::steady_clock::duration& time)
        {
            if(m_started)
            {
                m_counters[name] += std::chrono::duration_cast<std::chrono::microseconds>(time);
            }
        }

        void reset()
        {
            m_counters.clear();
        }
        void start()
        {
            m_started = true;
        }
        void stop()
        {
            m_started = false;
        }

        static perf& instance()
        {
            static perf inst;
            return inst;
        }
    };

    class perf_counter
    {
        std::string_view m_name;

        std::chrono::time_point<std::chrono::steady_clock> m_start;

      public:
        perf_counter(const std::string_view name) :
            m_name(name),
            m_start(std::chrono::steady_clock::now())
        {
        }

        ~perf_counter()
        {
            auto end = std::chrono::steady_clock::now();
            perf::instance().add_counter(m_name, end - m_start);
        }
    };

    class perf_print
    {
      public:
        ~perf_print()
        {
            perf::instance().print_all();
        }
    };

} // namespace litehtml

#endif // LITEHTML_PERF_H
