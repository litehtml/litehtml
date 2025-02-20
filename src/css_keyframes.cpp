#include "html.h"

#include "css_keyframes.h"
#include "css_parser.h"
#include "css_properties.h"
#include "css_tokenizer.h"
#include "css_timing_function.h"
#include "interpolation.h"
#include "render_item.h"

namespace litehtml
{
css_keyframes::css_keyframes(const css_token_vector& prelude, const css_token_vector& block_value,
                             const string& baseurl, document_container* container)
{
    for (const auto& css_prelude_token : prelude)
    {
        if (css_prelude_token.type == css_token_type::IDENT)
        {
            m_name = css_prelude_token.name;
            break;
        }
    }

    if (m_name.empty())
    {
        return;
    }

    litehtml::vector<float> percentages{};

    for (const auto& kf_selector_block_value : block_value)
    {
        if (kf_selector_block_value.type == css_token_type::PERCENTAGE)
        {
            percentages.emplace_back(kf_selector_block_value.n.number);
            continue;
        }

        if (kf_selector_block_value.type == css_token_type::IDENT)
        {
            const auto ident = kf_selector_block_value.ident();
            const bool is_from = ident == "from";
            const bool is_to = ident == "to";

            if (is_from || is_to)
            {
                percentages.emplace_back(is_from ? 0.f : 100.f);
            }
            continue;
        }

        if (kf_selector_block_value.type != css_token_type::CURLY_BLOCK)
        {
            continue;
        }

        fill_selector(percentages, kf_selector_block_value.value, baseurl, container);

        percentages.clear();
    }
}

const litehtml::string& css_keyframes::name() const&
{
    return m_name;
}

void css_keyframes::fill_selector(const litehtml::vector<float>& percentages,
                                  const litehtml::vector<css_token>& block_value, const string& baseurl,
                                  document_container* container)
{
    auto not_existed_percentages = percentages;

    for (const float percentage : percentages)
    {
        if (auto* selector = find_selector_for(percentage))
        {
            selector->add_properties(block_value, baseurl, container);

            not_existed_percentages.erase(
                std::find(not_existed_percentages.begin(), not_existed_percentages.end(), percentage));
        }
    }

    if (not_existed_percentages.empty())
    {
        return;
    }

    m_selectors.emplace_back(not_existed_percentages).add_properties(block_value, baseurl, container);
}

bool css_keyframes::apply_frames_from(const css_keyframe_selector& selector,
                                      const out_of_range_predicate_type& out_of_range_predicate,
                                      css_keyframe_selector& applied_frames, float& low, float& high)
{
    const bool is_out_of_range =
        std::all_of(selector.m_percentages.begin(), selector.m_percentages.end(), out_of_range_predicate);

    if (is_out_of_range)
    {
        high = selector.m_percentages.front();
        return is_out_of_range;
    }

    low = selector.m_percentages.front();

    for (const auto& [id, property] : selector.properties())
    {
        applied_frames.m_properties[id] = property;
    }

    return is_out_of_range;
}

css_keyframes::keyframe_point css_keyframes::create_keyframe_point_for(const passed_percentage passed_percentage) const
{
    const float percentage = passed_percentage.percentage();

    css_keyframe_selector applied_frames{{percentage}};

    float low = 0.f;
    float high = percentage;

    if (passed_percentage.is_reverse())
    {
        auto reverse_out_of_range_predicate = [percentage](const float value) { return value < percentage; };

        for (auto selector_it = m_selectors.rbegin(); selector_it != m_selectors.rend(); ++selector_it)
        {
            auto& selector = *selector_it;

            const bool is_out_of_range =
                apply_frames_from(selector, reverse_out_of_range_predicate, applied_frames, low, high);

            if (is_out_of_range)
            {
                break;
            }
        }
    }
    else
    {
        auto out_of_range_predicate = [percentage](const float value) { return value > percentage; };

        for (auto& selector : selectors())
        {
            const bool is_out_of_range = apply_frames_from(selector, out_of_range_predicate, applied_frames, low, high);

            if (is_out_of_range)
            {
                break;
            }
        }
    }

    css_keyframe_selector next_frames{{high}};

    if (const auto* high_selector = find_selector_for(high))
    {
        next_frames.m_properties = high_selector->m_properties;
    }

    if (passed_percentage.is_reverse())
    {
        std::swap(low, high);
    }

    float interpolation_coefficient = keyframe_point::calculate_interpolation_coefficient(percentage, low, high);

    if (passed_percentage.is_reverse())
    {
        interpolation_coefficient = 1.f - interpolation_coefficient;
    }

    return keyframe_point{interpolation_coefficient, std::move(applied_frames), std::move(next_frames)};
}

css_keyframe_selector* css_keyframes::find_selector_for(const float percentage) const
{
    for (auto& selector : m_selectors)
    {
        for (const float percent : selector.m_percentages)
        {
            if (static_cast<int>(percentage) == static_cast<int>(percent))
            {
                return &selector;
            }
        }
    }
    return nullptr;
}

const css_keyframe_selector::vector& css_keyframes::selectors() const&
{
    return m_selectors;
}

css_keyframe_selector::css_keyframe_selector(const std::vector<float>& percentages) : m_percentages{percentages}
{
}

void css_keyframe_selector::add_properties(const std::vector<css_token>& block_value, const std::string& baseurl,
                                           document_container* container)
{
    raw_declaration::vector decls;
    raw_rule::vector rules;
    css_parser(block_value).consume_style_block_contents(decls, rules);

    if (!rules.empty() || decls.empty())
    {
        css_parse_error("rule inside a style block");
        return;
    }

    init(decls, baseurl, container);
}

inline bool css_keyframe_selector::empty() const noexcept
{
    return m_percentages.empty() || m_properties.empty();
}

void css_keyframe_selector::init(const std::vector<raw_declaration>& raw_declarations, const std::string& baseurl,
                                 document_container* container)
{
    for (auto& declaration : raw_declarations)
    {
        // Note: decl.value is already componentized, see consume_qualified_rule
        // and consume_style_block_contents.
        // Note: decl.value may be empty.
        string name = declaration.name.substr(0, 2) == "--" ? declaration.name : lowcase(declaration.name);

        add_declaration(_id(name), declaration.value, baseurl, container);
    }
}

void css_keyframe_selector::add_declaration(const string_id id, const std::vector<css_token>& decl,
                                            const std::string& baseurl, document_container* container)
{
    style property_creator;
    property_creator.add_property(id, decl, baseurl, false, container);
    const auto& property = property_creator.get_property(id);

    if (property.is<invalid>())
    {
        return;
    }

    m_properties[id] = property;
}

void css_keyframes::keyframe_point::init_property_by_default_value(property_value& prop, const string_id id)
{
    // TODO init other value by ID

    switch (id)
    {
    case _width_:
    case _height_:
        prop.emplace<css_length>(100.f, css_units_percentage);
        break;
    case _transform_:
        prop.emplace<css_transform>();
        break;
    default:
        prop.emplace<invalid>();
    }
}

css_keyframe_selector css_keyframes::keyframe_point::create_final_selector(render_item& ri, int max_width) const
{
    if (m_interpolation_coefficient <= 0.f || m_interpolation_coefficient >= 1.f)
    {
        return m_applied_selector;
    }

    css_keyframe_selector final_selector{{std::lerp(
        m_applied_selector.m_percentages.front(), m_next_selector.m_percentages.front(), m_interpolation_coefficient)}};

    final_selector.m_properties = m_applied_selector.m_properties;

    for (const auto& [id, next_property] : m_next_selector.properties())
    {
        auto& applied_prop = final_selector.m_properties[id];

        if (applied_prop.is<invalid>())
        {
            init_property_by_default_value(applied_prop, id);
        }

        if (applied_prop.is<invalid>() == false)
        {
            applied_prop = interpolation::lerp_property_values(ri,
              max_width, applied_prop, next_property, m_interpolation_coefficient);
        }
    }

    return final_selector;
}

int css_keyframes::keyframe_point::calc_render_item_height(const render_item& ri, const int max_width)
{
    litehtml::size sz;
    ri.src_el()->get_content_size(sz, max_width);
    return sz.height;
}

int css_keyframes::keyframe_point::calc_render_item_width(const render_item& ri, const int max_width)
{
    litehtml::size sz;
    ri.src_el()->get_content_size(sz, max_width);
    return sz.width;
}

int css_keyframes::keyframe_point::css_length_to_pixels(const std::shared_ptr<document>& doc, const render_item& ri,
                                                        const css_length& length, const int max_width)
{
    return doc->to_pixels(length, ri.src_el()->css().get_font_metrics(), max_width);
}

int css_keyframes::keyframe_point::css_length_to_pixels(const std::shared_ptr<document>& doc, const render_item& ri,
                                                        const css_token& length, const int max_width)
{
    css_length pos{};
    pos.from_token(length, f_length_percentage);
    return css_length_to_pixels(doc, ri, pos, max_width);
}

bool css_keyframes::is_active_animation(const css_properties &properties,
                                        time t, time animation_start_time)
{
    auto time_diff = t - animation_start_time;
    if (time_diff.count() < 0)
    {
        return false;
    }

    const auto iteration_count = properties.get_animation_iteration_count();
    const time animation_duration = properties.get_animation_duration() + properties.get_animation_delay();

    if (iteration_count != std::numeric_limits<float>::infinity() &&
        animation_duration.count() * iteration_count < time_diff.count())
    {
        return false;
    }

    return true;
}

litehtml::passed_percentage css_keyframes::calculate_passed_animation_percentage(const css_properties& properties,
                                                                                      time t, time animation_start_time)
{
    auto percentage_diff = t - animation_start_time;

    const bool is_running =
        percentage_diff.count() > 0 && properties.get_animation_play_state() == animation_play_state_running;

    const auto direction = properties.get_animation_direction();

    bool is_reverse = direction == animation_direction_reverse || direction == animation_direction_alternate_reverse;

    if (is_running == false)
    {
        return css_timing_function::apply_timing_function(passed_percentage{is_reverse}, properties.get_animation_timing_function());
    }

    const time animation_duration = properties.get_animation_duration() + properties.get_animation_delay();
    int current_iteration = static_cast<int>(percentage_diff / animation_duration);

    // if percentage_diff == animation_duration it is a zero iteration not first
    if (percentage_diff >= animation_duration && (percentage_diff % animation_duration).count() == 0)
    {
        --current_iteration;
    }

    const bool is_ping_pong_animation =
        direction == animation_direction_alternate || direction == animation_direction_alternate_reverse;

    if (is_ping_pong_animation && current_iteration > 0)
    {
        const bool is_current_iteration_even = current_iteration % 2 == 0;
        if (direction == animation_direction_alternate)
        {
            is_reverse = is_current_iteration_even == false;
        }
        else if (direction == animation_direction_alternate_reverse)
        {
            is_reverse = is_current_iteration_even == true;
        }
    }

    percentage_diff %= animation_duration;

    if (percentage_diff.count() == 0)
    {
        return css_timing_function::apply_timing_function(passed_percentage{is_reverse}, properties.get_animation_timing_function());
    }

    const float result = (percentage_diff * 100.f) / animation_duration;

    const float percentage = is_reverse ? 100.f - result : result;
    return css_timing_function::apply_timing_function(passed_percentage{is_reverse, percentage}, properties.get_animation_timing_function());
}

const property_value& css_keyframe_selector::get_property(const string_id id) const&
{
    if (const auto it = m_properties.find(id); it != m_properties.end())
    {
        return it->second;
    }

    static property_value empty_prop{};
    return empty_prop;
}

const std::map<string_id, property_value>& css_keyframe_selector::properties() const&
{
    return m_properties;
}

css_keyframes::keyframe_point::keyframe_point(const float interpolation_coefficient,
                                              css_keyframe_selector applied_selector,
                                              css_keyframe_selector next_selector)
    : m_interpolation_coefficient{interpolation_coefficient}, m_applied_selector{std::move(applied_selector)},
      m_next_selector{std::move(next_selector)}
{
}

void css_keyframes::keyframe_point::apply_to(render_item& ri, const int max_width) const
{
    const auto doc = ri.src_el()->get_document();

    if (doc == nullptr)
    {
        return;
    }

    const auto selector = create_final_selector(ri, max_width);

    for (const auto& [name, value] : selector.properties())
    {
        switch (name)
        {
        case _transform_:
            ri.src_el()->css_w().set_transform(value.get<css_transform>());
            break;
        case _left_: {
            ri.pos().x += css_length_to_pixels(doc, ri, value.get<css_length>(), max_width);
        }
        break;
        case _right_: {
            ri.pos().x -= css_length_to_pixels(doc, ri, value.get<css_length>(), max_width);
        }
        break;
        case _top_: {
            ri.pos().y += css_length_to_pixels(doc, ri, value.get<css_length>(), max_width);
        }
        break;
        case _bottom_: {
            ri.pos().y -= css_length_to_pixels(doc, ri, value.get<css_length>(), max_width);
        }
        break;
        case _width_: {
            ri.src_el()->css_w().set_width(value.get<css_length>());
        }
        break;
        case _height_: {
            ri.src_el()->css_w().set_height(value.get<css_length>());
        }
        break;
        default:
            break;
        }
    }
}

} // namespace litehtml
