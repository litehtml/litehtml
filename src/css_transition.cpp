#include "html.h"
#include "css_timing_function.h"
#include "interpolation.h"
#include "render_item.h"
#include "css_transition.h"

namespace litehtml
{
css_transition::css_transition(style from, style to)
  : m_from(from)
  , m_to(to) {}

void css_transition::add_next_style(style next_style)
{
    m_state = state::invalid;
    if (m_from.empty())
    {
        m_from = next_style;
        m_intermediate = next_style;
        return;
    }
    m_from = m_intermediate;
    m_to = next_style;
    validate();
}

void css_transition::validate()
{
    if (m_from.empty() || m_to.empty())
        m_state = state::invalid;
    else if (m_state == state::invalid)
        m_state = state::ready;
}

void css_transition::start(time t)
{
    m_start_time = t;
    m_state = state::running;
}

void css_transition::apply_time(render_item& ri, int max_width, time t)
{
    if (m_state != state::running) return;
    const auto& css_data = ri.src_el()->css();
    const auto& properties = css_data.get_transition_properties();
    const auto& durations = css_data.get_transition_duration();
    const auto& delays = css_data.get_transition_delay();
    const auto& timing_functions = css_data.get_transition_timing_function();
    const auto time_diff = t - m_start_time;
    for (std::size_t property_id = 0; property_id < properties.size(); ++property_id)
    {
        const auto& property = properties[property_id];
        const auto duration = durations[property_id];
        const auto delay = delays[property_id];
        const auto timing_function_type = timing_functions[property_id];
        // const auto behavior = behaviors[property_id];
        const auto total_duration = duration + delay;
        if (time_diff >= total_duration)
        {
            // transition completed, no need for any recalculation
            // state = finished
            continue;
        }

        float k = 0;
        if (time_diff > delay)
        {
            // calculate transition intermediate state, color, position, size interpolation etc
            const auto percentage_value = 100 * static_cast<float>((time_diff - delay).count()) / duration.count();
            passed_percentage percentage(false, percentage_value);
            percentage = css_timing_function::apply_timing_function(percentage, timing_function_type);
            k = percentage.percentage() / 100;
        }
        const auto id = _id(property);
        const auto& from = m_from.get_property(id);
        const auto& to = m_to.get_property(id);
        if (from.is<invalid>() || to.is<invalid>()) continue;
        property_value intermediate_value = interpolation::lerp_property_values(ri, max_width, from, to, k);
        m_intermediate.add_property(id, intermediate_value);
    }
}

void css_transition::apply(css_properties &output)
{
    const auto& properties = output.get_transition_properties();
    for (const auto& property : properties)
    {
        const auto id = _id(property);
        const auto value = m_intermediate.get_property(id);
        if (value.is<invalid>()) continue;
        switch (id)
        {
            case _background_color_:
                {
                    if (!value.is<web_color>()) continue;
                    auto background = output.get_bg();
                    background.m_color = value.get<web_color>();
                    output.set_bg(background);
                }
                break;
            case _color_:
                {
                    if (!value.is<web_color>()) continue;
                    output.set_color(value.get<web_color>());
                }
                break;
            case _transform_:
                {
                    if (!value.is<css_transform>()) continue;
                    output.set_transform(value.get<css_transform>());
                }
                break;
            default:
              continue;
        }
    }
}

}