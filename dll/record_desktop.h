#ifndef RECORD_DESKTOP
#define RECORD_DESKTOP

#include "record_desktop_define.h"

#include "headers_ffmpeg.h"

#include <atomic>
#include <thread>
#include <functional>
#include <string>

namespace am {
	typedef std::function<void(AVFrame *frame)> cb_desktop_data;
	typedef std::function<void(int)> cb_desktop_error;

	class record_desktop
	{
	public:
		record_desktop();
		virtual ~record_desktop();

		virtual int init(
			const RECORD_DESKTOP_RECT &rect,
			const int fps
		) = 0;

		virtual int start() = 0;
		virtual int pause() = 0;
		virtual int resume() = 0;
		virtual int stop() = 0;

		inline const AVRational & get_time_base() { return _time_base; }

		inline int64_t get_start_time() { return _start_time; }

		inline AVPixelFormat get_pixel_fmt() { return _pixel_fmt; }

	public:
		inline bool is_recording() { return _running; }
		inline const std::string & get_device_name() { return _device_name; }
		inline const RECORD_DESKTOP_DATA_TYPES get_data_type() { return _data_type; }
		inline void registe_cb(
			cb_desktop_data on_data,
			cb_desktop_error on_error) {
			_on_data = on_data;
			_on_error = on_error;
		}
		inline const RECORD_DESKTOP_RECT & get_rect() { return _rect; }

		inline const int get_frame_rate() { return _fps; }

	protected:
		virtual void clean_up() = 0;

	protected:
		std::atomic_bool _running;
		std::atomic_bool _paused;
		std::atomic_bool _inited;

		std::thread _thread;

		std::string _device_name;

		RECORD_DESKTOP_RECT _rect;
		RECORD_DESKTOP_DATA_TYPES _data_type;

		int _fps;

		cb_desktop_data _on_data;
		cb_desktop_error _on_error;

		AVRational _time_base;
		int64_t _start_time;
		AVPixelFormat _pixel_fmt;
	};
}


#endif