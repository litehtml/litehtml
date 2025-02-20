#include "html.h"
#include "css_transform.h"

namespace litehtml
{
class translate : public transform
{
public:
    translate(const css_length& x, const css_length& y, const css_length& z)
        : m_x(x), m_y(y), m_z(z) {}

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<translate>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>& document, const font_metrics& metrics, size size) const override
    {
        const auto translation_vector = get_translation(document, metrics, size);
        return transformation::translation(translation_vector[0], translation_vector[1], translation_vector[2]);
    }

    transformation get_transformation(const std::shared_ptr<document>& document,
                                      const font_metrics& metrics, const size size, const float coefficient) const override
    {
        const auto translation_vector = get_translation(document, metrics, size);
        return transformation::translation(coefficient * translation_vector[0],
                                           coefficient * translation_vector[1],
                                           coefficient * translation_vector[2]);
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>&,
        const std::shared_ptr<document>& document, const font_metrics& metrics, size size) const override
    {
        return matrix * get_transformation(document, metrics, size);
    }

private:
    std::array<float, 3> get_translation(const std::shared_ptr<document>& document,
        const font_metrics& metrics, const size size) const
    {
        const auto x = static_cast<float>(document->to_pixels(m_x, metrics, size.width));
        const auto y = static_cast<float>(document->to_pixels(m_y, metrics, size.height));
        const auto z = static_cast<float>(document->to_pixels(m_z, metrics, 0));
        return std::array<float, 3>{x, y, z};
    }

private:
    css_length m_x;
    css_length m_y;
    css_length m_z;
};

class scale : public transform
{
public:
    scale(const float x, const float y, const float z)
        : m_x(x), m_y(y), m_z(z) {}

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<scale>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>&,
                                      const font_metrics&, const size, const float coefficient) const override
    {
        return transformation::scale(m_x * coefficient, m_y * coefficient, m_z * coefficient);
    }

    transformation get_transformation(const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        return transformation::scale(m_x, m_y, m_z);
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        transformation result = matrix;
        result *= transformation::translation(origin[0], origin[1], origin[2]);
        result *= transformation::scale(m_x, m_y, m_z);
        result *= transformation::translation(-origin[0], -origin[1], -origin[2]);
        return result;
    }

private:
    float m_x;
    float m_y;
    float m_z;
};

class rotate: public transform
{
public:
    enum class axis
    {
      X,
      Y,
      Z
    };

public:
    rotate(const css_angle& angle, const std::variant<axis, std::array<float, 3>>& axis)
        : m_angle(angle), m_axis(axis)
    {
        if(std::holds_alternative<std::array<float, 3>>(m_axis))
        {
            auto vector = std::get<std::array<float, 3>>(m_axis);
            const auto length = std::sqrt(
                vector[0] * vector[0] + vector[1] * vector[1] + vector[2] * vector[2]);
            if (length != 0)
            {
                vector[0] /= length;
                vector[1] /= length;
                vector[2] /= length;
            }
            m_axis = vector;
        }
    }

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<rotate>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>&,
                                      const font_metrics&, const size, const float coefficient) const override
    {
        return get_rotation(m_angle.radians() * coefficient);
    }

    transformation get_transformation(const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        return get_rotation(m_angle.radians());
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        transformation result = matrix;
        result *= transformation::translation(origin[0], origin[1], origin[2]);
        result *= get_rotation(m_angle.radians());
        result *= transformation::translation(-origin[0], -origin[1], -origin[2]);
        return result;
    }

private:
    transformation get_rotation(float angle) const
    {
        if (std::holds_alternative<std::array<float, 3>>(m_axis))
        {
            const auto axis = std::get<std::array<float, 3>>(m_axis);
            return transformation::rotation3d(axis[0], axis[1], axis[2], angle);
        }
        switch (std::get<axis>(m_axis))
        {
            case axis::Z:
                return transformation::rotationZ(angle);
                break;
            case axis::X:
                return transformation::rotationX(angle);
                break;
            case axis::Y:
                return transformation::rotationY(angle);
                break;
        }
        return transformation::identity();
    }

private:
    css_angle m_angle;
    std::variant<axis, std::array<float, 3>> m_axis;
};

class skew : public transform
{
public:
    skew(const css_angle& x, const css_angle& y)
        : m_x(x), m_y(y) {}

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<skew>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>&,
                                      const font_metrics&, const size, const float coefficient) const override
    {
        return transformation::skew(m_x.radians() * coefficient, m_y.radians() * coefficient);
    }

    transformation get_transformation(const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        return transformation::skew(m_x.radians(), m_y.radians());
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        transformation result = matrix;
        result *= transformation::translation(origin[0], origin[1], origin[2]);
        result *= transformation::skew(m_x.radians(), m_y.radians());
        result *= transformation::translation(-origin[0], -origin[1], -origin[2]);
        return result;
    }

private:
    css_angle m_x;
    css_angle m_y;
};

class matrix : public transform
{
public:
    matrix(const transformation& mat) : m_matrix(mat) {}
    matrix(float a, float b, float c, float d, float tx, float ty)
        : m_matrix(a, c, b, d, tx, ty) {}

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<matrix>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>&,
                                      const font_metrics&, const size, const float coefficient) const override
    {
        return transformation::get_with_coefficient(m_matrix, coefficient);
    }

    transformation get_transformation(const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        return m_matrix;
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>&, const font_metrics&, size) const override
    {
        transformation result = matrix;
        result *= transformation::translation(origin[0], origin[1], origin[2]);
        result *= m_matrix;
        result *= transformation::translation(-origin[0], -origin[1], -origin[2]);
        return result;
    }

private:
    transformation m_matrix;
};

class perspective : public transform
{
public:
  perspective(const css_length &length) : m_length(length) {}

    std::unique_ptr<transform> clone() const override
    {
        return std::make_unique<perspective>(*this);
    }

    transformation get_transformation(const std::shared_ptr<document>& document,
                                      const font_metrics& metrics, const size, const float coefficient) const override
    {
        const auto length = coefficient * get_length(document, metrics);
        return transformation::perspective(length);
    }

    transformation get_transformation(const std::shared_ptr<document>& document, const font_metrics& metrics, size) const override
    {
        return transformation::perspective(get_length(document, metrics));
    }

    transformation apply(const transformation& matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>& document, const font_metrics& metrics, size) const override
    {
        transformation result = matrix;
        result *= transformation::translation(origin[0], origin[1], origin[2]);
        result *= transformation::perspective(get_length(document, metrics));
        result *= transformation::translation(-origin[0], -origin[1], -origin[2]);
        return result;
    }

private:
    float get_length(const std::shared_ptr<document>& document, const font_metrics& metrics) const
    {
        return static_cast<float>(document->to_pixels(m_length, metrics, 0));
    }

private:
    css_length m_length;
};

const css_length css_transform::zero_length = css_length(0);
const css_angle css_transform::zero_angle = css_angle(0, css_angle_units_deg);

css_transform::css_transform(const css_transform& other)
{
    copy(other);
}

css_transform& css_transform::operator=(const css_transform& other)
{
    if (this == &other)
    {
        return *this;
    }
    copy(other);
    return *this;
}

void css_transform::copy(const css_transform& other)
{
    m_functions.clear();
    m_functions.reserve(other.m_functions.size());
    for (const auto& function : other.m_functions)
    {
        m_functions.emplace_back(function->clone());
    }
}

void css_transform::add_translate(const css_length& x, const css_length& y, const css_length& z)
{
    m_functions.emplace_back(std::make_unique<translate>(x, y, z));
}

void css_transform::add_rotate(const css_angle& angle)
{
    add_rotateZ(angle);
}

void css_transform::add_rotate(float x, float y, float z, const css_angle& angle)
{
    m_functions.emplace_back(std::make_unique<rotate>(angle, std::array<float, 3>{x, y, z}));
}

void css_transform::add_rotateX(const css_angle& angle)
{
    m_functions.emplace_back(std::make_unique<rotate>(angle, rotate::axis::X));
}

void css_transform::add_rotateY(const css_angle& angle)
{
    m_functions.emplace_back(std::make_unique<rotate>(angle, rotate::axis::Y));
}

void css_transform::add_rotateZ(const css_angle& angle)
{
    m_functions.emplace_back(std::make_unique<rotate>(angle, rotate::axis::Z));
}

void css_transform::add_scale(float x, float y, float z)
{
    m_functions.emplace_back(std::make_unique<scale>(x, y, z));
}

void css_transform::add_skew(const css_angle& x, const css_angle& y)
{
    m_functions.emplace_back(std::make_unique<skew>(x, y));
}

void css_transform::add_matrix(float a, float b, float c, float d, float tx, float ty)
{
    m_functions.emplace_back(std::make_unique<matrix>(a, b, c, d, tx, ty));
}

void css_transform::add_matrix(const transformation& mat)
{
    m_functions.emplace_back(std::make_unique<matrix>(mat));
}

void css_transform::add_perspective(const css_length& length)
{
    m_functions.emplace_back(std::make_unique<perspective>(length));
}

transformation css_transform::get_transformation(const std::shared_ptr<document> &document,
                                                 const font_metrics &metrics, size size, float coefficient) const
{
    auto result = transformation::identity();
    for (const auto& function : m_functions)
    {
        result *= function->get_transformation(document, metrics, size, coefficient);
    }
    return result;
}

transformation css_transform::get_transformation(const std::shared_ptr<document> &document, const font_metrics &metrics, size size) const
{
    auto result = transformation::identity();
    for (const auto& function : m_functions)
    {
        result *= function->get_transformation(document, metrics, size);
    }
    return result;
}

transformation css_transform::apply(const transformation &matrix, const std::array<float, 3>& origin,
        const std::shared_ptr<document>& document, const font_metrics& metrics, size size) const
{
    if (m_functions.empty()) return matrix;
    auto result = matrix;
    for (const auto& function : m_functions)
    {
        result = function->apply(result, origin, document, metrics, size);
    }
    return result;
}
}