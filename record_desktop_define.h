#ifndef RECORD_DESKTOP_DEFINE
#define RECORD_DESKTOP_DEFINE

/*
* Record typee
*
*/
typedef enum {
	DT_DESKTOP_NO = 0,
	DT_DESKTOP_FFMPEG_GDI,
	DT_DESKTOP_FFMPEG_DSHOW,
	DT_DESKTOP_WIN_GDI,
	DT_DESKTOP_WIN_DUPLICATION,
  DT_DESKTOP_WIN_WGC,
	DT_DESKTOP_WIN_MAG
}RECORD_DESKTOP_TYPES;

/*
* Record desktop data type
*
*/

typedef enum {
	AT_DESKTOP_NO = 0,
	AT_DESKTOP_RGBA,
	AT_DESKTOP_BGRA
}RECORD_DESKTOP_DATA_TYPES;

/**
* Record desktop rect
*
*/

typedef struct {
	int left;
	int top;
	int right;
	int bottom;
}RECORD_DESKTOP_RECT;





#endif
