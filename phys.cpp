#include <ostream>
#include "phys.h"
#include <print>

void phys::haps_reset() {
	//reset things if it become nan
	if (v.isnan() || p.isnan()) {
		v = vec2(0,0);
		p = vec2(0,0);
	}
}

std::ostream& operator<<(std::ostream& out, phys rhs) {
	out << "{p:" << rhs.p << ", v:" << rhs.v << "}";
	return out;
}


void phys::tied_to(float pull_length, vec2 target, float strength) {
	float distance = p.dist(target);
	if (distance > pull_length) {
		v += strength * expm1(distance - pull_length)*
							  (target - p).norm();
	}
}

void phys::tied_to(float pull_length, vec2 target, float strength, float max_length) {
	float distance = p.dist(target);
	if (distance > max_length) {
		p.towords(distance - max_length, target);
	}
	tied_to(pull_length, target, strength);
}

void phys::push_to(float push_length, vec2 target, float strength, float min_length) {
	float distance = p.dist(target);
	if (distance < min_length) {
		//negative value so it moves away
		p.towords(distance - min_length, target);
	}
	push_to(push_length, target, strength);
}

void phys::push_to(float push_length, vec2 target, float strength) {
	float distance = p.dist(target);
	if (distance < push_length) {
		v -= strength * expm1(push_length - distance)*
							  (target - p).norm();
	}
}

vec2 elbow_ik(float length, vec2 start, vec2 end, vec2 bias = {0, -1}) {
	//start down
	vec2 el = start + bias.norm() * length;	
	//move towards hands if out of range
	if (el.dist(end) > length) el = el.towords(el.dist(end) - length, end);
	//move back towards shoulder if needed
	if (el.dist(start) > length) el = el.towords(el.dist(start) - length, start);

	//if we are out of range of the hands, move back halfway
	if (el.dist(end) > length) el = el.towords((el.dist(end) - length) * 0.5, end);
	return el;
}

void phys::elbow_ik(float length, vec2 shoulder, vec2 hand) {
	this->wind(::elbow_ik(length, shoulder, hand));
}
/*
void phys::align_with(vec2 comparison, vec2 target, float strength=0.1) {
	float angle = p.arctan2(target);
	float angle2 = p.arctan2(comparision);
}
*/

// vim: sw=2 ts=2
