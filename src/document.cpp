#include "html.h"
#include "document.h"
#include "document_container.h"
#include "el_anchor.h"
#include "el_base.h"
#include "el_before_after.h"
#include "el_body.h"
#include "el_break.h"
#include "el_button.h"
#include "el_form.h"
#include "el_cdata.h"
#include "el_comment.h"
#include "el_div.h"
#include "element.h"
#include "el_font.h"
#include "el_image.h"
#include "el_input.h"
#include "el_label.h"
#include "el_link.h"
#include "el_optgroup.h"
#include "el_option.h"
#include "el_para.h"
#include "el_script.h"
#include "el_select.h"
#include "el_space.h"
#include "el_style.h"
#include "el_table.h"
#include "el_td.h"
#include "el_text.h"
#include "el_textarea.h"
#include "el_title.h"
#include "el_tr.h"
#include "gumbo.h"
#include "html_tag.h"
#include "render_block.h"
#include "render_item.h"
#include "render_table.h"
#include "stylesheet.h"
#include "types.h"
#include <algorithm>
#include <map>
#include <vector>

namespace litehtml
{

    namespace
    {
        // Focus navigation order: positive tabindex values first in ascending order, then the elements
        // with tabindex 0 or without an explicit tabindex in document order.
        int focus_order_index(const element::ptr& el)
        {
            const auto tab_index = el->tab_index();
            return tab_index && *tab_index > 0 ? *tab_index : 0;
        }
    } // namespace

    document::document(document_container* container)
    {
        m_container = container;
    }

    document::~document()
    {
        m_over_element = m_active_element = nullptr;
        if(m_container)
        {
            for(auto& font : m_fonts)
            {
                m_container->delete_font(font.second.font);
            }
        }
    }

    document::ptr document::createFromString(const estring& str, document_container* container,
                                             const std::string& master_styles, const std::string& user_styles)
    {
        // Create litehtml::document
        document::ptr doc = std::make_shared<document>(container);

        // Parse document into GumboOutput
        GumboOutput* output = doc->parse_html(str);

        // mode must be set before doc->create_node because it is used in html_tag::set_attr
        switch(output->document->v.document.doc_type_quirks_mode)
        {
        case GUMBO_DOCTYPE_NO_QUIRKS:
            doc->m_mode = no_quirks_mode;
            break;
        case GUMBO_DOCTYPE_QUIRKS:
            doc->m_mode = quirks_mode;
            break;
        case GUMBO_DOCTYPE_LIMITED_QUIRKS:
            doc->m_mode = limited_quirks_mode;
            break;
        }

        // Create litehtml::elements.
        elements_list root_elements;
        doc->create_node(output->root, root_elements, true, true);
        element::ptr root;
        if(!root_elements.empty())
        {
            root = root_elements.back();
        }

        // Destroy GumboOutput
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        doc->finalize_from_external_root(root, master_styles, user_styles);

        return doc;
    }

    void document::set_document_mode(document_mode mode)
    {
        m_mode = mode;
    }

    void document::finalize_from_external_root(const std::shared_ptr<element>& root, const std::string& master_styles,
                                               const std::string& user_styles)
    {
        // Finalization accumulates state that is never rolled back: the parsed stylesheets, the media
        // lists, the tabular elements, the render tree, and the used selectors of every element. A
        // second run would duplicate all of it, so a document is finalized once, exactly like a parsed
        // one. Build a new document to rebuild.
        if(m_finalized)
        {
            return;
        }
        m_finalized = true;

        m_root = root;

        if(master_styles != "")
        {
            m_master_css.parse_css_stylesheet(master_styles, "", shared_from_this());
            m_master_css.sort_selectors();
        }
        if(m_mode == quirks_mode)
        {
            m_master_css.parse_css_stylesheet(std::string(quirks_css), "", shared_from_this());
            m_master_css.sort_selectors();
        }
        if(user_styles != "")
        {
            m_user_css.parse_css_stylesheet(user_styles, "", shared_from_this());
            m_user_css.sort_selectors();
        }

        // Let's process created elements tree
        if(m_root)
        {
            container()->get_media_features(m_media);

            m_root->set_pseudo_class(_root_, true);

            // apply master CSS
            m_root->apply_stylesheet(m_master_css);

            // parse elements attributes
            m_root->parse_attributes();

            // parse style sheets linked in document
            for(const auto& css : m_css)
            {
                media_query_list_list::ptr media;
                if(css.media != "")
                {
                    auto mq_list = parse_media_query_list(css.media, shared_from_this());
                    media        = std::make_shared<media_query_list_list>();
                    media->add(mq_list);
                }
                m_styles.parse_css_stylesheet(css.text, css.baseurl, shared_from_this(), media);
            }
            // Sort css selectors using CSS rules.
            m_styles.sort_selectors();

            // Apply media features.
            update_media_lists(m_media);

            // Apply parsed styles.
            m_root->apply_stylesheet(m_styles);

            // Apply user styles if any
            m_root->apply_stylesheet(m_user_css);

            // Initialize element::m_css
            m_root->compute_styles();

            resolve_generated_quotes();

            rebuild_render_tree();
        }
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#change-the-encoding
    encoding adjust_meta_encoding(encoding meta_encoding, encoding current_encoding)
    {
        // 1.
        if(current_encoding == encoding::utf_16le || current_encoding == encoding::utf_16be)
        {
            return current_encoding;
        }

        // 2.
        if(meta_encoding == encoding::utf_16le || meta_encoding == encoding::utf_16be)
        {
            return encoding::utf_8;
        }

        // 3.
        if(meta_encoding == encoding::x_user_defined)
        {
            return encoding::windows_1252;
        }

        // 4,5,6.
        return meta_encoding;
    }

    // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead:change-the-encoding
    encoding get_meta_encoding(GumboNode* root)
    {
        // find <head>
        GumboNode* head = nullptr;
        for(size_t i = 0; i < root->v.element.children.length; i++)
        {
            auto* node = static_cast<GumboNode*>(root->v.element.children.data[i]);
            if(node->type == GUMBO_NODE_ELEMENT && node->v.element.tag == GUMBO_TAG_HEAD)
            {
                head = node;
                break;
            }
        }
        if(!head)
        {
            return encoding::null;
        }

        // go through <meta> tags in <head>
        for(size_t i = 0; i < head->v.element.children.length; i++)
        {
            auto* node = static_cast<GumboNode*>(head->v.element.children.data[i]);
            if(node->type != GUMBO_NODE_ELEMENT || node->v.element.tag != GUMBO_TAG_META)
            {
                continue;
            }

            auto* charset    = gumbo_get_attribute(&node->v.element.attributes, "charset");
            auto* http_equiv = gumbo_get_attribute(&node->v.element.attributes, "http-equiv");
            auto* content    = gumbo_get_attribute(&node->v.element.attributes, "content");
            // 1. If the element has a charset attribute...
            if(charset)
            {
                auto encoding = get_encoding(charset->value);
                if(encoding != encoding::null)
                {
                    return encoding;
                }
            }
            // 2. Otherwise, if the element has an http-equiv attribute...
            else if(http_equiv && t_strcasecmp(http_equiv->value, "content-type") == 0 && content)
            {
                auto encoding = extract_encoding_from_meta_element(content->value);
                if(encoding != encoding::null)
                {
                    return encoding;
                }
            }
        }

        return encoding::null;
    }

    // substitute for gumbo_parse that handles encodings
    GumboOutput* document::parse_html(estring str)
    {
        // https://html.spec.whatwg.org/multipage/parsing.html#the-input-byte-stream
        encoding_sniffing_algorithm(str);
        // cannot store output in local variable because gumbo keeps pointers into it,
        // which will be accessed later in gumbo_tag_from_original_text
        if(str.encoding == encoding::utf_8)
        {
            m_text = str;
        } else
        {
            decode(str, str.encoding, m_text);
        }

        // Gumbo does not support callbacks on node creation, so we cannot change encoding while parsing.
        // Instead, we parse entire file and then handle <meta> tags.

        // Using gumbo_parse_with_options to pass string length (m_text may contain NUL chars).
        GumboOutput* output = gumbo_parse_with_options(&kGumboDefaultOptions, m_text.data(), m_text.size());

        if(str.confidence == confidence::certain)
        {
            return output;
        }

        // Otherwise: confidence is tentative.

        // If valid HTML encoding is specified in <meta> tag...
        encoding meta_encoding = get_meta_encoding(output->root);
        if(meta_encoding != encoding::null)
        {
            // ...and it is different from currently used encoding...
            encoding new_encoding = adjust_meta_encoding(meta_encoding, str.encoding);
            if(new_encoding != str.encoding)
            {
                // ...reparse with the new encoding.
                gumbo_destroy_output(&kGumboDefaultOptions, output);
                m_text.clear();

                if(new_encoding == encoding::utf_8)
                {
                    m_text = str;
                } else
                {
                    decode(str, new_encoding, m_text);
                }
                output = gumbo_parse_with_options(&kGumboDefaultOptions, m_text.data(), m_text.size());
            }
        }

        return output;
    }

    void document::create_node(void* gnode, elements_list& elements, bool parseTextNode, bool process_root)
    {
        auto* node = static_cast<GumboNode*>(gnode);
        switch(node->type)
        {
        case GUMBO_NODE_ELEMENT:
            {
                if(process_root)
                {
                    string_map      attrs;
                    GumboAttribute* attr = nullptr;
                    for(unsigned int i = 0; i < node->v.element.attributes.length; i++)
                    {
                        attr              = static_cast<GumboAttribute*>(node->v.element.attributes.data[i]);
                        attrs[attr->name] = attr->value;
                    }

                    element::ptr ret;
                    const char*  tag = gumbo_normalized_tagname(node->v.element.tag);
                    if(tag[0])
                    {
                        ret = create_element(tag, attrs);
                    } else
                    {
                        if(node->v.element.original_tag.data && node->v.element.original_tag.length)
                        {
                            std::string str;
                            gumbo_tag_from_original_text(&node->v.element.original_tag);
                            str.append(node->v.element.original_tag.data, node->v.element.original_tag.length);
                            ret = create_element(str.c_str(), attrs);
                        }
                    }
                    if(!strcmp(tag, "script"))
                    {
                        parseTextNode = false;
                    }
                    if(ret)
                    {
                        elements_list child;
                        for(unsigned int i = 0; i < node->v.element.children.length; i++)
                        {
                            child.clear();
                            create_node(static_cast<GumboNode*>(node->v.element.children.data[i]), child, parseTextNode,
                                        true);
                            std::for_each(child.begin(), child.end(),
                                          [&ret](element::ptr& el) { ret->appendChild(el); });
                        }
                        elements.push_back(ret);
                    }
                } else
                {
                    for(unsigned int i = 0; i < node->v.element.children.length; i++)
                    {
                        create_node(static_cast<GumboNode*>(node->v.element.children.data[i]), elements, parseTextNode,
                                    true);
                    }
                }
            }
            break;
        case GUMBO_NODE_TEXT:
            {
                if(!parseTextNode)
                {
                    elements.push_back(std::make_shared<el_text>(node->v.text.text, shared_from_this()));
                } else
                {
                    m_container->split_text(
                        node->v.text.text,
                        [this, &elements](const char* text) {
                            elements.push_back(std::make_shared<el_text>(text, shared_from_this()));
                        },
                        [this, &elements](const char* text) {
                            elements.push_back(std::make_shared<el_space>(text, shared_from_this()));
                        });
                }
            }
            break;
        case GUMBO_NODE_CDATA:
            {
                element::ptr ret = std::make_shared<el_cdata>(shared_from_this());
                ret->set_data(node->v.text.text);
                elements.push_back(ret);
            }
            break;
        case GUMBO_NODE_COMMENT:
            {
                element::ptr ret = std::make_shared<el_comment>(shared_from_this());
                ret->set_data(node->v.text.text);
                elements.push_back(ret);
            }
            break;
        case GUMBO_NODE_WHITESPACE:
            {
                std::string str = node->v.text.text;
                for(size_t i = 0; i < str.length(); i++)
                {
                    elements.push_back(std::make_shared<el_space>(str.substr(i, 1).c_str(), shared_from_this()));
                }
            }
            break;
        default:
            break;
        }
    }

    element::ptr document::create_element(const char* tag_name, const string_map& attributes)
    {
        element::ptr  newTag;
        document::ptr this_doc = shared_from_this();
        if(m_container)
        {
            newTag = m_container->create_element(tag_name, attributes, this_doc);
        }
        if(!newTag)
        {
            if(!strcmp(tag_name, "br"))
            {
                newTag = std::make_shared<el_break>(this_doc);
            } else if(!strcmp(tag_name, "p"))
            {
                newTag = std::make_shared<el_para>(this_doc);
            } else if(!strcmp(tag_name, "img"))
            {
                newTag = std::make_shared<el_image>(this_doc);
            } else if(!strcmp(tag_name, "table"))
            {
                newTag = std::make_shared<el_table>(this_doc);
            } else if(!strcmp(tag_name, "td") || !strcmp(tag_name, "th"))
            {
                newTag = std::make_shared<el_td>(this_doc);
            } else if(!strcmp(tag_name, "link"))
            {
                newTag = std::make_shared<el_link>(this_doc);
            } else if(!strcmp(tag_name, "title"))
            {
                newTag = std::make_shared<el_title>(this_doc);
            } else if(!strcmp(tag_name, "a"))
            {
                newTag = std::make_shared<el_anchor>(this_doc);
            } else if(!strcmp(tag_name, "tr"))
            {
                newTag = std::make_shared<el_tr>(this_doc);
            } else if(!strcmp(tag_name, "style"))
            {
                newTag = std::make_shared<el_style>(this_doc);
            } else if(!strcmp(tag_name, "base"))
            {
                newTag = std::make_shared<el_base>(this_doc);
            } else if(!strcmp(tag_name, "body"))
            {
                newTag = std::make_shared<el_body>(this_doc);
            } else if(!strcmp(tag_name, "div"))
            {
                newTag = std::make_shared<el_div>(this_doc);
            } else if(!strcmp(tag_name, "script"))
            {
                newTag = std::make_shared<el_script>(this_doc);
            } else if(!strcmp(tag_name, "font"))
            {
                newTag = std::make_shared<el_font>(this_doc);
            } else if(!strcmp(tag_name, "form"))
            {
                newTag = std::make_shared<el_form>(this_doc);
            } else if(!strcmp(tag_name, "input"))
            {
                newTag = std::make_shared<el_input>(this_doc);
            } else if(!strcmp(tag_name, "textarea"))
            {
                newTag = std::make_shared<el_textarea>(this_doc);
            } else if(!strcmp(tag_name, "select"))
            {
                newTag = std::make_shared<el_select>(this_doc);
            } else if(!strcmp(tag_name, "option"))
            {
                newTag = std::make_shared<el_option>(this_doc);
            } else if(!strcmp(tag_name, "optgroup"))
            {
                newTag = std::make_shared<el_optgroup>(this_doc);
            } else if(!strcmp(tag_name, "label"))
            {
                newTag = std::make_shared<el_label>(this_doc);
            } else if(!strcmp(tag_name, "button"))
            {
                newTag = std::make_shared<el_button>(this_doc);
            } else
            {
                newTag = std::make_shared<html_tag>(this_doc);
            }
        }

        if(newTag)
        {
            newTag->set_tagName(tag_name);
            for(const auto& attribute : attributes)
            {
                newTag->set_attr(attribute.first.c_str(), attribute.second.c_str());
            }
        }

        return newTag;
    }

    uint_ptr document::add_font(const font_description& descr, font_metrics* fm)
    {
        uint_ptr ret = 0;

        std::string key = descr.hash();

        if(m_fonts.find(key) == m_fonts.end())
        {
            font_item fi = {0, {}};

            fi.font      = m_container->create_font(descr, this, &fi.metrics);
            m_fonts[key] = fi;
            ret          = fi.font;
            if(fm)
            {
                *fm = fi.metrics;
            }
        }
        return ret;
    }

    uint_ptr document::get_font(const font_description& descr, font_metrics* fm)
    {
        if(descr.size == 0_px)
        {
            return 0;
        }

        auto key = descr.hash();

        auto el = m_fonts.find(key);

        if(el != m_fonts.end())
        {
            if(fm)
            {
                *fm = el->second.metrics;
            }
            return el->second.font;
        }
        return add_font(descr, fm);
    }

    pixel_t document::render(pixel_t max_width, render_type rt)
    {
        if(rt != render_fixed_only)
        {
            m_render_width = max_width;
        }
        pixel_t ret = 0_px;
        if(m_root && m_root_render)
        {
            position viewport;
            m_container->get_viewport(viewport);
            containing_block_context cb_context;
            cb_context.width         = max_width;
            cb_context.width.type    = containing_block_context::cbc_value_type_absolute;
            cb_context.height        = viewport.height;
            cb_context.height.type   = containing_block_context::cbc_value_type_absolute;
            cb_context.quirks_height = cb_context.height;
            cb_context.render_width  = cb_context.width;

            m_root_render->update_intrinsic_size();
            ++m_layout_generation;

            if(rt == render_fixed_only)
            {
                m_fixed_boxes.clear();
                m_root_render->render_positioned(rt);
            } else
            {
                ret = m_root_render->render(0_px, 0_px, cb_context, nullptr);
                m_root_render->ensure_layout_tree();
                m_fixed_boxes.clear();
                m_root_render->render_positioned(rt);
                m_size.width  = 0;
                m_size.height = 0;
                m_root_render->calc_document_size(m_size);
            }
        } else if(rt != render_fixed_only)
        {
            m_size = {};
            m_fixed_boxes.clear();
        }
        return ret;
    }

    void document::draw(uint_ptr hdc, pixel_t x, pixel_t y, const position* clip)
    {
        if(m_root && m_root_render)
        {
            m_root->draw(hdc, x, y, clip, nullptr, m_root_render);
            m_root_render->draw_stacking_context(hdc, x, y, clip, true, nullptr, nullptr);
        }
    }

    pixel_t document::to_pixels(const css_length& val, const font_metrics& metrics, pixel_t size) const
    {
        if(val.is_predefined())
        {
            return 0_px;
        }
        if(val.is_expression())
        {
            auto resolved = val;
            resolved.resolve_units(
                [&](float value, css_units unit) { return to_pixels(css_length(value, unit), metrics, size).value(); });
            return resolved.calc_percent(size);
        }
        pixel_t ret;
        switch(val.units())
        {
        case css_units_percentage:
            ret = val.calc_percent(size);
            break;
        case css_units_em:
            ret = metrics.font_size * val.val();
            break;

        // https://drafts.csswg.org/css-values-4/#absolute-lengths
        case css_units_pt:
            ret = m_container->pt_to_px(val.val());
            break;
        case css_units_in:
            ret = m_container->pt_to_px(val.val() * 72); // 1in = 72pt
            break;
        case css_units_pc:
            ret = m_container->pt_to_px(val.val() * 12); // 1pc = (1/6)in = 12pt
            break;
        case css_units_cm:
            ret = m_container->pt_to_px(val.val() * 72 / 2.54f); // 1cm = (1/2.54)in = (72/2.54)pt
            break;
        case css_units_mm:
            ret = m_container->pt_to_px(val.val() * 72 / 2.54f / 10);
            break;

        case css_units_vw:
            ret = pixel_t(m_media.width.value() * val.val() / 100.0f);
            break;
        case css_units_vh:
            ret = pixel_t(m_media.height.value() * val.val() / 100.0f);
            break;
        case css_units_vmin:
            ret = pixel_t(std::min(m_media.height.value(), m_media.width.value()) * val.val() / 100.0f);
            break;
        case css_units_vmax:
            ret = pixel_t(std::max(m_media.height.value(), m_media.width.value()) * val.val() / 100.0f);
            break;
        case css_units_rem:
            ret = pixel_t(m_root->css().get_font_size().value() * val.val());
            break;
        case css_units_ex:
            ret = pixel_t(metrics.x_height.value() * val.val());
            break;
        case css_units_ch:
            ret = pixel_t(metrics.ch_width.value() * val.val());
            break;
        default:
            ret = pixel_t(val.val());
            break;
        }
        return ret;
    }

    void document::cvt_units(css_length& val, const font_metrics& metrics, pixel_t size) const
    {
        if(const auto* argument = val.fit_content_argument())
        {
            auto resolved = *argument;
            cvt_units(resolved, metrics, size);
            val.set_fit_content(resolved);
            return;
        }
        if(val.is_predefined())
        {
            return;
        }
        if(val.is_expression())
        {
            val.resolve_units(
                [&](float value, css_units unit) { return to_pixels(css_length(value, unit), metrics, size).value(); });
            return;
        }
        if(val.units() != css_units_percentage)
        {
            val.set_value(static_cast<float>(to_pixels(val, metrics, size)), css_units_px);
        }
    }

    pixel_t document::width() const
    {
        return m_size.width;
    }

    pixel_t document::height() const
    {
        return m_size.height;
    }

    void document::add_stylesheet(const char* str, const char* baseurl, const char* media)
    {
        if(str && str[0])
        {
            m_css.emplace_back(str, baseurl, media);
        }
    }

    bool document::on_mouse_over(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
                                 const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root || !m_root_render)
        {
            return false;
        }

        element::ptr over_el = m_root_render->get_element_by_point(x, y, client_x, client_y, x, y, nullptr);

        bool state_was_changed = false;

        if(over_el != m_over_element)
        {
            if(m_over_element)
            {
                if(m_over_element->on_mouse_leave())
                {
                    m_container->on_mouse_event(m_over_element, mouse_event_leave);
                    state_was_changed = true;
                }
            }
            m_over_element = over_el;
        }

        std::string cursor;

        if(m_over_element)
        {
            if(m_over_element->on_mouse_over())
            {
                state_was_changed = true;
            }
            cursor = m_over_element->css().get_cursor();
        }

        m_container->set_cursor(cursor.c_str());

        if(state_was_changed)
        {
            m_container->on_mouse_event(m_over_element, mouse_event_enter);
            return update_styles(redraw_box);
        }
        return false;
    }

    std::vector<scroll_values> document::on_scroll(pixel_t dx, pixel_t dy, pixel_t x, pixel_t y, pixel_t client_x,
                                                   pixel_t client_y) const
    {
        if(dy == 0_px && dx == 0_px)
        {
            return {};
        }

        element::ptr vscroll_el;
        element::ptr hscroll_el;

        if(dy != 0_px)
        {
            vscroll_el = m_root_render->get_element_by_point(
                x, y, client_x, client_y, x, y,
                [dy](const std::shared_ptr<render_item>& el) -> bool { return el->is_v_scrollable(dy); });
        }

        if(dx != 0_px)
        {
            hscroll_el = m_root_render->get_element_by_point(
                x, y, client_x, client_y, x, y,
                [dx](const std::shared_ptr<render_item>& el) -> bool { return el->is_h_scrollable(dx); });
        }

        if(!vscroll_el && !hscroll_el)
        {
            return {};
        }

        if(vscroll_el == hscroll_el)
        {
            scroll_values sv;
            sv.dx         = hscroll_el->h_scroll(dx);
            sv.dy         = vscroll_el->v_scroll(dy);
            sv.scroll_box = hscroll_el->get_placement();
            return {sv};
        }

        std::vector<scroll_values> ret;
        ret.reserve(2);

        if(vscroll_el)
        {
            scroll_values sv;
            sv.dy         = vscroll_el->v_scroll(dy);
            sv.scroll_box = vscroll_el->get_placement();
            ret.push_back(sv);
        }

        if(hscroll_el)
        {
            scroll_values sv;
            sv.dx         = hscroll_el->h_scroll(dx);
            sv.scroll_box = hscroll_el->get_placement();
            ret.push_back(sv);
        }

        return ret;
    }

    bool document::on_mouse_leave(const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root || !m_root_render)
        {
            return false;
        }
        if(m_over_element)
        {
            auto el        = m_over_element;
            m_over_element = nullptr;
            if(el->on_mouse_leave())
            {
                m_container->on_mouse_event(el, mouse_event_leave);
                return update_styles(redraw_box);
            }
        }
        return false;
    }

    bool document::on_lbutton_down(pixel_t x, pixel_t y, pixel_t client_x, pixel_t client_y,
                                   const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root || !m_root_render)
        {
            return false;
        }

        element::ptr over_el = m_root_render->get_element_by_point(x, y, client_x, client_y, x, y, nullptr);
        m_active_element     = over_el;

        bool state_was_changed = false;

        if(over_el != m_over_element)
        {
            if(m_over_element)
            {
                if(m_over_element->on_mouse_leave())
                {
                    m_container->on_mouse_event(m_over_element, mouse_event_leave);
                    state_was_changed = true;
                }
            }
            m_over_element = over_el;
            if(m_over_element)
            {
                if(m_over_element->on_mouse_over())
                {
                    state_was_changed = true;
                }
            }
        }

        std::string cursor;

        if(m_over_element)
        {
            if(m_over_element->on_lbutton_down_at(x, y))
            {
                state_was_changed = true;
            }
            cursor = m_over_element->css().get_cursor();
        }

        element::ptr focus_el;
        for(auto el = over_el; el; el = el->parent())
        {
            if(is_focusable(el))
            {
                focus_el = el;
                break;
            }
        }
        if(change_focus(focus_el, false))
        {
            state_was_changed = true;
        }

        m_container->set_cursor(cursor.c_str());

        if(state_was_changed)
        {
            m_container->on_mouse_event(m_over_element, mouse_event_enter);
            return update_styles(redraw_box);
        }

        return false;
    }

    bool document::on_lbutton_up(pixel_t /*x*/, pixel_t /*y*/, pixel_t /*client_x*/, pixel_t /*client_y*/,
                                 const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root || !m_root_render)
        {
            return false;
        }
        bool changed = false;
        if(m_over_element)
        {
            changed = m_over_element->on_lbutton_up(m_active_element == m_over_element);
        }
        changed = flush_redraw(redraw_box) || changed;
        if(changed)
        {
            return redraw_box ? update_styles(redraw_box) : true;
        }
        return false;
    }

    bool document::flush_redraw(const std::function<void(const position&)>& redraw_box)
    {
        if(!m_redraw_requested)
        {
            return false;
        }
        m_redraw_requested = false;
        if(redraw_box)
        {
            position viewport(0_px, 0_px, m_size.width, m_size.height);
            if(m_container)
            {
                m_container->get_viewport(viewport);
            }
            redraw_box(position(0_px, 0_px, std::max(m_size.width, viewport.right()),
                                std::max(m_size.height, viewport.bottom())));
        }
        return true;
    }

    bool document::on_button_cancel(const std::function<void(const position&)>& redraw_box)
    {
        m_active_element = nullptr;
        return on_mouse_leave(redraw_box);
    }

    bool document::change_focus(const element::ptr& el, bool focus_visible)
    {
        const auto current = m_focused_element.lock();

        // Text-entry controls always match :focus-visible while focused, even when the focus was moved
        // by the mouse or by a script. This matches Chrome's focus-visible heuristic.
        const bool visible = focus_visible || (el && el->accepts_keyboard_input());

        if(current == el)
        {
            if(!el || m_focus_visible == visible)
            {
                return false;
            }
            m_focus_visible = visible;
            return el->set_pseudo_class(_focus_visible_, visible);
        }

        bool changed = false;

        const auto set_focus_within = [&changed](const element::ptr& focused, bool add) {
            for(auto ancestor = focused; ancestor; ancestor = ancestor->parent())
            {
                if(ancestor->set_pseudo_class(_focus_within_, add))
                {
                    changed = true;
                }
            }
        };

        if(current)
        {
            if(current->set_pseudo_class(_focus_, false))
            {
                changed = true;
            }
            if(current->set_pseudo_class(_focus_visible_, false))
            {
                changed = true;
            }
            set_focus_within(current, false);
        }

        m_focused_element = el;
        m_focus_visible   = el && visible;

        if(el)
        {
            if(el->set_pseudo_class(_focus_, true))
            {
                changed = true;
            }
            if(el->set_pseudo_class(_focus_visible_, visible))
            {
                changed = true;
            }
            set_focus_within(el, true);
        }

        if(m_container)
        {
            m_container->on_focus_change(el, el ? el->get_attr("id") : nullptr,
                                         el ? el->get_control_type() : control_type::none,
                                         el ? el->get_control_data() : control_data{});
        }

        return changed;
    }

    bool document::update_focused(const control_data& data, const std::function<void(const position&)>& redraw_box)
    {
        const auto el = m_focused_element.lock();
        if(!el)
        {
            return false;
        }

        const bool styles_changed = el->update_focused(data);
        if(styles_changed && redraw_box)
        {
            update_styles(redraw_box);
        }
        if(redraw_box)
        {
            redraw_box(element_document_position(el));
            flush_redraw(redraw_box);
        }
        return true;
    }

    bool document::submit_form(const std::shared_ptr<element>& form, const std::shared_ptr<element>& submitter)
    {
        if(auto form_el = std::dynamic_pointer_cast<el_form>(form))
        {
            form_el->submit(submitter);
            return true;
        }
        return false;
    }

    bool document::submit_focused_form(const std::function<void(const position&)>& redraw_box)
    {
        auto el = m_focused_element.lock();
        if(!el)
        {
            return false;
        }
        while(el && el->tag() != _form_)
        {
            el = el->parent();
        }
        if(!el)
        {
            return false;
        }
        const bool ret = submit_form(el);
        if(ret && redraw_box)
        {
            redraw_box(element_document_position(el));
        }
        return ret;
    }

    bool document::set_focus(const element::ptr& el, bool focus_visible,
                             const std::function<void(const position&)>& redraw_box)
    {
        if(el && !is_focusable(el))
        {
            return false;
        }
        if(!change_focus(el, focus_visible))
        {
            return false;
        }
        if(redraw_box)
        {
            update_styles(redraw_box);
        }
        return true;
    }

    bool document::is_focusable(const element::ptr& el) const
    {
        if(!el->can_be_focused())
        {
            return false;
        }
        if(is_one_of(el->css().get_display(), display_none, display_contents))
        {
            return false;
        }
        return el->css().get_visibility() == visibility_visible;
    }

    void document::collect_tabbable(const element::ptr& el, bool inert, std::vector<element::ptr>& result) const
    {
        if(el->css().get_display() == display_none)
        {
            return;
        }

        const bool el_inert = inert || el->get_attr("inert") != nullptr;
        if(!el_inert && is_focusable(el))
        {
            const auto tab_index = el->tab_index();
            if(!tab_index || *tab_index >= 0)
            {
                result.push_back(el);
            }
        }

        for(const auto& child : el->children())
        {
            collect_tabbable(child, el_inert, result);
        }
    }

    std::vector<element::ptr> document::tabbable_elements() const
    {
        std::vector<element::ptr> elements;
        if(m_root)
        {
            collect_tabbable(m_root, false, elements);
            std::stable_sort(elements.begin(), elements.end(), [](const element::ptr& a, const element::ptr& b) {
                const int a_index = focus_order_index(a);
                const int b_index = focus_order_index(b);
                if(a_index == 0 || b_index == 0)
                {
                    // Positive tabindex values come first; zero/absent keep document order.
                    return a_index != 0;
                }
                return a_index < b_index;
            });
        }
        return elements;
    }

    std::optional<position> document::focus_next(const std::function<void(const position&)>& redraw_box)
    {
        return focus_step(true, redraw_box);
    }

    std::optional<position> document::focus_prev(const std::function<void(const position&)>& redraw_box)
    {
        return focus_step(false, redraw_box);
    }

    position document::element_document_position(const element::ptr& el) const
    {
        position pos;
        bool     is_first = true;

        const auto add_box = [&pos, &is_first](const position& box) {
            if(is_first)
            {
                pos      = box;
                is_first = false;
            } else
            {
                const pixel_t right  = std::max(pos.right(), box.right());
                const pixel_t bottom = std::max(pos.bottom(), box.bottom());
                pos.x                = std::min(pos.x, box.x);
                pos.y                = std::min(pos.y, box.y);
                pos.width            = right - pos.x;
                pos.height           = bottom - pos.y;
            }
        };

        for(const auto& weak_ri : el->m_renders)
        {
            if(const auto ri = weak_ri.lock())
            {
                // The focus/caret position is the element box, not the area painted by its outline.
                ri->get_rendering_boxes(add_box, false);
            }
        }

        if(is_first)
        {
            return el->get_placement();
        }
        return pos;
    }

    std::optional<position> document::focus_step(bool forward, const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root_render || !m_render_width)
        {
            return std::nullopt;
        }

        const auto elements = tabbable_elements();
        if(elements.empty())
        {
            return std::nullopt;
        }

        size_t     index   = forward ? 0 : elements.size() - 1;
        const auto current = m_focused_element.lock();
        const auto it      = std::find(elements.begin(), elements.end(), current);
        if(it != elements.end())
        {
            const auto current_index = static_cast<size_t>(std::distance(elements.begin(), it));
            if(forward)
            {
                index = (current_index + 1) % elements.size();
            } else
            {
                index = (current_index + elements.size() - 1) % elements.size();
            }
        }

        const auto& el = elements[index];
        set_focus(el, true, redraw_box);
        return element_document_position(el);
    }

    void document::get_fixed_boxes(position::vector& fixed_boxes)
    {
        fixed_boxes = m_fixed_boxes;
    }

    void document::add_fixed_box(const position& pos)
    {
        m_fixed_boxes.push_back(pos);
    }

    bool document::resolve_generated_quotes()
    {
        bool regenerated = false;
        int  quote_depth = 0;
        if(m_root)
        {
            resolve_generated_quotes_in(m_root.get(), quote_depth, regenerated);
        }
        return regenerated;
    }

    void document::resolve_generated_quotes_in(element* el, int& quote_depth, bool& regenerated)
    {
        if(el->css().get_display() == display_none)
        {
            // Content that is not rendered does not generate quote marks and does not change the nesting level.
            return;
        }
        if(el->tag() == __tag_before_ || el->tag() == __tag_after_)
        {
            if(auto* pseudo = dynamic_cast<el_before_after_base*>(el))
            {
                if(pseudo->resolve_quotes(quote_depth))
                {
                    // The regenerated text nodes need their computed styles before the render tree is built.
                    pseudo->compute_styles();
                    regenerated = true;
                }
            }
        }
        for(const auto& child : el->children())
        {
            resolve_generated_quotes_in(child.get(), quote_depth, regenerated);
        }
    }

    void document::rebuild_render_tree()
    {
        // Reuse scroll state for surviving boxes; layout will clamp it to their new sizes.
        std::map<element::ptr, std::shared_ptr<scroll_view>> scroll_views;
        const auto save_scroll_views = [&](const auto& self, const std::shared_ptr<render_item>& item) -> void {
            if(item->m_scroll_view)
            {
                scroll_views.emplace(item->src_el(), item->m_scroll_view);
            }
            for(const auto& child : item->children())
            {
                self(self, child);
            }
        };
        if(m_root_render)
        {
            save_scroll_views(save_scroll_views, m_root_render);
        }
        m_root_render.reset();
        if(!m_root)
        {
            return;
        }
        m_root->clear_renders();
        m_root_render = m_root->create_render_item(nullptr);
        fix_tables_layout();
        if(m_root_render)
        {
            m_root_render = m_root_render->init();
            if(!scroll_views.empty())
            {
                const auto restore_scroll_views = [&](const auto&                         self,
                                                      const std::shared_ptr<render_item>& item) -> void {
                    auto saved = scroll_views.find(item->src_el());
                    if(saved != scroll_views.end() &&
                       is_one_of(item->css().get_overflow(), overflow_scroll, overflow_auto))
                    {
                        item->m_scroll_view = saved->second;
                    }
                    for(const auto& child : item->children())
                    {
                        self(self, child);
                    }
                };
                restore_scroll_views(restore_scroll_views, m_root_render);
            }
        }
    }

    bool document::update_styles(const std::function<void(const position&)>& redraw_box)
    {
        styles_update update;
        const bool    changed = m_root->find_styles_changes(redraw_box, update);
        if(changed && resolve_generated_quotes())
        {
            // The quote text of the generated content changed, so the render tree has to be rebuilt.
            update.render_tree_changed = true;
        }

        const auto redraw_everything = [&]() {
            const auto old_size = m_size;
            render(*m_render_width);
            position viewport;
            container()->get_viewport(viewport);
            redraw_box(position(0_px, 0_px, std::max({old_size.width, m_size.width, viewport.right()}),
                                std::max({old_size.height, m_size.height, viewport.bottom()})));
        };

        if(update.render_tree_changed)
        {
            rebuild_render_tree();
            if(m_render_width)
            {
                redraw_everything();
            }
        } else if(update.layout_changed && !relayout_after_style_change(update, redraw_box) && m_render_width)
        {
            // The change could not be kept inside a subtree: lay out the whole document again.
            redraw_everything();
        }
        return changed;
    }

    std::shared_ptr<render_item> document::relayout_root_for(const std::shared_ptr<render_item>& item,
                                                             const styles_update&                update) const
    {
        const auto is_changed = [&update](const std::shared_ptr<render_item>& candidate) {
            const auto el = candidate->src_el();
            return std::find(update.changed_elements.begin(), update.changed_elements.end(), el) !=
                   update.changed_elements.end();
        };
        // Out-of-flow boxes are laid out by render_positioned() with a containing block that is not
        // the one stored on the item, so they can only be laid out again by a full render.
        for(auto el = item; el; el = el->parent())
        {
            if(is_one_of(el->src_el()->css().get_position(), element_position_absolute, element_position_fixed))
            {
                return nullptr;
            }
        }
        // The changed element's own border box may change, so its parent is the first candidate.
        for(auto candidate = item ? item->parent() : nullptr; candidate; candidate = candidate->parent())
        {
            if(is_changed(candidate))
            {
                continue;
            }
            if(candidate->is_flex_item() || candidate->is_grid_item())
            {
                // The surrounding flex/grid layout may depend on this item's intrinsic size.
                continue;
            }
            if(is_one_of(candidate->src_el()->css().get_display(), display_table_row_group, display_table_header_group,
                         display_table_footer_group, display_table_row, display_table_cell, display_table_column,
                         display_table_column_group, display_table_caption))
            {
                // Table boxes are laid out by the table algorithm, not on their own.
                continue;
            }
            if(!candidate->has_definite_size())
            {
                continue;
            }
            if(!candidate->src_el()->is_block_formatting_context() && candidate->has_outer_floats())
            {
                // A fresh formatting context would not flow the content around the outer floats.
                continue;
            }
            return candidate;
        }
        return m_root_render;
    }

    bool document::relayout_after_style_change(styles_update&                              update,
                                               const std::function<void(const position&)>& redraw_box)
    {
        if(!m_root_render || !m_render_width)
        {
            return false;
        }

        std::vector<std::shared_ptr<render_item>> roots;
        for(const auto& item : update.changed_renders)
        {
            if(!item)
            {
                continue;
            }
            auto root = relayout_root_for(item, update);
            if(!root)
            {
                return false;
            }
            if(std::find(roots.begin(), roots.end(), root) == roots.end())
            {
                roots.push_back(root);
            }
        }
        if(roots.empty())
        {
            // The changed boxes are not part of the render tree (e.g. display: contents).
            return false;
        }

        // Drop roots that are covered by another root.
        std::vector<std::shared_ptr<render_item>> top_roots;
        for(const auto& root : roots)
        {
            bool covered = false;
            for(auto parent = root->parent(); parent; parent = parent->parent())
            {
                if(std::find(roots.begin(), roots.end(), parent) != roots.end())
                {
                    covered = true;
                    break;
                }
            }
            if(!covered)
            {
                top_roots.push_back(root);
            }
        }
        roots = std::move(top_roots);

        const auto outline_box = [](const std::shared_ptr<render_item>& item) {
            position box  = item->pos();
            box          += item->get_paddings();
            box          += item->get_borders();
            box          += item->get_margins();
            return box;
        };

        ++m_layout_generation;
        for(const auto& root : roots)
        {
            if(root == m_root_render)
            {
                return false;
            }
            const position before = outline_box(root);
            root->update_intrinsic_size();
            const auto origin = root->render_origin();
            root->render(origin.x, origin.y, root->cb_context(), nullptr);
            if(root->src_el()->css().get_position() == element_position_relative)
            {
                // The parent applied the relative shift after the original layout.
                root->apply_relative_shift(root->cb_context());
            }
            const position after = outline_box(root);
            if(after.x != before.x || after.y != before.y || after.width != before.width ||
               after.height != before.height)
            {
                // The change was not contained after all; the caller lays out the document again.
                return false;
            }
        }

        const size old_size = m_size;

        // Positioned descendants and fixed boxes may depend on the new layout of the subtrees.
        m_fixed_boxes.clear();
        m_root_render->render_positioned(render_all);
        m_size = {};
        m_root_render->calc_document_size(m_size);

        // Report the new areas; the old ones were reported before the styles were recomputed.
        for(const auto& root : roots)
        {
            root->get_rendering_boxes(redraw_box);
        }
        for(const auto& item : update.changed_renders)
        {
            if(item)
            {
                item->get_rendering_boxes(redraw_box);
            }
        }
        if(old_size.width != m_size.width || old_size.height != m_size.height)
        {
            redraw_box(position(0_px, 0_px, old_size.width, old_size.height));
            redraw_box(position(0_px, 0_px, m_size.width, m_size.height));
        }
        return true;
    }

    bool document::media_changed()
    {
        container()->get_media_features(m_media);
        if(update_media_lists(m_media))
        {
            m_root->refresh_styles();
            m_root->compute_styles();
            resolve_generated_quotes();
            rebuild_render_tree();
            return true;
        }
        return false;
    }

    bool document::lang_changed()
    {
        if(m_root)
        {
            std::string culture;
            container()->get_language(m_lang, culture);
            if(!culture.empty())
            {
                m_culture = m_lang + '-' + culture;
            } else
            {
                m_culture.clear();
            }
            m_root->refresh_styles();
            m_root->compute_styles();
            resolve_generated_quotes();
            rebuild_render_tree();
            return true;
        }
        return false;
    }

    // Apply media features (determine which selectors are active).
    bool document::update_media_lists(const media_features& features)
    {
        bool update_styles = false;
        for(auto& media_list : m_media_lists)
        {
            if(media_list->apply_media_features(features))
            {
                update_styles = true;
            }
        }
        return update_styles;
    }

    void document::add_media_list(const media_query_list_list::ptr& list)
    {
        if(list && !contains(m_media_lists, list))
        {
            m_media_lists.push_back(list);
        }
    }

    namespace
    {
        // https://www.w3.org/TR/CSS21/tables.html#anonymous-boxes
        bool is_table_box(style_display display)
        {
            return is_one_of(display, display_table, display_inline_table);
        }

        bool is_row_group_box(style_display display)
        {
            return is_one_of(display, display_table_row_group, display_table_header_group, display_table_footer_group);
        }

        // A "proper table child" per CSS 2.1 17.2.1.
        bool is_proper_table_child(style_display display)
        {
            return is_one_of(display, display_table_row, display_table_row_group, display_table_header_group,
                             display_table_footer_group, display_table_column, display_table_column_group,
                             display_table_caption);
        }

        // A box that does not interrupt a sequence of consecutive siblings. Only collapsible white
        // space may be skipped: with `white-space: pre` the spaces are content and belong to the
        // anonymous cell generated around them.
        bool is_skippable_table_sibling(const render_item& el)
        {
            const element& src = *el.src_el();
            return src.is_white_space() || src.is_comment() || src.css().get_display() == display_none;
        }

        // CSS 2.1 17.2.1 step 3.2: a proper table child is misparented when its parent cannot hold
        // it, in which case an anonymous table is generated around it.
        bool is_misparented_table_child(style_display display, const std::shared_ptr<render_item>& parent)
        {
            const style_display parent_display = parent->src_el()->css().get_display();
            switch(display)
            {
            case display_table_row:
                return !is_row_group_box(parent_display) && !is_table_box(parent_display);
            case display_table_column:
                return parent_display != display_table_column_group && !is_table_box(parent_display);
            default:
                return !is_table_box(parent_display);
            }
        }

        // An internal table box or a caption: the boxes white space between which is not rendered
        // (CSS 2.1 17.2.1 stage 1).
        bool is_internal_table_box(style_display display)
        {
            return is_proper_table_child(display) || display == display_table_cell;
        }

        // The parent can hold table rows, row groups, columns, column groups and captions, so a
        // white space only inline box inside it is not part of the content.
        bool is_tabular_container(style_display display)
        {
            return is_table_box(display) || is_row_group_box(display) || display == display_table_row;
        }
    } // namespace

    void document::fix_tables_layout()
    {
        if(!m_root_render)
        {
            return;
        }
        // The stages of CSS 2.1 17.2.1 run in order and are repeated until the tree stops changing:
        // a box generated by one stage can make another one necessary (a row around cells still
        // needs a table, a table row needs its cell wrappers). The stages must not short-circuit
        // each other, so every one of them always runs.
        for(int round = 0; round < 100; round++)
        {
            bool changed  = walk_render_tree(m_root_render, [this](const std::shared_ptr<render_item>& box) {
                return drop_irrelevant_table_boxes(box);
            });
            changed      |= walk_render_tree(m_root_render, [this](const std::shared_ptr<render_item>& box) {
                return fix_table_child_wrappers(box);
            });
            changed      |= walk_render_tree(
                m_root_render, [this](const std::shared_ptr<render_item>& box) { return fix_table_cell_rows(box); });
            changed |= walk_render_tree(
                m_root_render, [this](const std::shared_ptr<render_item>& box) { return fix_table_parents(box); });
            if(!changed)
            {
                break;
            }
        }
    }

    bool document::walk_render_tree(const std::shared_ptr<render_item>&                             box,
                                    const std::function<bool(const std::shared_ptr<render_item>&)>& process)
    {
        bool changed = false;
        // process() can move children into anonymous wrappers, so walk a snapshot.
        const std::vector<std::shared_ptr<render_item>> children(box->children().begin(), box->children().end());
        for(const auto& child : children)
        {
            if(walk_render_tree(child, process))
            {
                changed = true;
            }
        }
        if(process(box))
        {
            changed = true;
        }
        return changed;
    }

    // Stage 1 of CSS 2.1 17.2.1: a white space only inline box that sits between two table boxes
    // (or next to the boundary of the tabular container) is not rendered. This holds whatever the
    // white-space property says: the spaces inside a `white-space: pre` table are only the
    // indentation of the markup, while spaces next to real content stay and become a cell.
    bool document::drop_irrelevant_table_boxes(const std::shared_ptr<render_item>& box)
    {
        if(!is_tabular_container(box->src_el()->css().get_display()))
        {
            return false;
        }

        auto& children = box->children();

        // The rendered children; a run of white space only boxes is looked at as a whole.
        struct rendered_child
        {
            std::shared_ptr<render_item> el;
            style_display                display;
            bool                         is_space;
        };
        std::vector<rendered_child> items;
        for(const auto& el : children)
        {
            const element& src = *el->src_el();
            if(src.is_comment() || src.css().get_display() == display_none)
            {
                continue;
            }
            items.push_back({el, src.css().get_display(), src.is_space()});
        }

        std::vector<std::shared_ptr<render_item>> to_remove;
        for(size_t i = 0; i < items.size();)
        {
            if(!items[i].is_space)
            {
                i++;
                continue;
            }
            const size_t first = i;
            while(i < items.size() && items[i].is_space)
            {
                i++;
            }
            const bool previous_is_table_box = first == 0 || is_internal_table_box(items[first - 1].display);
            const bool next_is_table_box     = i == items.size() || is_internal_table_box(items[i].display);
            if(previous_is_table_box && next_is_table_box)
            {
                for(size_t cur = first; cur < i; cur++)
                {
                    to_remove.push_back(items[cur].el);
                }
            }
        }

        for(const auto& el : to_remove)
        {
            if(auto it = std::find(children.begin(), children.end(), el); it != children.end())
            {
                children.erase(it);
            }
        }
        return !to_remove.empty();
    }

    // Stage 2 of CSS 2.1 17.2.1: generate the missing child wrappers of a table, row group or row.
    bool document::fix_table_child_wrappers(const std::shared_ptr<render_item>& box)
    {
        switch(box->src_el()->css().get_display())
        {
        case display_table:
        case display_inline_table:
            // A table's improper children are wrapped in an anonymous table-row.
            return wrap_improper_children(box, is_proper_table_child, display_table_row);
        case display_table_row_group:
        case display_table_header_group:
        case display_table_footer_group:
            return wrap_improper_children(
                box, [](style_display child_display) { return child_display == display_table_row; }, display_table_row);
        case display_table_row:
            return wrap_improper_children(
                box, [](style_display child_display) { return child_display == display_table_cell; },
                display_table_cell);
        default:
            return false;
        }
    }

    // Stage 3 of CSS 2.1 17.2.1: a cell outside a row gets an anonymous table-row.
    bool document::fix_table_cell_rows(const std::shared_ptr<render_item>& box)
    {
        if(box->src_el()->css().get_display() != display_table_cell)
        {
            return false;
        }
        const auto parent = box->parent();
        if(!parent || parent->src_el()->css().get_display() == display_table_row)
        {
            return false;
        }
        return wrap_siblings(
            box, [](style_display sibling_display) { return sibling_display == display_table_cell; },
            display_table_row);
    }

    // Stage 3 of CSS 2.1 17.2.1: a proper table child that cannot stay in its parent gets an
    // anonymous table around it and its consecutive proper table child siblings.
    bool document::fix_table_parents(const std::shared_ptr<render_item>& box)
    {
        const style_display display = box->src_el()->css().get_display();
        if(!is_proper_table_child(display))
        {
            return false;
        }
        const auto parent = box->parent();
        if(!parent || !is_misparented_table_child(display, parent))
        {
            return false;
        }
        // The anonymous table is inline-level when it is generated inside an inline box.
        const style_display table_display =
            parent->src_el()->css().get_display() == display_inline ? display_inline_table : display_table;
        return wrap_siblings(box, is_proper_table_child, table_display);
    }

    std::shared_ptr<render_item> document::create_anonymous_table_box(const std::shared_ptr<render_item>& parent,
                                                                      style_display                       display)
    {
        element::ptr anon_tag = std::make_shared<html_tag>(
            parent->src_el(), std::string("display:") + std::string(style_display_strings[display]));

        std::shared_ptr<render_item> anon_ri;
        if(is_table_box(display))
        {
            anon_ri = std::make_shared<render_item_table>(anon_tag);
        } else if(display == display_table_row)
        {
            anon_ri = std::make_shared<render_item_table_row>(anon_tag);
        } else if(display == display_table_cell)
        {
            anon_tag->set_tagName("table_cell");
            anon_ri = std::make_shared<render_item_block>(anon_tag);
        } else
        {
            anon_ri = std::make_shared<render_item_table_part>(anon_tag);
        }
        anon_ri->parent(parent);
        return anon_ri;
    }

    bool document::wrap_children_in_anonymous_box(const std::shared_ptr<render_item>&              parent,
                                                  style_display                                    wrapper_display,
                                                  const std::vector<std::shared_ptr<render_item>>& items)
    {
        if(items.empty())
        {
            return false;
        }

        auto& children  = parent->children();
        auto  insert_at = std::find(children.begin(), children.end(), items.front());
        if(insert_at == children.end())
        {
            return false;
        }

        auto anon_ri = create_anonymous_table_box(parent, wrapper_display);
        for(const auto& el : items)
        {
            anon_ri->add_child(el);
        }
        children.insert(insert_at, anon_ri);
        for(const auto& el : items)
        {
            if(auto it = std::find(children.begin(), children.end(), el); it != children.end())
            {
                children.erase(it);
            }
        }
        return true;
    }

    bool document::wrap_improper_children(const std::shared_ptr<render_item>&       box,
                                          const std::function<bool(style_display)>& is_content,
                                          style_display                             wrapper_display)
    {
        std::vector<std::vector<std::shared_ptr<render_item>>> runs;
        std::vector<std::shared_ptr<render_item>>              run;

        for(const auto& el : box->children())
        {
            if(is_content(el->src_el()->css().get_display()))
            {
                if(!run.empty())
                {
                    runs.push_back(run);
                    run.clear();
                }
                continue;
            }
            if(run.empty() && is_skippable_table_sibling(*el))
            {
                continue;
            }
            run.push_back(el);
        }
        if(!run.empty())
        {
            runs.push_back(run);
        }

        bool changed = false;
        for(auto& items : runs)
        {
            // Skippable boxes at the end of a run stay where they are.
            while(!items.empty() && is_skippable_table_sibling(*items.back()))
            {
                items.pop_back();
            }
            changed = wrap_children_in_anonymous_box(box, wrapper_display, items) || changed;
        }
        return changed;
    }

    bool document::wrap_siblings(const std::shared_ptr<render_item>&       el,
                                 const std::function<bool(style_display)>& in_run, style_display wrapper_display)
    {
        const auto parent = el->parent();
        if(!parent)
        {
            return false;
        }

        auto& children = parent->children();
        auto  cur      = std::find(children.begin(), children.end(), el);
        if(cur == children.end())
        {
            return false;
        }

        auto first = cur;
        for(auto prev = cur; prev != children.begin();)
        {
            --prev;
            if(in_run((*prev)->src_el()->css().get_display()))
            {
                first = prev;
            } else if(!is_skippable_table_sibling(**prev))
            {
                break;
            }
        }

        auto last = cur;
        for(auto next = std::next(cur); next != children.end(); ++next)
        {
            if(in_run((*next)->src_el()->css().get_display()))
            {
                last = next;
            } else if(!is_skippable_table_sibling(**next))
            {
                break;
            }
        }

        std::vector<std::shared_ptr<render_item>> items;
        for(auto it = first;; ++it)
        {
            items.push_back(*it);
            if(it == last)
            {
                break;
            }
        }
        return wrap_children_in_anonymous_box(parent, wrapper_display, items);
    }

    void document::append_children_from_string(element& parent, const char* str, bool replace_existing)
    {
        // parent must belong to this document
        if(parent.get_document().get() != this)
        {
            return;
        }

        GumboOptions opts = kGumboDefaultOptions;
        // This is require to prevent creating html, head, body tags around the fragment
        // Although Gumbo always creates html tag anyway. We have to ignore it in create_node.
        opts.fragment_context = GUMBO_TAG_BODY;
        // parse document into GumboOutput
        GumboOutput* output = gumbo_parse_with_options(&opts, str, strlen(str));

        // Create litehtml::elements.
        elements_list child_elements;
        // Create elements excluding the root node
        create_node(output->root, child_elements, true, false);

        // Destroy GumboOutput
        gumbo_destroy_output(&kGumboDefaultOptions, output);

        if(replace_existing)
        {
            parent.clearRecursive();
        }

        // Let's process created elements tree
        for(const auto& child : child_elements)
        {
            // Add the child element to parent
            parent.appendChild(child);

            // apply master CSS
            child->apply_stylesheet(m_master_css);

            // parse elements attributes
            child->parse_attributes();

            // Apply parsed styles.
            child->apply_stylesheet(m_styles);

            // Apply user styles if any
            child->apply_stylesheet(m_user_css);

            // Initialize m_css
            child->compute_styles();
        }
        resolve_generated_quotes();
        rebuild_render_tree();
    }

    void document::dump(dumper& cout)
    {
        if(m_root_render)
        {
            m_root_render->dump(cout);
        }
    }

} // namespace litehtml
