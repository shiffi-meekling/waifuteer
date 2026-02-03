#include "face_tracking.h"
#include <cstdio>
#include <print>
#include "face_tracking_debug.h"
#include "tracking_conditioning.h"
#include <boost/asio.hpp>
#include <thread>
#include "fps_tracker.h"

using boost::asio::ip::udp;

namespace tracking {
	const std::string config_filename = "tracking.ini";
	static bool initialized = false;
	static std::atomic<uint16_t> port; 
  boost::asio::io_context io_context;
	static std::atomic<OpenSeeFacePacket> packet;

	void listen() {
		udp::socket socket(io_context, udp::endpoint(udp::v4(), port));
		std::println("listening on port {}", port.load());
    for (;;) {
      std::array<char, sizeof( OpenSeeFacePacket)> buffer;
      udp::endpoint remote_endpoint;
      socket.receive_from(boost::asio::buffer(buffer), remote_endpoint);
			packet = bit_cast<OpenSeeFacePacket>(buffer);	
		}
	}

	void init() {
		//load the configuration for how to condition
		conditioning::init(config_filename);	
		port.store(conditioning::port());

		std::jthread server(listen);
		server.detach();

		initialized = true;
	}

	//This fuction doesn't do anything right now	
	//it was useful for a synchronous interfaces or something?
	//idk why I have it
	void new_frame() {
		if (!initialized) {
			std::println(stderr, "error: cannot get frame from camera,"
				" because tracking::init() was not called");
			std::exit(1);
		}
	}

	///converts the camera space to render space
	vec2 transform2world(vec2 in, vec2 screen_dims) {
		in = in - (screen_dims*0.5);
		in = in / screen_dims.y; 
		//we use height for both dimentions to ensure motion doesn't become
		//muted on one dimention depsite a non-square camera
		in = -in; //flip motion on both dimentions
		return in;
	}

	std::optional<metrics> get_metrics() {
		static metrics old_m = {{1.0, 1.0}, 0, 0};
		static constexpr int top_lip = 60;
		static constexpr int bottom_lip = 64;
		static constexpr int left_lip = 58;
		static constexpr int right_lip = 62;
		static constexpr int nose = 30;
		static constexpr int nose_bridge = 27;
		
		auto p = get_packet();
		const auto& points3D = p.pts_3d;
		const auto& points2D = p.lms_yx;
		vec2 screen_dim = {p.width, p.height}; 
		//we use height, height above, because the camera
		//is wider than it is tall, and thus height, width
		//results in muted x dimention movement
		vec2 nose_val = points2D[nose];
		vec2 nose_val_transformed = transform2world(nose_val, screen_dim);

		vec2 head_up_unit_vector = (points2D[nose] - points2D[nose_bridge]).norm();

		metrics m = {
			conditioning::absolute_excite(nose_val_transformed),
			head_up_unit_vector,
			conditioning::mouth_open_scale(points3D[top_lip].dist(points3D[bottom_lip])),
			conditioning::mouth_wide_scale(points3D[left_lip].dist(points3D[right_lip])),
		};
		old_m = m;
		return m;
	} 

	OpenSeeFacePacket get_packet() {
		return packet;
	}
	
}

// vim: ts=2 sw=2
