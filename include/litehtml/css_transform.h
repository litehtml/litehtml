#ifndef LH_CSS_TRANSFORM_H
#define LH_CSS_TRANSFORM_H
#include "css_angle.h"
#include "css_length.h"
#include <memory>

namespace litehtml
{
class transform
{
public:
    virtual ~transform() = default;
    virtual std::unique_ptr<transform> clone() const = 0;
    virtual transformation get_transformation(const std::shared_ptr<document>& document,
                                              const font_metrics& metrics,
                                              size size,
                                              float coeficient) const = 0;
    virtual transformation get_transformation(const std::shared_ptr<document>& document,
                                              const font_metrics& metrics, size size) const = 0;
    virtual transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>& document, const font_metrics& metrics, size size) const = 0;
};

class css_transform
{
public:
    static const css_length zero_length;
    static const css_angle zero_angle;

public:
    css_transform() = default;
    css_transform(const css_transform& other);
    css_transform& operator=(const css_transform& other);

    void add_translate(const css_length& x, const css_length& y, const css_length& z);

    void add_rotate(const css_angle& angle);
    void add_rotate(float x, float y, float z, const css_angle& angle);
    void add_rotateX(const css_angle& angle);
    void add_rotateY(const css_angle& angle);
    void add_rotateZ(const css_angle& angle);

    void add_scale(float x, float y, float z);

    void add_skew(const css_angle& x, const css_angle& y);

    void add_matrix(float a, float b, float c, float d, float tx, float ty);
    void add_matrix(const transformation& matrix);

    void add_perspective(const css_length& length);

    transformation get_transformation(const std::shared_ptr<document>& document,
                                      const font_metrics& metrics, size size, float coefficient) const;
    transformation get_transformation(const std::shared_ptr<document>& document,
                                      const font_metrics& metrics, size size) const;
    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>& document, const font_metrics& metrics, size size) const;

    void clear() { m_functions.clear(); }

private:
    void copy(const css_transform& other);

private:
    std::vector<std::unique_ptr<transform>> m_functions;
};
}
#endif // !LH_CSS_TRANSFORM_H
