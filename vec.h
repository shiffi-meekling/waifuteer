#pragma once

#include <math.h>
#include <format>
#include <ostream>

class vec2 {
	public:
	float x;
	float y;

	//builds a vector from other types, scaled such that {width,height} -> {1,1}
	template<class T> static vec2 from(T x, T y, T width, T height) {
		double dx = static_cast<double>(x) / static_cast<double>(width);
		double dy = static_cast<double>(y) / static_cast<double>(height);
		return {static_cast<float>(dx), static_cast<float>(dy)} ;
	};

	bool operator==(const vec2& rhs) const = default;

	vec2 operator+(vec2 rhs) const {
		return {x + rhs.x, y + rhs.y};
	};

	vec2 operator+=(vec2 rhs) {
		x += rhs.x;
		y += rhs.y;
		return {x,y};
	}

	vec2 operator*=(vec2 rhs) {
		x *= rhs.x;
		y *= rhs.y;
		return {x,y};
	}

	vec2 operator*=(float rhs) {
		x *= rhs;
		y *= rhs;
		return {x,y};
	}

	vec2 operator/=(vec2 rhs) {
		x /= rhs.x;
		y /= rhs.y;
		return {x,y};
	}

	vec2 operator-=(vec2 rhs) {
		x -= rhs.x;
		y -= rhs.y;
		return {x,y};
	}

	vec2 operator-(vec2 rhs) const {
		return {x - rhs.x, y - rhs.y};
	};

	vec2 operator/(vec2 rhs) const {
		return {x / rhs.x, y / rhs.y};
	};

	bool isnan() const {
		return std::isnan(x) || std::isnan(y);
	}

	float dist(vec2 rhs) const {
		return sqrt(pow(x-rhs.x,2) + pow(y-rhs.y, 2));
	}

	float dot(vec2 rhs) const {
		return x*rhs.x+y*rhs.y;
	}

	float cross(vec2 rhs) const {
		return x*rhs.y - y*rhs.x;
	}

	//perpendicular
	vec2 perpen() const {
		return {y, -x};
	}

	float mag() const {
		return sqrt(x*x + y*y);
	}


	float operator[](int i) const {
		if (i<=0) return x;
		else return y;
	};

	vec2 norm() const;
	vec2 towards(float percent, vec2 target) const;
	vec2 towards(float percent, vec2 target, vec2 relative2) const;

	//non-relative towords
	vec2 towords(float dist, vec2 target) const;

	//poor woman's matrix multiplication
	vec2 emplace(vec2 u, vec2 v, vec2 base) const;
};

vec2 operator-(vec2 rhs) ;

vec2 operator*(float c, vec2 rhs) ;
vec2 operator*(vec2 rhs, float c) ;
vec2 operator*(vec2 l, vec2 rhs) ;

vec2 operator/(vec2 rhs,  float c ) ;
vec2 operator/(float c, vec2 rhs) ;

std::ostream& operator<<(std::ostream& out, vec2 rhs) ;
// vec2 exp(vec2);

inline float abs(vec2 a) { return a.mag(); };
inline bool isNan(vec2 a) { return a.isnan(); };


bool circle_lock(vec2& lockee, const vec2 target, const float radius);
bool circle_bump(vec2& bumped, const vec2 target, const float radius);

#define VEC2_MAP_HEADER(x) vec2 x(vec2);
#define VEC2_MAP(xxx) vec2 xxx(vec2 a) {return {xxx(a.x), xxx(a.y)};};

VEC2_MAP_HEADER(exp);
VEC2_MAP_HEADER(expm1);

template<>
struct std::formatter<vec2> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const vec2& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {})", v.x, v.y);
    }
};

class vec3 {
	public:
	float x;
	float y;
	float z;

	float dist(vec3 rhs) const {
		return sqrt(pow(x-rhs.x,2) + pow(y-rhs.y, 2) + pow(z-rhs.z, 2));
	}

	float operator[](int i) const {
		if (i<=0) return x;
		else if (i==1) return y;
		else return z;
	};
};


template<>
struct std::formatter<vec3> {
    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const vec3& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "({}, {}, {})", v.x, v.y, v.z);
    }
};


// vim: ts=2 sw=2
