#include "litehtml/css_parser.h"
#include "litehtml/document.h"
#include "litehtml/style.h"
#include "grid_test_container.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <utility>

namespace
{
    bool check(bool condition, const std::string& message)
    {
        if(!condition)
        {
            std::cerr << message << '\n';
        }
        return condition;
    }

    bool near(float actual, float expected)
    {
        const bool match = std::abs(actual - expected) < 0.01f;
        if(!match)
        {
            std::cerr << "actual " << actual << ", expected " << expected << '\n';
        }
        return match;
    }

    bool parse(const std::string& text, litehtml::css_length& result, int options = litehtml::f_length_percentage)
    {
        auto tokens = litehtml::normalize(text, litehtml::f_componentize | litehtml::f_remove_whitespace);
        return tokens.size() == 1 && result.from_token(tokens[0], options);
    }

    class math_container : public grid_test_container
    {
      public:
        int                                                  viewport_width = 800;
        int                                                  image_size_calls = 0;
        litehtml::position                                   image_box;
        std::vector<litehtml::background_layer::color_point> stops;
        litehtml::uint_ptr create_font(const litehtml::font_description& description, const litehtml::document* doc,
                                       litehtml::font_metrics* metrics) override
        {
            const auto font   = grid_test_container::create_font(description, doc, metrics);
            metrics->x_height = description.size / 2;
            metrics->ch_width = description.size / 4;
            return font;
        }
        void get_media_features(litehtml::media_features& media) const override
        {
            grid_test_container::get_media_features(media);
            media.width = viewport_width;
        }
        void get_viewport(litehtml::position& viewport) const override
        {
            grid_test_container::get_viewport(viewport);
            viewport.width = viewport_width;
        }
        void get_image_size(const char*, const char*, litehtml::size& size) override
        {
            ++image_size_calls;
            size = {10, 10};
        }
        void draw_image(litehtml::uint_ptr, const litehtml::background_layer& layer, const std::string&,
                        const std::string&) override
        {
            image_box = layer.origin_box;
        }
        void draw_linear_gradient(litehtml::uint_ptr, const litehtml::background_layer&,
                                  const litehtml::background_layer::linear_gradient& gradient) override
        {
            stops = gradient.color_points;
        }
    };
} // namespace

int main()
{
    using namespace litehtml;
    bool           ok = true;
    math_container container;
    const auto     context = document::createFromString("<style>html{font-size:16px}</style>", &container);
    font_metrics   metrics;
    metrics.font_size = 20;
    metrics.x_height  = 10;
    metrics.ch_width  = 8;
    css_length length;
    struct example
    {
        const char* css;
        float       basis;
        float       expected;
    };
    const example valid[] = {
        {"calc(100% - 20px)",                        300, 280        },
        {"calc(10px + 2 * (3px - 1px))",             0,   14         },
        {"calc(1em + 2rem + 3ex + 4ch)",             0,   114        },
        {"calc(1in - 72pt + 2.54cm - 25.4mm + 1pc)", 0,   16         },
        {"calc(10vw + 10vh + 1vmin + 1vmax)",        0,   154        },
        {"calc((100vw / 20px) * 1px)",               0,   40         },
        {"calc((1in / 24px) * 2em)",                 0,   160        },
        {"calc(2px * 3px / 1px)",                    0,   6          },
        {"calc(1 / 2px * 4px * 3px)",                0,   6          },
        {"calc((100% / 50%) * 10px)",                200, 20         },
        {"min(50%, 150px)",                          200, 100        },
        {"min(50%, 150px)",                          400, 150        },
        {"max(calc(50% - 20px), 40px)",              100, 40         },
        {"clamp(40px, 50%, 120px)",                  200, 100        },
        {"clamp(120px, 50%, 40px)",                  200, 120        },
        {"clamp(none, 50%, 40px)",                   200, 40         },
        {"clamp(40px, 50%, none)",                   200, 100        },
        {"CALC(MAX(1px, 2px) + calc(3px))",          0,   5          },
        {"calc(-10px + +5px)",                       0,   -5         },
        {"calc((1px / 0) * 0)",                      0,   0          },
        {"min(calc(0px / 0), 10px)",                 0,   0          },
        {"calc(pi * 1px)",                           0,   3.14159265f}
    };
    for(const auto& value : valid)
    {
        ok &= check(parse(value.css, length), std::string("parse: ") + value.css);
        ok &= check(near(context->to_pixels(length, metrics, pixel_t(value.basis)).value(), value.expected),
                    std::string("evaluate: ") + value.css);
    }
    const char* invalid[] = {"calc()",
                             "calc(1px +)",
                             "calc(1px, 2px)",
                             "calc(1px + 1)",
                             "calc(0)",
                             "calc(2px * 3px)",
                             "calc(1px / 2px)",
                             "calc(1px +2px)",
                             "calc(1px+ 2px)",
                             "calc(1px/**/+/**/2px)",
                             "calc(1px + 1fr)",
                             "min()",
                             "min(1px,)",
                             "max(1px, 2)",
                             "clamp(1px, 2px)",
                             "clamp(1px, 2px, 3px, 4px)",
                             "clamp(1px, none, 3px)",
                             "calc(sin(1px))",
                             "calc(auto)"};
    for(const auto* css : invalid)
    {
        length.set_value(37, css_units_px);
        ok &= check(!parse(css, length) && near(length.val(), 37), std::string("reject atomically: ") + css);
    }
    style sizing;
    sizing.add("width:min-content; height:max-content; min-width:fit-content; "
               "min-height:fit-content(calc(10px + 50%)); max-width:fit-content(2em); max-height:fit-content(20px)");
    ok                  &= check(sizing.get_property(_width_).is<css_length>() &&
                                     sizing.get_property(_width_).get<css_length>().predef() == sizing_min_content &&
                                     sizing.get_property(_height_).get<css_length>().predef() == sizing_max_content &&
                                     sizing.get_property(_min_width_).get<css_length>().predef() == sizing_fit_content,
                                 "parse intrinsic sizing keywords");
    auto calculated_fit  = sizing.get_property(_min_height_).get<css_length>();
    auto relative_fit    = sizing.get_property(_max_width_).get<css_length>();
    context->cvt_units(calculated_fit, metrics, 0_px);
    context->cvt_units(relative_fit, metrics, 0_px);
    ok &= check(calculated_fit.fit_content_argument() && calculated_fit.has_percentage() &&
                    near(calculated_fit.fit_content_argument()->calc_percent(100_px).value(), 60) &&
                    relative_fit.fit_content_argument() &&
                    near(relative_fit.fit_content_argument()->calc_percent(0_px).value(), 40) &&
                    sizing.get_property(_max_height_).get<css_length>().fit_content_argument(),
                "preserve and resolve fit-content arguments");
    style invalid_sizing;
    invalid_sizing.add("width:17px; width:fit-content(-1px); width:fit-content(10px,20px); "
                       "padding-left:3px; padding-left:fit-content(10px)");
    ok                   &= check(near(invalid_sizing.get_property(_width_).get<css_length>().val(), 17) &&
                                      near(invalid_sizing.get_property(_padding_left_).get<css_length>().val(), 3),
                                  "reject invalid fit-content and keep it out of unrelated properties");
    std::string long_sum = "calc(1px";
    for(int i = 1; i < 32; i++)
    {
        long_sum += " + 1px";
    }
    long_sum += ')';
    ok       &= check(parse(long_sum, length) && near(length.calc_percent(0).value(), 32),
                      "operator whitespace survives the large-token normalization path");
    long_sum.erase(8, 1);
    ok &= check(!parse(long_sum, length), "large-token normalization does not manufacture missing operator whitespace");
    ok &= check(parse("calc(-20px)", length, f_length | f_positive) &&
                    near(context->to_pixels(length, metrics, 0).value(), 0),
                "negative calculated lengths clamp at the consuming property");
    ok &= check(parse("calc(100% / 50% * 10px)", length, f_length) &&
                    near(context->to_pixels(length, metrics, 0).value(), 20),
                "unhinted percentage ratios cancel without a containing-block basis");
    ok &= check(parse("calc(100% - 100% + 10px)", length) && length.has_percentage(),
                "cancelled percentages retain computed-value dependency");
    ok &=
        check(parse("calc(infinity * 1px)", length) && std::isfinite(context->to_pixels(length, metrics, 0).value()) &&
                  context->to_pixels(length, metrics, 0) > 1000000_px,
              "infinity is censored to the supported finite pixel range");
    ok        &= check(parse("min(50%, 150px)", length), "parse shared expression");
    auto copy  = length;
    copy.resolve_units(
        [&](float value, css_units unit) { return context->to_pixels(css_length(value, unit), metrics, 0).value(); });
    ok &= check(near(copy.calc_percent(200).value(), 100) && near(copy.calc_percent(400).value(), 150) &&
                    near(context->to_pixels(length, metrics, 200).value(), 100),
                "copying and repeated evaluation retain independent percentage bases");
    css_length roundtrip;
    ok &= check(parse(copy.to_string(), roundtrip) && near(context->to_pixels(roundtrip, metrics, 400).value(), 150),
                "math diagnostics retain the expression rather than freezing an arbitrary percentage basis");

    const auto doc = document::createFromString(
        "<!doctype html><style>body{margin:0}#host{width:100%;height:200px;font-size:20px}"
        "#box{--size:20px;width:calc(100% - var(--size));height:clamp(20px,50%,120px);"
        "padding:calc(2px + 1%);margin-left:calc(-10px + 5px);border:calc(1px + 1px) solid}"
        "#font{font-size:calc(50% + 2px);line-height:calc(100% + 3px)}"
        "#metrics{font-size:calc(1ex + 2ch + 1px)}"
        "#number{line-height:calc(1em / 10px)}"
        "#number span{font-size:10px}"
        "#bad{width:77px;width:calc(1px + 1)}"
        "#vars{--a:10px;--sum:10px + 20px;width:calc(var(--a) + var(--a) + var(--sum))}"
        "#fallback{width:calc(var(--missing, 20px + 10px) + 5px)}"
        "#cycle{--loop:var(--loop,11px);width:calc(var(--loop,19px) + 1px)}"
        "#invalidvar{--bad:1px + 1;width:77px;width:calc(var(--bad))}"
        "#background{width:200px;height:100px;background:url(test) no-repeat;"
        "background-size:calc(50% - 10px) max(10px,20%);background-position:calc(50% + 5px) 0}"
        "#gradient{width:200px;height:20px;background:linear-gradient(to right,red calc(50% - 10px),blue 100%)}"
        "</style><div id=host><div id=box></div><div id=font>x</div><div id=metrics></div>"
        "<div id=number><span>x</span></div>"
        "<div id=bad></div><div id=vars></div><div id=fallback></div><div id=cycle></div><div "
        "id=invalidvar></div></div>"
        "<div id=background></div><div id=gradient></div>",
        &container);
    const auto box = doc->root()->select_one("#box");
    doc->render(300);
    ok &= check(box && near(box->get_placement().width.value(), 280) &&
                    near(box->get_placement().height.value(), 100) && near(box->get_placement().x.value(), 2),
                "content dimensions and padding/border/margin offsets");
    doc->render(400);
    ok &= check(box && near(box->get_placement().width.value(), 380) && near(box->get_placement().x.value(), 3),
                "percentage calculations re-evaluate on render");
    const auto font  = doc->root()->select_one("#font");
    ok              &= check(font && near(font->css().get_font_size().value(), 12) &&
                                 near(font->css().line_height().computed_value.value(), 15),
                             "font percentages use inherited font size; line height uses current font size");
    const auto font_metrics_probe = doc->root()->select_one("#metrics");
    ok &= check(font_metrics_probe && near(font_metrics_probe->css().get_font_size().value(), 21),
                "font-size calculations use parent x-height and character metrics, not just its font size");
    const auto number  = doc->root()->select_one("#number span");
    ok                &= check(number && near(number->css().line_height().computed_value.value(), 20),
                               "calculated unitless line height inherits as a multiplier");
    const auto bad     = doc->root()->select_one("#bad");
    ok &= check(bad && near(bad->get_placement().width.value(), 77), "invalid calculations preserve the cascade");
    const auto vars     = doc->root()->select_one("#vars");
    const auto fallback = doc->root()->select_one("#fallback");
    ok &=
        check(vars && near(vars->get_placement().width.value(), 50), "repeated variables and expression substitution");
    ok &= check(fallback && near(fallback->get_placement().width.value(), 35), "math in variable fallbacks");
    const auto cycle      = doc->root()->select_one("#cycle");
    const auto invalidvar = doc->root()->select_one("#invalidvar");
    ok &= check(cycle && near(cycle->get_placement().width.value(), 20), "cyclic variables use the consuming fallback");
    ok &= check(invalidvar && near(invalidvar->get_placement().width.value(), 400),
                "invalid-at-computed-value math uses the property's initial value, not an earlier declaration");

    struct nested_layout
    {
        const char* display;
        int         depth;
    };
    const nested_layout nested_layouts[] = {
        {"block",                      12},
        {"flex;flex-direction:column", 7 },
        {"grid",                       7 },
    };
    for(const auto& layout : nested_layouts)
    {
        const auto measure_nested = [&](const char* property) {
            std::string markup  = "<!doctype html><style>body{margin:0}div{display:";
            markup             += layout.display;
            if(property)
            {
                markup += ';';
                markup += property;
                markup += ":max-content";
            }
            markup += "}img{display:block}</style><div id=outer>";
            for(int i = 0; i < layout.depth; ++i)
            {
                markup += "<div>";
            }
            markup += "<img src=test>";
            for(int i = 0; i < layout.depth; ++i)
            {
                markup += "</div>";
            }
            markup                     += "</div>";
            const auto nested_doc       = document::createFromString(markup.c_str(), &container);
            container.image_size_calls  = 0;
            nested_doc->render(100);
            const auto nested = nested_doc->root()->select_one("#outer");
            return std::make_pair(container.image_size_calls, nested ? nested->get_placement().height.value() : -1.0f);
        };
        const auto control = measure_nested(nullptr);
        for(const auto* property : {"min-height", "max-height"})
        {
            const auto feature = measure_nested(property);
            const auto limit   = std::max(64, control.first * 4);
            if(feature.first > limit)
            {
                std::cerr << "image measurements: " << feature.first << ", limit: " << limit << '\n';
            }
            ok &= check(control.second >= 0 && near(feature.second, control.second) && feature.first <= limit,
                        std::string("nested intrinsic ") + property + " measurement: " + layout.display);
        }
    }

    doc->draw(0, 0, 0, nullptr);
    ok &= check(near(container.image_box.width.value(), 90) && near(container.image_box.height.value(), 20),
                "background size uses the background positioning area");
    ok &= check(container.stops.size() == 3 && near(container.stops[1].offset, 0.45f),
                "gradient stops resolve mixed percentages against the gradient line");

    const auto layouts = document::createFromString(
        "<!doctype html><style>body{margin:0}.flex{display:flex;width:300px}.flex div{height:10px}"
        "#flexitem{flex:0 0 calc(50% - 10px)}"
        ".grid{display:grid;width:300px;grid-template-columns:calc(50% - 10px) 1fr;column-gap:calc(5% + 5px)}"
        ".grid div{height:10px}"
        ".repeat{display:grid;width:300px;grid-template-columns:repeat(auto-fit,calc(25% - 5px));gap:10px}"
        ".repeat div{height:10px}"
        ".floor{display:grid;width:100px;grid-template-columns:"
        "repeat(auto-fit,minmax(calc(100% - 10px),calc(100% - 100px)))}.floor div{height:50px}"
        "#indefinite{height:calc(50% + 10px)}#indefinite div{height:30px}"
        "table{border-spacing:0;width:300px}td{padding:0;height:10px}"
        "#cell{width:calc(50% - 10px)}"
        "#rowgap{display:grid;width:100px;row-gap:calc(10px + 0%)}#rowgap div{height:10px}"
        "#shorthand{--g:20px;margin:0 var(--g) 5px;height:10px}"
        "</style><div class=flex><div id=flexitem></div></div>"
        "<div class=grid><div id=grid1></div><div id=grid2></div></div>"
        "<div class=repeat><div id=repeat1></div><div id=repeat2></div><div></div><div></div></div>"
        "<div class=floor><div></div><div id=floor2></div></div>"
        "<div id=indefinite><div></div></div>"
        "<table><tr><td id=cell></td><td></td></tr></table>"
        "<div id=rowgap><div></div><div></div></div><div id=shorthand></div>",
        &container);
    layouts->render(800);
    const auto flexitem    = layouts->root()->select_one("#flexitem");
    const auto grid1       = layouts->root()->select_one("#grid1");
    const auto grid2       = layouts->root()->select_one("#grid2");
    const auto repeat2     = layouts->root()->select_one("#repeat2");
    const auto indefinite  = layouts->root()->select_one("#indefinite");
    const auto cell        = layouts->root()->select_one("#cell");
    const auto floor2      = layouts->root()->select_one("#floor2");
    ok                    &= check(flexitem && near(flexitem->get_placement().width.value(), 140), "mixed flex basis");
    ok &= check(grid1 && grid2 && near(grid1->get_placement().width.value(), 140) &&
                    near(grid2->get_placement().x.value(), 160) && near(grid2->get_placement().width.value(), 140),
                "grid tracks and mixed percentage gaps");
    ok &= check(repeat2 && near(repeat2->get_placement().x.value(), 80), "calc tracks in grid auto-repeat");
    const auto floor      = layouts->root()->select_one(".floor");
    ok                   &= check(floor2 && floor && near(floor2->get_placement().x.value(), 0) &&
                                      near((floor2->get_placement().y - floor->get_placement().y).value(), 50),
                                  "auto-repeat floors its definite maximum by its definite minimum");
    ok                   &= check(indefinite && near(indefinite->get_placement().height.value(), 30),
                                  "percentage-dependent height stays auto in an indefinite containing block");
    ok                   &= check(cell && near(cell->get_placement().width.value(), 140), "mixed table cell sizing");
    const auto rowgap     = layouts->root()->select_one("#rowgap");
    const auto shorthand  = layouts->root()->select_one("#shorthand");
    ok                   &= check(rowgap && near(rowgap->get_placement().height.value(), 30),
                                  "indefinite row gaps keep the fixed part of a mixed percentage calculation");
    ok                   &= check(shorthand && near(shorthand->get_placement().x.value(), 20) &&
                                      near(shorthand->get_placement().width.value(), 760),
                                  "variables in non-grid shorthands expand into their longhands");

    const auto root_font = document::createFromString("<style>html{font-size:calc(1rem + 4px)}</style>", &container);
    ok &= check(near(root_font->root()->css().get_font_size().value(), 20), "root font rem uses the initial font size");
    const auto initial_metrics =
        document::createFromString("<style>html{font-size:calc(1ex + 1ch + 4px)}</style>", &container);
    ok &= check(near(initial_metrics->root()->css().get_font_size().value(), 16),
                "root font calculations use the initial font's actual x-height and character metrics");
    container.viewport_width = 400;
    const auto viewport_doc  = document::createFromString(
        "<style>body{margin:0}div{width:calc(100vw / 20px * 1em)}</style><div id=viewport></div>", &container);
    viewport_doc->render(400);
    const auto viewport  = viewport_doc->root()->select_one("#viewport");
    ok                  &= check(viewport && near(viewport->get_placement().width.value(), 320),
                                 "typed viewport ratios resolve before length multiplication");
    const auto overflow  = document::createFromString(
        "<style>div{line-height:calc(infinity)}span{font-size:10px}</style><div><span id=huge></span></div>",
        &container);
    const auto huge  = overflow->root()->select_one("#huge");
    ok              &= check(huge && std::isfinite(huge->css().line_height().computed_value.value()) &&
                                 huge->css().line_height().computed_value > 1000000_px,
                             "inherited non-finite calculated number values cannot overflow line-height geometry");
    return ok ? 0 : 1;
}
