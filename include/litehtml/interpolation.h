#ifndef LH_INTERPOLATION_H
#define LH_INTERPOLATION_H

#include "document.h"
#include "style.h"
#include <memory>

namespace litehtml
{
    namespace interpolation
    {
        [[nodiscard]] property_value lerp_property_values(render_item& ri, int max_width,
                                                          const property_value& low, const property_value& high,
                                                          float coefficient);
        [[nodiscard]] css_token lerp_token_values(const css_token& low, const css_token& high,
                                                  float coefficient);
        [[nodiscard]] std::vector<css_token> lerp_token_vector_values(const std::vector<css_token>& low,
                                                                      const std::vector<css_token>& high,
                                                                      float coefficient);
        [[nodiscard]] web_color lerp_color(const web_color& low, const web_color& high, float coefficient);
        [[nodiscard]] css_length lerp_length(const std::shared_ptr<document>& doc, int max_width,
                                             const css_length& low, const css_length& high, float coefficient);
        [[nodiscard]] css_transform lerp_transform(render_item& ri, const css_transform& low,
                                                   const css_transform& high, float coefficient);
    }
}

#endif // !LH_INTERPOLATION_H
