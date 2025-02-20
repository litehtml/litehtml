#include "css_timing_function.h"

namespace litehtml
{
    passed_percentage::passed_percentage(const bool is_reverse)
        : passed_percentage{is_reverse, is_reverse ? 100.f : 0.f}
    {
    }
    
    passed_percentage::passed_percentage(const bool is_reverse, const float percentage)
        : m_percentage{percentage}, m_is_reverse{is_reverse}
    {
    }
    
    float passed_percentage::percentage() const noexcept
    {
        return m_percentage;
    }
    
    bool passed_percentage::is_reverse() const noexcept
    {
        return m_is_reverse;
    }

    namespace internal
    {
        struct cubic_bezier
        {
            struct point
            {
                float x = 0.0f;
                float y = 0.0f;
        
                point operator*(const float f) const
                {
                    return point{x * f, y * f};
                }
        
                point operator+(const point& other) const
                {
                    return point{x + other.x, y + other.y};
                }
            };
        
            cubic_bezier() = default;
        
            explicit cubic_bezier(point p0, point p1, point p2, point p3) : m_p0{p0}, m_p1{p1}, m_p2{p2}, m_p3{p3}
            {
            }
        
            [[nodiscard]] float calculate(const float t) const
            {
                const float u = 1.0f - t;
                const point p0 = m_p0 * (u * u * u);
                const point p1 = m_p1 * (3 * u * u * t);
                const point p2 = m_p2 * (3 * u * t * t);
                const point p3 = m_p3 * (t * t * t);
                auto [_, y] = p0 + p1 + p2 + p3;
                return y;
            }
        
          private:
            point m_p0, m_p1, m_p2, m_p3;
        };
        
        static cubic_bezier create_cubic_bezier_for(const timing_function_type fun_type)
        {
            constexpr cubic_bezier::point p0{0.0f, 0.0f};
            constexpr cubic_bezier::point p3{1.0f, 1.0f};
        
            switch (fun_type)
            {
            case timing_function_linear:
                return cubic_bezier{p0, p0, p3, p3};
            case timing_function_ease: {
                constexpr cubic_bezier::point p1{0.25f, 0.1f};
                constexpr cubic_bezier::point p2{0.25f, 1.0f};
                return cubic_bezier{p0, p1, p2, p3};
            }
            case timing_function_ease_in: {
                constexpr cubic_bezier::point p1{0.42f, 0.0f};
                constexpr cubic_bezier::point p2{1.0f, 1.0f};
                return cubic_bezier{p0, p1, p2, p3};
            }
            case timing_function_ease_out: {
                constexpr cubic_bezier::point p1{0.0f, 0.0f};
                constexpr cubic_bezier::point p2{0.58f, 1.0f};
                return cubic_bezier{p0, p1, p2, p3};
            }
            case timing_function_ease_in_out: {
                constexpr cubic_bezier::point p1{0.42f, 0.0f};
                constexpr cubic_bezier::point p2{0.58f, 1.0f};
                return cubic_bezier{p0, p1, p2, p3};
            }
            default:
                return cubic_bezier{p0, p0, p3, p3};
            }
        }
        
    } // namespace internal

    namespace css_timing_function
    {
        passed_percentage apply_timing_function(passed_percentage percentage, timing_function_type fun_type)
        {
            float old_percentage = std::clamp(percentage.percentage(), 0.0f, 100.0f);
        
            if (old_percentage > 0.f)
            {
                old_percentage = old_percentage / 100.f;
            }
        
            const auto timing_function{internal::create_cubic_bezier_for(fun_type)};
            const float new_percentage = timing_function.calculate(old_percentage) * 100.0f;
        
            return passed_percentage{percentage.is_reverse(), new_percentage};
        }
    }
}