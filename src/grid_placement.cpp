#include "css_grid.h"
#include <algorithm>
#include <map>
#include <numeric>

namespace litehtml
{
    namespace
    {
        constexpr int max_tracks = 1024;

        struct placed_item
        {
            resolved_grid_axis major;
            resolved_grid_axis minor;
        };

        struct rectangle
        {
            int major_start;
            int major_end;
            int minor_start;
            int minor_end;
        };

        bool overlaps(const rectangle& first, const rectangle& second)
        {
            return first.major_start < second.major_end && second.major_start < first.major_end &&
                   first.minor_start < second.minor_end && second.minor_start < first.minor_end;
        }
    } // namespace

    bool place_grid_items(const std::vector<grid_item_placement>& items, int explicit_columns, int explicit_rows,
                          grid_auto_flow flow, std::vector<grid_area>& areas, grid_area& bounds,
                          const grid_track_list* column_names, const grid_track_list* row_names, bool bounded_columns,
                          bool bounded_rows)
    {
        if(explicit_columns < 0 || explicit_columns > max_tracks || explicit_rows < 0 || explicit_rows > max_tracks ||
           (bounded_columns && explicit_columns == 0) || (bounded_rows && explicit_rows == 0))
        {
            return false;
        }
        const bool               column_flow = flow == grid_auto_flow_column || flow == grid_auto_flow_column_dense;
        const bool               dense       = flow == grid_auto_flow_row_dense || flow == grid_auto_flow_column_dense;
        const int                explicit_major = column_flow ? explicit_columns : explicit_rows;
        const int                explicit_minor = column_flow ? explicit_rows : explicit_columns;
        const bool               bounded_major  = column_flow ? bounded_columns : bounded_rows;
        const bool               bounded_minor  = column_flow ? bounded_rows : bounded_columns;
        std::vector<placed_item> resolved(items.size());
        std::vector<rectangle>   occupied;
        std::vector<grid_area>   result(items.size());
        std::vector<size_t>      order(items.size());
        std::iota(order.begin(), order.end(), 0);
        std::stable_sort(order.begin(), order.end(),
                         [&](size_t first, size_t second) { return items[first].order < items[second].order; });
        int major_min = 0, major_max = explicit_major;
        int minor_min = 0, minor_max = explicit_minor;
        int widest_span = 1;
        for(size_t index : order)
        {
            const auto& major = column_flow ? items[index].columns : items[index].rows;
            const auto& minor = column_flow ? items[index].rows : items[index].columns;
            if(!resolve_grid_axis(major, explicit_major, resolved[index].major, column_flow ? column_names : row_names,
                                  bounded_major) ||
               !resolve_grid_axis(minor, explicit_minor, resolved[index].minor, column_flow ? row_names : column_names,
                                  bounded_minor))
            {
                return false;
            }
            const auto& item = resolved[index];
            if(item.major.start)
            {
                major_min = std::min(major_min, *item.major.start);
                major_max = std::max(major_max, *item.major.start + item.major.span);
            }
            if(item.minor.start)
            {
                minor_min = std::min(minor_min, *item.minor.start);
                minor_max = std::max(minor_max, *item.minor.start + item.minor.span);
            }
            widest_span = std::max(widest_span, item.minor.span);
        }
        if(!items.empty())
        {
            minor_max = std::max(minor_max, minor_min + widest_span);
        }

        auto place = [&](size_t index, int major, int minor) {
            const auto& item = resolved[index];
            rectangle   box{major, major + item.major.span, minor, minor + item.minor.span};
            const auto  occupied_box = box;
            auto        clamp_axis   = [](int& first, int& last, int count) {
                if(last <= 0)
                {
                    first = 0;
                    last  = 1;
                } else if(first >= count)
                {
                    first = count - 1;
                    last  = count;
                } else
                {
                    first = std::max(first, 0);
                    last  = std::min(last, count);
                }
            };
            if(bounded_major)
            {
                clamp_axis(box.major_start, box.major_end, explicit_major);
            }
            if(bounded_minor)
            {
                clamp_axis(box.minor_start, box.minor_end, explicit_minor);
            }
            major_min = std::min(major_min, box.major_start);
            major_max = std::max(major_max, box.major_end);
            minor_min = std::min(minor_min, box.minor_start);
            minor_max = std::max(minor_max, box.minor_end);
            if(major_max - major_min > max_tracks || minor_max - minor_min > max_tracks)
            {
                return false;
            }
            occupied.push_back(occupied_box);
            result[index] = column_flow ? grid_area{box.major_start, box.major_end, box.minor_start, box.minor_end}
                                        : grid_area{box.minor_start, box.minor_end, box.major_start, box.major_end};
            return true;
        };
        auto vacant = [&](const rectangle& box) {
            return std::none_of(occupied.begin(), occupied.end(),
                                [&](const rectangle& other) { return overlaps(box, other); });
        };
        for(size_t index : order)
        {
            const auto& item = resolved[index];
            if(item.major.start && item.minor.start && !place(index, *item.major.start, *item.minor.start))
            {
                return false;
            }
        }
        std::map<int, int> locked_cursors;
        for(size_t index : order)
        {
            const auto& item = resolved[index];
            if(!item.major.start || item.minor.start)
            {
                continue;
            }
            auto cursor = locked_cursors.emplace(*item.major.start, minor_min).first;
            int  minor  = dense ? minor_min : cursor->second;
            while(minor - minor_min <= max_tracks - item.minor.span &&
                  !vacant({*item.major.start, *item.major.start + item.major.span, minor, minor + item.minor.span}))
            {
                ++minor;
            }
            if(minor - minor_min > max_tracks - item.minor.span || !place(index, *item.major.start, minor))
            {
                return false;
            }
            cursor->second = minor + item.minor.span;
        }
        int cursor_major = major_min;
        int cursor_minor = minor_min;
        for(size_t index : order)
        {
            const auto& item = resolved[index];
            if(item.major.start)
            {
                continue;
            }
            if(dense)
            {
                cursor_major = major_min;
                cursor_minor = minor_min;
            }
            if(item.minor.start)
            {
                if(*item.minor.start < cursor_minor)
                {
                    ++cursor_major;
                }
                cursor_minor = *item.minor.start;
                while(cursor_major - major_min <= max_tracks - item.major.span &&
                      !vacant(
                          {cursor_major, cursor_major + item.major.span, cursor_minor, cursor_minor + item.minor.span}))
                {
                    ++cursor_major;
                }
            } else
            {
                while(cursor_major - major_min <= max_tracks - item.major.span)
                {
                    if(cursor_minor + item.minor.span > minor_max)
                    {
                        ++cursor_major;
                        cursor_minor = minor_min;
                    } else if(vacant({cursor_major, cursor_major + item.major.span, cursor_minor,
                                      cursor_minor + item.minor.span}))
                    {
                        break;
                    } else
                    {
                        ++cursor_minor;
                    }
                }
            }
            if(cursor_major - major_min > max_tracks - item.major.span || !place(index, cursor_major, cursor_minor))
            {
                return false;
            }
        }
        if(major_max - major_min > max_tracks || minor_max - minor_min > max_tracks)
        {
            return false;
        }
        bounds = column_flow ? grid_area{major_min, major_max, minor_min, minor_max}
                             : grid_area{minor_min, minor_max, major_min, major_max};
        areas  = std::move(result);
        return true;
    }
} // namespace litehtml
