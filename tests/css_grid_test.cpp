#include "litehtml/css_grid.h"
#include "litehtml/css_parser.h"
#include "litehtml/document.h"
#include "litehtml/style.h"
#include "grid_test_container.h"
#include <cmath>
#include <iostream>
#include <string>

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

    bool parse(const std::string& value, litehtml::grid_track_list& tracks)
    {
        return litehtml::parse_grid_track_list(
            litehtml::normalize(value, litehtml::f_componentize | litehtml::f_remove_whitespace), tracks);
    }
} // namespace

int main()
{
    using namespace litehtml;
    bool            ok = true;
    grid_track_list tracks;

    grid_track_list subgrid_tracks;
    ok                 &= check(parse("subgrid", subgrid_tracks), "subgrid should be a valid template track list");
    ok                 &= check(parse("SUBGRID [first] [middle] [last]", subgrid_tracks),
                                "subgrid should accept consecutive named lines");
    auto subgrid_names  = subgrid_tracks.resolve_subgrid_names(2);
    ok            &= check(subgrid_tracks.is_subgrid() && subgrid_tracks.subgrid_explicit_span() == 2 &&
                               subgrid_names.has_line_name(0, "first") && subgrid_names.has_line_name(1, "middle") &&
                               subgrid_names.has_line_name(2, "last"),
                           "subgrid names should identify successive lines");
    ok            &= check(!parse("subgrid 10px", subgrid_tracks) && subgrid_tracks.is_subgrid() &&
                               subgrid_tracks.resolve_subgrid_names(2).has_line_name(1, "middle"),
                           "invalid subgrid track sizes should not replace the previous value");
    ok            &= check(parse("subgrid [first] repeat(2, [a] [b]) repeat(auto-fill, [c]) [last]", subgrid_tracks),
                           "subgrid should accept fixed and auto-fill name repetitions");
    subgrid_names  = subgrid_tracks.resolve_subgrid_names(7);
    ok            &= check(subgrid_tracks.subgrid_explicit_span() == 5 && subgrid_names.has_line_name(0, "first") &&
                               subgrid_names.has_line_name(1, "a") && subgrid_names.has_line_name(4, "b") &&
                               subgrid_names.has_line_name(5, "c") && subgrid_names.has_line_name(6, "c") &&
                               subgrid_names.has_line_name(7, "last"),
                           "subgrid repeated names should resolve to the parent's span");
    ok            &= check(!parse("subgrid repeat(auto-fit, [x])", subgrid_tracks) &&
                               !parse("subgrid repeat(auto-fill, [x]) repeat(auto-fill, [y])", subgrid_tracks) &&
                               !parse("subgrid repeat(2, 10px)", subgrid_tracks),
                           "invalid subgrid name repetitions should be rejected");
    ok            &= check(parse("subgrid [first] repeat(auto-fill, [a] [b]) [last]", subgrid_tracks),
                           "subgrid auto-fill should accept multiple line-name sets");
    subgrid_names  = subgrid_tracks.resolve_subgrid_names(4);
    ok            &= check(subgrid_names.has_line_name(1, "a") && subgrid_names.has_line_name(2, "b") &&
                               subgrid_names.has_line_name(3, "last") && !subgrid_names.has_line_name(4, "a"),
                           "auto-fill should repeat only complete patterns");
    ok            &= check(parse("subgrid repeat(2000000000, [x])", subgrid_tracks) &&
                               subgrid_tracks.subgrid_explicit_span() == 1999999999 &&
                               subgrid_tracks.resolve_subgrid_names(2).has_line_name(2, "x"),
                           "large subgrid name repetitions should remain compact");

    ok &= check(parse("none", tracks) && tracks.empty(), "none should produce an empty track list");
    ok &= check(parse("100px 25% 2fr auto min-content max-content", tracks) && tracks.size() == 6,
                "simple track list should parse");
    if(tracks.size() == 6)
    {
        ok &= check(tracks[0].min.type == grid_breadth_type::length &&
                        std::abs(tracks[0].min.length.val() - 100) < 0.001f,
                    "fixed track minimum should match its maximum");
        ok &= check(tracks[1].max.length.units() == css_units_percentage, "percentage unit should be preserved");
        ok &= check(tracks[2].min.type == grid_breadth_type::automatic &&
                        tracks[2].max.type == grid_breadth_type::fraction &&
                        std::abs(tracks[2].max.fraction - 2) < 0.001f,
                    "fraction track should have an automatic minimum");
        ok &= check(tracks[4].max.type == grid_breadth_type::min_content &&
                        tracks[5].max.type == grid_breadth_type::max_content,
                    "intrinsic track keywords should be preserved");
    }
    ok &= check(parse("repeat(2, 20px minmax(10%, 3fr))", tracks) && tracks.size() == 4,
                "repeat should expand a multi-track pattern");
    if(tracks.size() == 4)
    {
        ok &= check(tracks[1].min.length.units() == css_units_percentage &&
                        tracks[1].max.type == grid_breadth_type::fraction &&
                        std::abs(tracks[1].max.fraction - 3) < 0.001f,
                    "minmax should preserve its bounds");
    }
    const char* invalid[] = {"",
                             "none 1fr",
                             "-1px",
                             "-1fr",
                             "1",
                             "minmax(1fr, 2fr)",
                             "minmax(auto 1fr)",
                             "repeat(0, 1fr)",
                             "repeat(2.5, 1fr)",
                             "repeat(auto-fit, 1fr)",
                             "repeat(auto-fill, 1fr)",
                             "repeat(auto-fit, minmax(auto, 1fr))",
                             "repeat(auto-fill, auto)",
                             "repeat(auto-fill)",
                             "repeat(auto-fill, 40px) repeat(auto-fit, 40px)",
                             "auto-fill 40px",
                             "repeat(2, repeat(2, 1fr))",
                             "repeat(2147483648, 1fr)",
                             "repeat(1073741824, 1fr 1fr)"};
    for(const auto* value : invalid)
    {
        ok &= check(!parse(value, tracks) && tracks.size() == 4, std::string("invalid track list: ") + value);
    }
    ok &= check(parse("repeat(1025, 1fr)", tracks) && tracks.size() == 1025,
                "explicit track lists should support more than 1024 tracks");
    ok &= check(parse("repeat(1024, 1fr 1fr)", tracks) && tracks.size() == 2048,
                "repeated multi-track patterns should support more than 1024 tracks");
    ok &= check(!parse("repeat(2000000000, 20px 1fr)", tracks) && tracks.size() == 2048,
                "unrepresentable track counts should not change the previous list");
    ok &= check(parse("repeat(2000000000, 1fr)", tracks) && tracks.size() == 2000000000 &&
                    tracks[1999999999].max.type == grid_breadth_type::fraction,
                "large repeated track lists should remain compact and indexable");
    ok &= check(
        parse("1px repeat(2, 2px 3px) 4px", tracks) && tracks.size() == 6 &&
            std::abs(tracks[0].min.length.val() - 1) < 0.001f && std::abs(tracks[1].min.length.val() - 2) < 0.001f &&
            std::abs(tracks[4].min.length.val() - 3) < 0.001f && std::abs(tracks[5].min.length.val() - 4) < 0.001f,
        "track indexing should cross segment and repeat boundaries");
    ok                        &= check(parse("REPEAT(2, MINMAX(AUTO, 1FR))", tracks) && tracks.size() == 2,
                                       "CSS keywords and units should be case-insensitive");
    ok                        &= check(parse("repeat(2, fit-content(50%))", tracks) && tracks.size() == 2 &&
                                           tracks[1].min.type == grid_breadth_type::automatic &&
                                           tracks[1].max.type == grid_breadth_type::fit_content &&
                                           tracks[1].max.length.units() == css_units_percentage,
                                       "fit-content should preserve its limit in repeated tracks");
    const char* invalid_fit[]  = {"fit-content()",     "fit-content(-1px)",     "fit-content(1fr)",
                                  "fit-content(auto)", "fit-content(1px, 2px)", "minmax(auto, fit-content(10px))"};
    for(const auto* value : invalid_fit)
    {
        ok &= check(!parse(value, tracks) && tracks.size() == 2, std::string("invalid fit-content: ") + value);
    }
    ok &= check(parse("repeat(auto-fill, minmax(40px, 1fr))", tracks) && tracks.size() == 1 &&
                    tracks.has_auto_repeat() && tracks[0].min.type == grid_breadth_type::length &&
                    std::abs(tracks[0].min.length.val() - 40) < 0.001f &&
                    tracks[0].max.type == grid_breadth_type::fraction,
                "auto-fill should keep its pattern unresolved with a single placeholder repetition");
    ok &= check(parse("repeat(auto-fit, 40px 20px)", tracks) && tracks.size() == 2 && tracks.has_auto_repeat(),
                "auto-fit should accept multi-track fixed-size patterns");
    ok &= check(parse("repeat(auto-fill, 40px) repeat(2, 100px)", tracks) && tracks.size() == 3 &&
                    tracks.has_auto_repeat() && std::abs(tracks[2].min.length.val() - 100) < 0.001f,
                "a fixed repeat may follow an auto-repeat");
    {
        grid_track_list auto_tracks;
        ok &= check(parse("repeat(auto-fill, minmax(40px, 1fr))", auto_tracks) && auto_tracks.size() == 1,
                    "auto-fill should parse a single placeholder track");
        auto_repeat_info info;
        auto             resolved = auto_tracks.resolve_auto_repeat(200_px, 0_px, true, info);
        ok   &= check(info.present && !info.auto_fit && info.start == 0 && info.count == 5 && resolved.size() == 5 &&
                          resolved[0].min.type == grid_breadth_type::length &&
                          resolved[4].max.type == grid_breadth_type::fraction,
                      "auto-fill should resolve to the largest non-overflowing repetition count");
        info  = {};
        resolved  = auto_tracks.resolve_auto_repeat(0_px, 0_px, false, info);
        ok       &= check(info.present && info.count == 1 && resolved.size() == 1,
                          "auto-fill should resolve to one repetition when the axis is indefinite");
    }
    {
        grid_track_list auto_tracks;
        ok &= check(parse("repeat(auto-fit, 25%)", auto_tracks) && auto_tracks.size() == 1,
                    "auto-fit should accept percentage fixed sizes");
        auto_repeat_info info;
        auto             resolved  = auto_tracks.resolve_auto_repeat(200_px, 0_px, true, info);
        ok                        &= check(info.present && info.auto_fit && info.count == 4 && resolved.size() == 4,
                                           "auto-fit should resolve percentage tracks against the definite container");
    }
    {
        grid_track_list auto_tracks;
        ok &= check(parse("repeat(auto-fill, [col] 40px)", auto_tracks) && auto_tracks.size() == 1,
                    "auto-repeat should accept named lines inside its pattern");
        auto_repeat_info info;
        auto             resolved  = auto_tracks.resolve_auto_repeat(200_px, 0_px, true, info);
        ok                        &= check(info.present && info.count == 5 && resolved.has_line_name(0, "col") &&
                                               resolved.has_line_name(4, "col"),
                                           "resolved auto-repeat should repeat its named lines");
    }
    grid_track_list named_tracks;
    ok &= check(parse("[start] 10px [middle] 20px [end]", named_tracks) && named_tracks.size() == 2,
                "track lists should accept named grid lines");
    ok &= check(named_tracks.has_line_name(0, "start") && named_tracks.has_line_name(1, "middle") &&
                    named_tracks.has_line_name(2, "end") && !named_tracks.has_line_name(0, "Start"),
                "grid line names should preserve positions and case");
    ok &= check(parse("[start] repeat(2, [Col] 10px [end]) [finish] 20px", named_tracks) &&
                    named_tracks.has_line_name(0, "Col") && named_tracks.has_line_name(1, "Col") &&
                    named_tracks.has_line_name(1, "end") && named_tracks.has_line_name(2, "end") &&
                    named_tracks.has_line_name(2, "finish"),
                "repeated line names should merge at pattern boundaries");
    grid_track_list huge_named_tracks;
    ok &= check(parse("repeat(2000000000, [line] 1fr)", huge_named_tracks) &&
                    huge_named_tracks.has_line_name(1999999999, "line"),
                "large named repeats should stay compact and resolve their line names");

    grid_placement placement;
    auto           parse_placement = [&](const std::string& value) {
        return parse_grid_placement(normalize(value, f_componentize | f_remove_whitespace), placement);
    };
    ok &=
        check(parse_placement("-2 / span 3") && placement.start.type == grid_line_type::line &&
                  placement.start.value == -2 && placement.end.type == grid_line_type::span && placement.end.value == 3,
              "placement should preserve negative lines and spans");
    ok &= check(parse_placement("2 span") && placement.start.type == grid_line_type::span &&
                    placement.start.value == 2 && placement.end.type == grid_line_type::automatic,
                "placement should accept reversed span syntax and default its end");
    const char* invalid_placements[] = {"",          "0",   "1.5", "span",      "span -1",   "span 0",
                                        "span span", "1 /", "/ 1", "1 / 2 / 3", "2147483648"};
    for(const auto* value : invalid_placements)
    {
        ok &= check(!parse_placement(value), std::string("invalid placement: ") + value);
    }
    resolved_grid_axis named_axis;
    ok &= check(parse_placement("Col 2 / span end") && resolve_grid_axis(placement, 3, named_axis, &named_tracks) &&
                    named_axis.start && *named_axis.start == 1 && named_axis.span == 1,
                "named spans should search from their opposite edge");
    ok &= check(parse_placement("Col -1") && resolve_grid_axis(placement, 3, named_axis, &named_tracks) &&
                    named_axis.start && *named_axis.start == 1 && named_axis.span == 1,
                "negative named indexes should count from the explicit grid end");
    ok &= check(parse_placement("missing 3") && resolve_grid_axis(placement, 3, named_axis, &named_tracks) &&
                    named_axis.start && *named_axis.start == 6,
                "missing named lines should resolve into the implicit grid");
    ok &= check(parse_placement("span end / Col 2") && resolve_grid_axis(placement, 3, named_axis, &named_tracks) &&
                    named_axis.start && *named_axis.start == -1 && named_axis.span == 2,
                "backward named spans should search implicit lines on the start side");
    ok &= check(parse_placement("-5 / 2") && resolve_grid_axis(placement, 3, named_axis, nullptr, true) &&
                    named_axis.start && *named_axis.start == 0 && named_axis.span == 1,
                "subgrid placement should clamp a span crossing the start edge");
    ok &= check(parse_placement("10000 / span 3000") && resolve_grid_axis(placement, 3, named_axis, nullptr, true) &&
                    named_axis.start && *named_axis.start == 2 && named_axis.span == 1,
                "subgrid placement entirely beyond the end should use its last track");
    ok &= check(parse_placement("span 3000") && resolve_grid_axis(placement, 3, named_axis, nullptr, true) &&
                    !named_axis.start && named_axis.span == 3,
                "an automatic subgrid span should not exceed the inherited grid");

    style declarations;
    style named_areas;
    named_areas.add("grid-template-areas: 'header header' 'main side';");
    ok &= check(named_areas.get_property(_grid_template_areas_).is<grid_template_areas>() &&
                    named_areas.get_property(_grid_template_areas_).get<grid_template_areas>().areas.size() == 3,
                "grid-template-areas should accept rectangular named areas");
    named_areas.add("grid-template-areas: 'header header' 'header side';");
    ok &= check(named_areas.get_property(_grid_template_areas_).get<grid_template_areas>().areas.size() == 3,
                "non-rectangular named areas should not replace a valid template");
    named_areas.add("grid-template-areas: 'header header' 'main';");
    ok &= check(named_areas.get_property(_grid_template_areas_).get<grid_template_areas>().columns == 2,
                "named area rows must have equal widths");
    named_areas.add("grid-template-areas: 'a .' '. a';");
    ok &= check(named_areas.get_property(_grid_template_areas_).get<grid_template_areas>().areas.size() == 3,
                "disconnected named areas should not replace a valid template");
    style area_shorthand;
    area_shorthand.add("grid-area: main; grid-row-start: 2 !important; grid-area: other;");
    ok &= check(area_shorthand.get_property(_grid_row_start_).get<grid_line>().value == 2 &&
                    area_shorthand.get_property(_grid_column_start_).get<grid_line>().name == "other" &&
                    area_shorthand.get_property(_grid_row_end_).get<grid_line>().name == "other" &&
                    area_shorthand.get_property(_grid_column_end_).get<grid_line>().name == "other",
                "grid-area should expand named areas while respecting important longhands");
    area_shorthand.add("grid-area: 1 / 2 / 3 / 4 / 5;");
    ok &= check(area_shorthand.get_property(_grid_column_start_).get<grid_line>().name == "other",
                "invalid grid-area shorthands should preserve prior longhands");
    style grid_template_shorthand;
    grid_template_shorthand.add("grid-template: 20px / 1fr 2fr;");
    ok &= check(grid_template_shorthand.get_property(_grid_template_rows_).is<grid_track_list>() &&
                    grid_template_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().size() == 1 &&
                    grid_template_shorthand.get_property(_grid_template_columns_).is<grid_track_list>() &&
                    grid_template_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().size() == 2,
                "grid-template should expand its row and column track lists");
    grid_template_shorthand.add("grid-auto-columns: 17px; grid-auto-flow: column; grid-template: none;");
    ok &= check(grid_template_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().empty() &&
                    grid_template_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().empty() &&
                    grid_template_shorthand.get_property(_grid_auto_columns_).get<grid_track_list>().size() == 1 &&
                    grid_template_shorthand.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column,
                "grid-template should reset explicit tracks without resetting the implicit grid");
    style grid_shorthand;
    grid_shorthand.add("grid-auto-columns: 5px !important; gap: 7px; grid: dense auto-flow 20px 30px / 1fr 2fr;");
    ok &=
        check(grid_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().empty() &&
                  grid_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().size() == 2 &&
                  grid_shorthand.get_property(_grid_auto_rows_).get<grid_track_list>().size() == 2 &&
                  std::abs(grid_shorthand.get_property(_grid_auto_columns_).get<grid_track_list>()[0].max.length.val() -
                           5) < 0.001f &&
                  grid_shorthand.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_row_dense &&
                  std::abs(grid_shorthand.get_property(_row_gap_).get<css_length>().val() - 7) < 0.001f,
              "grid row-flow shorthand should reset implicit properties, preserve gaps and respect importance");
    grid_shorthand.add("grid: 10px 20px / auto-flow dense 15px;");
    ok &= check(grid_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().size() == 2 &&
                    grid_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().empty() &&
                    grid_shorthand.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column_dense &&
                    grid_shorthand.get_property(_grid_auto_rows_).get<grid_track_list>()[0].max.type ==
                        grid_breadth_type::automatic,
                "grid column-flow shorthand should reset rows and set dense column flow");
    grid_shorthand.add("grid: auto-flow 1fr / invalid; grid: 1fr / auto-flow repeat(2, 1px);");
    ok &= check(grid_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().size() == 2 &&
                    grid_shorthand.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column_dense,
                "invalid grid shorthands must not partially replace previous longhands");
    grid_shorthand.add("grid: inherit;");
    ok &= check(grid_shorthand.get_property(_grid_template_rows_).is<inherit>() &&
                    grid_shorthand.get_property(_grid_auto_flow_).is<inherit>(),
                "grid shorthand inheritance should expand to its longhands");
    grid_shorthand.add("grid: unset;");
    ok &= check(grid_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().empty() &&
                    grid_shorthand.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_row,
                "grid unset should reset its non-inherited longhands");
    grid_template_shorthand.add("grid-template: 10px / 1fr; grid-template: \"header header\" / 1fr 1fr;");
    ok &= check(
        grid_template_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().size() == 1 &&
            grid_template_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().size() == 2 &&
            grid_template_shorthand.get_property(_grid_template_areas_).get<grid_template_areas>().areas.size() == 1,
        "grid-template should expand named area strings and columns");
    grid_template_shorthand.add("grid-template: \"header header\" \"header side\" / 1fr 1fr;");
    ok &=
        check(grid_template_shorthand.get_property(_grid_template_areas_).get<grid_template_areas>().areas.size() == 1,
              "invalid area strings should not partially replace a shorthand");
    declarations.add("grid-template-columns: 2fr 20px; grid-template-columns: minmax(1fr, 2fr);");
    const auto& columns  = declarations.get_property(_grid_template_columns_);
    ok                  &= check(columns.is<grid_track_list>() && columns.get<grid_track_list>().size() == 2,
                                 "invalid declarations must not replace valid track lists");
    declarations.add("grid-template-columns: repeat(1025, 1fr);");
    ok &= check(declarations.get_property(_grid_template_columns_).get<grid_track_list>().size() == 2,
                "unrenderable track lists should not replace a supported fallback");
    declarations.add("grid-template-columns: subgrid [a] [b]; grid-auto-columns: subgrid;");
    ok &= check(declarations.get_property(_grid_template_columns_).get<grid_track_list>().is_subgrid() &&
                    !declarations.get_property(_grid_auto_columns_).is<grid_track_list>(),
                "subgrid should be accepted for templates but not for auto tracks");
    grid_template_shorthand.add("grid-template: subgrid / subgrid;");
    ok &= check(grid_template_shorthand.get_property(_grid_template_columns_).get<grid_track_list>().is_subgrid() &&
                    grid_template_shorthand.get_property(_grid_template_rows_).get<grid_track_list>().is_subgrid(),
                "grid-template should accept subgrid on both axes");
    declarations.add("grid-column-start: 3 !important; grid-column: 1 / span 2;");
    const auto& start  = declarations.get_property(_grid_column_start_);
    const auto& end    = declarations.get_property(_grid_column_end_);
    ok                &= check(start.is<grid_line>() && start.get<grid_line>().value == 3 && end.is<grid_line>() &&
                                   end.get<grid_line>().type == grid_line_type::span && end.get<grid_line>().value == 2,
                               "grid shorthand must respect longhand importance");
    declarations.add("grid-row: 2 / span 4; grid-row: 1 / 0;");
    ok &= check(declarations.get_property(_grid_row_start_).is<grid_line>() &&
                    declarations.get_property(_grid_row_start_).get<grid_line>().value == 2 &&
                    declarations.get_property(_grid_row_end_).get<grid_line>().value == 4,
                "invalid grid shorthand must leave both longhands unchanged");
    declarations.add("grid-row: inherit;");
    ok &= check(declarations.get_property(_grid_row_start_).is<inherit>() &&
                    declarations.get_property(_grid_row_end_).is<inherit>(),
                "grid shorthand inheritance must expand to both longhands");
    style invalid_auto;
    invalid_auto.add("grid-auto-columns: repeat(1, 1fr);");
    ok &= check(invalid_auto.empty(), "grid-auto-columns must not accept repeat");
    style patterned_auto;
    patterned_auto.add("grid-auto-columns: 10px 20px;");
    ok &= check(patterned_auto.get_property(_grid_auto_columns_).is<grid_track_list>() &&
                    patterned_auto.get_property(_grid_auto_columns_).get<grid_track_list>().size() == 2,
                "grid-auto-columns should accept a repeating track pattern");
    patterned_auto.add("grid-auto-columns: 1px repeat(1, 1fr); grid-auto-columns: none;");
    patterned_auto.add("grid-auto-columns: [name] 10px;");
    ok &= check(patterned_auto.get_property(_grid_auto_columns_).get<grid_track_list>().size() == 2,
                "invalid implicit track patterns should not replace a valid declaration");
    patterned_auto.add("grid-auto-columns: 2em 3em !important; grid-auto-columns: 1px;");
    ok &= check(std::abs(patterned_auto.get_property(_grid_auto_columns_).get<grid_track_list>()[0].max.length.val() -
                         2) < 0.001f,
                "implicit track patterns should respect declaration importance");
    declarations.add("grid-gap: 1em 10%; row-gap: 5px !important; gap: 2px 3px;");
    const auto& row_gap     = declarations.get_property(_row_gap_);
    const auto& column_gap  = declarations.get_property(_column_gap_);
    ok                     &= check(row_gap.is<css_length>() && column_gap.is<css_length>() &&
                                        std::abs(row_gap.get<css_length>().val() - 5) < 0.001f &&
                                        std::abs(column_gap.get<css_length>().val() - 3) < 0.001f,
                                    "gap aliases and shorthand should respect importance");
    declarations.add("gap: 1px -1px; grid-auto-flow: column; grid-auto-flow: row column;");
    ok &= check(std::abs(declarations.get_property(_row_gap_).get<css_length>().val() - 5) < 0.001f &&
                    std::abs(declarations.get_property(_column_gap_).get<css_length>().val() - 3) < 0.001f &&
                    declarations.get_property(_grid_auto_flow_).is<int>() &&
                    declarations.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column,
                "invalid gap or flow should not replace valid declarations");
    declarations.add("grid-auto-flow: dense column;");
    ok &= check(declarations.get_property(_grid_auto_flow_).is<int>() &&
                    declarations.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column_dense,
                "dense and direction should parse in either order");
    declarations.add("grid-auto-flow: dense dense;");
    ok &= check(declarations.get_property(_grid_auto_flow_).get<int>() == grid_auto_flow_column_dense,
                "invalid dense flow should not replace the previous declaration");

    grid_test_container container;
    const auto          doc = document::createFromString(
        "<div id='parent' style='font-size: 20px; --tracks: 2em 25%; grid-template-columns: var(--tracks); "
        "grid-column: -2 / span 3'><div id='child' style='font-size: 10px; grid-template-columns: inherit; "
        "grid-column: inherit; grid-auto-columns: minmax(1em, 2fr); grid-auto-flow: column; "
        "grid-gap: 2em 5%'></div></div>"
        "<div id='cascade' style='--lines: 1 / span 2; --spacing: 1em 2em; grid-column: var(--lines); "
        "grid-column-start: 4; gap: var(--spacing); column-gap: 3px'></div>"
        "<div id='cascade-reverse' style='--lines: 1 / 2; grid-column-start: 4; grid-column: var(--lines)'></div>"
        "<div id='bad-var' style='grid-column-start: 4; grid-column: var(--missing)'></div>"
        "<div id='grid-var' style='--definition:auto-flow 20px / 30px 40px;grid:var(--definition);"
        "grid-template-columns:50px'></div>"
        "<div id='grid-bad-var' style='grid-template-columns:50px;grid:var(--missing)'></div>"
        "<div id='repeated-units' style='font-size:10px; grid-template-columns:repeat(512, 2em 1fr)'></div>",
        &container);
    const auto parent  = doc->root()->select_one("#parent");
    const auto child   = doc->root()->select_one("#child");
    ok                &= check(parent && child, "test document should contain both elements");
    if(parent && child)
    {
        const auto& parent_tracks = parent->css().get_grid_template_columns();
        const auto& child_tracks  = child->css().get_grid_template_columns();
        ok &= check(parent_tracks.size() == 2 && child_tracks.size() == 2, "track list should resolve var and inherit");
        if(parent_tracks.size() == 2 && child_tracks.size() == 2)
        {
            ok &= check(parent_tracks[0].min.length.units() == css_units_px &&
                            std::abs(parent_tracks[0].min.length.val() - 40) < 0.001f &&
                            std::abs(child_tracks[0].min.length.val() - 40) < 0.001f &&
                            child_tracks[1].max.length.units() == css_units_percentage,
                        "computed tracks should resolve em once and retain percentages");
        }
        ok &= check(child->css().get_grid_column_start().value == -2 &&
                        child->css().get_grid_column_end().type == grid_line_type::span &&
                        child->css().get_grid_column_end().value == 3,
                    "grid line shorthand should inherit computed values");
        ok &= check(child->css().get_grid_auto_rows().max.type == grid_breadth_type::automatic,
                    "implicit track sizing should default to auto");
        ok &= check(child->css().get_grid_auto_columns().min.type == grid_breadth_type::length &&
                        std::abs(child->css().get_grid_auto_columns().min.length.val() - 10) < 0.001f &&
                        child->css().get_grid_auto_columns().max.type == grid_breadth_type::fraction &&
                        child->css().get_grid_auto_flow() == grid_auto_flow_column &&
                        std::abs(child->css().get_row_gap().val() - 20) < 0.001f &&
                        child->css().get_column_gap().units() == css_units_percentage,
                    "implicit track, auto-flow and gaps should compute using the element's font");
    }
    const auto cascade      = doc->root()->select_one("#cascade");
    const auto reverse      = doc->root()->select_one("#cascade-reverse");
    const auto bad_var      = doc->root()->select_one("#bad-var");
    const auto grid_var     = doc->root()->select_one("#grid-var");
    const auto grid_bad_var = doc->root()->select_one("#grid-bad-var");
    ok &= check(grid_var && grid_var->css().get_grid_template_columns().size() == 1 &&
                    std::abs(grid_var->css().get_grid_template_columns()[0].max.length.val() - 50) < 0.001f &&
                    std::abs(grid_var->css().get_grid_auto_rows().max.length.val() - 20) < 0.001f && grid_bad_var &&
                    grid_bad_var->css().get_grid_template_columns().empty(),
                "variable grid shorthands should preserve cascade order and reset invalid values");
    const auto repeated_units = doc->root()->select_one("#repeated-units");
    if(repeated_units)
    {
        const auto& repeated_tracks = repeated_units->css().get_grid_template_columns();
        ok &= check(repeated_tracks.size() == 1024 && repeated_tracks[1022].min.length.units() == css_units_px &&
                        std::abs(repeated_tracks[1022].min.length.val() - 20) < 0.001f &&
                        repeated_tracks[1023].max.type == grid_breadth_type::fraction,
                    "computed repeated patterns should preserve converted units and indexing");
    } else
    {
        ok &= check(false, "repeated unit test element should exist");
    }
    ok &= check(cascade && reverse && bad_var, "cascade test elements should exist");
    if(cascade && reverse && bad_var)
    {
        ok &= check(cascade->css().get_grid_column_start().value == 4 &&
                        cascade->css().get_grid_column_end().value == 2 &&
                        std::abs(cascade->css().get_column_gap().val() - 3) < 0.001f,
                    "later longhands must override variable shorthands");
        ok &=
            check(reverse->css().get_grid_column_start().value == 1 && reverse->css().get_grid_column_end().value == 2,
                  "later variable shorthands must override earlier longhands");
        ok &= check(bad_var->css().get_grid_column_start().type == grid_line_type::automatic,
                    "invalid variable shorthands must reset the affected longhands");
    }
    const auto supports_doc = document::createFromString(
        "<style>@supports (grid-template-columns: 1fr) { #probe { width: 111px } } "
        "@supports (grid-template-columns: minmax(1fr,2fr)) { #probe { width: 222px } } "
        "@supports (grid-template-columns: repeat(1025,1fr)) { #probe { width: 333px } } "
        "@supports (gap: 1px) { #probe { height: 111px } } "
        "@supports (grid-auto-flow: row dense) { #probe { height: 222px } } "
        "@supports (grid-auto-columns: 10px fit-content(20px)) { #pattern-probe { width: 42px } } "
        "@supports (grid-template-columns: fit-content(1fr)) { #pattern-probe { width: 84px } }"
        "@supports (grid-template: 10px / 1fr) { #shorthand-probe { width: 31px } } "
        "@supports (grid: auto-flow 10px / 1fr) { #shorthand-probe { height: 32px } } "
        "@supports (grid-template: \"a a\" / 1fr 1fr) { #shorthand-probe { width: 62px } }"
        "</style><div id='probe'></div><div id='pattern-probe'></div><div id='shorthand-probe'></div>",
        &container);
    const auto probe  = supports_doc->root()->select_one("#probe");
    ok               &= check(probe && std::abs(probe->css().get_width().val() - 111) < 0.001f &&
                                  std::abs(probe->css().get_height().val() - 222) < 0.001f,
                              "feature queries should accept supported grid syntax and reject unsupported syntax");
    const auto pattern_probe = supports_doc->root()->select_one("#pattern-probe");
    ok &= check(pattern_probe && std::abs(pattern_probe->css().get_width().val() - 42) < 0.001f,
                "feature queries should recognize valid implicit patterns and reject invalid fit-content");
    const auto shorthand_probe = supports_doc->root()->select_one("#shorthand-probe");
    ok &= check(shorthand_probe && std::abs(shorthand_probe->css().get_width().val() - 62) < 0.001f &&
                    std::abs(shorthand_probe->css().get_height().val() - 32) < 0.001f,
                "feature queries should recognize named-area shorthand syntax");
    resolved_grid_axis axis;
    placement.start  = {grid_line_type::line, -4};
    placement.end    = {grid_line_type::line, -1};
    ok              &= check(resolve_grid_axis(placement, 2, axis) && axis.start && *axis.start == -1 && axis.span == 3,
                             "negative lines should resolve relative to the explicit grid");
    placement.start  = {grid_line_type::span, 2};
    placement.end    = {grid_line_type::line, 3};
    ok              &= check(resolve_grid_axis(placement, 4, axis) && axis.start && *axis.start == 0 && axis.span == 2,
                             "a span before a definite line should resolve backwards");
    placement.start  = {grid_line_type::automatic, 0};
    placement.end    = {grid_line_type::span, 3};
    ok              &= check(resolve_grid_axis(placement, 4, axis) && !axis.start && axis.span == 3,
                             "an automatic axis should retain its span");
    placement.start  = {grid_line_type::line, 2000000000};
    placement.end    = {grid_line_type::automatic, 0};
    ok              &= check(!resolve_grid_axis(placement, 2, axis), "extreme line numbers must not overflow");
    std::vector<grid_item_placement> items(4);
    std::vector<grid_area>           areas;
    grid_area                        bounds;
    items[1].columns.start = {grid_line_type::line, 2};
    items[1].rows.start    = {grid_line_type::line, 1};
    items[2].columns.end   = {grid_line_type::span, 2};
    ok &= check(place_grid_items(items, 3, 0, grid_auto_flow_row, areas, bounds) && areas.size() == 4,
                "row auto-placement should succeed");
    if(areas.size() == 4)
    {
        ok &= check(areas[0].column_start == 0 && areas[0].row_start == 0 && areas[1].column_start == 1 &&
                        areas[1].row_start == 0 && areas[2].column_start == 0 && areas[2].column_end == 2 &&
                        areas[2].row_start == 1 && areas[3].column_start == 2 && areas[3].row_start == 1,
                    "row auto-placement should skip occupied cells and fit spans");
    }
    ok &= check(bounds.column_end == 3 && bounds.row_end == 2, "implicit rows should extend grid bounds");
    items.assign(3, {});
    items[0].order         = 1;
    items[2].rows.start    = {grid_line_type::line, -2};
    items[2].columns.start = {grid_line_type::line, 1};
    ok &= check(place_grid_items(items, 0, 2, grid_auto_flow_column, areas, bounds) && areas.size() == 3,
                "column auto-placement should succeed");
    if(areas.size() == 3)
    {
        ok &= check(areas[2].column_start == 0 && areas[2].row_start == 1 && areas[1].column_start == 0 &&
                        areas[1].row_start == 0 && areas[0].column_start == 1 && areas[0].row_start == 0,
                    "column flow should honor order and negative lines");
    }
    items.assign(2, {});
    items[0].columns.start = items[1].columns.start = {grid_line_type::line, -4};
    items[0].rows.start = items[1].rows.start = {grid_line_type::line, 1};
    ok &= check(place_grid_items(items, 2, 1, grid_auto_flow_row, areas, bounds) && areas.size() == 2 &&
                    areas[0].column_start == -1 && areas[1].column_start == -1 && bounds.column_start == -1,
                "explicit placements may overlap and create tracks before the explicit grid");
    items[0].columns.start  = {grid_line_type::line, 2000000000};
    ok                     &= check(!place_grid_items(items, 2, 1, grid_auto_flow_row, areas, bounds),
                                    "extreme line numbers must not overflow or allocate an unbounded grid");
    items.assign(4, {});
    items[0].rows.start    = {grid_line_type::line, 1};
    items[0].columns.start = {grid_line_type::line, 3};
    items[1].rows.start    = {grid_line_type::line, 1};
    items[2].rows.start    = {grid_line_type::line, 1};
    items[3].columns.start = {grid_line_type::line, 1};
    ok &= check(place_grid_items(items, 3, 0, grid_auto_flow_row, areas, bounds) && areas.size() == 4,
                "partially positioned items should be placed");
    if(areas.size() == 4)
    {
        ok &= check(areas[0].column_start == 2 && areas[1].column_start == 0 && areas[2].column_start == 1 &&
                        areas[3].column_start == 0 && areas[3].row_start == 1,
                    "row-locked items should fill earlier vacancies before advancing the sparse cursor");
    }
    items.assign(3, {});
    items[0].columns.end = {grid_line_type::span, 2};
    items[1].columns.end = {grid_line_type::span, 2};
    ok &= check(place_grid_items(items, 3, 0, grid_auto_flow_row_dense, areas, bounds) && areas.size() == 3 &&
                    areas[2].column_start == 2 && areas[2].row_start == 0,
                "dense row placement should backfill an earlier gap");
    items.assign(3, {});
    items[0].rows.end = {grid_line_type::span, 2};
    items[1].rows.end = {grid_line_type::span, 2};
    ok &= check(place_grid_items(items, 0, 3, grid_auto_flow_column_dense, areas, bounds) && areas.size() == 3 &&
                    areas[2].row_start == 2 && areas[2].column_start == 0,
                "dense column placement should backfill an earlier gap");
    items.assign(3, {});
    items[0].rows.start = items[1].rows.start = items[2].rows.start = {grid_line_type::line, 1};
    items[0].columns.start                                          = {grid_line_type::line, 2};
    items[1].columns.end                                            = {grid_line_type::span, 2};
    ok &= check(place_grid_items(items, 3, 1, grid_auto_flow_row_dense, areas, bounds) && areas.size() == 3 &&
                    areas[2].column_start == 0 && areas[2].row_start == 0,
                "dense placement should backfill gaps among row-locked items");
    items.assign(6, {});
    ok &= check(place_grid_items(items, 2, 2, grid_auto_flow_row, areas, bounds, nullptr, nullptr, true, true) &&
                    areas.size() == 6 && areas[0].column_start == 0 && areas[0].row_start == 0 &&
                    areas[3].column_start == 1 && areas[3].row_start == 1 && areas[4].column_start == 0 &&
                    areas[4].row_start == 1 && areas[5].column_start == 1 && areas[5].row_start == 1 &&
                    bounds.column_start == 0 && bounds.column_end == 2 && bounds.row_start == 0 && bounds.row_end == 2,
                "bounded auto-placement should clamp overflow without creating implicit tracks");
    ok &= check(place_grid_items(items, 2, 2, grid_auto_flow_row, areas, bounds, nullptr, nullptr, false, true) &&
                    areas.size() == 6 && areas[4].column_start == 0 && areas[4].row_start == 1 &&
                    areas[5].column_start == 1 && areas[5].row_start == 1 && bounds.row_end == 2,
                "a bounded row axis should clamp overflow independently of standalone columns");
    ok &= check(place_grid_items(items, 2, 2, grid_auto_flow_column, areas, bounds, nullptr, nullptr, true, true) &&
                    areas.size() == 6 && areas[4].column_start == 1 && areas[4].row_start == 0 &&
                    areas[5].column_start == 1 && areas[5].row_start == 1 && bounds.column_end == 2,
                "column auto-placement should clamp overflow in the bounded column axis");
    items.assign(1, {});
    items[0].columns.start = {grid_line_type::line, 10000};
    items[0].rows.start    = {grid_line_type::line, -10000};
    ok &= check(place_grid_items(items, 2, 2, grid_auto_flow_row, areas, bounds, nullptr, nullptr, true, true) &&
                    areas[0].column_start == 1 && areas[0].row_start == 0,
                "bounded explicit placement should clamp to inherited tracks");
    std::vector<pixel_t> sizes = {100, 0, 0};
    ok    &= check(distribute_grid_fractions(400, {0, 2, 1}, sizes) && sizes[0] == 100_px && sizes[1] == 200_px &&
                       sizes[2] == 100_px,
                   "fractional tracks should divide space after fixed tracks");
    sizes  = {80, 0};
    ok    &= check(distribute_grid_fractions(100, {1, 1}, sizes) && sizes[0] == 80_px && sizes[1] == 20_px,
                   "fractional tracks should freeze at their minimum sizes");
    sizes  = {0, 0};
    ok    &= check(distribute_grid_fractions(100, {0.25f, 0.25f}, sizes) && sizes[0] == 25_px && sizes[1] == 25_px,
                   "fractional factors below one should leave unused space");
    sizes  = {50, 0};
    ok    &= check(distribute_grid_fractions(20, {1, 1}, sizes) && sizes[0] == 50_px && sizes[1] == 0_px,
                   "fractional tracks should not shrink below their minimum sizes");
    ok    &= check(!distribute_grid_fractions(100, {-1, 1}, sizes) && sizes[0] == 50_px && sizes[1] == 0_px,
                   "invalid fractional factors should leave sizes unchanged");
    const auto layout_doc = document::createFromString(
        "<style>body{margin:0} .grid{display:grid;width:300px;grid-template-columns:100px 1fr 2fr;"
        "grid-auto-rows:20px;gap:10px}.grid div{height:20px}</style>"
        "<div class='grid'><div id='one'></div><div id='two'></div><div id='three'></div>"
        "<div id='four'></div></div>",
        &container);
    layout_doc->render(800);
    const auto one    = layout_doc->root()->select_one("#one");
    const auto two    = layout_doc->root()->select_one("#two");
    const auto three  = layout_doc->root()->select_one("#three");
    const auto four   = layout_doc->root()->select_one("#four");
    ok               &= check(one && two && three && four, "grid layout test elements should exist");
    if(one && two && three && four)
    {
        auto first   = one->get_placement();
        auto second  = two->get_placement();
        auto third   = three->get_placement();
        auto fourth  = four->get_placement();
        ok          &= check(first.x == 0_px && first.width == 100_px && second.x == 110_px && second.width == 60_px &&
                                 third.x == 180_px && third.width == 120_px && fourth.x == 0_px && fourth.y == 30_px,
                             "grid should size fractional columns, gaps and implicit rows");
    }
    const auto subgrid_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:230px;grid-template-columns:40px 70px 100px;"
        "grid-template-rows:20px 30px;gap:10px}.sub{display:grid;grid-column:2 / 4;grid-row:1 / 3;"
        "grid-template-columns:subgrid;grid-template-rows:subgrid}</style>"
        "<div class='parent'><div class='sub'><div id='sub-first'></div><div id='sub-second'></div>"
        "<div id='sub-third'></div><div id='sub-fourth'></div></div></div>",
        &container);
    subgrid_doc->render(800);
    const auto sub_first  = subgrid_doc->root()->select_one("#sub-first");
    const auto sub_second = subgrid_doc->root()->select_one("#sub-second");
    const auto sub_third  = subgrid_doc->root()->select_one("#sub-third");
    const auto sub_fourth = subgrid_doc->root()->select_one("#sub-fourth");
    ok &= check(sub_first && sub_second && sub_third && sub_fourth && sub_first->get_placement().width == 70_px &&
                    sub_first->get_placement().height == 20_px && sub_second->get_placement().x == 130_px &&
                    sub_second->get_placement().width == 100_px && sub_third->get_placement().y == 30_px &&
                    sub_third->get_placement().height == 30_px && sub_fourth->get_placement().x == 130_px &&
                    sub_fourth->get_placement().y == 30_px,
                "subgrid items should use the parent grid's spanned tracks and gaps");
    const auto subgrid_stretch_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:180px;grid-template-columns:70px 100px;"
        "grid-template-rows:20px 30px;gap:10px}.sub{display:grid;grid-column:1 / 3;grid-row:1 / 3;"
        "grid-template:subgrid / subgrid;width:10px;height:10px;justify-self:end;align-self:end}</style>"
        "<div class='parent'><div class='sub' id='stretch-sub'><div></div></div></div>",
        &container);
    subgrid_stretch_doc->render(800);
    const auto stretch_subgrid = subgrid_stretch_doc->root()->select_one("#stretch-sub");
    ok &= check(stretch_subgrid && stretch_subgrid->get_placement().x == 0_px &&
                    stretch_subgrid->get_placement().y == 0_px && stretch_subgrid->get_placement().width == 180_px &&
                    stretch_subgrid->get_placement().height == 60_px,
                "subgrids should stretch to their parent area on each subgridded axis");
    const auto subgrid_span_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:120px;grid-template-columns:30px 30px 30px 30px;"
        "grid-template-rows:20px}.sub{display:grid;grid-template-columns:subgrid [a] [b] [c]}</style>"
        "<div class='parent'><div class='sub' id='implicit-span-sub'><div></div><div id='implicit-span-second'></div>"
        "</div></div>",
        &container);
    subgrid_span_doc->render(800);
    const auto implicit_span_sub    = subgrid_span_doc->root()->select_one("#implicit-span-sub");
    const auto implicit_span_second = subgrid_span_doc->root()->select_one("#implicit-span-second");
    ok &= check(implicit_span_sub && implicit_span_second && implicit_span_sub->get_placement().width == 60_px &&
                    implicit_span_second->get_placement().x == 30_px,
                "an automatic subgrid span should follow its specified line-name list");
    const auto subgrid_intrinsic_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:300px;grid-template-columns:max-content max-content;"
        "grid-template-rows:20px;column-gap:10px;justify-content:start}.sub{display:grid;grid-column:1 / 3;"
        "grid-template-columns:subgrid}#intrinsic-first{width:40px}#intrinsic-second{width:100px}</style>"
        "<div class='parent'><div class='sub'><div id='intrinsic-first'></div><div id='intrinsic-second'></div>"
        "</div></div>",
        &container);
    subgrid_intrinsic_doc->render(800);
    const auto intrinsic_first  = subgrid_intrinsic_doc->root()->select_one("#intrinsic-first");
    const auto intrinsic_second = subgrid_intrinsic_doc->root()->select_one("#intrinsic-second");
    ok &= check(intrinsic_first && intrinsic_second && intrinsic_first->get_placement().x == 0_px &&
                    intrinsic_second->get_placement().x == 50_px && intrinsic_second->get_placement().width == 100_px,
                "subgrid descendants should size their individual parent tracks");
    const auto subgrid_intrinsic_names_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:300px;grid-template-columns:[a] max-content [b] max-content "
        "[c];"
        "grid-template-rows:20px;column-gap:10px;justify-content:start}.sub{display:grid;grid-column:1 / 3;"
        "grid-template-columns:subgrid}#intrinsic-named-first{grid-column:a / b;width:40px}"
        "#intrinsic-named-second{grid-column:b / c;width:100px}</style>"
        "<div class='parent'><div class='sub'><div id='intrinsic-named-first'></div>"
        "<div id='intrinsic-named-second'></div></div></div>",
        &container);
    subgrid_intrinsic_names_doc->render(800);
    const auto intrinsic_named_first  = subgrid_intrinsic_names_doc->root()->select_one("#intrinsic-named-first");
    const auto intrinsic_named_second = subgrid_intrinsic_names_doc->root()->select_one("#intrinsic-named-second");
    ok &= check(intrinsic_named_first && intrinsic_named_second && intrinsic_named_first->get_placement().x == 0_px &&
                    intrinsic_named_second->get_placement().x == 50_px,
                "inherited line names should determine subgrid intrinsic contributions");
    const auto subgrid_names_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:230px;grid-template-columns:[a] 40px [b] 70px [c] 100px [d];"
        "grid-template-rows:20px;column-gap:10px}.sub{display:grid;grid-column:2 / 4;"
        "grid-template-columns:subgrid [local-start] [local-middle] [local-end];column-gap:0}"
        "#named-sub-first{grid-column:b / c}#named-sub-second{grid-column:local-middle / local-end}</style>"
        "<div class='parent'><div class='sub'><div id='named-sub-first'></div><div id='named-sub-second'></div>"
        "</div></div>",
        &container);
    subgrid_names_doc->render(800);
    const auto named_sub_first  = subgrid_names_doc->root()->select_one("#named-sub-first");
    const auto named_sub_second = subgrid_names_doc->root()->select_one("#named-sub-second");
    ok &= check(named_sub_first && named_sub_second && named_sub_first->get_placement().x == 50_px &&
                    named_sub_first->get_placement().width == 75_px && named_sub_second->get_placement().x == 125_px &&
                    named_sub_second->get_placement().width == 105_px,
                "subgrids should combine parent and local line names and honor an explicit zero gap");
    const auto subgrid_rows_doc = document::createFromString(
        "<style>body{margin:0}.parent{display:grid;width:100px;grid-template-columns:100px;"
        "grid-template-rows:auto auto;row-gap:10px}.sub{display:grid;grid-row:1 / 3;grid-template-rows:subgrid}"
        "#row-sub-first{height:20px}#row-sub-second{height:30px}</style>"
        "<div class='parent'><div class='sub'><div id='row-sub-first'></div><div id='row-sub-second'></div>"
        "</div></div>",
        &container);
    subgrid_rows_doc->render(800);
    const auto row_sub_first  = subgrid_rows_doc->root()->select_one("#row-sub-first");
    const auto row_sub_second = subgrid_rows_doc->root()->select_one("#row-sub-second");
    ok &= check(row_sub_first && row_sub_second && row_sub_first->get_placement().y == 0_px &&
                    row_sub_first->get_placement().height == 20_px && row_sub_second->get_placement().y == 30_px &&
                    row_sub_second->get_placement().height == 30_px,
                "subgrid descendants should size their individual parent rows");
    const auto named_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:120px;grid-template-columns:[left] 40px [center] 80px [right];"
        "grid-template-rows:20px 30px;grid-template-areas:'head head' 'main side'}"
        "#named-head{grid-area:head}#named-main{grid-area:main}"
        "#named-side{grid-column:side-start / side-end;grid-row:side}</style>"
        "<div class='grid'><div id='named-side'></div><div id='named-head'></div><div id='named-main'></div></div>",
        &container);
    named_doc->render(800);
    const auto named_head = named_doc->root()->select_one("#named-head");
    const auto named_main = named_doc->root()->select_one("#named-main");
    const auto named_side = named_doc->root()->select_one("#named-side");
    ok &=
        check(named_head && named_head->get_placement().x == 0_px && named_head->get_placement().y == 0_px &&
                  named_head->get_placement().width == 120_px && named_head->get_placement().height == 20_px &&
                  named_main && named_main->get_placement().x == 0_px && named_main->get_placement().y == 20_px &&
                  named_main->get_placement().width == 40_px && named_side && named_side->get_placement().x == 40_px &&
                  named_side->get_placement().y == 20_px && named_side->get_placement().width == 80_px,
              "named areas and their generated lines should place grid items independently of source order");
    const auto named_shorthand_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:120px;"
        "grid-template:[top] 'head head' 20px [bottom] [content] 'main side' 30px [end] / "
        "[left] 40px [center] 80px [right]}#short-head{grid-area:head}"
        "#short-main{grid-column:left / center;grid-row:content / end}"
        "#short-side{--area:side;grid-area:var(--area)}</style>"
        "<div class='grid'><div id='short-side'></div><div id='short-head'></div><div id='short-main'></div></div>",
        &container);
    named_shorthand_doc->render(800);
    const auto short_head  = named_shorthand_doc->root()->select_one("#short-head");
    const auto short_main  = named_shorthand_doc->root()->select_one("#short-main");
    const auto short_side  = named_shorthand_doc->root()->select_one("#short-side");
    ok                    &= check(
        short_head && short_head->get_placement().width == 120_px && short_head->get_placement().height == 20_px &&
            short_main && short_main->get_placement().x == 0_px && short_main->get_placement().y == 20_px &&
            short_main->get_placement().width == 40_px && short_side && short_side->get_placement().x == 40_px &&
            short_side->get_placement().y == 20_px && short_side->get_placement().width == 80_px,
        "named shorthand areas and adjacent row line-name blocks should drive rendered placement");
    const auto area_extent_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:70px;grid-template-columns:30px;grid-template-rows:10px;"
        "grid-auto-columns:40px 50px;grid-auto-rows:20px;grid-template-areas:'head head' 'main side'}"
        "#extent-side{grid-area:side}#extent-last{grid-column:-2 / -1;grid-row:2}</style>"
        "<div class='grid'><div id='extent-side'></div><div id='extent-last'></div></div>",
        &container);
    area_extent_doc->render(800);
    const auto extent_side = area_extent_doc->root()->select_one("#extent-side");
    const auto extent_last = area_extent_doc->root()->select_one("#extent-last");
    ok &=
        check(extent_side && extent_side->get_placement().x == 30_px && extent_side->get_placement().y == 10_px &&
                  extent_side->get_placement().width == 40_px && extent_side->get_placement().height == 20_px &&
                  extent_last && extent_last->get_placement().x == 30_px && extent_last->get_placement().width == 40_px,
              "area dimensions should extend the explicit grid and use implicit track patterns");
    const auto empty_area_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:80px;grid:'. .' 10px '. .' 20px}</style>"
        "<div class='grid'><div></div><div></div><div id='empty-area-third'></div></div>",
        &container);
    empty_area_doc->render(800);
    const auto empty_area_third = empty_area_doc->root()->select_one("#empty-area-third");
    ok &= check(empty_area_third && empty_area_third->get_placement().x == 0_px &&
                    empty_area_third->get_placement().y == 10_px && empty_area_third->get_placement().width == 40_px &&
                    empty_area_third->get_placement().height == 20_px,
                "empty area cells should still define the explicit grid");
    const auto shorthand_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:90px;grid:auto-flow 20px / 30px 60px}"
        ".columns{grid:10px 20px / auto-flow 25px}</style>"
        "<div class='grid'><div></div><div></div><div id='grid-row-flow'></div></div>"
        "<div class='grid columns'><div></div><div></div><div id='grid-column-flow'></div></div>",
        &container);
    shorthand_doc->render(800);
    const auto shorthand_row    = shorthand_doc->root()->select_one("#grid-row-flow");
    const auto shorthand_column = shorthand_doc->root()->select_one("#grid-column-flow");
    ok &= check(shorthand_row && shorthand_row->get_placement().x == 0_px &&
                    shorthand_row->get_placement().y == 20_px && shorthand_row->get_placement().width == 30_px,
                "grid row-flow shorthand should drive rendered placement");
    ok &= check(shorthand_column && shorthand_column->get_placement().x == 25_px &&
                    shorthand_column->get_placement().y == 40_px && shorthand_column->get_placement().width == 25_px,
                "grid column-flow shorthand should drive rendered placement");
    const auto fit_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;grid-template-columns:fit-content(50px) 1fr}"
        ".small{grid-template-columns:fit-content(20px) 1fr}.large{grid-template-columns:fit-content(90px) 1fr}"
        ".percent{grid-template-columns:fit-content(20%) 1fr}"
        ".inline{display:inline-grid;grid-template-columns:fit-content(50px)}</style>"
        "<div class='grid'><div>four four</div><div id='fit-capped'></div></div>"
        "<div class='grid small'><div>four four</div><div id='fit-minimum'></div></div>"
        "<div class='grid large'><div>four four</div><div id='fit-natural'></div></div>"
        "<div class='grid percent'><div>four four</div><div id='fit-percent'></div></div>"
        "<div class='inline' id='fit-inline'><div>four four</div></div>",
        &container);
    fit_doc->render(800);
    const auto fit_capped  = fit_doc->root()->select_one("#fit-capped");
    const auto fit_minimum = fit_doc->root()->select_one("#fit-minimum");
    const auto fit_natural = fit_doc->root()->select_one("#fit-natural");
    const auto fit_percent = fit_doc->root()->select_one("#fit-percent");
    const auto fit_inline  = fit_doc->root()->select_one("#fit-inline");
    ok &= check(fit_capped && fit_capped->get_placement().x == 50_px && fit_minimum &&
                    fit_minimum->get_placement().x == 32_px && fit_natural && fit_natural->get_placement().x == 72_px &&
                    fit_percent && fit_percent->get_placement().x == 60_px && fit_inline &&
                    fit_inline->get_placement().width == 50_px,
                "fit-content should cap intrinsic growth without shrinking below the automatic minimum");
    const auto fit_span_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;justify-content:start;"
        "grid-template-columns:fit-content(20px) auto}.span{grid-column:span 2}</style>"
        "<div class='grid'><div class='span'>four four</div><div></div><div id='fit-span-rest'></div></div>",
        &container);
    fit_span_doc->render(800);
    const auto fit_span_rest      = fit_span_doc->root()->select_one("#fit-span-rest");
    ok                           &= check(fit_span_rest && fit_span_rest->get_placement().x == 20_px &&
                                              fit_span_rest->get_placement().width == 52_px,
                                          "fit-content should redistribute a spanning item's growth past the limit");
    const auto pattern_value_doc  = document::createFromString(
        "<div style='font-size:20px;grid-auto-columns:2em fit-content(3em)'>"
        "<div id='inherited-pattern' style='font-size:10px;grid-auto-columns:inherit'></div></div>"
        "<div id='variable-pattern' style='font-size:10px;--sizes:1em fit-content(2em);"
        "grid-auto-columns:var(--sizes)'></div>",
        &container);
    const auto inherited_pattern = pattern_value_doc->root()->select_one("#inherited-pattern");
    const auto variable_pattern  = pattern_value_doc->root()->select_one("#variable-pattern");
    if(inherited_pattern && variable_pattern)
    {
        const auto& inherited  = inherited_pattern->css().get_grid_auto_column_pattern();
        const auto& variable   = variable_pattern->css().get_grid_auto_column_pattern();
        ok                    &= check(inherited.size() == 2 && variable.size() == 2 &&
                                           std::abs(inherited[0].max.length.val() - 40) < 0.001f &&
                                           std::abs(inherited[1].max.length.val() - 60) < 0.001f &&
                                           std::abs(variable[0].max.length.val() - 10) < 0.001f &&
                                           std::abs(variable[1].max.length.val() - 20) < 0.001f,
                                       "implicit track patterns should resolve inherited and variable units");
    } else
    {
        ok &= check(false, "implicit track pattern test elements should exist");
    }
    const auto pattern_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:200px;grid-template-columns:30px;grid-template-rows:30px;"
        "grid-auto-columns:10px 20px 40px;grid-auto-rows:10px 20px 40px}"
        "#p0{grid-column:-4;grid-row:-4}#p1{grid-column:-3;grid-row:-3}"
        "#p2{grid-column:1;grid-row:1}#p3{grid-column:2;grid-row:2}"
        "#p4{grid-column:3;grid-row:3}#p5{grid-column:4;grid-row:4}</style>"
        "<div class='grid'><div id='p0'></div><div id='p1'></div><div id='p2'></div>"
        "<div id='p3'></div><div id='p4'></div><div id='p5'></div></div>",
        &container);
    pattern_doc->render(800);
    const char* pattern_ids[]       = {"#p0", "#p1", "#p2", "#p3", "#p4", "#p5"};
    const int   pattern_positions[] = {0, 20, 60, 90, 100, 120};
    const int   pattern_sizes[]     = {20, 40, 30, 10, 20, 40};
    for(size_t index = 0; index < 6; ++index)
    {
        const auto item  = pattern_doc->root()->select_one(pattern_ids[index]);
        ok              &= check(item && item->get_placement().x == pixel_t(pattern_positions[index]) &&
                                     item->get_placement().y == pixel_t(pattern_positions[index]) &&
                                     item->get_placement().width == pixel_t(pattern_sizes[index]) &&
                                     item->get_placement().height == pixel_t(pattern_sizes[index]),
                                 std::string("implicit track pattern geometry: ") + pattern_ids[index]);
    }
    const auto empty_grid_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:200px;grid-auto-flow:column;"
        "grid-auto-columns:10px 20px 40px;grid-auto-rows:10px}</style>"
        "<div class='grid'><div></div><div></div><div id='empty-explicit-third'></div></div>",
        &container);
    empty_grid_doc->render(800);
    const auto empty_explicit_third = empty_grid_doc->root()->select_one("#empty-explicit-third");
    ok &= check(empty_explicit_third && empty_explicit_third->get_placement().x == 30_px &&
                    empty_explicit_third->get_placement().width == 40_px,
                "implicit track patterns should begin at the first track when the explicit grid is empty");
    const auto dense_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:30px;grid-template-columns:repeat(3,10px);"
        "grid-auto-rows:10px;grid-auto-flow:dense}.wide{grid-column:span 2}</style>"
        "<div class='grid'><div class='wide'></div><div class='wide'></div><div id='dense-last'></div></div>",
        &container);
    dense_doc->render(800);
    const auto dense_last = dense_doc->root()->select_one("#dense-last");
    ok &= check(dense_last && dense_last->get_placement().x == 20_px && dense_last->get_placement().y == 0_px,
                "dense grid should render later items in earlier gaps");
    const auto more_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:220px;grid-template-columns:repeat(2, 1fr);"
        "grid-auto-rows:30px;gap:20px}.spanning{grid-column:span 2}.positioned{position:absolute}"
        ".inner{display:grid;grid-template-columns:25px 1fr}.inner div{height:10px}</style>"
        "<div class='grid'><div id='span' class='spanning'></div><div class='positioned'></div>"
        "<div id='nested' class='inner'><div id='nested-one'></div><div id='nested-two'></div></div>"
        "<div id='last'></div></div>",
        &container);
    more_doc->render(800);
    const auto span       = more_doc->root()->select_one("#span");
    const auto nested     = more_doc->root()->select_one("#nested");
    const auto nested_one = more_doc->root()->select_one("#nested-one");
    const auto nested_two = more_doc->root()->select_one("#nested-two");
    const auto last       = more_doc->root()->select_one("#last");
    ok &= check(span && nested && nested_one && nested_two && last, "nested grid test elements should exist");
    if(span && nested && nested_one && nested_two && last)
    {
        const auto spanning_box = span->get_placement();
        const auto nested_box   = nested->get_placement();
        const auto second_box   = nested_two->get_placement();
        const auto last_box     = last->get_placement();
        ok &= check(spanning_box.width == 220_px && spanning_box.height == 30_px && nested_box.x == 0_px &&
                        nested_box.y == 50_px && nested_box.width == 100_px && second_box.x == 25_px &&
                        second_box.width == 75_px && last_box.x == 120_px && last_box.y == 50_px,
                    "spans, positioned children, nested grids and item stretch should preserve geometry");
    }
    const auto minmax_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;grid-template-columns:minmax(10px,100px) 1fr}"
        "</style><div class='grid'><div id='minmax-a'></div><div id='minmax-b'></div></div>",
        &container);
    minmax_doc->render(800);
    const auto minmax_a  = minmax_doc->root()->select_one("#minmax-a");
    const auto minmax_b  = minmax_doc->root()->select_one("#minmax-b");
    ok                  &= check(minmax_a && minmax_b && minmax_a->get_placement().width == 100_px &&
                                     minmax_b->get_placement().x == 100_px && minmax_b->get_placement().width == 200_px,
                                 "fixed minmax tracks should grow before fractional tracks");
    const auto inline_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:inline-grid;grid-template-columns:40px 140px;gap:10px}"
        "</style><span class='grid' id='inline'><span id='inline-a'></span><span id='inline-b'></span></span>",
        &container);
    inline_doc->render(800);
    const auto inline_grid = inline_doc->root()->select_one("#inline");
    const auto inline_b    = inline_doc->root()->select_one("#inline-b");
    ok                   &= check(inline_grid && inline_b && inline_grid->get_placement().width == 190_px &&
                                      inline_b->get_placement().x == 50_px && inline_b->get_placement().width == 140_px,
                                  "inline-grid should use the intrinsic width of its explicit tracks");
    const auto align_doc  = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:200px;height:100px;grid-template-columns:200px;"
        "grid-template-rows:100px;justify-items:center;align-items:end}.item{width:40px;height:20px}</style>"
        "<div class='grid'><div class='item' id='aligned'></div></div>",
        &container);
    align_doc->render(800);
    const auto aligned  = align_doc->root()->select_one("#aligned");
    ok                 &= check(aligned && aligned->get_placement().x == 80_px && aligned->get_placement().y == 80_px &&
                                    aligned->get_placement().width == 40_px && aligned->get_placement().height == 20_px,
                                "grid item alignment should position fixed-size items within their areas");
    const auto content_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;grid-template-columns:auto 1fr;"
        "grid-template-rows:minmax(10px,100px)}</style>"
        "<div class='grid'><div id='content'>four four</div><div id='remaining'></div></div>",
        &container);
    content_doc->render(800);
    const auto content   = content_doc->root()->select_one("#content");
    const auto remaining = content_doc->root()->select_one("#remaining");
    ok &= check(content && remaining && content->get_placement().width == 72_px &&
                    remaining->get_placement().x == 72_px && remaining->get_placement().width == 228_px &&
                    remaining->get_placement().height == 100_px,
                "auto tracks should use max-content and indefinite minmax rows should reach their maximum");
    const auto distribution_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;height:120px;grid-template-columns:50px 50px;"
        "grid-template-rows:20px;justify-content:space-between;align-content:center}</style>"
        "<div class='grid'><div id='dist-a'></div><div id='dist-b'></div></div>",
        &container);
    distribution_doc->render(800);
    const auto dist_a = distribution_doc->root()->select_one("#dist-a");
    const auto dist_b = distribution_doc->root()->select_one("#dist-b");
    ok &= check(dist_a && dist_b && dist_a->get_placement().x == 0_px && dist_a->get_placement().y == 50_px &&
                    dist_b->get_placement().x == 250_px && dist_b->get_placement().y == 50_px,
                "grid content alignment should distribute columns and center rows");
    const auto margin_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:200px;height:100px;grid-template-columns:200px;"
        "grid-template-rows:100px;justify-items:center;align-items:center}.item{width:40px;height:20px;"
        "margin-left:auto;margin-top:auto}</style><div class='grid'><div class='item' id='margin'></div></div>",
        &container);
    margin_doc->render(800);
    const auto margin_item = margin_doc->root()->select_one("#margin");
    ok &= check(margin_item && margin_item->get_placement().x == 160_px && margin_item->get_placement().y == 80_px,
                "grid item auto margins should take precedence over alignment");
    const auto percent_gap_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;grid-template-rows:40px 40px;row-gap:10%}</style>"
        "<div class='grid' id='gap-grid'><div></div><div id='gap-child'></div></div>",
        &container);
    percent_gap_doc->render(800);
    const auto gap_grid  = percent_gap_doc->root()->select_one("#gap-grid");
    const auto gap_child = percent_gap_doc->root()->select_one("#gap-child");
    ok &= check(gap_grid && gap_child && gap_grid->get_placement().height == 80_px &&
                    gap_child->get_placement().y == 48_px,
                "percentage row gaps should resolve after intrinsic height without enlarging the container");
    const auto resize_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;grid-template-columns:1fr 2fr;column-gap:20px}</style>"
        "<div class='grid'><div id='resize-a'></div><div id='resize-b'></div></div>",
        &container);
    const auto resize_a = resize_doc->root()->select_one("#resize-a");
    const auto resize_b = resize_doc->root()->select_one("#resize-b");
    resize_doc->render(320);
    ok &= check(resize_a && resize_b && resize_a->get_placement().width == 100_px &&
                    resize_b->get_placement().x == 120_px && resize_b->get_placement().width == 200_px,
                "grid should size fractional columns at a narrow width");
    resize_doc->render(620);
    ok &= check(resize_a && resize_b && resize_a->get_placement().width == 200_px &&
                    resize_b->get_placement().x == 220_px && resize_b->get_placement().width == 400_px,
                "grid should recalculate fractional columns after a resize");
    const auto intrinsic_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:300px;grid-template-columns:max-content 1fr}</style>"
        "<div class='grid'><div id='max-content'>four four</div><div id='max-rest'></div></div>",
        &container);
    intrinsic_doc->render(800);
    const auto max_content   = intrinsic_doc->root()->select_one("#max-content");
    const auto max_rest      = intrinsic_doc->root()->select_one("#max-rest");
    ok                      &= check(max_content && max_rest && max_content->get_placement().width == 72_px &&
                                         max_rest->get_placement().x == 72_px,
                                     "max-content tracks should use unwrapped content width");
    const auto overflow_doc  = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:50px;grid-template-columns:max-content 1fr}</style>"
        "<div class='grid'><div id='overflow-content'>four four</div><div id='overflow-rest'></div></div>",
        &container);
    overflow_doc->render(800);
    const auto overflow_content = overflow_doc->root()->select_one("#overflow-content");
    const auto overflow_rest    = overflow_doc->root()->select_one("#overflow-rest");
    ok &= check(overflow_content && overflow_rest && overflow_content->get_placement().width == 72_px &&
                    overflow_rest->get_placement().x == 72_px,
                "max-content tracks should not shrink below their unwrapped content when overflowing");
    const auto intrinsic_inline_doc = document::createFromString(
        "<style>body{margin:0}.grid{display:inline-grid;grid-template-columns:1fr 2fr}.grid div{width:50px}</style>"
        "<div class='grid' id='intrinsic-inline'><div></div><div></div></div>",
        &container);
    intrinsic_inline_doc->render(800);
    const auto intrinsic_inline  = intrinsic_inline_doc->root()->select_one("#intrinsic-inline");
    ok                          &= check(intrinsic_inline && intrinsic_inline->get_placement().width == 150_px,
                                         "inline-grid fractional tracks should use their intrinsic flex fraction");
    const auto min_inline_doc    = document::createFromString(
        "<style>body{margin:0}.grid{display:inline-grid;grid-template-columns:min-content}</style>"
        "<div class='grid' id='min-inline'><div>four four</div></div>",
        &container);
    min_inline_doc->render(800);
    const auto min_inline    = min_inline_doc->root()->select_one("#min-inline");
    ok                      &= check(min_inline && min_inline->get_placement().width == 32_px,
                                     "inline-grid min-content tracks should use the longest unbreakable segment");
    const auto spanning_doc  = document::createFromString(
        "<style>body{margin:0}.grid{display:grid;width:120px;"
        "grid-template-columns:minmax(auto,50px) minmax(auto,200px)}.span{grid-column:span 2;width:200px}</style>"
        "<div class='grid'><div class='span'></div><div id='bounded-a'></div><div id='bounded-b'></div></div>",
        &container);
    spanning_doc->render(800);
    const auto bounded_a = spanning_doc->root()->select_one("#bounded-a");
    const auto bounded_b = spanning_doc->root()->select_one("#bounded-b");
    ok &= check(bounded_a && bounded_b && bounded_a->get_placement().width == 50_px &&
                    bounded_b->get_placement().x == 50_px && bounded_b->get_placement().width == 150_px,
                "spanning contributions should redistribute after an auto track reaches its limit");
    const auto flex_doc =
        document::createFromString("<style>body{margin:0}.flex{display:flex;flex-wrap:wrap;width:100px;height:100px;"
                                   "align-content:space-evenly}.flex div{width:100px;height:10px}</style>"
                                   "<div class='flex'><div id='flex-a'></div><div id='flex-b'></div></div>",
                                   &container);
    flex_doc->render(800);
    const auto flex_a  = flex_doc->root()->select_one("#flex-a");
    const auto flex_b  = flex_doc->root()->select_one("#flex-b");
    ok                &= check(flex_a && flex_b && std::abs(flex_a->get_placement().y.value() - 26.6667f) < 0.01f &&
                                   std::abs(flex_b->get_placement().y.value() - 63.3333f) < 0.01f,
                               "shared space-evenly parsing should preserve flex content alignment");
    return ok ? 0 : 1;
}
