#include "html.h"
#include "css_length.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <iomanip>
#include <sstream>

namespace litehtml
{
    struct css_math_expression
    {
        enum operation
        {
            literal,
            sum,
            difference,
            product,
            quotient,
            minimum,
            maximum,
            clamp
        };
        operation                                               op               = literal;
        double                                                  value            = 0;
        css_units                                               unit             = css_units_none;
        int                                                     length_power     = 0;
        int                                                     percentage_power = 0;
        bool                                                    percentage       = false;
        bool                                                    needs_conversion = false;
        int                                                     tree_depth       = 0;
        std::vector<std::shared_ptr<const css_math_expression>> args;

        bool has_nonzero_percentage() const
        {
            if(op == literal)
            {
                return unit == css_units_percentage && std::fpclassify(value) != FP_ZERO;
            }
            return std::any_of(args.begin(), args.end(),
                               [](const auto& arg) { return arg && arg->has_nonzero_percentage(); });
        }

        std::string to_string() const
        {
            if(op == literal)
            {
                const auto suffix = unit == css_units_none ? "" : css_values(css_units_strings).value_by_index(unit);
                if(!std::isfinite(value))
                {
                    const auto keyword = std::isnan(value) ? "NaN" : std::signbit(value) ? "-infinity" : "infinity";
                    if(unit == css_units_none)
                    {
                        return keyword;
                    }
                    std::string result  = std::string("calc(") + keyword + " * 1";
                    result             += suffix;
                    return result + ")";
                }
                std::ostringstream text;
                text << std::setprecision(std::numeric_limits<double>::max_digits10) << value << suffix;
                return text.str();
            }
            const char* name      = op == minimum ? "min(" : op == maximum ? "max(" : op == clamp ? "clamp(" : "calc(";
            const char* separator = op == sum          ? " + "
                                    : op == difference ? " - "
                                    : op == product    ? " * "
                                    : op == quotient   ? " / "
                                                       : ", ";
            std::string result    = name;
            for(size_t i = 0; i < args.size(); i++)
            {
                if(i)
                {
                    result += separator;
                }
                result += args[i] ? args[i]->to_string() : "none";
            }
            return result + ")";
        }

        static double compare(double left, double right, bool smallest)
        {
            if(std::isnan(left) || std::isnan(right))
            {
                return std::numeric_limits<double>::quiet_NaN();
            }
            if(std::fpclassify(left) == FP_ZERO && std::fpclassify(right) == FP_ZERO)
            {
                return smallest ? (std::signbit(left) || std::signbit(right) ? -0.0 : 0.0)
                                : (std::signbit(left) && std::signbit(right) ? -0.0 : 0.0);
            }
            return smallest ? std::min(left, right) : std::max(left, right);
        }

        double evaluate(double basis) const
        {
            if(op == literal)
            {
                return unit == css_units_percentage ? (length_power ? basis * value / 100 : value / 100) : value;
            }
            double result = args[0] ? args[0]->evaluate(basis) : -std::numeric_limits<double>::infinity();
            if(op == clamp)
            {
                const double center = args[1]->evaluate(basis);
                const double upper  = args[2] ? args[2]->evaluate(basis) : std::numeric_limits<double>::infinity();
                return compare(result, compare(center, upper, true), false);
            }
            for(size_t i = 1; i < args.size(); i++)
            {
                const double right = args[i]->evaluate(basis);
                switch(op)
                {
                case sum:
                    result += right;
                    break;
                case difference:
                    result -= right;
                    break;
                case product:
                    result *= right;
                    break;
                case quotient:
                    result /= right;
                    break;
                case minimum:
                case maximum:
                    // CSS comparisons distinguish positive and negative zero.
                    result = compare(result, right, op == minimum);
                    break;
                default:
                    break;
                }
            }
            return result;
        }
    };

    namespace
    {
        using expression = std::shared_ptr<const css_math_expression>;
        using math       = css_math_expression;

        class math_parser
        {
            const css_token_vector& tokens;
            size_t                  index = 0;
            int                     options;
            int                     depth;

            void whitespace()
            {
                while(index < tokens.size() && tokens[index].type == WHITESPACE)
                {
                    index++;
                }
            }

            expression primary()
            {
                whitespace();
                if(index == tokens.size() || depth > 128)
                {
                    return nullptr;
                }
                const auto& token = tokens[index++];
                if(token.type == ROUND_BLOCK)
                {
                    return math_parser(token.value, options, depth + 1).parse();
                }
                if(token.type == CV_FUNCTION)
                {
                    return function(token, options, depth + 1);
                }
                auto node = std::make_shared<math>();
                if(token.type == IDENT)
                {
                    const auto name = lowcase(token.name());
                    if(name == "infinity")
                    {
                        node->value = std::numeric_limits<double>::infinity();
                    } else if(name == "-infinity")
                    {
                        node->value = -std::numeric_limits<double>::infinity();
                    } else if(name == "nan")
                    {
                        node->value = std::numeric_limits<double>::quiet_NaN();
                    } else if(name == "e")
                    {
                        node->value = std::exp(1.0);
                    } else if(name == "pi")
                    {
                        node->value = std::acos(-1.0);
                    } else
                    {
                        return nullptr;
                    }
                } else if(token.type == NUMBER || token.type == PERCENTAGE || token.type == DIMENSION)
                {
                    node->value = token.n.number;
                    if(token.type == PERCENTAGE)
                    {
                        node->unit = css_units_percentage;
                        if((options & f_length_percentage) == f_length_percentage)
                        {
                            node->length_power = 1;
                            node->percentage   = true;
                        } else
                        {
                            node->percentage_power = 1;
                        }
                    } else if(token.type == DIMENSION)
                    {
                        const auto unit = css_values(css_units_strings).value_index(lowcase(token.unit()));
                        if(!unit || *unit == css_units_none || *unit == css_units_percentage)
                        {
                            return nullptr;
                        }
                        node->unit             = static_cast<css_units>(*unit);
                        node->length_power     = 1;
                        node->needs_conversion = node->unit != css_units_px;
                    }
                } else
                {
                    return nullptr;
                }
                return node;
            }

            static expression combine(expression left, expression right, math::operation op)
            {
                if(!left || !right)
                {
                    return nullptr;
                }
                auto node        = std::make_shared<math>();
                node->op         = op;
                node->tree_depth = std::max(left->tree_depth, right->tree_depth) + 1;
                if(node->tree_depth > 128)
                {
                    return nullptr;
                }
                if(op == math::product || op == math::quotient)
                {
                    const int sign         = op == math::product ? 1 : -1;
                    node->length_power     = left->length_power + sign * right->length_power;
                    node->percentage_power = left->percentage_power + sign * right->percentage_power;
                } else
                {
                    if(left->length_power != right->length_power || left->percentage_power != right->percentage_power)
                    {
                        return nullptr;
                    }
                    node->length_power     = left->length_power;
                    node->percentage_power = left->percentage_power;
                }
                node->percentage       = left->percentage || right->percentage;
                node->needs_conversion = left->needs_conversion || right->needs_conversion;
                node->args             = {std::move(left), std::move(right)};
                return node;
            }

            expression product()
            {
                auto result = primary();
                while(result)
                {
                    whitespace();
                    if(index == tokens.size() || (tokens[index].ch != '*' && tokens[index].ch != '/'))
                    {
                        break;
                    }
                    const int op = tokens[index++].ch;
                    result       = combine(result, primary(), op == '*' ? math::product : math::quotient);
                }
                return result;
            }

            expression sum()
            {
                auto result = product();
                while(result)
                {
                    // product() consumed whitespace before the operator.
                    if(index == tokens.size() || (tokens[index].ch != '+' && tokens[index].ch != '-'))
                    {
                        break;
                    }
                    const auto& token  = tokens[index];
                    const bool  before = token.whitespace_before || (index > 0 && tokens[index - 1].type == WHITESPACE);
                    const bool  after =
                        token.whitespace_after || (index + 1 < tokens.size() && tokens[index + 1].type == WHITESPACE);
                    if(!before || !after)
                    {
                        return nullptr;
                    }
                    const int op = tokens[index++].ch;
                    result       = combine(result, product(), op == '+' ? math::sum : math::difference);
                }
                return result;
            }

          public:
            math_parser(const css_token_vector& input, int flags, int nesting = 0) :
                tokens(input),
                options(flags),
                depth(nesting)
            {
            }

            expression parse()
            {
                auto result = sum();
                whitespace();
                return index == tokens.size() ? result : nullptr;
            }

            static expression function(const css_token& token, int options, int depth = 0)
            {
                if(depth > 128)
                {
                    return nullptr;
                }
                const auto name = lowcase(token.name());
                if(name == "calc")
                {
                    return math_parser(token.value, options, depth).parse();
                }
                if(name != "min" && name != "max" && name != "clamp")
                {
                    return nullptr;
                }
                auto node    = std::make_shared<math>();
                node->op     = name == "min" ? math::minimum : name == "max" ? math::maximum : math::clamp;
                size_t begin = 0;
                for(size_t end = 0; end <= token.value.size(); end++)
                {
                    if(end != token.value.size() && token.value[end].type != COMMA)
                    {
                        continue;
                    }
                    css_token_vector part(token.value.begin() + static_cast<std::ptrdiff_t>(begin),
                                          token.value.begin() + static_cast<std::ptrdiff_t>(end));
                    expression       arg;
                    size_t           first = 0;
                    while(first < part.size() && part[first].type == WHITESPACE)
                    {
                        first++;
                    }
                    size_t last = part.size();
                    while(last > first && part[last - 1].type == WHITESPACE)
                    {
                        last--;
                    }
                    if(name == "clamp" && last == first + 1 && part[first].ident() == "none" &&
                       (node->args.empty() || node->args.size() == 2))
                    {
                        arg = nullptr;
                    } else
                    {
                        arg = math_parser(part, options, depth + 1).parse();
                        if(!arg)
                        {
                            return nullptr;
                        }
                    }
                    node->args.push_back(arg);
                    begin = end + 1;
                }
                if(name == "clamp" && node->args.size() != 3)
                {
                    return nullptr;
                }
                expression type;
                for(const auto& arg : node->args)
                {
                    if(!arg)
                    {
                        continue;
                    }
                    if(type &&
                       (type->length_power != arg->length_power || type->percentage_power != arg->percentage_power))
                    {
                        return nullptr;
                    }
                    type                   = arg;
                    node->percentage       = node->percentage || arg->percentage;
                    node->needs_conversion = node->needs_conversion || arg->needs_conversion;
                    node->tree_depth       = std::max(node->tree_depth, arg->tree_depth + 1);
                }
                if(!type)
                {
                    return nullptr;
                }
                if(node->tree_depth > 128)
                {
                    return nullptr;
                }
                node->length_power     = type->length_power;
                node->percentage_power = type->percentage_power;
                return node;
            }
        };

        expression convert_expression(const expression& input, const std::function<float(float, css_units)>& convert)
        {
            auto node              = std::make_shared<math>(*input);
            node->needs_conversion = false;
            if(node->op == math::literal && node->unit != css_units_none && node->unit != css_units_percentage)
            {
                // Convert a unit scale, retaining IEEE values inside the calculation.
                node->value *= convert(1, node->unit);
                node->unit   = css_units_px;
            }
            for(auto& arg : node->args)
            {
                if(arg)
                {
                    arg = convert_expression(arg, convert);
                }
            }
            return node;
        }

        float calculation_value(double value, bool non_negative, css_units unit)
        {
            // CSS Values 4 censors NaN to zero and infinities to the supported range.
            if(std::isnan(value))
            {
                value = 0;
            }
            // Pixel geometry is also converted to integers by rendering containers.
            const double limit =
                unit == css_units_none ? std::numeric_limits<float>::max() : css_length::max_math_pixels;
            value = std::max(non_negative ? 0.0 : -limit, std::min(value, limit));
            return static_cast<float>(value);
        }
    } // namespace

    bool css_length::has_percentage() const
    {
        if(m_fit_content_argument)
        {
            return m_fit_content_argument->has_percentage();
        }
        return !m_is_predefined && (m_units == css_units_percentage || (m_expression && m_expression->percentage));
    }

    bool css_length::has_nonzero_percentage() const
    {
        if(m_fit_content_argument)
        {
            return m_fit_content_argument->has_nonzero_percentage();
        }
        if(m_is_predefined)
        {
            return false;
        }
        if(m_expression)
        {
            return m_expression->has_nonzero_percentage();
        }
        return m_units == css_units_percentage && std::fpclassify(m_value) != FP_ZERO;
    }

    float css_length::val() const
    {
        if(m_is_predefined)
        {
            return 0;
        }
        return m_expression ? calculation_value(m_expression->evaluate(0), m_non_negative, m_units) : m_value;
    }

    pixel_t css_length::calc_percent(pixel_t width) const
    {
        if(m_is_predefined)
        {
            return 0_px;
        }
        if(m_expression)
        {
            double value = m_expression->evaluate(width.value());
            if(m_units == css_units_percentage)
            {
                value *= width.value();
            }
            return {calculation_value(value, m_non_negative, m_units)};
        }
        return m_units == css_units_percentage ? width * m_value / 100.0_px : pixel_t(m_value);
    }

    void css_length::resolve_units(const std::function<float(float, css_units)>& convert)
    {
        if(!m_expression)
        {
            return;
        }
        if(m_expression->needs_conversion)
        {
            m_expression = convert_expression(m_expression, convert);
        }
        if(!has_percentage())
        {
            const auto unit = m_units;
            // Keep number-valued calculations deferred until their consuming context
            // (notably inherited line-height) resolves them.
            if(unit != css_units_none)
            {
                set_value(val(), unit);
            }
        }
    }

    bool css_length::from_token(const css_token& token, int options, css_values keywords)
    {
        if(token.type == CV_FUNCTION)
        {
            if(!(options & f_length))
            {
                return false;
            }
            auto expr = math_parser::function(token, options);
            if(!expr)
            {
                return false;
            }
            css_units unit;
            if(expr->length_power == 1 && expr->percentage_power == 0 && (options & f_length))
            {
                unit = css_units_px;
            } else if(expr->length_power == 0 && expr->percentage_power == 1 && (options & f_percentage))
            {
                unit = css_units_percentage;
            } else if(expr->length_power == 0 && expr->percentage_power == 0 && (options & f_number))
            {
                unit = css_units_none;
            } else
            {
                return false;
            }
            m_expression    = std::move(expr);
            m_fit_content_argument.reset();
            m_is_predefined = false;
            m_units         = unit;
            m_value         = 0;
            m_non_negative  = (options & f_positive) != 0;
            return true;
        }
        if((options & f_positive) && is_one_of(token.type, NUMBER, DIMENSION, PERCENTAGE) && token.n.number < 0)
        {
            return false;
        }

        if(token.type == IDENT)
        {
            auto idx = keywords.value_index(lowcase(token.name()));
            if(!idx.has_value())
            {
                return false;
            }
            predef(idx.value());
            return true;
        }
        if(token.type == DIMENSION)
        {
            if(!(options & f_length))
            {
                return false;
            }

            auto idx = css_values(css_units_strings).value_index(lowcase(token.unit()));
            // note: 1none and 1\% are invalid
            if(!idx.has_value() || idx.value() == css_units_none || idx.value() == css_units_percentage)
            {
                return false;
            }

            set_value(token.n.number, static_cast<css_units>(idx.value()));
            return true;
        }
        if(token.type == PERCENTAGE)
        {
            if(!(options & f_percentage))
            {
                return false;
            }

            set_value(token.n.number, css_units_percentage);
            return true;
        }
        if(token.type == NUMBER)
        {
            // if token is a nonzero number and neither f_number nor f_integer is specified in the options
            if(!(options & (f_number | f_integer)) && pixel_t(token.n.number) != 0_px)
            {
                return false;
            }
            // if token is a zero number and neither of f_number, f_integer or f_length are specified in the options
            if(!(options & (f_number | f_integer | f_length)) && pixel_t(token.n.number) == 0_px)
            {
                return false;
            }
            if((options & f_integer) && token.n.number_type != css_number_integer)
            {
                return false;
            }

            set_value(token.n.number, css_units_none);
            return true;
        }
        return false;
    }

    std::string css_length::to_string() const
    {
        if(m_fit_content_argument)
        {
            return "fit-content(" + m_fit_content_argument->to_string() + ")";
        }
        if(m_is_predefined)
        {
            return "def(" + std::to_string(m_predef) + ")";
        }
        if(m_expression)
        {
            return m_expression->to_string();
        }
        std::string res  = std::to_string(m_value) + "{";
        res             += css_values(css_units_strings).value_by_index(m_units);
        res             += '}';
        return res;
    }

    css_length css_length::predef_value(int val)
    {
        css_length len;
        len.predef(val);
        return len;
    }

} // namespace litehtml
