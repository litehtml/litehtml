#include "types.h"
#include <string>

#include "string_hash.h"

namespace litehtml
{
    const std::vector<string_hash> style_display_strings =          { _t("none"), _t("block"), _t("inline"), _t("inline-block"), _t("inline-table"), _t("list-item"), _t("table"), _t("table-caption"), _t("table-cell"), _t("table-column"), _t("table-column-group"), _t("table-footer-group"), _t("table-header-group"), _t("table-row"), _t("table-row-group") };
    const std::vector<string_hash> font_size_strings =              { _t("xx-small"), _t("x-small"), _t("small"), _t("medium"), _t("large"), _t("x-large"), _t("xx-large"), _t("smaller"), _t("larger") };
    const std::vector<string_hash> font_style_strings =             { _t("normal"), _t("italic") };
    const std::vector<string_hash> font_variant_strings =           { _t("normal"), _t("small-caps") };
    const std::vector<string_hash> font_weight_strings =            { _t("normal"), _t("bold"), _t("bolder"), _t("lighter100"), _t("200"), _t("300"), _t("400"), _t("500"), _t("600"), _t("700") };
    const std::vector<string_hash> list_style_type_strings =        { _t("none"), _t("circle"), _t("disc"), _t("square"), _t("armenian"), _t("cjk-ideographic"), _t("decimal"), _t("decimal-leading-zero"), _t("georgian"), _t("hebrew"), _t("hiragana"), _t("hiragana-iroha"), _t("katakana"), _t("katakana-iroha"), _t("lower-alpha"), _t("lower-greek"), _t("lower-latin"), _t("lower-roman"), _t("upper-alpha"), _t("upper-latin"), _t("upper-roman") };
    const std::vector<string_hash> list_style_position_strings =    { _t("inside"), _t("outside") };
    const std::vector<string_hash> vertical_align_strings =         { _t("baseline"), _t("sub"), _t("super"), _t("top"), _t("text-top"), _t("middle"), _t("bottom"), _t("text-bottom") };
    const std::vector<string_hash> border_width_strings =           { _t("thin"), _t("medium"), _t("thick") };
    const std::vector<string_hash> border_style_strings =           { _t("none"), _t("hidden"), _t("dotted"), _t("dashed"), _t("solid"), _t("double"), _t("groove"), _t("ridge"), _t("inset"), _t("outset") };
    const std::vector<string_hash> element_float_strings =          { _t("none"), _t("left"), _t("right") };
    const std::vector<string_hash> element_clear_strings =          { _t("none"), _t("left"), _t("right"), _t("both") };
    const std::vector<string_hash> css_units_strings =              { _t("none"), _t("%"), _t("in"), _t("cm"), _t("mm"), _t("em"), _t("ex"), _t("pt"), _t("pc"), _t("px"), _t("dpi"), _t("dpcm"), _t("vw"), _t("vh"), _t("vmin"), _t("vmax") };
    const std::vector<string_hash> background_attachment_strings =  { _t("scroll"), _t("fixed") };
    const std::vector<string_hash> background_repeat_strings =      { _t("repeat"), _t("repeat-x"), _t("repeat-y"), _t("no-repeat") };
    const std::vector<string_hash> background_box_strings =         { _t("border-box"), _t("padding-box"), _t("content-box") };
    const std::vector<string_hash> element_position_strings =       { _t("static"), _t("relative"), _t("absolute"), _t("fixed") };
    const std::vector<string_hash> text_align_strings =             { _t("left"), _t("right"), _t("center"), _t("justify") };
    const std::vector<string_hash> text_transform_strings =         { _t("none"), _t("capitalize"), _t("uppercase"), _t("lowercase") };
    const std::vector<string_hash> white_space_strings =            { _t("normal"), _t("nowrap"), _t("pre"), _t("pre-line"), _t("pre-wrap") };
    const std::vector<string_hash> overflow_strings =               { _t("visible"), _t("hidden"), _t("scroll"), _t("auto"), _t("no-display"), _t("no-content") };
    const std::vector<string_hash> background_size_strings =        { _t("auto"), _t("cover"), _t("contain") };
    const std::vector<string_hash> visibility_strings =             { _t("visible"), _t("hidden"), _t("collapse") };
    const std::vector<string_hash> border_collapse_strings =        { _t("collapse"), _t("separate") };
    const std::vector<string_hash> pseudo_class_strings =           { _t("only-child"), _t("only-of-type"), _t("first-child"), _t("first-of-type"), _t("last-child"), _t("last-of-type"), _t("nth-child"), _t("nth-of-type"), _t("nth-last-child"), _t("nth-last-of-type"), _t("not"), _t("lang") };
    const std::vector<string_hash> content_property_string =		{ _t("none"), _t("normal"), _t("open-quote"), _t("close-quote"), _t("no-open-quote"), _t("no-close-quote")};
    const std::vector<string_hash> media_orientation_strings =      { _t("portrait"), _t("landscape") };
    const std::vector<string_hash> media_feature_strings =          { _t("none"), _t("width"), _t("min-width"), _t("max-width"), _t("height"), _t("min-height"), _t("max-height"), _t("device-width"), _t("min-device-width"), _t("max-device-width"), _t("device-height"), _t("min-device-height"), _t("max-device-height"), _t("orientation"), _t("aspect-ratio"), _t("min-aspect-ratio"), _t("max-aspect-ratio"), _t("device-aspect-ratio"), _t("min-device-aspect-ratio"), _t("max-device-aspect-ratio"), _t("color"), _t("min-color"), _t("max-color"), _t("color-index"), _t("min-color-index"), _t("max-color-index"), _t("monochrome"), _t("min-monochrome"), _t("max-monochrome"), _t("resolution"), _t("min-resolution"), _t("max-resolution") };
    const std::vector<string_hash> box_sizing_strings =             { _t("content-box"), _t("border-box") };
    const std::vector<string_hash> pointer_events_strings =         { _t("auto"), _t("none"), _t("visiblePainted"), _t("visibleFill"), _t("visibleStroke"), _t("visible"), _t("painted"), _t("fill"), _t("stroke"), _t("all") };
    const std::vector<string_hash> media_type_strings =             { _t("none"), _t("all"), _t("screen"), _t("print"), _t("braille"), _t("embossed"), _t("handheld"), _t("projection"), _t("speech"), _t("tty"), _t("tv") };
    const std::vector<string_hash> void_elements =                  { _t( "area" ), _t( "base" ), _t( "br" ), _t( "col" ), _t( "command" ), _t( "embed" ), _t( "hr" ), _t( "img" ), _t( "input" ), _t( "keygen" ), _t( "link" ), _t( "meta" ), _t( "param" ), _t( "source" ), _t( "track" ), _t( "wbr" ) };
}
