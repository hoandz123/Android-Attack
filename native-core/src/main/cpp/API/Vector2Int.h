//
// Created by PC on 11/10/2025.
//

#pragma once

#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include "Vector2.h"

struct Vector2Int {
    int x;
    int y;

    inline Vector2Int();
    inline Vector2Int(int x, int y);
    inline float Magnitude();
    inline std::string ToString();

    // Unary
    inline Vector2Int operator+() const {
        return *this;
    }
    inline Vector2Int operator-() const {
        return Vector2Int(-x, -y);
    }

    // Compound
    inline Vector2Int& operator+=(const Vector2Int& o);
    inline Vector2Int& operator-=(const Vector2Int& o);
    inline Vector2Int& operator*=(int s);
    inline Vector2Int& operator/=(int s);

    // Utility
    inline int Dot(const Vector2Int& o) const;
    inline float Magnitude() const;


    inline static Vector2Int Zero();
    inline static Vector2Int One();
    inline static Vector2Int Up();
    inline static Vector2Int Down();
    inline static Vector2Int Left();
    inline static Vector2Int Right();
    inline static Vector2 ToVector2(const Vector2Int& v);
    inline static Vector2Int FloorToInt(const Vector2& v);
};
// Binary & comparison
inline Vector2Int operator+(const Vector2Int& a, const Vector2Int& b);
inline Vector2Int operator-(const Vector2Int& a, const Vector2Int& b);
inline Vector2Int operator*(const Vector2Int& a, int b);
inline Vector2Int operator*(int a, const Vector2Int& b);
inline Vector2Int operator/(const Vector2Int& a, int b);
inline Vector2Int operator/(const Vector2Int& a, const Vector2Int& b);
inline bool operator==(const Vector2Int& a, const Vector2Int& b);
inline bool operator!=(const Vector2Int& a, const Vector2Int& b);
inline bool operator<(const Vector2Int& a, const Vector2Int& b);
inline bool operator>(const Vector2Int& a, const Vector2Int& b);
inline bool operator<=(const Vector2Int& a, const Vector2Int& b);
inline bool operator>=(const Vector2Int& a, const Vector2Int& b);





Vector2Int::Vector2Int() {
    x = 0;
    y = 0;
}
Vector2Int::Vector2Int(int x, int y) {
    this->x = x;
    this->y = y;
}
float Vector2Int::Magnitude() {
    return std::sqrt(float(x * x + y * y));
}
std::string Vector2Int::ToString() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "(" << x << ", " << y << ")";
    return oss.str();
}

// Compound
inline Vector2Int& Vector2Int::operator+=(const Vector2Int& o) {
    x += o.x;
    y += o.y;
    return *this;
}
inline Vector2Int& Vector2Int::operator-=(const Vector2Int& o) {
    x -= o.x;
    y -= o.y;
    return *this;
}
inline Vector2Int& Vector2Int::operator*=(int s) {
    x *= s;
    y *= s;
    return *this;
}
inline Vector2Int& Vector2Int::operator/=(int s) {
    x /= s;
    y /= s;
    return *this;
}

// Utility
inline int Vector2Int::Dot(const Vector2Int& o) const {
    return x * o.x + y * o.y;
}
inline float Vector2Int::Magnitude() const {
    return std::sqrt(float(x * x + y * y));
}

// Binary
inline Vector2Int operator+(const Vector2Int& a, const Vector2Int& b) {
    return {a.x + b.x, a.y + b.y};
}
inline Vector2Int operator-(const Vector2Int& a, const Vector2Int& b) {
    return {a.x - b.x, a.y - b.y};
}
inline Vector2Int operator*(const Vector2Int& a, int b) {
    return {a.x * b, a.y * b};
}
inline Vector2Int operator*(int a, const Vector2Int& b) {
    return {a * b.x, a * b.y};
}
inline Vector2Int operator/(const Vector2Int& a, int b) {
    return {a.x / b, a.y / b};
}
inline Vector2Int operator/(const Vector2Int& a, const Vector2Int& b) {
    return {a.x / b.x, a.y / b.y};
}

// Compare
inline bool operator==(const Vector2Int& a, const Vector2Int& b) {
    return a.x == b.x && a.y == b.y;
}
inline bool operator!=(const Vector2Int& a, const Vector2Int& b) {
    return !(a == b);
}
inline bool operator<(const Vector2Int& a, const Vector2Int& b) {
    return a.x < b.x && a.y < b.y;
}
inline bool operator>(const Vector2Int& a, const Vector2Int& b) {
    return a.x > b.x && a.y > b.y;
}
inline bool operator<=(const Vector2Int& a, const Vector2Int& b) {
    return a.x <= b.x && a.y <= b.y;
}
inline bool operator>=(const Vector2Int& a, const Vector2Int& b) {
    return a.x >= b.x && a.y >= b.y;
}
Vector2Int Vector2Int::Zero() {
    return Vector2Int(0, 0);
}
Vector2Int Vector2Int::One() {
    return Vector2Int(1, 1);
}
Vector2Int Vector2Int::Up() {
    return Vector2Int(0, 1);
}
Vector2Int Vector2Int::Down() {
    return Vector2Int(0, -1);
}
Vector2Int Vector2Int::Left() {
    return Vector2Int(-1, 0);
}
Vector2Int Vector2Int::Right() {
    return Vector2Int(1, 0);
}
Vector2 Vector2Int::ToVector2(const Vector2Int& v) {
    return Vector2(static_cast<float>(v.x), static_cast<float>(v.y));
}
Vector2Int Vector2Int::FloorToInt(const Vector2& v) {
    return Vector2Int(static_cast<int>(std::floor(v.x)), static_cast<int>(std::floor(v.y)));
}



