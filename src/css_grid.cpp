#include "css_grid.h"
#include "html.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>

namespace litehtml
{
    namespace
    {
        constexpr size_t max_grid_tracks = 1024;

        bool parse_breadth(const css_token& token, grid_breadth& breadth, bool allow_fraction)
        {
            if(is_one_of(token.type, NUMBER, DIMENSION, PERCENTAGE) && !std::isfinite(token.n.number))
            {
                return false;
            }
            if(token.type == DIMENSION && lowcase(token.unit()) == "fr")
            {
                if(!allow_fraction || token.n.number < 0)
                {
                    return false;
                }
                breadth.type     = grid_breadth_type::fraction;
                breadth.fraction = token.n.number;
                return true;
            }
            if(breadth.length.from_token(token, f_length_percentage | f_positive))
            {
                breadth.type = grid_breadth_type::length;
                return true;
            }
            const auto keyword = token.ident();
            if(keyword == "auto")
            {
                breadth.type = grid_breadth_type::automatic;
            } else if(keyword == "min-content")
            {
                breadth.type = grid_breadth_type::min_content;
            } else if(keyword == "max-content")
            {
                breadth.type = grid_breadth_type::max_content;
            } else
            {
                return false;
            }
            return true;
        }

        bool parse_track(const css_token& token, grid_track_size& track)
        {
            if(token.type == CV_FUNCTION && lowcase(token.name()) == "fit-content")
            {
                if(token.value.size() != 1 || !parse_breadth(token.value[0], track.max, false) ||
                   track.max.type != grid_breadth_type::length)
                {
                    return false;
                }
                track.max.type = grid_breadth_type::fit_content;
                return true;
            }
            if(token.type == CV_FUNCTION && lowcase(token.name()) == "minmax")
            {
                return token.value.size() == 3 && token.value[1].type == COMMA &&
                       parse_breadth(token.value[0], track.min, false) &&
                       parse_breadth(token.value[2], track.max, true);
            }
            if(!parse_breadth(token, track.max, true))
            {
                return false;
            }
            if(track.max.type != grid_breadth_type::fraction)
            {
                track.min = track.max;
            }
            return true;
        }

        bool parse_line_names(const css_token& token, std::vector<std::string>& names)
        {
            if(token.type != SQUARE_BLOCK)
            {
                return false;
            }
            for(const auto& part : token.value)
            {
                if(part.type != IDENT || is_one_of(part.ident(), "auto", "span", "inherit", "initial", "unset",
                                                   "revert", "revert-layer", "default"))
                {
                    return false;
                }
                names.push_back(part.name());
            }
            return true;
        }

        bool append_track(const css_token& token, grid_track_list& tracks, bool& has_auto_repeat)
        {
            if(token.type == CV_FUNCTION && lowcase(token.name()) == "repeat")
            {
                if(token.value.size() < 3 || token.value[1].type != COMMA)
                {
                    return false;
                }
                grid_repeat_kind repeat_kind = grid_repeat_kind::fixed;
                size_t           count       = 1;
                if(token.value[0].type == NUMBER)
                {
                    if(token.value[0].n.number_type != css_number_integer || !std::isfinite(token.value[0].n.number) ||
                       token.value[0].n.number < 1 ||
                       static_cast<double>(token.value[0].n.number) > std::numeric_limits<int>::max())
                    {
                        return false;
                    }
                    count = static_cast<size_t>(token.value[0].n.number);
                } else if(token.value[0].type == IDENT)
                {
                    const auto keyword = token.value[0].ident();
                    if(keyword == "auto-fill")
                    {
                        repeat_kind = grid_repeat_kind::auto_fill;
                    } else if(keyword == "auto-fit")
                    {
                        repeat_kind = grid_repeat_kind::auto_fit;
                    } else
                    {
                        return false;
                    }
                    if(has_auto_repeat)
                    {
                        return false;
                    }
                    has_auto_repeat = true;
                } else
                {
                    return false;
                }
                std::vector<grid_track_size>          repeated;
                std::vector<std::vector<std::string>> line_names(1);
                bool                                  previous_names = false;
                for(size_t index = 2; index < token.value.size(); ++index)
                {
                    if(token.value[index].type == SQUARE_BLOCK)
                    {
                        if(previous_names || !parse_line_names(token.value[index], line_names.back()))
                        {
                            return false;
                        }
                        previous_names = true;
                        continue;
                    }
                    grid_track_size track;
                    if(!parse_track(token.value[index], track))
                    {
                        return false;
                    }
                    repeated.push_back(track);
                    line_names.emplace_back();
                    previous_names = false;
                }
                if(repeat_kind != grid_repeat_kind::fixed)
                {
                    for(const auto& track : repeated)
                    {
                        if(track.min.type != grid_breadth_type::length && track.max.type != grid_breadth_type::length)
                        {
                            return false;
                        }
                    }
                }
                return tracks.append(std::move(repeated), count, std::move(line_names), repeat_kind);
            }
            grid_track_size track;
            if(!parse_track(token, track))
            {
                return false;
            }
            return tracks.append({track});
        }

        bool is_definite_breadth(const grid_breadth& breadth, bool definite_container)
        {
            return breadth.type == grid_breadth_type::length &&
                   (!breadth.length.has_percentage() || definite_container);
        }

        pixel_t track_fixed_size(const grid_track_size& track, pixel_t available, bool definite_container)
        {
            if(is_definite_breadth(track.max, definite_container))
            {
                const auto minimum = is_definite_breadth(track.min, definite_container)
                                         ? track.min.length.calc_percent(available)
                                         : 0_px;
                return std::max(minimum, std::max(0_px, track.max.length.calc_percent(available)));
            }
            if(track.min.type == grid_breadth_type::length)
            {
                if(track.min.length.has_percentage() && !definite_container)
                {
                    return 0_px;
                }
                return std::max(0_px, track.min.length.calc_percent(available));
            }
            return 0_px;
        }
    } // namespace

    bool parse_grid_track_list(const css_token_vector& tokens, grid_track_list& tracks)
    {
        if(tokens.empty())
        {
            return false;
        }
        grid_track_list parsed;
        if(tokens.size() == 1 && tokens[0].ident() == "none")
        {
            tracks = std::move(parsed);
            return true;
        }
        if(tokens[0].ident() == "subgrid")
        {
            parsed.set_subgrid();
            bool has_auto_fill = false;
            for(size_t index = 1; index < tokens.size(); ++index)
            {
                const auto&                           token = tokens[index];
                std::vector<std::vector<std::string>> pattern;
                size_t                                count     = 1;
                bool                                  auto_fill = false;
                if(token.type == SQUARE_BLOCK)
                {
                    pattern.emplace_back();
                    if(!parse_line_names(token, pattern.back()))
                    {
                        return false;
                    }
                } else if(token.type == CV_FUNCTION && lowcase(token.name()) == "repeat" && token.value.size() >= 3 &&
                          token.value[1].type == COMMA)
                {
                    const auto& repetition = token.value[0];
                    if(repetition.type == NUMBER && repetition.n.number_type == css_number_integer &&
                       std::isfinite(repetition.n.number) && repetition.n.number >= 1 &&
                       static_cast<double>(repetition.n.number) <= std::numeric_limits<int>::max())
                    {
                        count = static_cast<size_t>(repetition.n.number);
                    } else if(repetition.ident() == "auto-fill" && !has_auto_fill)
                    {
                        auto_fill     = true;
                        has_auto_fill = true;
                    } else
                    {
                        return false;
                    }
                    for(size_t name_index = 2; name_index < token.value.size(); ++name_index)
                    {
                        pattern.emplace_back();
                        if(!parse_line_names(token.value[name_index], pattern.back()))
                        {
                            return false;
                        }
                    }
                } else
                {
                    return false;
                }
                if(!parsed.append_subgrid_names(std::move(pattern), count, auto_fill))
                {
                    return false;
                }
            }
            tracks = std::move(parsed);
            return true;
        }
        bool previous_names  = false;
        bool has_auto_repeat = false;
        for(const auto& token : tokens)
        {
            if(token.type == SQUARE_BLOCK)
            {
                std::vector<std::string> names;
                if(previous_names || !parse_line_names(token, names))
                {
                    return false;
                }
                parsed.add_line_names(parsed.size(), names);
                previous_names = true;
            } else if(!append_track(token, parsed, has_auto_repeat))
            {
                return false;
            } else
            {
                previous_names = false;
            }
        }
        if(parsed.empty())
        {
            return false;
        }
        tracks = std::move(parsed);
        return true;
    }

    bool grid_track_list::append_subgrid_names(std::vector<std::vector<std::string>> pattern, size_t count,
                                               bool auto_fill)
    {
        if(pattern.empty() || count == 0 ||
           count > static_cast<size_t>(std::numeric_limits<int>::max()) / pattern.size())
        {
            return false;
        }
        m_name_segments.push_back({std::move(pattern), count, auto_fill});
        return true;
    }

    size_t grid_track_list::subgrid_explicit_span() const
    {
        size_t     lines = 0;
        const auto limit = static_cast<size_t>(std::numeric_limits<int>::max());
        for(const auto& part : m_name_segments)
        {
            if(!part.auto_fill)
            {
                const auto size = part.count * part.pattern.size();
                lines           = size > limit - lines ? limit : lines + size;
            }
        }
        return lines > 1 ? lines - 1 : 1;
    }

    grid_track_list grid_track_list::resolve_subgrid_names(size_t span) const
    {
        grid_track_list resolved;
        const auto      line_count  = std::min(span, max_grid_tracks) + 1;
        size_t          fixed_count = 0;
        for(const auto& part : m_name_segments)
        {
            if(!part.auto_fill)
            {
                fixed_count =
                    std::min(line_count, fixed_count + std::min(line_count, part.count * part.pattern.size()));
            }
        }
        size_t index = 0;
        for(const auto& part : m_name_segments)
        {
            const auto count = part.auto_fill ? (line_count - fixed_count) / part.pattern.size() : part.count;
            for(size_t repetition = 0; repetition < count && index < line_count; ++repetition)
            {
                for(const auto& names : part.pattern)
                {
                    if(index == line_count)
                    {
                        break;
                    }
                    resolved.add_line_names(index++, names);
                }
            }
        }
        return resolved;
    }

    bool grid_track_list::has_auto_repeat() const
    {
        return std::any_of(m_segments.begin(), m_segments.end(),
                           [](const segment& entry) { return entry.repeat_kind != grid_repeat_kind::fixed; });
    }

    grid_track_list grid_track_list::resolve_auto_repeat(pixel_t available, pixel_t gap, bool definite,
                                                         auto_repeat_info& info) const
    {
        info = {};
        grid_track_list resolved;

        const segment* auto_segment = nullptr;
        size_t         fixed_tracks = 0;
        double         fixed_size   = 0;
        size_t         running      = 0;
        for(const auto& entry : m_segments)
        {
            if(entry.repeat_kind == grid_repeat_kind::fixed)
            {
                const auto count     = (entry.end - running) / entry.pattern.size();
                fixed_tracks        += count * entry.pattern.size();
                double pattern_size  = 0;
                for(const auto& track : entry.pattern)
                {
                    pattern_size += track_fixed_size(track, available, definite).value();
                }
                fixed_size += pattern_size * static_cast<double>(count);
            } else if(auto_segment == nullptr)
            {
                auto_segment = &entry;
            }
            running = entry.end;
        }

        size_t count = 1;
        if(auto_segment != nullptr)
        {
            if(definite)
            {
                double pattern_size = 0;
                for(const auto& track : auto_segment->pattern)
                {
                    pattern_size += track_fixed_size(track, available, definite).value();
                }
                const double per_repetition =
                    pattern_size + static_cast<double>(auto_segment->pattern.size()) * gap.value();
                if(per_repetition <= 0)
                {
                    count = max_grid_tracks;
                } else
                {
                    const double numerator = available.value() + gap.value() - fixed_size -
                                             (static_cast<double>(fixed_tracks) - 1) * gap.value();
                    const auto   quotient  = static_cast<long long>(std::floor(numerator / per_repetition));
                    count                  = quotient < 1 ? 1 : static_cast<size_t>(quotient);
                    if(count > max_grid_tracks)
                    {
                        count = max_grid_tracks;
                    }
                }
                const auto remaining = max_grid_tracks > fixed_tracks ? max_grid_tracks - fixed_tracks : 0;
                count                = std::min(count, remaining / auto_segment->pattern.size());
                if(count < 1)
                {
                    count = 1;
                }
            }
        }

        running = 0;
        for(const auto& entry : m_segments)
        {
            if(entry.repeat_kind == grid_repeat_kind::fixed)
            {
                const auto segment_count = (entry.end - running) / entry.pattern.size();
                resolved.append(entry.pattern, segment_count, entry.line_names);
            } else
            {
                resolved.append(entry.pattern, count, entry.line_names);
                info = {true, entry.repeat_kind == grid_repeat_kind::auto_fit, running, count * entry.pattern.size()};
            }
            running = entry.end;
        }

        for(const auto& names : m_line_names)
        {
            auto mapped = names.first;
            if(auto_segment != nullptr && info.present && mapped > info.start)
            {
                mapped += (count - 1) * auto_segment->pattern.size();
            }
            resolved.add_line_names(mapped, names.second);
        }
        return resolved;
    }

    bool parse_grid_template_areas(const css_token_vector& tokens, grid_template_areas& areas)
    {
        if(tokens.empty())
        {
            return false;
        }
        grid_template_areas parsed;
        if(tokens.size() == 1 && tokens[0].ident() == "none")
        {
            areas = std::move(parsed);
            return true;
        }
        std::map<std::string, int> counts;
        for(const auto& token : tokens)
        {
            if(token.type != STRING || parsed.rows == static_cast<int>(max_grid_tracks))
            {
                return false;
            }
            const auto& text   = token.str();
            int         column = 0;
            size_t      index  = 0;
            while(index < text.size())
            {
                const auto character = static_cast<unsigned char>(text[index]);
                if(character == ' ' || character == '\t' || character == '\n' || character == '\r' || character == '\f')
                {
                    ++index;
                    continue;
                }
                if(column == static_cast<int>(max_grid_tracks))
                {
                    return false;
                }
                const auto is_name = [](unsigned char ch) {
                    return ch >= 128 || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                           (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
                };
                const auto start = index;
                if(character == '.')
                {
                    while(index < text.size() && text[index] == '.')
                    {
                        ++index;
                    }
                } else if(is_name(character))
                {
                    while(index < text.size() && is_name(static_cast<unsigned char>(text[index])))
                    {
                        ++index;
                    }
                    const auto name  = text.substr(start, index - start);
                    auto       found = std::find_if(parsed.areas.begin(), parsed.areas.end(),
                                                    [&](const auto& entry) { return entry.first == name; });
                    if(found == parsed.areas.end())
                    {
                        parsed.areas.emplace_back(name, grid_area{column, column + 1, parsed.rows, parsed.rows + 1});
                        found = parsed.areas.end() - 1;
                    }
                    auto& area        = found->second;
                    area.column_start = std::min(area.column_start, column);
                    area.column_end   = std::max(area.column_end, column + 1);
                    area.row_start    = std::min(area.row_start, parsed.rows);
                    area.row_end      = std::max(area.row_end, parsed.rows + 1);
                    ++counts[name];
                } else
                {
                    return false;
                }
                ++column;
            }
            if(column == 0 || (parsed.rows > 0 && column != parsed.columns))
            {
                return false;
            }
            parsed.columns = column;
            ++parsed.rows;
        }
        for(const auto& entry : parsed.areas)
        {
            const auto& area = entry.second;
            if(counts[entry.first] != (area.column_end - area.column_start) * (area.row_end - area.row_start))
            {
                return false;
            }
        }
        areas = std::move(parsed);
        return true;
    }

    bool parse_grid_line(const css_token_vector& tokens, grid_line& line)
    {
        if(tokens.size() == 1 && tokens[0].ident() == "auto")
        {
            line = grid_line{};
            return true;
        }
        bool        has_span   = false;
        bool        has_number = false;
        int         number     = 0;
        std::string name;
        for(const auto& token : tokens)
        {
            if(token.ident() == "span" && !has_span)
            {
                has_span = true;
            } else if(token.type == NUMBER && token.n.number_type == css_number_integer && !has_number &&
                      std::isfinite(token.n.number) &&
                      static_cast<double>(token.n.number) >= std::numeric_limits<int>::min() &&
                      static_cast<double>(token.n.number) <= std::numeric_limits<int>::max())
            {
                number     = static_cast<int>(token.n.number);
                has_number = true;
            } else if(token.type == IDENT && name.empty() &&
                      !is_one_of(token.ident(), "auto", "span", "inherit", "initial", "unset", "revert", "revert-layer",
                                 "default"))
            {
                name = token.name();
            } else
            {
                return false;
            }
        }
        if((!has_number && name.empty()) || (has_number && number == 0) || (has_span && number < 0))
        {
            return false;
        }
        grid_line parsed;
        parsed.type      = has_span ? grid_line_type::span : grid_line_type::line;
        parsed.value     = has_number ? number : 1;
        parsed.name      = std::move(name);
        parsed.name_only = !has_span && !has_number;
        line             = parsed;
        return true;
    }

    bool parse_grid_placement(const css_token_vector& tokens, grid_placement& placement)
    {
        auto separator =
            std::find_if(tokens.begin(), tokens.end(), [](const css_token& token) { return token.ch == '/'; });
        grid_placement parsed;
        if(!parse_grid_line(css_token_vector(tokens.begin(), separator), parsed.start))
        {
            return false;
        }
        if(separator != tokens.end() && !parse_grid_line(css_token_vector(separator + 1, tokens.end()), parsed.end))
        {
            return false;
        }
        if(separator == tokens.end() && parsed.start.name_only)
        {
            parsed.end = parsed.start;
        }
        placement = parsed;
        return true;
    }

    bool resolve_grid_axis(const grid_placement& placement, int explicit_tracks, resolved_grid_axis& result,
                           const grid_track_list* names, bool bounded)
    {
        if(explicit_tracks < 0 || explicit_tracks > static_cast<int>(max_grid_tracks) ||
           (bounded && explicit_tracks == 0))
        {
            return false;
        }
        auto resolve_line = [explicit_tracks, names](const grid_line& line, bool is_start) -> std::optional<int64_t> {
            if(line.type != grid_line_type::line || line.value == 0)
            {
                return std::nullopt;
            }
            if(line.name.empty())
            {
                return line.value > 0 ? static_cast<int64_t>(line.value) - 1
                                      : static_cast<int64_t>(explicit_tracks) + 1 + line.value;
            }
            auto name = line.name;
            if(line.name_only && names)
            {
                const auto area_name = name + (is_start ? "-start" : "-end");
                for(int index = 0; index <= explicit_tracks; ++index)
                {
                    if(names->has_line_name(static_cast<size_t>(index), area_name))
                    {
                        name = area_name;
                        break;
                    }
                }
            }
            int64_t remaining = line.value > 0 ? static_cast<int64_t>(line.value) : -static_cast<int64_t>(line.value);
            if(line.value > 0)
            {
                for(int index = 0; index <= explicit_tracks; ++index)
                {
                    if(names && names->has_line_name(static_cast<size_t>(index), name) && --remaining == 0)
                    {
                        return index;
                    }
                }
                return static_cast<int64_t>(explicit_tracks) + remaining;
            }
            for(int index = explicit_tracks; index >= 0; --index)
            {
                if(names && names->has_line_name(static_cast<size_t>(index), name) && --remaining == 0)
                {
                    return index;
                }
            }
            return -remaining;
        };
        auto resolve_span = [explicit_tracks, names](int64_t anchor, const grid_line& line, bool forward) {
            int64_t remaining = line.value;
            if(line.name.empty())
            {
                return forward ? anchor + remaining : anchor - remaining;
            }
            if(forward)
            {
                int64_t index = std::max<int64_t>(anchor + 1, 0);
                for(; index <= explicit_tracks; ++index)
                {
                    if(names && names->has_line_name(static_cast<size_t>(index), line.name) && --remaining == 0)
                    {
                        return index;
                    }
                }
                return std::max<int64_t>(anchor, explicit_tracks) + remaining;
            }
            int64_t index = std::min<int64_t>(anchor - 1, explicit_tracks);
            for(; index >= 0; --index)
            {
                if(names && names->has_line_name(static_cast<size_t>(index), line.name) && --remaining == 0)
                {
                    return index;
                }
            }
            return std::min<int64_t>(anchor, 0) - remaining;
        };
        auto    start = resolve_line(placement.start, true);
        auto    end   = resolve_line(placement.end, false);
        int64_t span  = 1;
        if(start && end)
        {
            if(*start > *end)
            {
                std::swap(start, end);
            }
            if(*start == *end)
            {
                ++*end;
            }
            span = *end - *start;
        } else if(placement.start.type == grid_line_type::span && end)
        {
            start = resolve_span(*end, placement.start, false);
            span  = *end - *start;
        } else if(placement.end.type == grid_line_type::span && start)
        {
            const auto resolved_end = resolve_span(*start, placement.end, true);
            span                    = resolved_end - *start;
        } else if(placement.start.type == grid_line_type::span)
        {
            span = placement.start.name.empty() ? placement.start.value : 1;
        } else if(placement.end.type == grid_line_type::span)
        {
            span = placement.end.name.empty() ? placement.end.value : 1;
        }
        if(end && !start)
        {
            start = *end - span;
        }
        if(bounded)
        {
            if(start)
            {
                const auto area_end = *start + span;
                if(area_end <= 0)
                {
                    *start = 0;
                    span   = 1;
                } else if(*start >= explicit_tracks)
                {
                    *start = explicit_tracks - 1;
                    span   = 1;
                } else
                {
                    *start = std::max<int64_t>(0, *start);
                    span   = std::min<int64_t>(explicit_tracks, area_end) - *start;
                }
            } else
            {
                span = std::min<int64_t>(span, explicit_tracks);
            }
        }
        if(span < 1 || span > static_cast<int64_t>(max_grid_tracks))
        {
            return false;
        }
        resolved_grid_axis resolved;
        if(start)
        {
            if(*start < -static_cast<int64_t>(max_grid_tracks) || *start + span > static_cast<int64_t>(max_grid_tracks))
            {
                return false;
            }
            resolved.start = static_cast<int>(*start);
        }
        resolved.span = static_cast<int>(span);
        result        = resolved;
        return true;
    }
} // namespace litehtml
