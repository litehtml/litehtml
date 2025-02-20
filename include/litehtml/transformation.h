#ifndef LH_TRANSFORMATION_H
#define LH_TRANSFORMATION_H

#include <array>
#include <cmath>

namespace litehtml
{
struct transformation
{
	float xx; float yx; float zx; float px;
	float xy; float yy; float zy; float py;
	float xz; float yz; float zz; float pz;
	float x0; float y0; float z0; float w;

	transformation();
	transformation(float xx, float xy, float yx, float yy, float x0, float y0);
	transformation(float xx, float xy, float xz, float yx, float yy, float yz,
				   float zx, float zy, float zz, float x0, float y0, float z0);
	transformation(
		float xx, float yx, float zx, float px,
		float xy, float yy, float zy, float py,
		float xz, float yz, float zz, float pz,
		float x0, float y0, float z0, float w);

	transformation operator*(const transformation& other) const;
	transformation& operator*=(const transformation& other);
	bool operator==(const transformation& other) const;

	std::array<float, 2> apply(const std::array<float, 2>& point) const;
	std::array<float, 3> apply(const std::array<float, 3>& point) const;
	std::array<float, 4> apply(const std::array<float, 4>& point) const;

	transformation inverse() const;
	transformation inverse3x3() const;

	float determinant3x3() const;

	transformation normalized() const;

	transformation transposed() const;

	void decompose(std::array<float, 3>& translation,
				   std::array<float, 4>& rotation_quaternion,
				   std::array<float, 3>& scale,
				   std::array<float, 3>& skew,
				   std::array<float, 4>& projection) const;
	static transformation compose(const std::array<float, 3>& translation,
								  const std::array<float, 4>& rotation_quaternion,
								  const std::array<float, 3>& scale,
								  const std::array<float, 3>& skew,
								  const std::array<float, 4>& projection);

	static transformation interpolate(const transformation &low,
									  const transformation &high,
									  float coefficient);

	static transformation get_with_coefficient(const transformation& transform, float coefficient);

	static transformation identity()
	{
		return transformation{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
	}

	static transformation translation(float x, float y, float z)
	{
		return transformation{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, x, y, z};
	}

	static transformation rotationX(float angle)
	{
		const auto cos = std::cos(angle);
		const auto sin = std::sin(angle);
		return transformation{1.0f, 0.0f, 0.0f, 0.0f, cos, -sin, 0.0f, sin, cos, 0.0f, 0.0f, 0.0f};
	}

	static transformation rotationY(float angle)
	{
		const auto cos = std::cos(angle);
		const auto sin = std::sin(angle);
		return transformation{cos, 0.0f, sin, 0.0f, 1.0f, 0.0f, -sin, 0.0f, cos, 0.0f, 0.0f, 0.0f};
	}

	static transformation rotationZ(float angle)
	{
		const auto sin = std::sin(angle);
		const auto cos = std::cos(angle);
		return transformation{cos, -sin, sin, cos, 0.0f, 0.0f};
	}

	static transformation rotation3d(float x, float y, float z, float angle)
	{
		const auto sin = std::sin(angle);
		const auto cos = std::cos(angle);
		const auto a = 1.0f - cos;
		return transformation{
			1.0f + a * (x * x - 1.0f), -z * sin + x * y * a,       y * sin + x * z * a,
			z * sin + x * y * a,      1.0f + a * (y * y - 1.0f), -x * sin + y * z * a,
			-y * sin + x * z * a,       x * sin + y * z * a,      1.0f + a * (z * z - 1.0f),
			0.0f,                      0.0f,                      0.0f
		};
	}

	static transformation scale(float x, float y, float z)
	{
		return transformation{x, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f};
	}

	static transformation skew(float x_angle, float y_angle)
	{
		const auto tanx = std::tan(x_angle);
		const auto tany = std::tan(y_angle);
		return transformation{1.0f, tanx, tany, 1.0f, 0.0f, 0.0f};
	}

	static transformation perspective(float length)
	{
		transformation result = transformation::identity();
		result.pz = -1.0f / length;
		return result;
	}

	static transformation orthogonal()
	{
		transformation result = transformation::identity();
		result.zz = 0.0f;
		return result;
	}
};
}

#endif