#include <ostream>
#include "vec.h"

vec2 operator*(float c, vec2 rhs) {
	return {c*rhs.x, c*rhs.y};
}

vec2 operator*(vec2 rhs, float c) {
	return c*rhs;
}

vec2 operator*(vec2 l, vec2 rhs) {
	return {l.x*rhs.x, l.y*rhs.y};
}

vec2 operator/(float c, vec2 rhs) {
	return {c/rhs.x, c/rhs.y};
}

vec2 operator/(vec2 rhs,  float c ) {
	return {rhs.x/c, rhs.y/c};
}

vec2 operator-(vec2 rhs) {
	return {-rhs.x, -rhs.y};
}

VEC2_MAP(exp);
VEC2_MAP(expm1);

vec2 vec2::norm() const {
		return (*this)/this->mag();
}

//relative
vec2 vec2::towards(float percent, vec2 target) const {
	return (*this)*(1-percent) + target*percent;
}

///absolute
vec2 vec2::towords (float dist, vec2 target) const {
		return *this + dist*(target-*this).norm();
}
	
vec2 vec2::towards(float percent, vec2 target, vec2 relative_to) const {
	return *this + percent*relative_to.mag()*(target-*this).norm();
}

//poor woman's matrix multiplication
vec2 vec2::emplace(vec2 u, vec2 v, vec2 base) const {
	return x*u + y*v + base;
}	

std::ostream& operator<<(std::ostream& out, vec2 rhs) {
	out << "(" << rhs.x << "," << rhs.y << ")";
	return out;
}

//force a vec2 to be within a radius
bool circle_lock(vec2& lockee, const vec2 target, const float radius) {
	float distance = lockee.dist(target);
	if (distance > radius) {
		lockee = lockee.towords(distance - radius, target);
		return true;
	}
	return false;
}

//untested
//force a vec2 to be without a radius
bool circle_bump(vec2& bumped, const vec2 target, const float radius) {
	float distance = bumped.dist(target);
	if (distance < radius) {
		bumped = bumped.towords(radius - distance, target);
		return true;
	}
	return false;
}
