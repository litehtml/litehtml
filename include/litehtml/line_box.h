#ifndef LH_LINE_BOX_H
#define LH_LINE_BOX_H

#include <vector>
#include <memory>
#include "os_types.h"
#include "types.h"

namespace litehtml
{
    class render_item;

    struct line_context
    {
        int calculatedTop;
        int top;
        int left;
        int right;

        int width() const
        {
            return right - left;
        }
        void fix_top()
        {
            calculatedTop = top;
        }
    };

    class line_box
    {
        int		                m_box_top;
        int		                m_box_left;
        int		                m_box_right;
        std::vector< std::shared_ptr<render_item> > m_items;
        int						m_height;
        int						m_width;
        int						m_line_height;
        font_metrics			m_font_metrics;
        int						m_baseline;
        text_align				m_text_align;
    public:
        line_box(int top, int left, int right, int line_height, font_metrics& fm, text_align align)
        {
            m_box_top	    = top;
            m_box_left	    = left;
            m_box_right	    = right;
            m_height		= 0;
            m_width			= 0;
            m_font_metrics	= fm;
            m_line_height	= line_height;
            m_baseline		= 0;
            m_text_align	= align;
        }

        int		bottom() const	{ return m_box_top + height();	}
        int		top() const		{ return m_box_top;				}
        int		right() const	{ return m_box_left + width();	}
        int		left() const	{ return m_box_left;			}

        int					height() const;
        int					width() const;
        void				add_element(const std::shared_ptr<render_item> &el);
        bool				can_hold(const std::shared_ptr<render_item> &el, white_space ws) const;
        void				finish(bool last_box = false);
        bool				is_empty() const;
        int					baseline() const;
        void				get_elements(std::vector< std::shared_ptr<render_item> >& els);
        int					top_margin() const;
        int					bottom_margin() const;
        void				y_shift(int shift);
        void				new_width(int left, int right, std::vector< std::shared_ptr<render_item> >& els);

    private:
        bool				have_last_space() const;
        bool				is_break_only() const;
    };
}

#endif //LH_LINE_BOX_H
