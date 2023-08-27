#include "record_desktop.h"

am::record_desktop::record_desktop()
{
	_running = false;
	_paused = false;
	_inited = false;

	_on_data = nullptr;
	_on_error = nullptr;

	_device_name = "";
	_data_type = RECORD_DESKTOP_DATA_TYPES::AT_DESKTOP_BGRA;

	_time_base = { 1,AV_TIME_BASE };
	_start_time = 0;
	_pixel_fmt = AV_PIX_FMT_NONE;
}

am::record_desktop::~record_desktop()
{
	
}
