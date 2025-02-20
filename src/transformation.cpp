#include "transformation.h"

namespace
{
	float vector_length(const std::array<float, 3>& vector)
	{
		return std::hypot(vector[0], vector[1], vector[2]);
	}

	void normalize_vector(std::array<float, 3>& vector)
	{
		float length = vector_length(vector);
		if (length != 0)
		{
			vector[0] /= length;
			vector[1] /= length;
			vector[2] /= length;
		}
	}

	std::array<float, 3> multiply(const std::array<float, 3>& vector, float factor)
	{
		return { vector[0] * factor, vector[1] * factor, vector[2] * factor };
	}

	std::array<float, 3> subtract(const std::array<float, 3>& a, const std::array<float, 3>& b)
	{
		return {a[0] - b[0], a[1] - b[1], a[2] - b[2] };
	}

	float dot(const std::array<float, 3>& a, const std::array<float, 3>& b)
	{
		return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
	}

	template<std::size_t size>
	std::array<float, size> lerp_vector(
		const std::array<float, size> &low_array, const std::array<float, size> &high_array, const float coefficient)
	{
		std::array<float, size> result;
		for (std::size_t i = 0; i < size; ++i)
		{
			result[i] = std::lerp(low_array[i], high_array[i], coefficient);
		}
		return result;
	};

	std::array<float, 4> slerp(const std::array<float, 4>& low_quat, const std::array<float, 4>& high_quat, const float coefficient)
	{
		std::array<float, 4> result;
		// Calculate angle between quaternions
		const auto cos_half_angle =
			low_quat[0] * high_quat[0] + low_quat[1] * high_quat[1] +
			low_quat[2] * high_quat[2] + low_quat[3] * high_quat[3];
		// if low_quat == high_quat or low_quat == -high_quat
		// then angle is 0 and any input quaternion may be returned
		if (std::abs(cos_half_angle) >= 1.0f)
		{
			return low_quat;
		}

		const auto sin_half_angle = std::sqrt(1.0f - cos_half_angle * cos_half_angle);
		// If angle is 180 degrees then result is not fully defined
		// rotation may be done around any axis normal to low_quat or high_quat
		if (std::abs(sin_half_angle) < 0.001f)
		{
			result[0] = (low_quat[0] * 0.5f + high_quat[0] * 0.5f);
			result[1] = (low_quat[1] * 0.5f + high_quat[1] * 0.5f);
			result[2] = (low_quat[2] * 0.5f + high_quat[2] * 0.5f);
			result[3] = (low_quat[3] * 0.5f + high_quat[3] * 0.5f);
			return result;
		}
		// https://en.wikipedia.org/wiki/Slerp
		const auto half_angle = std::acos(cos_half_angle);
		const auto low_ratio = std::sin((1.0f - coefficient) * half_angle) / sin_half_angle;
		const auto high_ratio = std::sin(coefficient * half_angle) / sin_half_angle;
		result[0] = (low_quat[0] * low_ratio + high_quat[0] * high_ratio);
		result[1] = (low_quat[1] * low_ratio + high_quat[1] * high_ratio);
		result[2] = (low_quat[2] * low_ratio + high_quat[2] * high_ratio);
		result[3] = (low_quat[3] * low_ratio + high_quat[3] * high_ratio);
		return result;
	};
}

namespace litehtml
{
	transformation::transformation()
		: xx(0.0f), yx(0.0f), zx(0.0f), px(0.0f),
		  xy(0.0f), yy(0.0f), zy(0.0f), py(0.0f),
		  xz(0.0f), yz(0.0f), zz(0.0f), pz(0.0f),
		  x0(0.0f), y0(0.0f), z0(0.0f), w(0.0f)
	{}

	transformation::transformation(float xx, float xy, float yx, float yy, float x0, float y0)
		: xx(xx), yx(yx), zx(0.0f), px(0.0f),
		  xy(xy), yy(yy), zy(0.0f), py(0.0f),
		  xz(0.0f), yz(0.0f), zz(1.0f), pz(0.0f),
		  x0(x0), y0(y0), z0(0.0f), w(1.0f)
	{}

	transformation::transformation(float xx, float xy, float xz, float yx, float yy, float yz, float zx, float zy, float zz, float x0, float y0, float z0)
		: xx(xx), yx(yx), zx(zx), px(0.0f),
		  xy(xy), yy(yy), zy(zy), py(0.0f),
		  xz(xz), yz(yz), zz(zz), pz(0.0f),
		  x0(x0), y0(y0), z0(z0), w(1.0f) {}

	transformation::transformation(float xx, float yx, float zx, float px,
								   float xy, float yy, float zy, float py,
								   float xz, float yz, float zz, float pz,
								   float x0, float y0, float z0, float w)
		: xx(xx), yx(yx), zx(zx), px(px),
		  xy(xy), yy(yy), zy(zy), py(py),
		  xz(xz), yz(yz), zz(zz), pz(pz),
		  x0(x0), y0(y0), z0(z0), w(w) {}

	transformation& transformation::operator*=(const transformation& other)
	{
		*this = *this * other;
		return *this;
	}

	transformation transformation::operator*(const transformation& other) const
	{
		transformation result = *this;
		result.xx = other.xx * xx + other.yx * xy + other.zx * xz + other.px * x0;
		result.xy = other.xy * xx + other.yy * xy + other.zy * xz + other.py * x0;
		result.xz = other.xz * xx + other.yz * xy + other.zz * xz + other.pz * x0;
		result.yx = other.xx * yx + other.yx * yy + other.zx * yz + other.px * y0;
		result.yy = other.xy * yx + other.yy * yy + other.zy * yz + other.py * y0;
		result.yz = other.xz * yx + other.yz * yy + other.zz * yz + other.pz * y0;
		result.zx = other.xx * zx + other.yx * zy + other.zx * zz + other.px * z0;
		result.zy = other.xy * zx + other.yy * zy + other.zy * zz + other.py * z0;
		result.zz = other.xz * zx + other.yz * zy + other.zz * zz + other.pz * z0;
		result.px = other.xx * px + other.yx * py + other.zx * pz + other.px * w;
		result.py = other.xy * px + other.yy * py + other.zy * pz + other.py * w;
		result.pz = other.xz * px + other.yz * py + other.zz * pz + other.pz * w;
		result.x0 = other.x0 * xx + other.y0 * xy + other.z0 * xz + other.w * x0;
		result.y0 = other.x0 * yx + other.y0 * yy + other.z0 * yz + other.w * y0;
		result.z0 = other.x0 * zx + other.y0 * zy + other.z0 * zz + other.w * z0;
		result.w = other.x0 * px + other.y0 * py + other.z0 * pz + other.w * w;
		return result;
	}

	bool transformation::operator==(const transformation& other) const
	{
		return xx == other.xx && yx == other.yx && zx == other.zx && px == other.px &&
			xy == other.xy && yy == other.yy && zy == other.zy && py == other.py &&
			xz == other.xz && yz == other.yz && zz == other.zz && pz == other.pz &&
			x0 == other.x0 && y0 == other.y0 && z0 == other.z0 && w == other.w;
	}

	std::array<float, 2> transformation::apply(const std::array<float, 2>& point) const
	{
		const auto _w = px * point[0] + py * point[1] + w;
		std::array<float, 2> result;
		result[0] = (xx * point[0] + xy * point[1] + x0) / _w;
		result[1] = (yx * point[0] + yy * point[1] + y0) / _w;
		return result;
	}

	std::array<float, 3> transformation::apply(const std::array<float, 3>& point) const
	{
	  const auto _w = px * point[0] + py * point[1] + pz * point[2] + w;
		std::array<float, 3> result;
		result[0] = (xx * point[0] + xy * point[1] + xz * point[2] + x0) / _w;
		result[1] = (yx * point[0] + yy * point[1] + yz * point[2] + y0) / _w;
		result[2] = (zx * point[0] + zy * point[1] + zz * point[2] + z0) / _w;
		return result;
	}

	std::array<float, 4> transformation::apply(const std::array<float, 4>& point) const
	{
		std::array<float, 4> result;
		result[0] = point[0] * xx + point[1] * xy + point[2] * xz + point[3] * x0;
		result[1] = point[0] * yx + point[1] * yy + point[2] * yz + point[3] * y0;
		result[2] = point[0] * zx + point[1] * zy + point[2] * zz + point[3] * z0;
		result[3] = point[0] * px + point[1] * py + point[2] * pz + point[3] * w;
		return result;
	}

	transformation transformation::inverse() const
	{
		float s0 = xx * yy - xy * yx;
		float s1 = xx * zy - xy * zx;
		float s2 = xx * py - xy * px;
		float s3 = yx * zy - yy * zx;
		float s4 = yx * py - yy * px;
		float s5 = zx * py - zy * px;
		float c5 = zz * w - z0 * pz;
		float c4 = yz * w - y0 * pz;
		float c3 = yz * z0 - y0 * zz;
		float c2 = xz * w - x0 * pz;
		float c1 = xz * z0 - x0 * zz;
		float c0 = xz * y0 - x0 * yz;

		// TODO: check for 0 determinant
		float invdet = 1.0f / (s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0);

		transformation inverse;

		inverse.xx = ( yy * c5 - zy * c4 + py * c3) * invdet;
		inverse.yx = (-yx * c5 + zx * c4 - px * c3) * invdet;
		inverse.zx = ( y0 * s5 - z0 * s4 + w * s3) * invdet;
		inverse.px = (-yz * s5 + zz * s4 - pz * s3) * invdet;

		inverse.xy = (-xy * c5 + zy * c2 - py * c1) * invdet;
		inverse.yy = ( xx * c5 - zx * c2 + px * c1) * invdet;
		inverse.zy = (-x0 * s5 + z0 * s2 - w * s1) * invdet;
		inverse.py = ( xz * s5 - zz * s2 + pz * s1) * invdet;

		inverse.xz = ( xy * c4 - yy * c2 + py * c0) * invdet;
		inverse.yz = (-xx * c4 + yx * c2 - px * c0) * invdet;
		inverse.zz = ( x0 * s4 - y0 * s2 + w * s0) * invdet;
		inverse.pz = (-xz * s4 + yz * s2 - pz * s0) * invdet;

		inverse.x0 = (-xy * c3 + yy * c1 - zy * c0) * invdet;
		inverse.y0 = ( xx * c3 - yx * c1 + zx * c0) * invdet;
		inverse.z0 = (-x0 * s3 + y0 * s1 - z0 * s0) * invdet;
		inverse.w = ( xz * s3 - yz * s1 + zz * s0) * invdet;

		return inverse;
	}

	transformation transformation::inverse3x3() const
	{
		transformation result = identity();
		// First find cofactor matrix for 3x3 submatrix
		result.xx = yy * zz - yz * zy;
		result.xy = -(yx * zz - yz * zx);
		result.xz = yx * zy - yy * zx;
		result.yx = -(xy * zz - xz * zy);
		result.yy = xx * zz - xz * zx;
		result.yz = -(xx * zy - xy * zx);
		result.zx = xy * yz - xz * yy;
		result.zy = -(xx * yz - xz * yx);
		result.zz = xx * yy - xy * yx;
		// Fast determinant calculation from cofactor matrix
		// TODO add processing for cases when determinant is zero
		const float det = xx * result.xx + yx * result.yx + zx * result.zx;
		// By transposing cofactor matrix it becomes adjoint matrix
		result = result.transposed();
		// Dividing adjoint matrix by determinant of the original matrix it becomes inverse
		result.xx /= det;
		result.xy /= det;
		result.xz /= det;
		result.yx /= det;
		result.yy /= det;
		result.yz /= det;
		result.zx /= det;
		result.zy /= det;
		result.zz /= det;
		return result;
	}

	float transformation::determinant3x3() const
	{
		const float a = yy * zz - yz * zy;
		const float b = -(xy * zz - xz * zx);
		const float c = xy * yz - xz * yy;
		const float det = xx * a + yx * b * xz * c;
		return det;
	}

	transformation transformation::normalized() const
	{
		transformation result = *this;
		if (w != 1.0f && w != 0.0f)
		{
		  result.xx /= w;
			result.xy /= w;
			result.xz /= w;
			result.yx /= w;
			result.yy /= w;
			result.yz /= w;
			result.zx /= w;
			result.zy /= w;
			result.zz /= w;
			result.px /= w;
			result.py /= w;
			result.pz /= w;
			result.x0 /= w;
			result.y0 /= w;
			result.z0 /= w;
			result.w = 1.0f;
		}
		return result;
	}

	transformation transformation::transposed() const
	{
		transformation result = *this;
		std::swap(result.xy, result.yx);
		std::swap(result.xz, result.zx);
		std::swap(result.yz, result.zy);
		std::swap(result.x0, result.px);
		std::swap(result.y0, result.py);
		std::swap(result.z0, result.pz);
		return result;
	}

	void transformation::decompose(std::array<float, 3>& translation, std::array<float, 4>& rotation_quaternion, std::array<float, 3>& scale,
		std::array<float, 3>& skew, std::array<float, 4>& projection) const
	{
		// Starting from matrix A that is initial matrix that will be simplified step by step
		transformation A = normalized();

		// 1. Projection extraction A = P*B, where P - projection matrix, B - other matrices
		transformation B = A;
		B.px = B.py = B.pz = 0.0f;

		// Procced calculations only if we have projection components
		if (A.px != 0 || A.py != 0 || A.pz != 0) {
			const std::array<float, 4> projection_vector{A.px, A.py, A.pz, A.w};
			// To find final projection result we need transposed inversed matrix B applied to current projection vector
			// Since translation component doesn't take actions in this calculations, matrix B can be treated as 3x3 matrix
			// for simplier calculations
			projection = B.inverse3x3().transposed().apply(projection_vector);
			A.px = A.py = A.pz = 0.0f;
		} else {
			projection[0] = projection[1] = projection[2] = 0.0f;
			projection[3] = 1.0f;
		}
		
		// 2. Translation extraction B = T*C, where T - translation matrix, C - other matrices
		translation[0] = B.x0;
		translation[1] = B.y0;
		translation[2] = B.z0;
		B.x0 = B.y0 = B.z0 = 0.0f;
		
		// At this point matrix B is 3x3 matrix that consists of scale, skew and rotation
		// B = S*H*R, where S - scale, H - skew/shear, R - rotation
		// For simpler calculations represent B as vectors
		std::array<std::array<float, 3>, 3> vectors;
		vectors[0][0] = B.xx;
		vectors[0][1] = B.yx;
		vectors[0][2] = B.zx;
		vectors[1][0] = B.xy;
		vectors[1][1] = B.yy;
		vectors[1][2] = B.zy;
		vectors[2][0] = B.xz;
		vectors[2][1] = B.yz;
		vectors[2][2] = B.zz;

		// 3. Scale and skew extraction and orthogonalize rotation matrix
		// Scale components is equal to vectors length with additional ortogonalization of other vectors
		scale[0] = vector_length(vectors[0]);
		normalize_vector(vectors[0]);
		// Skew components is equal to dot product between two vectors, for XY skew it's 0 and 1 vector
		skew[0] = dot(vectors[0], vectors[1]);
		vectors[1] = subtract(vectors[1], multiply(vectors[0], skew[0]));
		scale[1] = vector_length(vectors[1]);
		normalize_vector(vectors[1]);
		skew[0] /= scale[1];
		skew[1] = dot(vectors[0], vectors[2]);
		vectors[2] = subtract(vectors[2], multiply(vectors[0], skew[1]));
		skew[2] = dot(vectors[1], vectors[2]);
		vectors[2] = subtract(vectors[2], multiply(vectors[1], skew[2]));
		scale[2] = vector_length(vectors[2]);
		normalize_vector(vectors[2]);
		skew[1] /= scale[2];
		skew[2] /= scale[2];

		// After scale and skew extraction matrix becomes orthonormal and this is pure rotation matrix
		// 4. Quaternionization of rotation matrix
		transformation R = identity();
		R.xx = vectors[0][0];
		R.yx = vectors[0][1];
		R.zx = vectors[0][2];
		R.xy = vectors[1][0];
		R.yy = vectors[1][1];
		R.zy = vectors[1][2];
		R.xz = vectors[2][0];
		R.yz = vectors[2][1];
		R.zz = vectors[2][2];
		// If the determinant of rotation matrix is negative then its coordinate system is flipped
		// negate rotation matrix and the scale components extracted in previous step
		if (R.determinant3x3() < 0)
		{
			scale = multiply(scale, -1.0f);
			R.xx *= -1.0f;
			R.yx *= -1.0f;
			R.zx *= -1.0f;
			R.xy *= -1.0f;
			R.yy *= -1.0f;
			R.zy *= -1.0f;
			R.xz *= -1.0f;
			R.yz *= -1.0f;
			R.zz *= -1.0f;
		}

		// Axis-angle form for rotation matrix:
		//     [ c+(1-c)x^2  (1-c)xy-sz  (1-c)xz+sy ]
		// R = [ (1-c)xy+sz  c+(1-c)y^2  (1-c)yz-sx ]
		//     [ (1-c)xz-sy  (1-c)yz+sx  c+(1-c)z^2 ]
		// Where c - cos(theta), s - sin(theta), [x,y,z] - axis or rotation
		// Quaternion form:
		//     [ 1-2(y^2+z^2)    2(xy-zw)      2(xz+yw)   ]
		// r = [   2(xy+zw)    1-2(x^2+z^2)    2(yz-xw)   ]
		//     [   2(xz-yw)      2(yz+xw)    1-2(x^2+y^2) ]
		// Where quaternion q = (x,y,y,w)

		// https://en.wikipedia.org/wiki/Rotation_matrix#Quaternion
		float r, s, x, y, z, w;
		float trace = R.xx + R.yy + R.zz;
		// This is numerically stable so long as the trace is not negative;
		// otherwise, we risk dividing by (nearly) zero
		// When trace is negative look for diagonal element with the greatest magnitude
		// and cycling between w, x, y, z
		if (1 + trace > 0.001f) {
			r = std::sqrt(1.0f + trace);
			s = 0.5f / r;
			w = 0.5f * r;
			x = (R.zy - R.yz) * s;
			y = (R.xz - R.zx) * s;
			z = (R.yx - R.xy) * s;
		} else if (R.xx > R.yy && R.xx > R.zz) {
			// R_xx is largest.
			r = std::sqrt(1.0f + R.xx - R.yy - R.zz);
			s = 0.5f / r;
			w = (R.zy - R.yz) * s;
			x = 0.5f * r;
			y = (R.xy - R.yx) * s;
			z = (R.xz + R.zx) * s;
		} else if (R.yy > R.zz) {
			// R_yy is largest.
			r = std::sqrt(1.0f - R.xx + R.yy - R.zz);
			s = 0.5f / r;
			w = (R.xz - R.zx) * s;
			x = (R.xy + R.yx) * s;
			y = 0.5f * r;
			z = (R.yz + R.zy) * s;
		} else {
			// R_zz is largest.
			r = std::sqrt(1.0f - R.xx - R.yy + R.zz);
			s = 0.5f / r;
			w = (R.yx - R.xy) * s;
			x = (R.xz + R.zx) * s;
			y = (R.yz + R.zy) * s;
			z = 0.5f * r;
		}
		rotation_quaternion[0] = x;
		rotation_quaternion[1] = y;
		rotation_quaternion[2] = z;
		rotation_quaternion[3] = w;
	}

	transformation transformation::compose(const std::array<float, 3>& translation,
								 const std::array<float, 4>& rotation_quaternion,
								 const std::array<float, 3>& scale,
								 const std::array<float, 3>& skew,
								 const std::array<float, 4>& projection)
	{
		transformation result = identity();
		
		result.x0 = translation[0];
		result.y0 = translation[1];
		result.z0 = translation[2];
		
		result.px = projection[0];
		result.py = projection[1];
		result.pz = projection[2];
		result.w = projection[3];

		float xx = rotation_quaternion[0] * rotation_quaternion[0];
		float yy = rotation_quaternion[1] * rotation_quaternion[1];
		float zz = rotation_quaternion[2] * rotation_quaternion[2];
		float xz = rotation_quaternion[0] * rotation_quaternion[2];
		float xy = rotation_quaternion[0] * rotation_quaternion[1];
		float yz = rotation_quaternion[1] * rotation_quaternion[2];
		float xw = rotation_quaternion[3] * rotation_quaternion[0];
		float yw = rotation_quaternion[3] * rotation_quaternion[1];
		float zw = rotation_quaternion[3] * rotation_quaternion[2];
		transformation rotation_matrix(
			1.0f - 2.0f * (yy + zz), 2.0f * (xy - zw), 2.0f * (xz + yw),
			2.0f * (xy + zw), 1.0f - 2.0f * (xx + zz), 2.0f * (yz - xw),
			2.0f * (xz - yw), 2.0f * (yz + xw), 1.0f - 2.0f * (xx + yy),
			0.0f, 0.0f, 0.0f
		);
		result *= rotation_matrix;

		if (skew[2] != 0.0f)
		{
			transformation temp = transformation::identity();
			temp.yz = skew[2];
			result *= temp;
		}
		if (skew[1] != 0.0f)
		{
			transformation temp = transformation::identity();
			temp.xz = skew[1];
			result *= temp;
		}
		if (skew[0] != 0.0f)
		{
			transformation temp = transformation::identity();
			temp.xy = skew[0];
			result *= temp;
		}

		result.xx *= scale[0];
		result.yx *= scale[0];
		result.zx *= scale[0];
		result.px *= scale[0];
		result.xy *= scale[1];
		result.yy *= scale[1];
		result.zy *= scale[1];
		result.py *= scale[1];
		result.xz *= scale[2];
		result.yz *= scale[2];
		result.zz *= scale[2];
		result.pz *= scale[2];

		return result;
	}

	transformation transformation::interpolate(const transformation& low, const transformation& high, float coefficient)
	{
		if (low == high)
		{
			return low;
		}
		if (coefficient <= 0.0f)
		{
			return low;
		}
		if (coefficient >= 1.0f)
		{
			return high;
		}

		std::array<float, 3> low_translation, high_translation;
		std::array<float, 3> low_scale, high_scale;
		std::array<float, 3> low_skew, high_skew;
		std::array<float, 4> low_projection, high_projection;
		std::array<float, 4> low_rotation, high_rotation;

		low.decompose(low_translation, low_rotation, low_scale, low_skew, low_projection);
		high.decompose(high_translation, high_rotation, high_scale, high_skew, high_projection);

		const std::array<float, 3> translation = lerp_vector(low_translation, high_translation, coefficient);
		const std::array<float, 3> scale = lerp_vector(low_scale, high_scale, coefficient);
		const std::array<float, 3> skew = lerp_vector(low_skew, high_skew, coefficient);
		const std::array<float, 4> rotation = slerp(low_rotation, high_rotation, coefficient);
		const std::array<float, 4> projection = lerp_vector(low_projection, high_projection, coefficient);

		return compose(translation, rotation, scale, skew, projection);
	}

	transformation transformation::get_with_coefficient(const transformation& transform, float coefficient)
	{
		if (coefficient <= 0.0f)
		{
			return identity();
		}
		if (coefficient >= 1.0f)
		{
			return transform;
		}
		
		std::array<float, 3> translation, scale, skew;
		std::array<float, 4> projection, rotation;
		transform.decompose(translation, rotation, scale, skew, projection);

		translation = lerp_vector(std::array<float, 3>{0.0f, 0.0f, 0.0f}, translation, coefficient);
		scale = lerp_vector(std::array<float, 3>{1.0f, 1.0f, 1.0f}, scale, coefficient);
		skew = lerp_vector(std::array<float, 3>{0.0f, 0.0f, 0.0f}, skew, coefficient);
		projection = lerp_vector(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}, projection, coefficient);
		rotation = slerp(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}, rotation, coefficient);

		return compose(translation, rotation, scale, skew, projection);
	}
}