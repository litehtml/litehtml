#ifndef LH_CSS_KEYFRAMES_H
#define LH_CSS_KEYFRAMES_H

#include "string_id.h"
#include "types.h"

namespace litehtml
{

struct css_token;
struct property_value;
struct raw_declaration;
struct passed_percentage;

class css_keyframes;
class render_item;
class css_properties;
class document_container;

class css_keyframe_selector
{
  public:
    friend css_keyframes;
    using vector = std::vector<css_keyframe_selector>;

    explicit css_keyframe_selector(const std::vector<float>& percentages);

    void add_properties(const std::vector<css_token>& block_value, const std::string& baseurl,
                        document_container* container);

    [[nodiscard]] inline bool empty() const noexcept;
    [[nodiscard]] const property_value& get_property(string_id id) const&;
    [[nodiscard]] const std::map<string_id, property_value>& properties() const&;

  private:
    void init(const std::vector<raw_declaration>& raw_declarations, const std::string& baseurl,
              document_container* container);

    void add_declaration(string_id id, const std::vector<css_token>& decl, const std::string& baseurl,
                         document_container* container);

  private:
    std::vector<float> m_percentages{};
    std::map<string_id, property_value> m_properties{};
};

class css_keyframes
{
  public:
    using ptr = std::shared_ptr<css_keyframes>;
    using vector = std::vector<css_keyframes::ptr>;
    using out_of_range_predicate_type = std::function<bool(float)>;

  public:
    explicit css_keyframes(const std::vector<css_token>& prelude, const std::vector<css_token>& block_value,
                           const std::string& baseurl, document_container* container);

    struct keyframe_point
    {
        explicit keyframe_point(float interpolation_coefficient, css_keyframe_selector applied_selector,
                                css_keyframe_selector next_selector);

        void apply_to(render_item& ri, int max_width) const;

        [[nodiscard]] static float calculate_interpolation_coefficient(const float percentage, const float low,
                                                                       const float high)
        {
            const float coefficient = (percentage - low) / (high - low);
            return std::isnan(coefficient) || std::isinf(coefficient) ? 0.f : coefficient;
        }

      private:
        static void init_property_by_default_value(property_value& prop, string_id id);

        [[nodiscard]] css_keyframe_selector create_final_selector(render_item& ri, int max_width) const;

        [[nodiscard]] static int css_length_to_pixels(const std::shared_ptr<document>& doc, const render_item& ri,
                                                      const css_token& length, int max_width);
        [[nodiscard]] static int css_length_to_pixels(const std::shared_ptr<document>& doc, const render_item& ri,
                                                      const css_length& length, int max_width);

        [[nodiscard]] static int calc_render_item_height(const render_item& ri, int max_width);
        [[nodiscard]] static int calc_render_item_width(const render_item& ri, int max_width);

      private:
        float m_interpolation_coefficient;
        css_keyframe_selector m_applied_selector;
        css_keyframe_selector m_next_selector;
    };

    [[nodiscard]] keyframe_point create_keyframe_point_for(passed_percentage passed_percentage) const;

    static bool apply_frames_from(const css_keyframe_selector& selector,
                                  const out_of_range_predicate_type& out_of_range_predicate,
                                  css_keyframe_selector& applied_frames, float& low, float& high);

    [[nodiscard]] const css_keyframe_selector::vector& selectors() const&;

    [[nodiscard]] const std::string& name() const&;

    [[nodiscard]] static bool
    is_active_animation(const css_properties &properties, time t,
                        time animation_start_time);

    [[nodiscard]] static passed_percentage calculate_passed_animation_percentage(const css_properties& properties,
                                                                                 time t, time animation_start_time);

  private:
    [[nodiscard]] css_keyframe_selector* find_selector_for(float percentage) const;

    void fill_selector(const std::vector<float>& percentages, const std::vector<css_token>& block_value,
                       const std::string& baseurl, document_container* container);

  private:
    std::string m_name{};
    mutable css_keyframe_selector::vector m_selectors{};
};

} // namespace litehtml

#endif // LH_CSS_KEYFRAMES_H
