#ifndef LITEHTML_CSS_GRID_H
#define LITEHTML_CSS_GRID_H

#include "css_length.h"
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace litehtml
{
    inline constexpr auto grid_auto_flow_strings = split_css_values<2>("row;column");

    enum grid_auto_flow
    {
        grid_auto_flow_row,
        grid_auto_flow_column,
        grid_auto_flow_row_dense,
        grid_auto_flow_column_dense
    };

    enum class grid_breadth_type
    {
        length,
        fraction,
        automatic,
        min_content,
        max_content,
        fit_content
    };

    enum class grid_repeat_kind
    {
        fixed,
        auto_fill,
        auto_fit
    };

    struct grid_breadth
    {
        grid_breadth_type type = grid_breadth_type::automatic;
        css_length        length;
        float             fraction = 0;
    };

    struct grid_track_size
    {
        grid_breadth min;
        grid_breadth max;
    };

    struct auto_repeat_info
    {
        bool   present  = false;
        bool   auto_fit = false;
        size_t start    = 0;
        size_t count    = 0;
    };

    class grid_track_list
    {
        struct segment
        {
            std::vector<grid_track_size>          pattern;
            size_t                                end;
            std::vector<std::vector<std::string>> line_names;
            grid_repeat_kind                      repeat_kind = grid_repeat_kind::fixed;
        };
        struct name_segment
        {
            std::vector<std::vector<std::string>> pattern;
            size_t                                count;
            bool                                  auto_fill;
        };

      public:
        bool append(std::vector<grid_track_size> pattern, size_t count = 1,
                    std::vector<std::vector<std::string>> line_names  = {},
                    grid_repeat_kind                      repeat_kind = grid_repeat_kind::fixed)
        {
            const auto limit = static_cast<size_t>(std::numeric_limits<int>::max());
            if(pattern.empty() || count == 0 || count > (limit - m_size) / pattern.size())
            {
                return false;
            }
            const auto end = m_size + pattern.size() * count;
            m_segments.push_back({std::move(pattern), end, std::move(line_names), repeat_kind});
            m_size = end;
            return true;
        }

        size_t size() const
        {
            return m_size;
        }
        bool empty() const
        {
            return m_size == 0;
        }
        bool is_subgrid() const
        {
            return m_subgrid;
        }
        void set_subgrid()
        {
            m_subgrid = true;
        }
        bool   append_subgrid_names(std::vector<std::vector<std::string>> pattern, size_t count, bool auto_fill);
        size_t subgrid_explicit_span() const;
        grid_track_list resolve_subgrid_names(size_t span) const;

        bool            has_auto_repeat() const;
        grid_track_list resolve_auto_repeat(pixel_t available, pixel_t gap, bool definite,
                                            auto_repeat_info& info) const;

        void add_line_names(size_t index, const std::vector<std::string>& names)
        {
            m_line_names.emplace_back(index, names);
        }

        std::vector<std::string> line_names_at(size_t index) const
        {
            std::vector<std::string> names;
            for(const auto& direct : m_line_names)
            {
                if(direct.first == index)
                {
                    names.insert(names.end(), direct.second.begin(), direct.second.end());
                }
            }
            size_t start = 0;
            for(const auto& part : m_segments)
            {
                if(!part.line_names.empty() && index >= start && index <= part.end)
                {
                    const auto offset = (index - start) % part.pattern.size();
                    if(index < part.end)
                    {
                        const auto& current = part.line_names[offset];
                        names.insert(names.end(), current.begin(), current.end());
                    }
                    if(index > start && offset == 0)
                    {
                        const auto& previous = part.line_names[part.pattern.size()];
                        names.insert(names.end(), previous.begin(), previous.end());
                    }
                }
                start = part.end;
            }
            return names;
        }

        bool has_line_name(size_t index, const std::string& name) const
        {
            for(const auto& direct : m_line_names)
            {
                if(direct.first == index &&
                   std::find(direct.second.begin(), direct.second.end(), name) != direct.second.end())
                {
                    return true;
                }
            }
            size_t start = 0;
            for(const auto& part : m_segments)
            {
                if(!part.line_names.empty() && index >= start && index <= part.end)
                {
                    const auto offset   = (index - start) % part.pattern.size();
                    const auto contains = [&](size_t position) {
                        const auto& names = part.line_names[position];
                        return std::find(names.begin(), names.end(), name) != names.end();
                    };
                    if((index < part.end && contains(offset)) ||
                       (index > start && offset == 0 && contains(part.pattern.size())))
                    {
                        return true;
                    }
                }
                start = part.end;
            }
            return false;
        }

        const grid_track_size& operator[](size_t index) const
        {
            auto       segment = std::lower_bound(m_segments.begin(), m_segments.end(), index,
                                                  [](const auto& entry, size_t value) { return entry.end <= value; });
            const auto start   = segment == m_segments.begin() ? 0 : (segment - 1)->end;
            return segment->pattern[(index - start) % segment->pattern.size()];
        }

        template <typename Function> void for_each_definition(Function function)
        {
            for(auto& segment : m_segments)
            {
                for(auto& track : segment.pattern)
                {
                    function(track);
                }
            }
        }

      private:
        std::vector<segment>                                     m_segments;
        std::vector<name_segment>                                m_name_segments;
        std::vector<std::pair<size_t, std::vector<std::string>>> m_line_names;
        size_t                                                   m_size    = 0;
        bool                                                     m_subgrid = false;
    };

    enum class grid_line_type
    {
        automatic,
        line,
        span
    };

    struct grid_line
    {
        grid_line_type type      = grid_line_type::automatic;
        int            value     = 0;
        std::string    name      = {};
        bool           name_only = false;
    };

    struct grid_placement
    {
        grid_line start;
        grid_line end;
    };

    struct resolved_grid_axis
    {
        std::optional<int> start;
        int                span = 1;
    };

    struct grid_item_placement
    {
        grid_placement columns;
        grid_placement rows;
        int            order = 0;
    };

    struct grid_area
    {
        int column_start = 0;
        int column_end   = 0;
        int row_start    = 0;
        int row_end      = 0;
    };

    struct grid_template_areas
    {
        int                                            rows    = 0;
        int                                            columns = 0;
        std::vector<std::pair<std::string, grid_area>> areas;
    };

    bool parse_grid_track_list(const css_token_vector& tokens, grid_track_list& tracks);
    bool parse_grid_template_areas(const css_token_vector& tokens, grid_template_areas& areas);
    bool parse_grid_line(const css_token_vector& tokens, grid_line& line);
    bool parse_grid_placement(const css_token_vector& tokens, grid_placement& placement);
    bool resolve_grid_axis(const grid_placement& placement, int explicit_tracks, resolved_grid_axis& result,
                           const grid_track_list* names = nullptr, bool bounded = false);
    bool place_grid_items(const std::vector<grid_item_placement>& items, int explicit_columns, int explicit_rows,
                          grid_auto_flow flow, std::vector<grid_area>& areas, grid_area& bounds,
                          const grid_track_list* column_names = nullptr, const grid_track_list* row_names = nullptr,
                          bool bounded_columns = false, bool bounded_rows = false);
    bool distribute_grid_fractions(pixel_t available, const std::vector<float>& fractions, std::vector<pixel_t>& sizes);
} // namespace litehtml

#endif // LITEHTML_CSS_GRID_H
