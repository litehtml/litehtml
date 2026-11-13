#ifndef LITEHTML_CSS_LENGTH_H
#define LITEHTML_CSS_LENGTH_H

#include <string>
#include <functional>
#include <memory>
#include "css_tokenizer.h"
#include "css_values.h"
#include "pixel_type.h"

namespace litehtml
{
    struct css_math_expression;
    // from_token options (flags)
    enum
    {
        f_length            = 1,                       // <length> (includes unitless zero)
        f_percentage        = 2,                       // <percentage>
        f_length_percentage = f_length | f_percentage, // <length-percentage>
        f_number            = 4,                       // <number>
        f_integer           = 8,                       // <integer>
        f_positive          = 16                       // ⩾0 (non-negative)
    };

    // <length> | <percentage> | <number> | auto | none | normal
    // https://developer.mozilla.org/en-US/docs/Web/CSS/length
    class css_length
    {
        union {
            float m_value = 0;
            int   m_predef;
        };
        css_units                                  m_units         = css_units_none;
        bool                                       m_is_predefined = false;
        std::shared_ptr<const css_math_expression> m_expression;
        std::shared_ptr<const css_length>          m_fit_content_argument;
        bool                                       m_non_negative = false;

      public:
        static constexpr float max_math_pixels = 16777215.0f;
        css_length()                           = default;
        css_length(float val, css_units units = css_units_px);
        css_length& operator=(float val);

        bool              is_predefined() const;
        void              predef(int val);
        int               predef() const;
        static css_length predef_value(int val = 0);
        void              set_value(float val, css_units units);
        float             val() const;
        css_units         units() const;
        pixel_t           calc_percent(pixel_t width) const;
        bool              from_token(const css_token& token, int options, css_values predefined_keywords = {});
        std::string       to_string() const;
        bool              is_intrinsic_sizing() const;
        bool              is_auto_or_none() const;
        void              set_fit_content(const css_length& argument);
        const css_length* fit_content_argument() const;
        bool              is_expression() const
        {
            return m_expression != nullptr;
        }
        bool has_percentage() const;
        // Conservatively includes percentages that may cancel within an expression.
        bool has_nonzero_percentage() const;
        void resolve_units(const std::function<float(float, css_units)>& convert);
    };

    using length_vector = std::vector<css_length>;

    inline css_length::css_length(float val, css_units units) :
        m_value(val),
        m_units(units)
    {
    }

    inline css_length& css_length::operator=(float val)
    {
        m_value         = val;
        m_units         = css_units_px;
        m_is_predefined = false;
        m_expression.reset();
        m_fit_content_argument.reset();
        m_non_negative = false;
        return *this;
    }

    inline bool css_length::is_predefined() const
    {
        return m_is_predefined;
    }

    inline void css_length::predef(int val)
    {
        m_predef        = val;
        m_is_predefined = true;
        m_expression.reset();
        m_fit_content_argument.reset();
        m_non_negative = false;
    }

    inline int css_length::predef() const
    {
        if(m_is_predefined)
        {
            return m_predef;
        }
        return 0;
    }

    inline void css_length::set_value(float val, css_units units)
    {
        m_value         = val;
        m_is_predefined = false;
        m_units         = units;
        m_expression.reset();
        m_fit_content_argument.reset();
        m_non_negative = false;
    }

    inline bool css_length::is_intrinsic_sizing() const
    {
        return m_is_predefined && m_predef >= sizing_min_content && m_predef <= sizing_fit_content_function;
    }

    inline bool css_length::is_auto_or_none() const
    {
        return m_is_predefined && m_predef == sizing_auto;
    }

    inline void css_length::set_fit_content(const css_length& argument)
    {
        predef(sizing_fit_content_function);
        m_fit_content_argument = std::make_shared<const css_length>(argument);
    }

    inline const css_length* css_length::fit_content_argument() const
    {
        return m_fit_content_argument.get();
    }

    inline css_units css_length::units() const
    {
        return m_units;
    }

} // namespace litehtml

#endif // LITEHTML_CSS_LENGTH_H
