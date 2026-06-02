//
// Created by PC on 11/10/2025.
//

#pragma once
#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <cstdlib>

struct Color {
    float r, g, b, a;

    // Constructors
    inline Color();
    inline Color(float red, float green, float blue, float alpha = 255.0f);
    inline Color(uint32_t hexValue);

    // Conversion
    inline std::string toHexString() const;
    inline uint32_t toHexValue() const;

    // Static factory
    inline static Color fromHexString(const std::string& hexString);
    inline static Color fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255);
    inline static Color fromHSLA(float h, float s, float l, float a = 1.0f);
    inline static Color fromHSVA(float h, float s, float v, float a = 1.0f);
    inline static Color Random();

    // Predefined
    inline static Color Red();
    inline static Color Green();
    inline static Color Blue();
    inline static Color White();
    inline static Color Black();
    inline static Color Transparent();
    inline static Color Gray();
    inline static Color Yellow();
    inline static Color Pink();
    inline static Color Silver();
    inline static Color Gold();

    // Operators
    inline bool operator==(const Color& other) const;
    inline bool operator!=(const Color& other) const;
    inline Color operator+(const Color& other) const;
    inline Color operator-(const Color& other) const;
    inline Color operator*(float scalar) const;
    inline Color operator/(float scalar) const;
    inline Color& operator+=(const Color& other);
    inline Color& operator-=(const Color& other);
    inline Color& operator*=(float scalar);
    inline Color& operator/=(float scalar);

    // Friend operator for scalar * Color
    inline friend Color operator*(float scalar, const Color& c);
    inline friend Color operator/(float scalar, const Color& c);
};

// ===================================================
// Implementation
// ===================================================

inline Color::Color(){
    r = g = b = 0;
    a = 255;
}
inline Color::Color(float red, float green, float blue, float alpha){
    r = red;
    g = green;
    b = blue;
    a = alpha;
}
inline Color::Color(uint32_t hexValue) {
    r = (hexValue >> 16) & 0xFF;
    g = (hexValue >> 8) & 0xFF;
    b = hexValue & 0xFF;
    a = (hexValue >> 24) & 0xFF;
}

// Conversion
inline std::string Color::toHexString() const {
    std::stringstream ss;
    ss << "#" << std::hex << std::setw(2) << std::setfill('0') << (int) a << std::setw(2) << std::setfill('0') << (int) r << std::setw(2) << std::setfill('0') << (int) g << std::setw(2) << std::setfill('0') << (int) b;
    return ss.str();
}
inline uint32_t Color::toHexValue() const {
    return (uint32_t(a) << 24) | (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(b);
}

// Factory
inline Color Color::fromHexString(const std::string& hexString) {
    std::string hex = hexString[0] == '#' ? hexString.substr(1) : hexString;
    uint32_t hexValue = std::stoul(hex, nullptr, 16);
    if (hex.length() == 6) {
        hexValue |= 0xFF000000;
    }
    return Color(hexValue);
}
inline Color Color::fromRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color((float) r, (float) g, (float) b, (float) a);
}
inline Color Color::fromHSLA(float h, float s, float l, float a) {
    float c = (1 - fabs(2 * l - 1)) * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = l - c / 2;
    float r1, g1, b1;
    if (h < 60) {
        r1 = c;
        g1 = x;
        b1 = 0;
    } else if (h < 120) {
        r1 = x;
        g1 = c;
        b1 = 0;
    } else if (h < 180) {
        r1 = 0;
        g1 = c;
        b1 = x;
    } else if (h < 240) {
        r1 = 0;
        g1 = x;
        b1 = c;
    } else if (h < 300) {
        r1 = x;
        g1 = 0;
        b1 = c;
    } else {
        r1 = c;
        g1 = 0;
        b1 = x;
    }
    return Color((r1 + m) * 255, (g1 + m) * 255, (b1 + m) * 255, a * 255);
}
inline Color Color::fromHSVA(float h, float s, float v, float a) {
    float c = v * s;
    float x = c * (1 - fabs(fmod(h / 60.0f, 2) - 1));
    float m = v - c;
    float r1, g1, b1;
    if (h < 60) {
        r1 = c;
        g1 = x;
        b1 = 0;
    } else if (h < 120) {
        r1 = x;
        g1 = c;
        b1 = 0;
    } else if (h < 180) {
        r1 = 0;
        g1 = c;
        b1 = x;
    } else if (h < 240) {
        r1 = 0;
        g1 = x;
        b1 = c;
    } else if (h < 300) {
        r1 = x;
        g1 = 0;
        b1 = c;
    } else {
        r1 = c;
        g1 = 0;
        b1 = x;
    }
    return Color((r1 + m) * 255, (g1 + m) * 255, (b1 + m) * 255, a * 255);
}
inline Color Color::Random() {
    return Color(rand() % 256, rand() % 256, rand() % 256, 255);
}

// Presets
inline Color Color::Red() {
    return Color(255, 0, 0, 255);
}
inline Color Color::Green() {
    return Color(0, 255, 0, 255);
}
inline Color Color::Blue() {
    return Color(0, 0, 255, 255);
}
inline Color Color::White() {
    return Color(255, 255, 255, 255);
}
inline Color Color::Black() {
    return Color(0, 0, 0, 255);
}
inline Color Color::Transparent() {
    return Color(0, 0, 0, 0);
}
inline Color Color::Gray() {
    return Color(128, 128, 128, 255);
}
inline Color Color::Yellow() {
    return Color(255, 255, 0, 255);
}
inline Color Color::Pink() {
    return Color(255, 192, 203, 255);
}
inline Color Color::Silver() {
    return Color(192, 192, 192, 255);
}
inline Color Color::Gold() {
    return Color(255, 215, 0, 255);
}

// Operators
inline bool Color::operator==(const Color& o) const {
    return r == o.r && g == o.g && b == o.b && a == o.a;
}
inline bool Color::operator!=(const Color& o) const {
    return !(*this == o);
}

inline Color Color::operator+(const Color& o) const {
    return Color(r + o.r, g + o.g, b + o.b, a + o.a);
}
inline Color Color::operator-(const Color& o) const {
    return Color(r - o.r, g - o.g, b - o.b, a - o.a);
}
inline Color Color::operator*(float s) const {
    return Color(r * s, g * s, b * s, a * s);
}
inline Color Color::operator/(float s) const {
    return Color(r / s, g / s, b / s, a / s);
}

inline Color& Color::operator+=(const Color& o) {
    r += o.r;
    g += o.g;
    b += o.b;
    a += o.a;
    return *this;
}
inline Color& Color::operator-=(const Color& o) {
    r -= o.r;
    g -= o.g;
    b -= o.b;
    a -= o.a;
    return *this;
}
inline Color& Color::operator*=(float s) {
    r *= s;
    g *= s;
    b *= s;
    a *= s;
    return *this;
}
inline Color& Color::operator/=(float s) {
    r /= s;
    g /= s;
    b /= s;
    a /= s;
    return *this;
}

// Friend scalar ops
inline Color operator*(float s, const Color& c) {
    return Color(c.r * s, c.g * s, c.b * s, c.a * s);
}
inline Color operator/(float s, const Color& c) {
    return Color(s / c.r, s / c.g, s / c.b, s / c.a);
}
