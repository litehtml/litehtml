#include "render_text.h"

void litehtml::render_text::calc_intrinsic_size()
{
    if(!src_el())
    {
        return;
    }
    litehtml::size sz;
    src_el()->get_content_size(sz, 0_px);
    m_intrinsic_min_size.width = m_intrinsic_max_size.width = sz.width;
    m_intrinsic_min_size.height = m_intrinsic_max_size.height = sz.height;
}
