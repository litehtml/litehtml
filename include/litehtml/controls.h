#ifndef LITEHTML_CONTROLS_H
#define LITEHTML_CONTROLS_H

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace litehtml
{
    // Kind of a form control. The document_container uses it to decide how the control should be
    // presented and edited.
    enum class control_type
    {
        none,
        button,
        checkbox,
        radio,
        text,
        number,
        password,
        search,
        submit,
        reset,
        image,
        textarea,
        select
    };

    // Text state of text-like controls. Offsets are byte offsets into text and are always kept on
    // UTF-8 codepoint boundaries.
    struct text_control_data
    {
        std::string text;
        size_t      caret           = 0;
        size_t      selection_start = 0;
        size_t      selection_end   = 0;
        bool        caret_visible   = true;
    };

    struct select_option_data
    {
        std::string value;
        std::string label;
        // Document order index of the option. Containers use it to identify the chosen item.
        int  index    = -1;
        bool disabled = false;
        bool selected = false;
    };

    struct select_group_data
    {
        // Empty for options that are not inside an optgroup.
        std::string                     label;
        bool                            disabled = false;
        std::vector<select_option_data> options;
    };

    struct select_control_data
    {
        std::vector<select_group_data> groups;
        bool                           multiple = false;
        int                            size     = 1;
    };

    // Payload passed between the document, its container and the focused control:
    // - bool: checked state of a checkbox or radio
    // - text_control_data: value/caret/selection of a text-like control
    // - select_control_data: options and selection of a select
    using control_data = std::variant<std::monostate, bool, text_control_data, select_control_data>;

    struct form_field
    {
        std::string name;
        std::string value;
    };

    // Data of a submitted form. The container resolves action/method and performs the submission.
    class form_data
    {
      public:
        std::string             id;
        std::string             action;
        std::string             method = "get";
        std::string             target;
        std::string             enctype = "application/x-www-form-urlencoded";
        std::vector<form_field> fields;
    };
} // namespace litehtml

#endif // LITEHTML_CONTROLS_H
