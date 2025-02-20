#ifndef LH_CSS_TIMING_FUNCTION_H
#define LH_CSS_TIMING_FUNCTION_H
#include "types.h"

namespace litehtml
{
    struct passed_percentage
    {
        explicit passed_percentage(bool is_reverse);
        explicit passed_percentage(bool is_reverse, float percentage);

        [[nodiscard]] float percentage() const noexcept;
        [[nodiscard]] bool is_reverse() const noexcept;

      private:
        float m_percentage = 0.f;
        bool m_is_reverse = false;
    };

    namespace css_timing_function
    {
        passed_percentage apply_timing_function(passed_percentage percentage, timing_function_type fun_type);
    }
}

#endif