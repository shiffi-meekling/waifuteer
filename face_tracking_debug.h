//see face_tracking.h for more main interface 
#pragma once
#include "face_tracking.h"
#include "vec.h"


namespace tracking {

	struct features {
		float eye_l;
		float eye_r;
		float eyebrow_steepness_l;
		float eyebrow_updown_l;
		float eyebrow_quirk_l;
		float eyebrow_steepness_r;
		float eyebrow_updown_r;
		float eyebrow_quirk_r;
		float mouth_corner_updown_l;
		float mouth_corner_inout_l;
		float mouth_corner_updown_r;
		float mouth_corner_inout_r;
		float mouth_open;
		float mouth_wide;
	};

#pragma pack(push, 1)
	struct OpenSeeFacePacket {
		//reconstructed from looking at facetracker.py
		//source code
		double now;
		int face_id;
		float width;
		float height;
		float eye_blink[2];
		unsigned char success; //either 0 or 1
		float pnp_error;
		float quaternion[4];
		float euler[3];
		float translation[3];
		//then lms values
		//then x,y for each point?
		float lms_confidence[68];
		vec2 lms_yx[68]; //yx pairs for each lms
		vec3 pts_3d[70]; //xyz tripples for each 3d point
		struct features features;
	};


#pragma pack(pop)
	OpenSeeFacePacket get_packet();
}
// vim: ts=2 sw=2

