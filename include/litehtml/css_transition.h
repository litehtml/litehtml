#ifndef LH_CSS_TRANSITION_H
#define LH_CSS_TRANSITION_H

#include "style.h"
#include "types.h"

namespace litehtml
{
class css_transition
{
public:
    enum class state
    {
        invalid,
        ready,
        running,
        finished
    };

public:
    css_transition() = default;
    css_transition(style from, style to);

    void add_next_style(style next_style);

    const style& from() const { return m_from; }
    const style& to() const { return m_to; }

    state get_state() const { return m_state; }
    void validate();

    void start(time t);

    void apply_time(render_item& ri, int max_width, time t);
    void apply(css_properties& output);

private:
    style m_from;
    style m_intermediate;
    style m_to;
    state m_state = state::invalid;
    time m_start_time{};
};
} // namespace litehtml

#endif
