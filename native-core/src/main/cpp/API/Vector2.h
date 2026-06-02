#pragma once
#include "string"
#include <cmath>

struct Vector2 {
    union {
        struct {
            float x;
            float y;
        };
        float data[2];
    };

    inline Vector2();
    inline Vector2(float data[]);
    inline Vector2(float value);
    inline Vector2(float x, float y);
    inline void Set(float newX, float newY);
    inline void Normalize();
    inline float Magnitude();
    inline std::string to_string();

    static inline float Angle(Vector2 a, Vector2 b);
    static inline Vector2 ClampMagnitude(Vector2 vector, float maxLength);
    static inline float Component(Vector2 a, Vector2 b);
    static inline float Distance(Vector2 a, Vector2 b);
    static inline float Dot(Vector2 lhs, Vector2 rhs);
    static inline Vector2 FromPolar(float rad, float theta);
    static inline Vector2 Lerp(Vector2 a, Vector2 b, float t);
    static inline Vector2 LerpUnclamped(Vector2 a, Vector2 b, float t);
    static inline Vector2 Max(Vector2 a, Vector2 b);
    static inline Vector2 Min(Vector2 a, Vector2 b);
    static inline Vector2 MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta);
    static inline Vector2 Normalized(Vector2 v);
    static inline void OrthoNormalize(Vector2& normal, Vector2& tangent);
    static inline Vector2 Project(Vector2 a, Vector2 b);
    static inline Vector2 Reflect(Vector2 vector, Vector2 line);
    static inline Vector2 Reject(Vector2 a, Vector2 b);
    static inline Vector2 RotateTowards(Vector2 current, Vector2 target, float maxRadiansDelta, float maxMagnitudeDelta);
    static inline Vector2 Scale(Vector2 a, Vector2 b);
    static inline Vector2 Slerp(Vector2 a, Vector2 b, float t);
    static inline Vector2 SlerpUnclamped(Vector2 a, Vector2 b, float t);
    static inline float SqrMagnitude(Vector2 v);
    static inline void ToPolar(Vector2 vector, float& rad, float& theta);

    static inline Vector2 Zero();
    static inline Vector2 One();
    static inline Vector2 Right();
    static inline Vector2 Left();
    static inline Vector2 Up();
    static inline Vector2 Down();

    inline struct Vector2& operator+=(const float rhs);
    inline struct Vector2& operator-=(const float rhs);
    inline struct Vector2& operator*=(const float rhs);
    inline struct Vector2& operator/=(const float rhs);
    inline struct Vector2& operator+=(const Vector2 rhs);
    inline struct Vector2& operator-=(const Vector2 rhs);
};

inline Vector2 operator-(Vector2 rhs);
inline Vector2 operator+(Vector2 lhs, const float rhs);
inline Vector2 operator-(Vector2 lhs, const float rhs);
inline Vector2 operator*(Vector2 lhs, const float rhs);
inline Vector2 operator/(Vector2 lhs, const float rhs);
inline Vector2 operator+(const float lhs, Vector2 rhs);
inline Vector2 operator-(const float lhs, Vector2 rhs);
inline Vector2 operator*(const float lhs, Vector2 rhs);
inline Vector2 operator*(Vector2 lhs, Vector2 rhs);
inline Vector2 operator/(const float lhs, Vector2 rhs);
inline Vector2 operator+(Vector2 lhs, const Vector2 rhs);
inline Vector2 operator-(Vector2 lhs, const Vector2 rhs);
inline bool operator==(const Vector2 lhs, const Vector2 rhs);
inline bool operator!=(const Vector2 lhs, const Vector2 rhs);

Vector2::Vector2(): x(0), y(0) {
}
Vector2::Vector2(float data[]): x(data[0]), y(data[1]) {
}
Vector2::Vector2(float value): x(value), y(value) {
}
Vector2::Vector2(float x, float y): x(x), y(y) {
}

Vector2 Vector2::Zero() {
    return Vector2(0, 0);
}
Vector2 Vector2::One() {
    return Vector2(1, 1);
}
Vector2 Vector2::Right() {
    return Vector2(1, 0);
}
Vector2 Vector2::Left() {
    return Vector2(-1, 0);
}
Vector2 Vector2::Up() {
    return Vector2(0, 1);
}
Vector2 Vector2::Down() {
    return Vector2(0, -1);
}

float Vector2::Angle(Vector2 a, Vector2 b) {
    float v = Dot(a, b) / (a.Magnitude() * b.Magnitude());
    v = fmax(v, -1.0);
    v = fmin(v, 1.0);
    return acos(v);
}

Vector2 Vector2::ClampMagnitude(Vector2 vector, float maxLength) {
    float length = vector.Magnitude();
    if (length > maxLength) {
        vector *= maxLength / length;
    }
    return vector;
}

float Vector2::Component(Vector2 a, Vector2 b) {
    return Dot(a, b) / b.Magnitude();
}

float Vector2::Distance(Vector2 a, Vector2 b) {
    return (a - b).Magnitude();
}

float Vector2::Dot(Vector2 lhs, Vector2 rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y;
}

Vector2 Vector2::FromPolar(float rad, float theta) {
    Vector2 v;
    v.x = rad * cos(theta);
    v.y = rad * sin(theta);
    return v;
}

Vector2 Vector2::Lerp(Vector2 a, Vector2 b, float t) {
    if (t < 0) {
        return a;
    } else if (t > 1) {
        return b;
    }
    return LerpUnclamped(a, b, t);
}

Vector2 Vector2::LerpUnclamped(Vector2 a, Vector2 b, float t) {
    return (b - a) * t + a;
}

float Vector2::Magnitude() {
    return sqrt(SqrMagnitude(*this));
}

Vector2 Vector2::Max(Vector2 a, Vector2 b) {
    float x = a.x > b.x ? a.x : b.x;
    float y = a.y > b.y ? a.y : b.y;
    return Vector2(x, y);
}

Vector2 Vector2::Min(Vector2 a, Vector2 b) {
    float x = a.x > b.x ? b.x : a.x;
    float y = a.y > b.y ? b.y : a.y;
    return Vector2(x, y);
}

Vector2 Vector2::MoveTowards(Vector2 current, Vector2 target, float maxDistanceDelta) {
    Vector2 d = target - current;
    float m = d.Magnitude();
    if (m < maxDistanceDelta || m == 0) {
        return target;
    }
    return current + (d * maxDistanceDelta / m);
}

Vector2 Vector2::Normalized(Vector2 v) {
    float mag = v.Magnitude();
    if (mag == 0) {
        return Vector2::Zero();
    }
    return v / mag;
}

void Vector2::OrthoNormalize(Vector2& normal, Vector2& tangent) {
    normal = Normalized(normal);
    tangent = Reject(tangent, normal);
    tangent = Normalized(tangent);
}

Vector2 Vector2::Project(Vector2 a, Vector2 b) {
    float m = b.Magnitude();
    return Dot(a, b) / (m * m) * b;
}

Vector2 Vector2::Reflect(Vector2 vector, Vector2 planeNormal) {
    return vector - 2 * Project(vector, planeNormal);
}

Vector2 Vector2::Reject(Vector2 a, Vector2 b) {
    return a - Project(a, b);
}

Vector2 Vector2::RotateTowards(Vector2 current, Vector2 target, float maxRadiansDelta, float maxMagnitudeDelta) {
    float magCur = current.Magnitude();
    float magTar = target.Magnitude();
    float newMag = magCur + maxMagnitudeDelta * ((magTar > magCur) - (magCur > magTar));
    newMag = fmin(newMag, fmax(magCur, magTar));
    newMag = fmax(newMag, fmin(magCur, magTar));

    float totalAngle = Angle(current, target) - maxRadiansDelta;
    if (totalAngle <= 0) {
        return Normalized(target) * newMag;
    } else if (totalAngle >= M_PI) {
        return Normalized(-target) * newMag;
    }

    float axis = current.x * target.y - current.y * target.x;
    axis = axis / fabs(axis);
    if (!(1 - fabs(axis) < 0.00001)) {
        axis = 1;
    }
    current = Normalized(current);
    Vector2 newVector = current * cos(maxRadiansDelta) + Vector2(-current.y, current.x) * sin(maxRadiansDelta) * axis;
    return newVector * newMag;
}

Vector2 Vector2::Scale(Vector2 a, Vector2 b) {
    return Vector2(a.x * b.x, a.y * b.y);
}

Vector2 Vector2::Slerp(Vector2 a, Vector2 b, float t) {
    if (t < 0) {
        return a;
    } else if (t > 1) {
        return b;
    }
    return SlerpUnclamped(a, b, t);
}

Vector2 Vector2::SlerpUnclamped(Vector2 a, Vector2 b, float t) {
    float magA = a.Magnitude();
    float magB = b.Magnitude();
    a /= magA;
    b /= magB;
    float dot = Dot(a, b);
    dot = fmax(dot, -1.0);
    dot = fmin(dot, 1.0);
    float theta = acos(dot) * t;
    Vector2 relativeVec = Normalized(b - a * dot);
    Vector2 newVec = a * cos(theta) + relativeVec * sin(theta);
    return newVec * (magA + (magB - magA) * t);
}

float Vector2::SqrMagnitude(Vector2 v) {
    return v.x * v.x + v.y * v.y;
}

void Vector2::ToPolar(Vector2 vector, float& rad, float& theta) {
    rad = vector.Magnitude();
    theta = atan2(vector.y, vector.x);
}

struct Vector2& Vector2::operator+=(const float rhs) {
    x += rhs;
    y += rhs;
    return *this;
}

struct Vector2& Vector2::operator-=(const float rhs) {
    x -= rhs;
    y -= rhs;
    return *this;
}

struct Vector2& Vector2::operator*=(const float rhs) {
    x *= rhs;
    y *= rhs;
    return *this;
}

struct Vector2& Vector2::operator/=(const float rhs) {
    x /= rhs;
    y /= rhs;
    return *this;
}

struct Vector2& Vector2::operator+=(const Vector2 rhs) {
    x += rhs.x;
    y += rhs.y;
    return *this;
}

struct Vector2& Vector2::operator-=(const Vector2 rhs) {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
}

Vector2 operator-(Vector2 rhs) {
    return rhs * -1;
}
Vector2 operator+(Vector2 lhs, const float rhs) {
    return lhs += rhs;
}
Vector2 operator-(Vector2 lhs, const float rhs) {
    return lhs -= rhs;
}
Vector2 operator*(Vector2 lhs, const float rhs) {
    return lhs *= rhs;
}
Vector2 operator/(Vector2 lhs, const float rhs) {
    return lhs /= rhs;
}
Vector2 operator+(const float lhs, Vector2 rhs) {
    return rhs += lhs;
}
Vector2 operator-(const float lhs, Vector2 rhs) {
    return rhs -= lhs;
}
Vector2 operator*(const float lhs, Vector2 rhs) {
    return rhs *= lhs;
}
Vector2 operator*(Vector2 lhs, Vector2 rhs) {
    return Vector2(lhs.x * rhs.x, lhs.y * rhs.y);
}
Vector2 operator/(const float lhs, Vector2 rhs) {
    return rhs /= lhs;
}
Vector2 operator+(Vector2 lhs, const Vector2 rhs) {
    return lhs += rhs;
}
Vector2 operator-(Vector2 lhs, const Vector2 rhs) {
    return lhs -= rhs;
}

bool operator==(const Vector2 lhs, const Vector2 rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!=(const Vector2 lhs, const Vector2 rhs) {
    return !(lhs == rhs);
}

std::string Vector2::to_string() {
    return std::to_string(x) + std::string(", ") + std::to_string(y);
}
void Vector2::Normalize() {
    float mag = Magnitude();
    if (mag == 0) {
        *this = Vector2::Zero();
    } else {
        *this /= mag;
    }
}
void Vector2::Set(float newX, float newY) {
    x = newX;
    y = newY;
}
