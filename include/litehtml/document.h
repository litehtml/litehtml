#ifndef LITEHTML_DOCUMENT_H
#define LITEHTML_DOCUMENT_H

#include "stylesheet.h"
#include "encodings.h"
#include "font_description.h"
#include "master_css.h"
#include "controls.h"
#include "types.h"

#include <functional>
#include <optional>
#include <vector>

using GumboOutput = struct GumboInternalOutput;

namespace litehtml
{
    struct css_text
    {
        using vector = std::vector<css_text>;

        std::string text;
        std::string baseurl;
        std::string media;

        css_text() = default;

        css_text(const char* txt, const char* url, const char* media_str) :
            text(txt ? txt : ""),
            baseurl(url ? url : ""),
            media(media_str ? media_str : "")
        {
        }
    };

    class dumper
    {
      public:
        virtual ~dumper()                                                        = default;
        virtual void begin_node(const std::string& descr)                        = 0;
        virtual void end_node()                                                  = 0;
        virtual void begin_attrs_group(const std::string& descr)                 = 0;
        virtual void end_attrs_group()                                           = 0;
        virtual void add_attr(const std::string& name, const std::string& value) = 0;
    };

    class html_tag;
    class render_item;
    struct styles_update;

    class document : public std::enable_shared_from_this<document>
    {
      public:
        using ptr      = std::shared_ptr<document>;
        using weak_ptr = std::weak_ptr<document>;

      private:
        std::shared_ptr<element>                m_root;
        std::shared_ptr<render_item>            m_root_render;
        document_container*                     m_container;
        fonts_map                               m_fonts;
        css_text::vector                        m_css;
        litehtml::css                           m_styles;
        litehtml::web_color                     m_def_color;
        litehtml::css                           m_master_css;
        litehtml::css                           m_user_css;
        litehtml::size                          m_size;
        std::optional<pixel_t>                  m_render_width;
        position::vector                        m_fixed_boxes;
        std::shared_ptr<element>                m_over_element;
        std::shared_ptr<element>                m_active_element;
        std::weak_ptr<element>                  m_focused_element;
        media_query_list_list::vector           m_media_lists;
        media_features                          m_media;
        std::string                             m_lang;
        std::string                             m_culture;
        std::string                             m_text;
        document_mode                           m_mode              = no_quirks_mode;
        bool                                    m_finalized         = false;
        bool                                    m_focus_visible     = false;
        bool                                    m_redraw_requested  = false;
        uint64_t                                m_layout_generation = 0;

      public:
        document(document_container* objContainer);
        virtual ~document();

        document_container* container() const
        {
            return m_container;
        }
        document_mode mode() const
        {
            return m_mode;
        }
        uint64_t layout_generation() const
        {
            return m_layout_generation;
        }
        uint_ptr  get_font(const font_description& descr, font_metrics* fm);
        pixel_t   render(pixel_t max_width, render_type rt = render_all);
        void      draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip);
        web_color get_def_color() const
        {
            return m_def_color;
        }
        void                       cvt_units(css_length& val, const font_metrics& metrics, pixel_t size) const;
        pixel_t                    to_pixels(const css_length& val, const font_metrics& metrics, pixel_t size) const;
        pixel_t                    width() const;
        pixel_t                    height() const;
        pixel_t                    content_width() const;
        pixel_t                    content_height() const;
        void                       add_stylesheet(const char* str, const char* baseurl, const char* media);
        bool                       on_mouse_over(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
                                                 const std::function<void(const position&)>& redraw_box);
        std::vector<scroll_values> on_scroll(pixel_t dx, pixel_t dy, pixel_t x, pixel_t y, pixel_t client_x,
                                             pixel_t client_y) const;
        bool                       on_lbutton_down(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
                                                   const std::function<void(const position&)>& redraw_box);
        bool                       on_lbutton_up(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
                                                 const std::function<void(const position&)>& redraw_box);
        bool                       on_button_cancel(const std::function<void(const position&)>& redraw_box);
        bool                       on_mouse_leave(const std::function<void(const position&)>& redraw_box);
        // Move the focus to the next/previous focusable element in the sequential focus navigation order.
        // The returned position is the document relative position of the focused element, so the caller can
        // scroll it into view. The optional is empty when the document has no focusable elements.
        std::optional<position> focus_next(const std::function<void(const position&)>& redraw_box);
        std::optional<position> focus_prev(const std::function<void(const position&)>& redraw_box);
        // Focus el (nullptr removes the focus). focus_visible controls the :focus-visible pseudo class and
        // should be true when the focus was moved by the keyboard. Returns true when the focus state changed;
        // redraw_box is called for the areas that need to be repainted.
        bool set_focus(const std::shared_ptr<element>& el, bool focus_visible,
                       const std::function<void(const position&)>& redraw_box);
        // Applies a control state change to the focused element and repaints its box. Returns true
        // when there is a focused element that accepted the update.
        bool update_focused(const control_data& data, const std::function<void(const position&)>& redraw_box);
        // Submits the given form. submitter is the button that triggered the submission (may be
        // nullptr). Returns false when form is not a form element.
        bool submit_form(const std::shared_ptr<element>& form, const std::shared_ptr<element>& submitter = nullptr);
        // Submits the form that contains the focused element (Enter key).
        bool submit_focused_form(const std::function<void(const position&)>& redraw_box);
        // Requests a full redraw on the next control update (used when a value changes without a
        // style change, for example a form reset).
        void request_redraw()
        {
            m_redraw_requested = true;
        }
        // Reports and clears a pending full redraw. Returns true when redraw_box was called.
        bool                     flush_redraw(const std::function<void(const position&)>& redraw_box);
        std::shared_ptr<element> get_focused_element() const
        {
            return m_focused_element.lock();
        }
        std::shared_ptr<element>       create_element(const char* tag_name, const string_map& attributes);
        std::shared_ptr<element>       root();
        std::shared_ptr<render_item>   root_render();
        void                           get_fixed_boxes(position::vector& fixed_boxes);
        void                           add_fixed_box(const position& pos);
        void                           add_media_list(const media_query_list_list::ptr& list);
        bool                           media_changed();
        bool                           lang_changed();
        bool                           match_lang(const std::string& lang);
        std::shared_ptr<const element> get_over_element() const
        {
            return m_over_element;
        }

        void append_children_from_string(element& parent, const char* str, bool replace_existing);
        void dump(dumper& cout);

        // see doc/document_createFromString.txt
        static document::ptr createFromString(const estring& str, document_container* container,
                                              const std::string& master_styles = litehtml::master_css,
                                              const std::string& user_styles   = {});

        // The mode must be set before any element is created, because it is used in html_tag::set_attr.
        void set_document_mode(document_mode mode);
        // Finish a document whose element tree was built by the caller with create_element() and
        // element::appendChild() instead of being parsed from an HTML string. Runs the same
        // finalization sequence createFromString() runs after parsing. A document is finalized
        // once, when its tree is complete; any further call does nothing.
        void finalize_from_external_root(const std::shared_ptr<element>& root,
                                         const std::string&              master_styles = litehtml::master_css,
                                         const std::string&              user_styles   = {});

      private:
        uint_ptr add_font(const font_description& descr, font_metrics* fm);

        GumboOutput* parse_html(estring str);
        void         create_node(void* gnode, elements_list& elements, bool parseTextNode, bool process_root);
        bool         update_media_lists(const media_features& features);
        bool         update_styles(const std::function<void(const position&)>& redraw_box);
        bool         change_focus(const std::shared_ptr<element>& el, bool focus_visible);
        bool         is_focusable(const std::shared_ptr<element>& el) const;
        void         collect_tabbable(const std::shared_ptr<element>& el, bool inert,
                                      std::vector<std::shared_ptr<element>>& result) const;
        std::vector<std::shared_ptr<element>> tabbable_elements() const;
        position                              element_document_position(const std::shared_ptr<element>& el) const;
        std::optional<position> focus_step(bool forward, const std::function<void(const position&)>& redraw_box);
        // Lays out again only the subtrees whose content changed. Returns false when the change
        // cannot be contained in a subtree and the whole document has to be laid out again.
        bool relayout_after_style_change(styles_update& update, const std::function<void(const position&)>& redraw_box);
        std::shared_ptr<render_item> relayout_root_for(const std::shared_ptr<render_item>& item,
                                                       const styles_update&                update) const;
        void                         rebuild_render_tree();
        // Regenerates all ::before/::after content in document order, resolving the quote keywords of the
        // `content` property against the document-wide quote nesting level. Returns true when any content
        // was regenerated.
        bool                         resolve_generated_quotes();
        void                         resolve_generated_quotes_in(element* el, int& quote_depth, bool& regenerated);
        void                         fix_tables_layout();
        // One stage of CSS 2.1 17.2.1 for one box. Each returns true when the tree changed.
        bool drop_irrelevant_table_boxes(const std::shared_ptr<render_item>& box);
        bool fix_table_child_wrappers(const std::shared_ptr<render_item>& box);
        bool fix_table_cell_rows(const std::shared_ptr<render_item>& box);
        bool fix_table_parents(const std::shared_ptr<render_item>& box);
        bool walk_render_tree(const std::shared_ptr<render_item>&                                      box,
                              const std::function<bool(const std::shared_ptr<render_item>&)>& process);
        // Wrappers for the anonymous boxes of CSS 2.1 17.2.1.
        std::shared_ptr<render_item> create_anonymous_table_box(const std::shared_ptr<render_item>& parent,
                                                                style_display                       display);
        bool wrap_children_in_anonymous_box(const std::shared_ptr<render_item>&               parent,
                                            style_display                                     wrapper_display,
                                            const std::vector<std::shared_ptr<render_item>>& items);
        bool wrap_improper_children(const std::shared_ptr<render_item>&     box,
                                    const std::function<bool(style_display)>& is_content,
                                    style_display                             wrapper_display);
        bool wrap_siblings(const std::shared_ptr<render_item>&     el,
                           const std::function<bool(style_display)>& in_run,
                           style_display                             wrapper_display);
    };

    inline std::shared_ptr<element> document::root()
    {
        return m_root;
    }

    inline std::shared_ptr<render_item> document::root_render()
    {
        return m_root_render;
    }

    inline bool document::match_lang(const std::string& lang)
    {
        return lang == m_lang || lang == m_culture;
    }
} // namespace litehtml

#endif // LITEHTML_DOCUMENT_H
