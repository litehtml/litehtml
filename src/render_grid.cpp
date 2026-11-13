#include "render_grid.h"
#include <algorithm>
#include <functional>

namespace litehtml
{
    namespace
    {
        grid_item_placement item_placement(const std::shared_ptr<render_item>& item)
        {
            grid_item_placement placement{
                {item->css().get_grid_column_start(), item->css().get_grid_column_end()},
                {item->css().get_grid_row_start(),    item->css().get_grid_row_end()   },
                item->css().get_order()
            };
            if(is_one_of(item->css().get_display(), display_grid, display_inline_grid))
            {
                auto apply_subgrid_span = [](grid_placement& axis, const grid_track_list& tracks) {
                    if(!tracks.is_subgrid())
                    {
                        return;
                    }
                    const auto span = static_cast<int>(std::min<size_t>(tracks.subgrid_explicit_span(), 1024));
                    if(axis.start.type != grid_line_type::span && axis.end.type == grid_line_type::automatic)
                    {
                        axis.end = {grid_line_type::span, span};
                    } else if(axis.start.type == grid_line_type::automatic && axis.end.type == grid_line_type::line)
                    {
                        axis.start = {grid_line_type::span, span};
                    }
                };
                apply_subgrid_span(placement.columns, item->css().get_grid_template_columns());
                apply_subgrid_span(placement.rows, item->css().get_grid_template_rows());
            }
            return placement;
        }

        void add_area_line_names(grid_track_list& columns, grid_track_list& rows, const grid_template_areas& areas)
        {
            for(const auto& entry : areas.areas)
            {
                const auto& area  = entry.second;
                const auto  start = entry.first + "-start";
                const auto  end   = entry.first + "-end";
                columns.add_line_names(static_cast<size_t>(area.column_start), {start});
                columns.add_line_names(static_cast<size_t>(area.column_end), {end});
                rows.add_line_names(static_cast<size_t>(area.row_start), {start});
                rows.add_line_names(static_cast<size_t>(area.row_end), {end});
            }
        }

        struct track_state
        {
            grid_track_size definition;
            pixel_t         size;
            pixel_t         target;
        };

        pixel_t breadth_length(const grid_breadth& breadth, pixel_t available, bool definite)
        {
            if((breadth.type != grid_breadth_type::length && breadth.type != grid_breadth_type::fit_content) ||
               (breadth.length.has_percentage() && !definite))
            {
                return 0_px;
            }
            return breadth.length.calc_percent(available);
        }

        std::vector<track_state> make_tracks(const grid_track_list& explicit_tracks, const grid_track_list& implicit,
                                             int first, int last, pixel_t available, bool definite)
        {
            std::vector<track_state> tracks;
            const grid_track_size    auto_track{};
            for(int index = first; index < last; ++index)
            {
                const grid_track_size* definition = &auto_track;
                if(index >= 0 && index < static_cast<int>(explicit_tracks.size()))
                {
                    definition = &explicit_tracks[static_cast<size_t>(index)];
                } else if(!implicit.empty())
                {
                    const auto pattern_size  = static_cast<int>(implicit.size());
                    const auto pattern_index = index < 0
                                                   ? (index % pattern_size + pattern_size) % pattern_size
                                                   : (index - static_cast<int>(explicit_tracks.size())) % pattern_size;
                    definition               = &implicit[static_cast<size_t>(pattern_index)];
                }
                auto initial = breadth_length(definition->min, available, definite);
                tracks.push_back({*definition, initial, initial});
            }
            return tracks;
        }

        void add_contribution(std::vector<track_state>& tracks, int first, int last, pixel_t contribution, pixel_t gap,
                              pixel_t available, bool definite, bool max_content_only = false,
                              bool max_intrinsic = false)
        {
            for(int iteration = first; iteration < last; ++iteration)
            {
                pixel_t current  = gap * std::max(0, last - first - 1);
                int     growable = 0;
                for(int index = first; index < last; ++index)
                {
                    auto& track     = tracks[static_cast<size_t>(index)];
                    current        += track.size;
                    auto type       = track.definition.min.type;
                    bool intrinsic  = type != grid_breadth_type::length ||
                                      (track.definition.min.length.has_percentage() && !definite);
                    bool eligible   = intrinsic && (!max_content_only || type == grid_breadth_type::max_content) &&
                                      (!max_intrinsic || track.definition.max.type != grid_breadth_type::min_content);
                    bool capped = type == grid_breadth_type::automatic &&
                                  (track.definition.max.type == grid_breadth_type::length ||
                                   (max_intrinsic && track.definition.max.type == grid_breadth_type::fit_content)) &&
                                  (!track.definition.max.length.has_percentage() || definite) &&
                                  track.size >= breadth_length(track.definition.max, available, definite);
                    if(eligible && !capped)
                    {
                        ++growable;
                    }
                }
                if(growable == 0 || current >= contribution)
                {
                    return;
                }
                auto addition = (contribution - current) / growable;
                for(int index = first; index < last; ++index)
                {
                    auto& track     = tracks[static_cast<size_t>(index)];
                    auto  type      = track.definition.min.type;
                    bool  intrinsic = type != grid_breadth_type::length ||
                                      (track.definition.min.length.has_percentage() && !definite);
                    if(!intrinsic || (max_content_only && type != grid_breadth_type::max_content) ||
                       (max_intrinsic && track.definition.max.type == grid_breadth_type::min_content))
                    {
                        continue;
                    }
                    if(type == grid_breadth_type::automatic &&
                       (track.definition.max.type == grid_breadth_type::length ||
                        (max_intrinsic && track.definition.max.type == grid_breadth_type::fit_content)) &&
                       (!track.definition.max.length.has_percentage() || definite))
                    {
                        auto limit = breadth_length(track.definition.max, available, definite);
                        track.size = std::min(track.size + addition, std::max(track.size, limit));
                    } else
                    {
                        track.size += addition;
                    }
                }
            }
        }

        void add_max_contribution(std::vector<track_state>& tracks, int first, int last, pixel_t contribution,
                                  pixel_t gap, pixel_t available, bool definite, bool max_content_only)
        {
            for(int iteration = first; iteration < last; ++iteration)
            {
                pixel_t current  = gap * std::max(0, last - first - 1);
                int     growable = 0;
                for(int index = first; index < last; ++index)
                {
                    auto& track    = tracks[static_cast<size_t>(index)];
                    track.target   = std::max(track.target, track.size);
                    current       += track.target;
                    auto type      = track.definition.max.type;
                    bool eligible  = type == grid_breadth_type::automatic || type == grid_breadth_type::max_content ||
                                     type == grid_breadth_type::fit_content ||
                                     (!max_content_only && type == grid_breadth_type::min_content);
                    bool capped =
                        type == grid_breadth_type::fit_content &&
                        (!track.definition.max.length.has_percentage() || definite) &&
                        track.target >= std::max(track.size, breadth_length(track.definition.max, available, definite));
                    if(eligible && !capped)
                    {
                        ++growable;
                    }
                }
                if(growable == 0 || current >= contribution)
                {
                    return;
                }
                auto addition = (contribution - current) / growable;
                for(int index = first; index < last; ++index)
                {
                    auto& track = tracks[static_cast<size_t>(index)];
                    auto  type  = track.definition.max.type;
                    if(type == grid_breadth_type::fit_content &&
                       (!track.definition.max.length.has_percentage() || definite))
                    {
                        auto limit   = std::max(track.size, breadth_length(track.definition.max, available, definite));
                        track.target = std::min(track.target + addition, std::max(track.target, limit));
                    } else if(type == grid_breadth_type::automatic || type == grid_breadth_type::max_content ||
                              type == grid_breadth_type::fit_content ||
                              (!max_content_only && type == grid_breadth_type::min_content))
                    {
                        track.target += addition;
                    }
                }
            }
        }

        void finish_tracks(std::vector<track_state>& tracks, pixel_t available, pixel_t gap, bool definite,
                           bool stretch)
        {
            if(tracks.empty())
            {
                return;
            }
            if(!definite)
            {
                float fraction_size = 0;
                for(auto& track : tracks)
                {
                    track.size = std::max(track.size, track.target);
                    if(track.definition.max.type == grid_breadth_type::length &&
                       !track.definition.max.length.has_percentage())
                    {
                        track.size = std::max(track.size, breadth_length(track.definition.max, available, false));
                    } else if(track.definition.max.type == grid_breadth_type::fraction &&
                              track.definition.max.fraction > 0)
                    {
                        fraction_size = std::max(fraction_size, track.size.value() / track.definition.max.fraction);
                    }
                }
                for(auto& track : tracks)
                {
                    if(track.definition.max.type == grid_breadth_type::fraction)
                    {
                        track.size = std::max(track.size, pixel_t(fraction_size * track.definition.max.fraction));
                    }
                }
                return;
            }
            auto                 space     = std::max(0_px, available - gap * static_cast<int>(tracks.size() - 1));
            pixel_t              remaining = space;
            std::vector<pixel_t> limits;
            for(const auto& track : tracks)
            {
                remaining -= track.size;
                limits.push_back(track.definition.max.type == grid_breadth_type::length
                                     ? std::max(track.size, breadth_length(track.definition.max, available, true))
                                     : std::max(track.size, track.target));
            }
            for(size_t iteration = 0; iteration < tracks.size() && remaining > 0_px; ++iteration)
            {
                int growable = 0;
                for(size_t index = 0; index < tracks.size(); ++index)
                {
                    if(tracks[index].size < limits[index])
                    {
                        ++growable;
                    }
                }
                if(growable == 0)
                {
                    break;
                }
                const auto share = remaining / growable;
                pixel_t    grown = 0_px;
                for(size_t index = 0; index < tracks.size(); ++index)
                {
                    auto addition       = std::min(share, limits[index] - tracks[index].size);
                    tracks[index].size += addition;
                    grown              += addition;
                }
                remaining -= grown;
            }
            std::vector<float>   fractions;
            std::vector<pixel_t> sizes;
            int                  auto_tracks = 0;
            for(const auto& track : tracks)
            {
                fractions.push_back(
                    track.definition.max.type == grid_breadth_type::fraction ? track.definition.max.fraction : 0);
                sizes.push_back(track.size);
                if(track.definition.max.type == grid_breadth_type::automatic)
                {
                    ++auto_tracks;
                }
            }
            distribute_grid_fractions(space, fractions, sizes);
            pixel_t used = 0_px;
            for(auto size : sizes)
            {
                used += size;
            }
            auto extra = stretch && auto_tracks > 0 ? std::max(0_px, space - used) / auto_tracks : 0_px;
            for(size_t index = 0; index < tracks.size(); ++index)
            {
                tracks[index].size = sizes[index];
                if(tracks[index].definition.max.type == grid_breadth_type::automatic)
                {
                    tracks[index].size += extra;
                }
            }
        }

        pixel_t track_extent(const std::vector<track_state>& tracks, int first, int last, pixel_t gap)
        {
            pixel_t extent = gap * std::max(0, last - first - 1);
            for(int index = first; index < last; ++index)
            {
                extent += tracks[static_cast<size_t>(index)].size;
            }
            return extent;
        }

        pixel_t track_position(const std::vector<track_state>& tracks, int index, pixel_t gap)
        {
            return track_extent(tracks, 0, index, gap) + (index > 0 ? gap : 0_px);
        }

        std::pair<pixel_t, pixel_t> align_tracks(pixel_t free_space, size_t count, pixel_t gap, int alignment)
        {
            pixel_t offset = 0_px;
            if(alignment == 1)
            {
                offset = free_space;
            } else if(alignment == 2)
            {
                offset = free_space / 2;
            } else if(free_space > 0_px && count > 0)
            {
                if(alignment == 3 && count > 1)
                {
                    gap += free_space / static_cast<int>(count - 1);
                } else if(alignment == 4)
                {
                    auto between  = free_space / static_cast<int>(count);
                    offset        = between / 2;
                    gap          += between;
                } else if(alignment == 5)
                {
                    auto between  = free_space / static_cast<int>(count + 1);
                    offset        = between;
                    gap          += between;
                }
            }
            return {offset, gap};
        }

        void collapse_empty_tracks(std::vector<track_state>& tracks, const auto_repeat_info& repeat,
                                   const std::vector<grid_area>& areas, int bounds_start, bool column_axis)
        {
            if(!repeat.present || !repeat.auto_fit)
            {
                return;
            }
            std::vector<bool> used(repeat.count, false);
            for(const auto& area : areas)
            {
                const int first = column_axis ? area.column_start : area.row_start;
                const int last  = column_axis ? area.column_end : area.row_end;
                for(int index = first; index < last; ++index)
                {
                    const auto offset = index - static_cast<int>(repeat.start);
                    if(offset >= 0 && offset < static_cast<int>(repeat.count))
                    {
                        used[static_cast<size_t>(offset)] = true;
                    }
                }
            }
            static const grid_breadth    zero_breadth{grid_breadth_type::length, css_length(0, css_units_px), 0};
            static const grid_track_size zero_track{zero_breadth, zero_breadth};
            for(size_t index = 0; index < repeat.count; ++index)
            {
                if(used[index])
                {
                    continue;
                }
                const auto track_index = static_cast<int>(repeat.start + index) - bounds_start;
                if(track_index >= 0 && track_index < static_cast<int>(tracks.size()))
                {
                    tracks[static_cast<size_t>(track_index)].definition = zero_track;
                    tracks[static_cast<size_t>(track_index)].size       = 0_px;
                    tracks[static_cast<size_t>(track_index)].target     = 0_px;
                }
            }
        }
    } // namespace

    void render_item_grid::calc_intrinsic_size()
    {
        std::vector<std::shared_ptr<render_item>> children;
        std::vector<grid_item_placement>          placements;
        for(const auto& child : m_children)
        {
            if(is_one_of(child->css().get_position(), element_position_absolute, element_position_fixed))
            {
                continue;
            }
            children.push_back(child);
            placements.push_back(item_placement(child));
            m_intrinsic_min_size.height += child->get_intrinsic_min_size().height;
            m_intrinsic_max_size.height += child->get_intrinsic_max_size().height;
        }
        std::vector<grid_area> areas;
        grid_area              bounds;
        auto                   column_names   = css().get_grid_template_columns();
        auto                   row_names      = css().get_grid_template_rows();
        const auto&            template_areas = css().get_grid_template_areas();
        add_area_line_names(column_names, row_names, template_areas);
        if(place_grid_items(placements, std::max(static_cast<int>(column_names.size()), template_areas.columns),
                            std::max(static_cast<int>(row_names.size()), template_areas.rows),
                            css().get_grid_auto_flow(), areas, bounds, &column_names, &row_names))
        {
            auto min_tracks = make_tracks(css().get_grid_template_columns(), css().get_grid_auto_column_pattern(),
                                          bounds.column_start, bounds.column_end, 0_px, false);
            auto max_tracks = min_tracks;
            // Percentages resolve against zero for intrinsic contributions.
            const auto gap = css().get_column_gap().calc_percent(0_px);
            for(size_t index = 0; index < children.size(); ++index)
            {
                int first = areas[index].column_start - bounds.column_start;
                int last  = areas[index].column_end - bounds.column_start;
                add_contribution(min_tracks, first, last, children[index]->get_intrinsic_min_size().width, gap, 0_px,
                                 false);
                add_contribution(min_tracks, first, last, children[index]->get_intrinsic_max_size().width, gap, 0_px,
                                 false, true);
                add_contribution(max_tracks, first, last, children[index]->get_intrinsic_min_size().width, gap, 0_px,
                                 false);
                add_contribution(max_tracks, first, last, children[index]->get_intrinsic_max_size().width, gap, 0_px,
                                 false, false, true);
            }
            for(size_t index = 0; index < children.size(); ++index)
            {
                int first = areas[index].column_start - bounds.column_start;
                int last  = areas[index].column_end - bounds.column_start;
                add_max_contribution(max_tracks, first, last, children[index]->get_intrinsic_min_size().width, gap,
                                     0_px, false, false);
                add_max_contribution(max_tracks, first, last, children[index]->get_intrinsic_max_size().width, gap,
                                     0_px, false, true);
            }
            finish_tracks(max_tracks, 0_px, gap, false, false);
            m_intrinsic_min_size.width = track_extent(min_tracks, 0, static_cast<int>(min_tracks.size()), gap);
            m_intrinsic_max_size.width = track_extent(max_tracks, 0, static_cast<int>(max_tracks.size()), gap);
        }
        m_intrinsic_min_size.width  += content_offset_width();
        m_intrinsic_max_size.width  += content_offset_width();
        m_intrinsic_min_size.height += content_offset_height();
        m_intrinsic_max_size.height += content_offset_height();
    }

    pixel_t render_item_grid::_render_content(pixel_t, pixel_t, bool second_pass,
                                              const containing_block_context& self_size, formatting_context* fmt_ctx)
    {
        std::vector<std::shared_ptr<render_item>> children;
        std::vector<grid_item_placement>          placements;
        for(const auto& child : m_children)
        {
            if(is_one_of(child->css().get_position(), element_position_absolute, element_position_fixed))
            {
                if(!second_pass)
                {
                    child->render(0_px, 0_px, self_size, fmt_ctx);
                }
                continue;
            }
            children.push_back(child);
            placements.push_back(item_placement(child));
        }
        std::vector<grid_area> areas;
        grid_area              bounds;
        auto                   column_names   = css().get_grid_template_columns();
        auto                   row_names      = css().get_grid_template_rows();
        const auto&            template_areas = css().get_grid_template_areas();

        const bool subgrid_columns = column_names.is_subgrid() && m_inherited_columns.has_value();
        const bool subgrid_rows    = row_names.is_subgrid() && m_inherited_rows.has_value();
        pixel_t    width           = self_size.render_width;
        bool       definite_height = self_size.height.type != containing_block_context::cbc_value_type_auto &&
                                     self_size.height.type != containing_block_context::cbc_value_type_none;
        pixel_t    height     = definite_height ? std::max(0_px, self_size.height.value - box_sizing_height()) : 0_px;
        const bool intrinsic_height_constraint =
            css().get_min_height().is_intrinsic_sizing() || css().get_max_height().is_intrinsic_sizing();
        if(definite_height && intrinsic_height_constraint &&
           (self_size.size_mode & containing_block_context::size_mode_measure_height) == 0)
        {
            if(self_size.max_height.type != containing_block_context::cbc_value_type_none)
            {
                height = std::min(height, self_size.max_height.value);
            }
            if(self_size.min_height.type != containing_block_context::cbc_value_type_none)
            {
                height = std::max(height, self_size.min_height.value);
            }
        }
        pixel_t    column_gap = css().get_column_gap().calc_percent(width);
        pixel_t    row_gap    = css().get_row_gap().calc_percent(height);
        if(subgrid_columns && css().get_column_gap().is_predefined())
        {
            column_gap = m_inherited_columns->gap;
        }
        if(subgrid_rows && css().get_row_gap().is_predefined())
        {
            row_gap = m_inherited_rows->gap;
        }

        auto_repeat_info column_repeat;
        auto_repeat_info row_repeat;
        auto columns_resolved = subgrid_columns
                                    ? column_names.resolve_subgrid_names(m_inherited_columns->sizes.size())
                                    : column_names.resolve_auto_repeat(width, column_gap, true, column_repeat);
        auto rows_resolved = subgrid_rows ? row_names.resolve_subgrid_names(m_inherited_rows->sizes.size())
                                          : row_names.resolve_auto_repeat(height, row_gap, definite_height, row_repeat);
        if(subgrid_columns)
        {
            for(size_t index = 0; index <= m_inherited_columns->sizes.size(); ++index)
            {
                columns_resolved.add_line_names(index, m_inherited_columns->names.line_names_at(index));
            }
        }
        if(subgrid_rows)
        {
            for(size_t index = 0; index <= m_inherited_rows->sizes.size(); ++index)
            {
                rows_resolved.add_line_names(index, m_inherited_rows->names.line_names_at(index));
            }
        }
        add_area_line_names(columns_resolved, rows_resolved, template_areas);

        const int column_count = subgrid_columns
                                     ? static_cast<int>(m_inherited_columns->sizes.size())
                                     : std::max(static_cast<int>(columns_resolved.size()), template_areas.columns);
        const int row_count    = subgrid_rows ? static_cast<int>(m_inherited_rows->sizes.size())
                                              : std::max(static_cast<int>(rows_resolved.size()), template_areas.rows);
        if(!place_grid_items(placements, column_count, row_count, css().get_grid_auto_flow(), areas, bounds,
                             &columns_resolved, &rows_resolved, subgrid_columns, subgrid_rows))
        {
            m_placed_areas.clear();
            pixel_t top = 0_px;
            for(const auto& child : children)
            {
                child->render(0_px, top, self_size, fmt_ctx, second_pass);
                top += child->height();
            }
            m_pos.width  = self_size.render_width;
            m_pos.height = top;
            return m_pos.width;
        }
        m_placed_areas = areas;
        auto columns   = make_tracks(columns_resolved, css().get_grid_auto_column_pattern(), bounds.column_start,
                                     bounds.column_end, width, true);
        auto rows      = make_tracks(rows_resolved, css().get_grid_auto_row_pattern(), bounds.row_start, bounds.row_end,
                                     height, definite_height);
        auto inherit_tracks = [](std::vector<track_state>& tracks, const inherited_axis& axis, pixel_t gap) {
            const auto adjustment = (axis.gap - gap) / 2;
            for(size_t index = 0; index < tracks.size(); ++index)
            {
                auto size = axis.sizes[index];
                if(index > 0)
                {
                    size += adjustment;
                }
                if(index + 1 < tracks.size())
                {
                    size += adjustment;
                }
                tracks[index].size   = std::max(0_px, size);
                tracks[index].target = tracks[index].size;
            }
        };
        if(subgrid_columns)
        {
            inherit_tracks(columns, *m_inherited_columns, column_gap);
        }
        if(subgrid_rows)
        {
            inherit_tracks(rows, *m_inherited_rows, row_gap);
        }
        collapse_empty_tracks(columns, column_repeat, areas, bounds.column_start, true);
        collapse_empty_tracks(rows, row_repeat, areas, bounds.row_start, false);
        struct column_contribution
        {
            std::shared_ptr<render_item> item;
            int                          first;
            int                          last;
        };
        std::vector<column_contribution> column_items;
        std::function<void(const std::shared_ptr<render_item>&, int, int, int, const grid_track_list&, int)>
            collect_column_items;
        collect_column_items = [&](const std::shared_ptr<render_item>& item, int first, int last, int row_span,
                                   const grid_track_list& parent_names, int parent_origin) {
            auto grid = std::dynamic_pointer_cast<render_item_grid>(item);
            if(grid && item->css().get_grid_template_columns().is_subgrid())
            {
                std::vector<std::shared_ptr<render_item>> nested_children;
                std::vector<grid_item_placement>          nested_placements;
                for(const auto& child : grid->children())
                {
                    if(is_one_of(child->css().get_position(), element_position_absolute, element_position_fixed))
                    {
                        continue;
                    }
                    nested_children.push_back(child);
                    nested_placements.push_back(item_placement(child));
                }
                auto names =
                    item->css().get_grid_template_columns().resolve_subgrid_names(static_cast<size_t>(last - first));
                for(int line = first; line <= last; ++line)
                {
                    if(line >= parent_origin)
                    {
                        names.add_line_names(static_cast<size_t>(line - first),
                                             parent_names.line_names_at(static_cast<size_t>(line - parent_origin)));
                    }
                }
                const auto& nested_rows      = item->css().get_grid_template_rows();
                const bool  bounded_rows     = nested_rows.is_subgrid();
                const int   nested_row_count = bounded_rows ? row_span : static_cast<int>(nested_rows.size());
                std::vector<grid_area> nested_areas;
                grid_area              nested_bounds;
                if(place_grid_items(nested_placements, last - first, nested_row_count, item->css().get_grid_auto_flow(),
                                    nested_areas, nested_bounds, &names, &nested_rows, true, bounded_rows))
                {
                    for(size_t index = 0; index < nested_children.size(); ++index)
                    {
                        collect_column_items(nested_children[index], first + nested_areas[index].column_start,
                                             first + nested_areas[index].column_end,
                                             nested_areas[index].row_end - nested_areas[index].row_start, names, first);
                    }
                    return;
                }
            }
            column_items.push_back({item, first, last});
        };
        if(!subgrid_columns)
        {
            for(size_t index = 0; index < children.size(); ++index)
            {
                collect_column_items(children[index], areas[index].column_start - bounds.column_start,
                                     areas[index].column_end - bounds.column_start,
                                     areas[index].row_end - areas[index].row_start, columns_resolved,
                                     -bounds.column_start);
            }
        }
        for(const auto& contribution : column_items)
        {
            int first = contribution.first;
            int last  = contribution.last;
            add_contribution(columns, first, last, contribution.item->get_intrinsic_min_size().width, column_gap, width,
                             true);
            add_contribution(columns, first, last, contribution.item->get_intrinsic_max_size().width, column_gap, width,
                             true, true);
        }
        for(const auto& contribution : column_items)
        {
            int first = contribution.first;
            int last  = contribution.last;
            add_max_contribution(columns, first, last, contribution.item->get_intrinsic_min_size().width, column_gap,
                                 width, true, false);
            add_max_contribution(columns, first, last, contribution.item->get_intrinsic_max_size().width, column_gap,
                                 width, true, true);
        }
        auto justify = css().get_flex_justify_content();
        if(!subgrid_columns)
        {
            finish_tracks(columns, width, column_gap, true,
                          is_one_of(justify, flex_justify_content_normal, flex_justify_content_stretch));
        }
        auto set_child_axes = [&](size_t index) {
            auto child_grid = std::dynamic_pointer_cast<render_item_grid>(children[index]);
            if(!child_grid)
            {
                return;
            }
            auto make_axis = [](const std::vector<track_state>& tracks, int first, int last, pixel_t gap,
                                const grid_track_list& names, int bounds_start) {
                inherited_axis axis;
                axis.gap = gap;
                for(int track = first; track < last; ++track)
                {
                    axis.sizes.push_back(tracks[static_cast<size_t>(track)].size);
                }
                for(int line = first; line <= last; ++line)
                {
                    if(line + bounds_start >= 0)
                    {
                        axis.names.add_line_names(static_cast<size_t>(line - first),
                                                  names.line_names_at(static_cast<size_t>(line + bounds_start)));
                    }
                }
                return axis;
            };
            child_grid->m_inherited_columns = make_axis(columns, areas[index].column_start - bounds.column_start,
                                                        areas[index].column_end - bounds.column_start, column_gap,
                                                        columns_resolved, bounds.column_start);
            child_grid->m_inherited_rows =
                make_axis(rows, areas[index].row_start - bounds.row_start, areas[index].row_end - bounds.row_start,
                          row_gap, rows_resolved, bounds.row_start);
        };
        std::function<void(const std::shared_ptr<render_item>&, int, int, pixel_t)> add_row_item;
        add_row_item = [&](const std::shared_ptr<render_item>& item, int first, int last, pixel_t automatic_minimum) {
            auto grid = std::dynamic_pointer_cast<render_item_grid>(item);
            if(grid && item->css().get_grid_template_rows().is_subgrid())
            {
                size_t nested_index = 0;
                for(const auto& child : grid->children())
                {
                    if(is_one_of(child->css().get_position(), element_position_absolute, element_position_fixed))
                    {
                        continue;
                    }
                    if(nested_index >= grid->m_placed_areas.size())
                    {
                        break;
                    }
                    const auto& nested_area = grid->m_placed_areas[nested_index++];
                    add_row_item(child, first + nested_area.row_start, first + nested_area.row_end, 0_px);
                }
                if(nested_index == grid->m_placed_areas.size())
                {
                    return;
                }
            }
            const auto contribution = std::max(item->height(), automatic_minimum);
            add_contribution(rows, first, last, contribution, row_gap, height, definite_height);
            add_max_contribution(rows, first, last, contribution, row_gap, height, definite_height, false);
        };
        for(size_t index = 0; index < children.size(); ++index)
        {
            int  first      = areas[index].column_start - bounds.column_start;
            int  last       = areas[index].column_end - bounds.column_start;
            auto area_width = track_extent(columns, first, last, column_gap);
            set_child_axes(index);
            pixel_t automatic_minimum = 0_px;
            if(intrinsic_height_constraint && children[index]->css().get_height().has_percentage() &&
               children[index]->css().get_min_height().is_auto_or_none() &&
               children[index]->css().get_overflow() == overflow_visible)
            {
                formatting_context measure_ctx = *fmt_ctx;
                children[index]->render(
                    0_px, 0_px, self_size.new_width(area_width, containing_block_context::size_mode_ignore_css_height),
                    &measure_ctx);
                automatic_minimum = children[index]->height();
            }
            children[index]->render(0_px, 0_px, self_size.new_width(area_width), fmt_ctx);
            if(!subgrid_rows)
            {
                add_row_item(children[index], areas[index].row_start - bounds.row_start,
                             areas[index].row_end - bounds.row_start, automatic_minimum);
            }
        }
        auto align = css().get_flex_align_content();
        if(!subgrid_rows)
        {
            finish_tracks(rows, height, row_gap, definite_height,
                          is_one_of(align, flex_align_content_normal, flex_align_content_stretch));
        }
        const auto intrinsic_height = track_extent(rows, 0, static_cast<int>(rows.size()), row_gap);
        if(!definite_height && css().get_row_gap().has_percentage())
        {
            row_gap = css().get_row_gap().calc_percent(intrinsic_height);
        }
        int justify_mode = 0;
        if(is_one_of(justify, flex_justify_content_end, flex_justify_content_flex_end, flex_justify_content_right))
        {
            justify_mode = 1;
        } else if(justify == flex_justify_content_center)
        {
            justify_mode = 2;
        } else if(justify == flex_justify_content_space_between)
        {
            justify_mode = 3;
        } else if(justify == flex_justify_content_space_around)
        {
            justify_mode = 4;
        } else if(justify == flex_justify_content_space_evenly)
        {
            justify_mode = 5;
        }
        int align_mode = 0;
        if(is_one_of(align, flex_align_content_end, flex_align_content_flex_end))
        {
            align_mode = 1;
        } else if(align == flex_align_content_center)
        {
            align_mode = 2;
        } else if(align == flex_align_content_space_between)
        {
            align_mode = 3;
        } else if(align == flex_align_content_space_around)
        {
            align_mode = 4;
        } else if(align == flex_align_content_space_evenly)
        {
            align_mode = 5;
        }
        const auto column_alignment = align_tracks(
            subgrid_columns ? 0_px : width - track_extent(columns, 0, static_cast<int>(columns.size()), column_gap),
            columns.size(), column_gap, justify_mode);
        const auto row_alignment = align_tracks(
            definite_height && !subgrid_rows ? height - track_extent(rows, 0, static_cast<int>(rows.size()), row_gap)
                                             : 0_px,
            rows.size(), row_gap, align_mode);
        column_gap = column_alignment.second;
        row_gap    = row_alignment.second;
        for(size_t index = 0; index < children.size(); ++index)
        {
            int  col            = areas[index].column_start - bounds.column_start;
            int  row            = areas[index].row_start - bounds.row_start;
            auto left           = column_alignment.first + track_position(columns, col, column_gap);
            auto top            = row_alignment.first + track_position(rows, row, row_gap);
            auto area_width     = track_extent(columns, col, areas[index].column_end - bounds.column_start, column_gap);
            auto area_height    = track_extent(rows, row, areas[index].row_end - bounds.row_start, row_gap);
            auto context        = self_size.new_width_height(area_width, area_height);
            context.height.type = containing_block_context::cbc_value_type_absolute;
            const auto child_grid = std::dynamic_pointer_cast<render_item_grid>(children[index]);
            const bool child_subgrid_columns =
                child_grid && children[index]->css().get_grid_template_columns().is_subgrid();
            const bool child_subgrid_rows = child_grid && children[index]->css().get_grid_template_rows().is_subgrid();
            auto       horizontal         = children[index]->css().get_grid_justify_self();
            if(horizontal == flex_align_items_auto)
            {
                horizontal = css().get_grid_justify_items();
            }
            auto vertical = children[index]->css().get_flex_align_self();
            if(vertical == flex_align_items_auto)
            {
                vertical = css().get_flex_align_items();
            }
            if(child_subgrid_columns)
            {
                horizontal = flex_align_items_stretch;
            }
            if(child_subgrid_rows)
            {
                vertical = flex_align_items_stretch;
            }
            bool        stretch_width  = is_one_of(horizontal, flex_align_items_normal, flex_align_items_stretch);
            bool        stretch_height = is_one_of(vertical, flex_align_items_normal, flex_align_items_stretch);
            const auto& margins        = children[index]->css().get_margins();
            const bool  auto_left      = margins.left.is_predefined();
            const bool  auto_right     = margins.right.is_predefined();
            const bool  auto_top       = margins.top.is_predefined();
            const bool  auto_bottom    = margins.bottom.is_predefined();
            stretch_width              = stretch_width && !auto_left && !auto_right;
            stretch_height             = stretch_height && !auto_top && !auto_bottom;
            children[index]->calc_outlines(area_width);
            if(child_subgrid_columns)
            {
                context.width.value         = std::max(0_px, area_width - children[index]->content_offset_width() +
                                                                 children[index]->box_sizing_width());
                context.render_width.value  = context.width.value;
                context.size_mode          |= containing_block_context::size_mode_exact_width;
            }
            if(!stretch_width && children[index]->css().get_width().is_auto_or_none() &&
               !children[index]->src_el()->is_replaced())
            {
                auto natural                = std::min(area_width, children[index]->get_intrinsic_max_size().width);
                natural                     = std::max(natural, children[index]->get_intrinsic_min_size().width);
                context.width.value         = std::max(0_px, natural - children[index]->content_offset_width() +
                                                                 children[index]->box_sizing_width());
                context.render_width.value  = context.width.value;
                context.size_mode          |= containing_block_context::size_mode_exact_width;
            }
            if((child_subgrid_rows || (stretch_height && children[index]->css().get_height().is_auto_or_none())) &&
               !children[index]->src_el()->is_replaced())
            {
                context.height.value  = std::max(0_px, area_height - children[index]->content_offset_height() +
                                                           children[index]->box_sizing_height());
                context.size_mode    |= containing_block_context::size_mode_exact_height;
            }
            set_child_axes(index);
            children[index]->render(left, top, context, fmt_ctx);
            auto free_width  = area_width - children[index]->width();
            auto free_height = area_height - children[index]->height();
            if(auto_left || auto_right)
            {
                auto margin = std::max(0_px, free_width) / (static_cast<int>(auto_left) + static_cast<int>(auto_right));
                if(auto_left)
                {
                    children[index]->get_margins().left  = margin;
                    children[index]->pos().x            += margin;
                }
                if(auto_right)
                {
                    children[index]->get_margins().right = margin;
                }
            } else if(horizontal == flex_align_items_end)
            {
                children[index]->pos().x += free_width;
            } else if(horizontal == flex_align_items_center)
            {
                children[index]->pos().x += free_width / 2;
            }
            if(auto_top || auto_bottom)
            {
                auto margin =
                    std::max(0_px, free_height) / (static_cast<int>(auto_top) + static_cast<int>(auto_bottom));
                if(auto_top)
                {
                    children[index]->get_margins().top  = margin;
                    children[index]->pos().y           += margin;
                }
                if(auto_bottom)
                {
                    children[index]->get_margins().bottom = margin;
                }
            } else if(vertical == flex_align_items_end || vertical == flex_align_items_flex_end)
            {
                children[index]->pos().y += free_height;
            } else if(vertical == flex_align_items_center)
            {
                children[index]->pos().y += free_height / 2;
            }
            children[index]->apply_relative_shift(self_size.new_width_height(area_width, area_height));
        }
        m_pos.width = width;
        m_pos.height =
            definite_height ? track_extent(rows, 0, static_cast<int>(rows.size()), row_gap) : intrinsic_height;
        return std::max(width, track_extent(columns, 0, static_cast<int>(columns.size()), column_gap));
    }
} // namespace litehtml
