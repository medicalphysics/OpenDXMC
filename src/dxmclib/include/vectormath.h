 
#pragma once

#include <cstdint>
#include <cmath>
//Header library for simple 3D vector math

namespace vectormath {



template<typename T>
inline void normalize(T vec[3])
{
    const T lsqr = vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2];
    const T norm = T(1) / std::sqrt(lsqr);
    vec[0] *= norm;
    vec[1] *= norm;
    vec[2] *= norm;
}

template<typename T>
inline T dot(const T v1[3], const T v2[3])
{
    return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

template<typename T>
inline void cross(const T v1[3], const T v2[3], T res[3])
{
    res[0] = v1[1] * v2[2] - v1[2] * v2[1];
    res[1] = v1[2] * v2[0] - v1[0] * v2[2];
    res[2] = v1[0] * v2[1] - v1[1] * v2[0];
}
template<typename T>
inline void cross(const T v1[6], T res[3])
{
    res[0] = v1[1] * v1[5] - v1[2] * v1[4];
    res[1] = v1[2] * v1[3] - v1[0] * v1[5];
    res[2] = v1[0] * v1[4] - v1[1] * v1[3];
}


template<typename T>
inline void rotate(T vec[3], const T axis[3], const T angle)
{
    const T sang = std::sin(angle);
    const T cang = std::cos(angle);
    constexpr T temp = T(1);
    const T midt = (temp - cang) * dot(vec, axis);

    T out[3];
    out[0] = cang * vec[0] + midt * axis[0] + sang * ( axis[1] * vec[2] - axis[2] * vec[1]);
    out[1] = cang * vec[1] + midt * axis[1] + sang * (-axis[0] * vec[2] + axis[2] * vec[0]);
    out[2] = cang * vec[2] + midt * axis[2] + sang * ( axis[0] * vec[1] - axis[1] * vec[0]);

    vec[0] = out[0];
    vec[1] = out[1];
    vec[2] = out[2];
}


template<typename U, typename T>
inline U argmin3(const T vec[3])
{
    const T x = std::abs(vec[0]);
    const T y = std::abs(vec[1]);
    const T z = std::abs(vec[2]);
    if((x <= y) && (x <= z))
    {
        return 0;
    }
    else if((y <= x) && y <= z)
    {
        return 1;
    }
    return 2;
}

template<typename U, typename T>
inline U argmax3(const T vec[3])
{
	const T x = std::abs(vec[0]);
	const T y = std::abs(vec[1]);
	const T z = std::abs(vec[2]);
	if ((x > y) && (x > z))
	{
		return 0;
	}
	else if ((y > x) && y > z)
	{
		return 1;
	}
	return 2;
}


template<typename T>
inline void changeBasis(const T b1[3], const T b2[3], const T b3[3], const T vector[3], T newVector[3])
{
	newVector[0] = b1[0] * vector[0] + b2[0] * vector[1] + b3[0] * vector[2];
	newVector[1] = b1[1] * vector[0] + b2[1] * vector[1] + b3[1] * vector[2];
	newVector[2] = b1[2] * vector[0] + b2[2] * vector[1] + b3[2] * vector[2];
}
template<typename T>
inline void changeBasis(const T b1[3], const T b2[3], const T b3[3], T vector[3])
{
	T newVector[3];
	newVector[0] = b1[0] * vector[0] + b2[0] * vector[1] + b3[0] * vector[2];
	newVector[1] = b1[1] * vector[0] + b2[1] * vector[1] + b3[1] * vector[2];
	newVector[2] = b1[2] * vector[0] + b2[2] * vector[1] + b3[2] * vector[2];
	vector[0] = newVector[0];
	vector[1] = newVector[1];
	vector[2] = newVector[2];
}

template<typename T>
inline void changeBasisInverse(const T b1[3], const T b2[3], const T b3[3], const T vector[3], T newVector[3])
{
    newVector[0] = b1[0] * vector[0] + b1[1] * vector[1] + b1[2] * vector[2];
    newVector[1] = b2[0] * vector[0] + b2[1] * vector[1] + b2[2] * vector[2];
    newVector[2] = b3[0] * vector[0] + b3[1] * vector[1] + b3[2] * vector[2];
}

template<typename T>
inline void changeBasisInverse(const T b1[3], const T b2[3], const T b3[3], T vector[3])
{
    T newVector[3];
    newVector[0] = b1[0] * vector[0] + b1[1] * vector[1] + b1[2] * vector[2];
    newVector[1] = b2[0] * vector[0] + b2[1] * vector[1] + b2[2] * vector[2];
    newVector[2] = b3[0] * vector[0] + b3[1] * vector[1] + b3[2] * vector[2];
    vector[0] = newVector[0];
    vector[1] = newVector[1];
    vector[2] = newVector[2];
}


template<typename T>
inline void peturb(T vec[3], const T theta, const T phi)
{
    // rotates a unit vector theta degrees from its current direction
    // phi degrees about a arbitrary axis orthogonal to the direction vector

    // First we find a vector orthogonal to the vector direction
    T vec_xy[3], k[3] = {0, 0, 0};

    auto minInd = argmin3<std::uint_fast32_t, T>(vec);

    k[minInd] = 1.0;

    vectormath::cross(vec, k , vec_xy);
    //normalize(vec_xy);


    
    // rotating the arbitrary orthogonal axis about vector direction
    rotate(vec_xy, vec, phi);

    //rotating vector about theta with vector addition (they are orthonormal)
    const T tsin = std::sin(theta);
    const T tcos = std::cos(theta);
    vec[0] = vec[0] * tcos + vec_xy[0] * tsin;
    vec[1] = vec[1] * tcos + vec_xy[1] * tsin;
    vec[2] = vec[2] * tcos + vec_xy[2] * tsin;
}

}
