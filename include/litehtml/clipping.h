#ifndef LITEHTML_CLIPPING_H
#define LITEHTML_CLIPPING_H

#include "borders.h"
#include "types.h"
#include <functional>

namespace litehtml
{

    // A non-owning, one-way linked list of clipping regions. Every node describes one clipped area in
    // document coordinates (the parameters document_container used to receive through set_clip).
    // Nodes are created on the stack while drawing and are only valid for the duration of the draw
    // call that passes them to the container; a parent node must outlive its children.
    class clipping
    {
        position        m_pos{};
        border_radiuses m_bdr_radius{};
        const clipping* m_parent = nullptr;

      public:
        using apply_func_t = std::function<void(const position&, const border_radiuses&)>;

        clipping() = default;
        explicit clipping(const position& pos, const border_radiuses& bdr_radius,
                          const clipping* parent = nullptr) noexcept :
            m_pos(pos),
            m_bdr_radius(bdr_radius),
            m_parent(parent)
        {
        }

        const position& pos() const noexcept
        {
            return m_pos;
        }
        const border_radiuses& radius() const noexcept
        {
            return m_bdr_radius;
        }
        const clipping* parent() const noexcept
        {
            return m_parent;
        }

        // Calls func for every clipping region, innermost first.
        void apply(const apply_func_t& func) const
        {
            for(auto current = this; current; current = current->m_parent)
            {
                func(current->m_pos, current->m_bdr_radius);
            }
        }

        // Null-safe variant of apply().
        static void apply(const clipping* clip, const apply_func_t& func)
        {
            if(clip)
            {
                clip->apply(func);
            }
        }
    };

} // namespace litehtml

#endif // LITEHTML_CLIPPING_H
