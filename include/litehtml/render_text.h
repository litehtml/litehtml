#ifndef LITEHTML_RENDER_TEXT_H
#define LITEHTML_RENDER_TEXT_H

#include "render_inline.h"

namespace litehtml
{
    class render_text : public render_item_inline
    {
      protected:
        void calc_intrinsic_size() override;

      public:
        explicit render_text(std::shared_ptr<element> src_el) :
            render_item_inline(std::move(src_el))
        {
        }

        bool for_inline_boxes([[maybe_unused]] const std::function<bool(const position& box, bool first, bool last)>&
                                  process) const override
        {
            return false;
        }
    };
} // namespace litehtml

#endif // LITEHTML_RENDER_TEXT_H
