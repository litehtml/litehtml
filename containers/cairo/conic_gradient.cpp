#include "conic_gradient.h"

#define INTERPOLATE_COLOR(C1, C2, t) (((C2) - (C1)) * (t) + (C1))

static double interpolate_channel(litehtml::byte color_A, litehtml::byte color_B, double weight)
{
    return (color_A + (color_B - color_A) * weight) / 255.0;
}

static void sector_patch(cairo_pattern_t* pattern, double radius, double angle_A, double A_r, double A_g, double A_b,
                         double A_a, double angle_B, double B_r, double B_g, double B_b, double B_a)
{
    double r_sin_A, r_cos_A;
    double r_sin_B, r_cos_B;
    double h;

    r_sin_A = radius * sin(angle_A);
    r_cos_A = radius * cos(angle_A);
    r_sin_B = radius * sin(angle_B);
    r_cos_B = radius * cos(angle_B);

    h = 4.0 / 3.0 * tan((angle_B - angle_A) / 4.0);

    double x0 = r_cos_A;
    double y0 = r_sin_A;
    double x1 = r_cos_A - h * r_sin_A;
    double y1 = r_sin_A + h * r_cos_A;
    double x2 = r_cos_B + h * r_sin_B;
    double y2 = r_sin_B - h * r_cos_B;
    double x3 = r_cos_B;
    double y3 = r_sin_B;

    cairo_mesh_pattern_begin_patch(pattern);

    cairo_mesh_pattern_move_to(pattern, 0, 0);
    cairo_mesh_pattern_line_to(pattern, x0, y0);

    cairo_mesh_pattern_curve_to(pattern, x1, y1, x2, y2, x3, y3);
    cairo_mesh_pattern_line_to(pattern, 0, 0);

    cairo_mesh_pattern_set_corner_color_rgba(pattern, 0, A_r, A_g, A_b, A_a);
    cairo_mesh_pattern_set_corner_color_rgba(pattern, 1, A_r, A_g, A_b, A_a);
    cairo_mesh_pattern_set_corner_color_rgba(pattern, 2, B_r, B_g, B_b, B_a);
    cairo_mesh_pattern_set_corner_color_rgba(pattern, 3, B_r, B_g, B_b, B_a);

    cairo_mesh_pattern_end_patch(pattern);
}

cairo_pattern_t* create_conic_gradient_pattern(double angle, double radius,
                                               const std::vector<bg_color_point>& color_points)
{
    if(color_points.empty())
    {
        return nullptr;
    }

    if(color_points.size() == 2 && !color_points[0].hint)
    {
        std::vector<bg_color_point> points;
        points.push_back(color_points[0]);
        bg_color_point cp;
        cp.offset    = 0.5f;
        cp.color.red = (litehtml::byte) INTERPOLATE_COLOR(color_points[0].color.red, color_points[1].color.red, 0.5f);
        cp.color.green =
            (litehtml::byte) INTERPOLATE_COLOR(color_points[0].color.green, color_points[1].color.green, 0.5f);
        cp.color.blue =
            (litehtml::byte) INTERPOLATE_COLOR(color_points[0].color.blue, color_points[1].color.blue, 0.5f);
        cp.color.alpha =
            (litehtml::byte) INTERPOLATE_COLOR(color_points[0].color.alpha, color_points[1].color.alpha, 0.5f);
        points.push_back(cp);
        points.push_back(color_points[1]);
        return create_conic_gradient_pattern(angle, radius, points);
    }

    const double two_pi = 2.0 * M_PI;

    cairo_pattern_t* pattern = cairo_pattern_create_mesh();

    constexpr size_t hint_subdivisions = 64;

    for(size_t i = 0; i < color_points.size() - 1; ++i)
    {
        const bg_color_point& cp_A     = color_points[i];
        const bg_color_point& cp_B     = color_points[i + 1];
        const double          offset_A = cp_A.offset;
        const double          offset_B = cp_B.offset;

        auto add_sector = [&](double start_offset, double start_weight, double end_offset, double end_weight) {
            double angle_A = start_offset * two_pi + angle;
            double angle_B = end_offset * two_pi + angle;

            sector_patch(pattern, radius, angle_A, interpolate_channel(cp_A.color.red, cp_B.color.red, start_weight),
                         interpolate_channel(cp_A.color.green, cp_B.color.green, start_weight),
                         interpolate_channel(cp_A.color.blue, cp_B.color.blue, start_weight),
                         interpolate_channel(cp_A.color.alpha, cp_B.color.alpha, start_weight), angle_B,
                         interpolate_channel(cp_A.color.red, cp_B.color.red, end_weight),
                         interpolate_channel(cp_A.color.green, cp_B.color.green, end_weight),
                         interpolate_channel(cp_A.color.blue, cp_B.color.blue, end_weight),
                         interpolate_channel(cp_A.color.alpha, cp_B.color.alpha, end_weight));
        };

        const double offset_span = offset_B - offset_A;
        if(cp_A.hint && offset_span > 0.0)
        {
            const double hint_fraction = (*cp_A.hint - offset_A) / offset_span;
            if(hint_fraction > 0.0 && hint_fraction < 1.0)
            {
                const double exponent    = log(0.5) / log(hint_fraction);
                size_t       before_hint = static_cast<size_t>(hint_fraction * hint_subdivisions);
                if(before_hint == 0)
                {
                    before_hint = 1;
                } else if(before_hint >= hint_subdivisions)
                {
                    before_hint = hint_subdivisions - 1;
                }

                double previous_offset = offset_A;
                double previous_weight = 0.0;
                for(size_t step = 1; step <= hint_subdivisions; ++step)
                {
                    double fraction;
                    if(step <= before_hint)
                    {
                        fraction = hint_fraction * step / before_hint;
                    } else
                    {
                        fraction = hint_fraction +
                                   (1.0 - hint_fraction) * (step - before_hint) / (hint_subdivisions - before_hint);
                    }

                    double next_offset = offset_A + offset_span * fraction;
                    double next_weight = pow(fraction, exponent);
                    add_sector(previous_offset, previous_weight, next_offset, next_weight);
                    previous_offset = next_offset;
                    previous_weight = next_weight;
                }
                continue;
            }
        }

        add_sector(offset_A, 0.0, offset_B, 1.0);
    }

    return pattern;
}
