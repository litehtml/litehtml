#include "html.h"
#include "render_item.h"
#include "interpolation.h"

namespace litehtml
{
namespace interpolation
{
css_token lerp_token_values(const css_token& low, const css_token& high, const float coefficient)
{
    // TODO add convert to equals type if PERCENTAGE & NUMBER
    if (low.type != high.type)
    {
        return low;
    }

    css_token result = low;

    if (low.type == css_token_type::CV_FUNCTION)
    {
        result.value = lerp_token_vector_values(low.value, high.value, coefficient);
    }
    else if (low.type == css_token_type::NUMBER || low.type == css_token_type::PERCENTAGE ||
             low.type == css_token_type::DIMENSION)
    {
        result.n.number = std::lerp(low.n.number, high.n.number, coefficient);
    }

    return result;
}

std::vector<css_token> lerp_token_vector_values(const std::vector<css_token>& low,
                                                const std::vector<css_token>& high,
                                                const float coefficient)
{
    // TODO add convert for vectors different size
    if (low.size() != high.size())
    {
        return low;
    }

    auto result{low};

    for (size_t i = 0; i < high.size(); ++i)
    {
        result[i] = lerp_token_values(low.at(i), high.at(i), coefficient);
    }
    return result;
}

web_color lerp_color(const web_color &low, const web_color &high, float coefficient)
{
    web_color result;
    result.red = static_cast<byte>((1.0f - coefficient) * low.red + coefficient * high.red);
    result.green = static_cast<byte>((1.0f - coefficient) * low.green + coefficient * high.green);
    result.blue = static_cast<byte>((1.0f - coefficient) * low.blue + coefficient * high.blue);
    return result;
}

css_length lerp_length(const std::shared_ptr<document>& doc, int max_width,
                       const css_length &low, const css_length &high, float coefficient)
{
    if (low.units() == high.units())
    {
        const float val = std::lerp(low.val(), high.val(), coefficient);
        return val;
    }
    else
    {
        font_metrics fm;
        fm.x_height = fm.font_size = doc->container()->get_default_font_size();

        const int val = static_cast<int>(std::lerp(doc->to_pixels(low, fm, max_width),
                                                   doc->to_pixels(high, fm, max_width), coefficient));

        return css_length{static_cast<float>(val), css_units_px};
    }
}

css_transform lerp_transform(render_item& ri, const css_transform& low, const css_transform& high, float coefficient)
{
    css_transform result;
    const size max_size{ri.pos().width, ri.pos().height};
    transformation low_matrix = low.get_transformation(
        ri.src_el()->get_document(), ri.css().get_font_metrics(), max_size, 1.0f - coefficient);
    transformation high_matrix = high.get_transformation(
        ri.src_el()->get_document(), ri.css().get_font_metrics(), max_size, coefficient);
    transformation result_matrix = low_matrix * high_matrix;
    result.add_matrix(result_matrix);
    return result;
}

property_value lerp_property_values(render_item& ri, int max_width,
                                    const property_value& low, const property_value& high, const float coefficient)
{
    if (low.index() != high.index())
    {
        return low;
    }

    property_value result = low;

    if (result.is<int>())
    {
        result.get<int>() = static_cast<int>(std::lerp(low.get<int>(), high.get<int>(), coefficient));
    }
    else if (result.is<float>())
    {
        result.get<float>() = std::lerp(low.get<float>(), high.get<float>(), coefficient);
    }
    else if (result.is<css_token_vector>())
    {
        result.get<css_token_vector>() =
            lerp_token_vector_values(low.get<css_token_vector>(), high.get<css_token_vector>(), coefficient);
    }
    else if (result.is<css_length>())
    {
        auto& low_length = low.get<css_length>();
        auto& high_length = high.get<css_length>();
        result.get<css_length>() = lerp_length(ri.src_el()->get_document(), max_width, low_length, high_length, coefficient);
    }
    else if (result.is<length_vector>())
    {
        // TODO
    }
    else if (result.is<size_vector>())
    {
        // TODO
    }
    else if (result.is<web_color>())
    {
        result.get<web_color>() = lerp_color(low.get<web_color>(), high.get<web_color>(), coefficient);
    }
    else if (result.is<int_vector>())
    {
        // TODO
    }
    else if (result.is<css_transform>())
    {
        result.get<css_transform>() = lerp_transform(ri, low.get<css_transform>(), high.get<css_transform>(), coefficient);
    }

    return result;
}
}
} // namespace litehtml