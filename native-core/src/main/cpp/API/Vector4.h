//
// Created by PC on 11/10/2025.
//

#pragma once
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

struct Vector4 {
    float x, y, z, w;

    inline Vector4();
    inline Vector4(float x, float y, float z, float w);
    inline Vector4(float x, float y, float z);
    inline Vector4 Normalize();
    inline float Magnitude();
    inline std::string ToString();


    inline static Vector4 Lerp(const Vector4& a, const Vector4& b, float t);
    inline static float Dot(const Vector4& a, const Vector4& b);

    // Unary
    inline Vector4 operator+() const { return *this; }
    inline Vector4 operator-() const { return Vector4(-x, -y, -z, -w); }

    // Compound
    inline Vector4& operator+=(const Vector4& o);
    inline Vector4& operator-=(const Vector4& o);
    inline Vector4& operator*=(float s);
    inline Vector4& operator/=(float s);
};

// Binary & comparison
inline Vector4 operator+(const Vector4& a, const Vector4& b);
inline Vector4 operator-(const Vector4& a, const Vector4& b);
inline Vector4 operator*(const Vector4& a, float b);
inline Vector4 operator*(float a, const Vector4& b);
inline Vector4 operator/(const Vector4& a, float b);
inline Vector4 operator/(const Vector4& a, const Vector4& b);
inline bool operator==(const Vector4& a, const Vector4& b);
inline bool operator!=(const Vector4& a, const Vector4& b);

Vector4::Vector4() {
    x = 0;
    y = 0;
    z = 0;
    w = 0;
}
Vector4::Vector4(float x, float y, float z, float w) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}
Vector4::Vector4(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = 0;
}
Vector4 Vector4::Normalize() {
    float mag = Magnitude();
    if (mag == 0) {
        throw std::runtime_error("Cannot normalize a zero-length vector");
    }
    return Vector4(x / mag, y / mag, z / mag, w / mag);
}
float Vector4::Magnitude() {
    return std::sqrt(x * x + y * y + z * z + w * w);
}
std::string Vector4::ToString() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "Vector4(" << x << ", " << y << ", " << z << ", " << w << ")";
    return oss.str();
}
Vector4 Vector4::Lerp(const Vector4& a, const Vector4& b, float t) {
    if (t < 0.0f || t > 1.0f) {
        throw std::out_of_range("t must be in the range [0, 1]");
    }
    return Vector4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}
float Vector4::Dot(const Vector4& a, const Vector4& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

// Compound
inline Vector4& Vector4::operator+=(const Vector4& o){ x+=o.x; y+=o.y; z+=o.z; w+=o.w; return *this; }
inline Vector4& Vector4::operator-=(const Vector4& o){ x-=o.x; y-=o.y; z-=o.z; w-=o.w; return *this; }
inline Vector4& Vector4::operator*=(float s){ x*=s; y*=s; z*=s; w*=s; return *this; }
inline Vector4& Vector4::operator/=(float s){ x/=s; y/=s; z/=s; w/=s; return *this; }

// Binary
inline Vector4 operator+(const Vector4& a,const Vector4& b){ return {a.x+b.x,a.y+b.y,a.z+b.z,a.w+b.w}; }
inline Vector4 operator-(const Vector4& a,const Vector4& b){ return {a.x-b.x,a.y-b.y,a.z-b.z,a.w-b.w}; }
inline Vector4 operator*(const Vector4& a,float b){ return {a.x*b,a.y*b,a.z*b,a.w*b}; }
inline Vector4 operator*(float a,const Vector4& b){ return {a*b.x,a*b.y,a*b.z,a*b.w}; }
inline Vector4 operator/(const Vector4& a,float b){ return {a.x/b,a.y/b,a.z/b,a.w/b}; }
inline Vector4 operator/(const Vector4& a,const Vector4& b){ return {a.x/b.x,a.y/b.y,a.z/b.z,a.w/b.w}; }

// Compare
inline bool operator==(const Vector4& a,const Vector4& b){ return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w; }
inline bool operator!=(const Vector4& a,const Vector4& b){ return !(a==b); }