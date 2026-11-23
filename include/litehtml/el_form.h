#ifndef LITEHTML_EL_FORM_H
#define LITEHTML_EL_FORM_H

#include "document.h"
#include "html_tag.h"
#include <vector>

namespace litehtml
{
    class el_form : public html_tag
    {
      public:
        explicit el_form(const document::ptr& doc);

        // Collects the fields and reports the form to the container. submitter is the button that
        // triggered the submission; when it is nullptr the first enabled submit button is used as
        // the implicit submitter (Enter key behavior).
        void submit(const element::ptr& submitter = nullptr);
        // Restores initial values of all descendant controls.
        void reset();

        element::ptr find_control_by_id(string_id id);
        element::ptr find_default_submit_button();

        std::string dump_get_name() override;

      private:
        void collect_controls(const element::ptr& parent, std::vector<element::ptr>& controls) const;
    };
} // namespace litehtml

#endif // LITEHTML_EL_FORM_H
