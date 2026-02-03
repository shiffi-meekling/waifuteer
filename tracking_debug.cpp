#include "face_tracking.h"
#include "face_tracking_debug.h"
#include <print>

int main() {
	tracking::init();
	double old_now = 0;
	for (;;) {
		tracking::new_frame();
		tracking::OpenSeeFacePacket packet = tracking::get_packet();
		if (packet.now > old_now) {
			tracking::metrics m = *tracking::get_metrics();
			std::println("new_frame:{:.11}, {} {}", packet.now, packet.pts_3d[60], packet.pts_3d[64]);
		
			std::println("metric: {}", m);
			old_now = packet.now;
		}
	}
}

// vim: ts=2 sw=2
