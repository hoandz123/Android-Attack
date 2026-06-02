//
// Created by PC on 11/10/2025.
//

#pragma once
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <stdexcept>
#include "Vector3.h"
#include "Vector2Int.h"


struct Vector3Int {
    int x, y, z;

    inline Vector3Int();
    inline Vector3Int(int x, int y, int z);
    inline bool Equals(const Vector3Int& other);
    inline int GetHashCode();
    inline std::string ToString();


    inline static Vector3 ToVector3(const Vector3Int& v);
    inline static Vector2Int ToVector2Int(const Vector3Int& v);

    inline static Vector3Int Zero();
    inline static Vector3Int One();
    inline static Vector3Int Up();
    inline static Vector3Int Down();
    inline static Vector3Int Left();
    inline static Vector3Int Right();
    inline static Vector3Int Forward();
    inline static Vector3Int Back();


    inline Vector3Int operator+() const {
        return *this;
    }
    inline Vector3Int operator-() const {
        return Vector3Int(-x, -y, -z);
    }
    inline Vector3Int& operator+=(const Vector3Int& o);
    inline Vector3Int& operator-=(const Vector3Int& o);
    inline Vector3Int& operator*=(int s);
    inline Vector3Int& operator/=(int s);

};

inline Vector3Int operator+(const Vector3Int& a, const Vector3Int& b);
inline Vector3Int operator-(const Vector3Int& a, const Vector3Int& b);
inline Vector3Int operator*(const Vector3Int& a, int b);
inline Vector3Int operator*(int a, const Vector3Int& b);
inline Vector3Int operator/(const Vector3Int& a, int b);
inline Vector3Int operator/(const Vector3Int& a, const Vector3Int& b);
inline bool operator==(const Vector3Int& a, const Vector3Int& b);
inline bool operator!=(const Vector3Int& a, const Vector3Int& b);
inline bool operator<(const Vector3Int& a, const Vector3Int& b);
inline bool operator>(const Vector3Int& a, const Vector3Int& b);
inline bool operator<=(const Vector3Int& a, const Vector3Int& b);
inline bool operator>=(const Vector3Int& a, const Vector3Int& b);


Vector3Int::Vector3Int() {
    x = 0;
    y = 0;
    z = 0;
}
Vector3Int::Vector3Int(int x, int y, int z) {
    this->x = x;
    this->y = y;
    this->z = z;
}
bool Vector3Int::Equals(const Vector3Int& other) {
    return x == other.x && y == other.y && z == other.z;
}
int Vector3Int::GetHashCode() {
    return std::hash<int>()(x) ^ std::hash<int>()(y) ^ std::hash<int>()(z);
}
std::string Vector3Int::ToString() {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "(" << x << ", " << y << ", " << z << ")";
    return oss.str();
}
Vector3 Vector3Int::ToVector3(const Vector3Int& v) {
    return Vector3(static_cast<float>(v.x), static_cast<float>(v.y), static_cast<float>(v.z));
}
Vector2Int Vector3Int::ToVector2Int(const Vector3Int& v) {
    return Vector2Int(v.x, v.y);
}
Vector3Int Vector3Int::Zero() {
    return Vector3Int(0, 0, 0);
}
Vector3Int Vector3Int::One() {
    return Vector3Int(1, 1, 1);
}
Vector3Int Vector3Int::Up() {
    return Vector3Int(0, 1, 0);
}
Vector3Int Vector3Int::Down() {
    return Vector3Int(0, -1, 0);
}
Vector3Int Vector3Int::Left() {
    return Vector3Int(-1, 0, 0);
}
Vector3Int Vector3Int::Right() {
    return Vector3Int(1, 0, 0);
}
Vector3Int Vector3Int::Forward() {
    return Vector3Int(0, 0, 1);
}
Vector3Int Vector3Int::Back() {
    return Vector3Int(0, 0, -1);
}

inline Vector3Int& Vector3Int::operator+=(const Vector3Int& o) {
    x += o.x;
    y += o.y;
    z += o.z;
    return *this;
}
inline Vector3Int& Vector3Int::operator-=(const Vector3Int& o) {
    x -= o.x;
    y -= o.y;
    z -= o.z;
    return *this;
}
inline Vector3Int& Vector3Int::operator*=(int s) {
    x *= s;
    y *= s;
    z *= s;
    return *this;
}
inline Vector3Int& Vector3Int::operator/=(int s) {
    x /= s;
    y /= s;
    z /= s;
    return *this;
}
inline Vector3Int operator+(const Vector3Int& a, const Vector3Int& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
inline Vector3Int operator-(const Vector3Int& a, const Vector3Int& b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
inline Vector3Int operator*(const Vector3Int& a, int b) {
    return {a.x * b, a.y * b, a.z * b};
}
inline Vector3Int operator*(int a, const Vector3Int& b) {
    return {a * b.x, a * b.y, a * b.z};
}
inline Vector3Int operator/(const Vector3Int& a, int b) {
    return {a.x / b, a.y / b, a.z / b};
}
inline Vector3Int operator/(const Vector3Int& a, const Vector3Int& b) {
    return {a.x / b.x, a.y / b.y, a.z / b.z};
}
inline bool operator==(const Vector3Int& a, const Vector3Int& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline bool operator!=(const Vector3Int& a, const Vector3Int& b) {
    return !(a == b);
}
inline bool operator<(const Vector3Int& a, const Vector3Int& b) {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}
inline bool operator>(const Vector3Int& a, const Vector3Int& b) {
    return a.x > b.x && a.y > b.y && a.z > b.z;
}
inline bool operator<=(const Vector3Int& a, const Vector3Int& b) {
    return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}
inline bool operator>=(const Vector3Int& a, const Vector3Int& b) {
    return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}
