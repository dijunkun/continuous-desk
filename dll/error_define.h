#ifndef ERROR_DEFINE
#define ERROR_DEFINE


enum AM_ERROR{
	AE_NO = 0,
	AE_ERROR,
	AE_UNSUPPORT,
	AE_INVALID_CONTEXT,
	AE_NEED_INIT,
	AE_TIMEOUT,
	AE_ALLOCATE_FAILED,

	//AE_CO_
	AE_CO_INITED_FAILED,
	AE_CO_CREATE_FAILED,
	AE_CO_GETENDPOINT_FAILED,
	AE_CO_ACTIVE_DEVICE_FAILED,
	AE_CO_GET_FORMAT_FAILED,
	AE_CO_AUDIOCLIENT_INIT_FAILED,
	AE_CO_GET_CAPTURE_FAILED,
	AE_CO_CREATE_EVENT_FAILED,
	AE_CO_SET_EVENT_FAILED,
	AE_CO_START_FAILED,
	AE_CO_ENUMENDPOINT_FAILED,
	AE_CO_GET_ENDPOINT_COUNT_FAILED,
	AE_CO_GET_ENDPOINT_ID_FAILED,
	AE_CO_OPEN_PROPERTY_FAILED,
	AE_CO_GET_VALUE_FAILED,
	AE_CO_GET_BUFFER_FAILED,
	AE_CO_RELEASE_BUFFER_FAILED,
	AE_CO_GET_PACKET_FAILED,
	AE_CO_PADDING_UNEXPECTED,

	//AE_FFMPEG_
	AE_FFMPEG_OPEN_INPUT_FAILED,
	AE_FFMPEG_FIND_STREAM_FAILED,
	AE_FFMPEG_FIND_DECODER_FAILED,
	AE_FFMPEG_OPEN_CODEC_FAILED,
	AE_FFMPEG_READ_FRAME_FAILED,
	AE_FFMPEG_READ_PACKET_FAILED,
	AE_FFMPEG_DECODE_FRAME_FAILED,
	AE_FFMPEG_NEW_SWSCALE_FAILED,
	AE_FFMPEG_FIND_ENCODER_FAILED,
	AE_FFMPEG_ALLOC_CONTEXT_FAILED,
	AE_FFMPEG_ENCODE_FRAME_FAILED,
	AE_FFMPEG_ALLOC_FRAME_FAILED,
	AE_FFMPEG_OPEN_IO_FAILED,
	AE_FFMPEG_CREATE_STREAM_FAILED,
	AE_FFMPEG_COPY_PARAMS_FAILED,
	AE_RESAMPLE_INIT_FAILED,
	AE_FFMPEG_NEW_STREAM_FAILED,
	AE_FFMPEG_FIND_INPUT_FMT_FAILED,
	AE_FFMPEG_WRITE_HEADER_FAILED,
	AE_FFMPEG_WRITE_TRAILER_FAILED,
	AE_FFMPEG_WRITE_FRAME_FAILED,

	//AE_FILTER_
	AE_FILTER_ALLOC_GRAPH_FAILED,
	AE_FILTER_CREATE_FILTER_FAILED,
	AE_FILTER_PARSE_PTR_FAILED,
	AE_FILTER_CONFIG_FAILED,
	AE_FILTER_INVALID_CTX_INDEX,
	AE_FILTER_ADD_FRAME_FAILED,

	//AE_GDI_
	AE_GDI_GET_DC_FAILED,
	AE_GDI_CREATE_DC_FAILED,
	AE_GDI_CREATE_BMP_FAILED,
	AE_GDI_BITBLT_FAILED,
	AE_GDI_GET_DIBITS_FAILED,

	//AE_D3D_
	AE_D3D_LOAD_FAILED,
	AE_D3D_GET_PROC_FAILED,
	AE_D3D_CREATE_DEVICE_FAILED,
	AE_D3D_QUERYINTERFACE_FAILED,
	AE_D3D_CREATE_VERTEX_SHADER_FAILED,
	AE_D3D_CREATE_INLAYOUT_FAILED,
	AE_D3D_CREATE_PIXEL_SHADER_FAILED,
	AE_D3D_CREATE_SAMPLERSTATE_FAILED,

	//AE_DXGI_
	AE_DXGI_GET_PROC_FAILED,
	AE_DXGI_GET_ADAPTER_FAILED,
	AE_DXGI_GET_FACTORY_FAILED,
	AE_DXGI_FOUND_ADAPTER_FAILED,

	//AE_DUP_
	AE_DUP_ATTATCH_FAILED,
	AE_DUP_QI_FAILED,
	AE_DUP_GET_PARENT_FAILED,
	AE_DUP_ENUM_OUTPUT_FAILED,
	AE_DUP_DUPLICATE_MAX_FAILED,
	AE_DUP_DUPLICATE_FAILED,
	AE_DUP_RELEASE_FRAME_FAILED,
	AE_DUP_ACQUIRE_FRAME_FAILED,
	AE_DUP_QI_FRAME_FAILED,
	AE_DUP_CREATE_TEXTURE_FAILED,
	AE_DUP_QI_DXGI_FAILED,
	AE_DUP_MAP_FAILED,
	AE_DUP_GET_CURSORSHAPE_FAILED,

	//AE_REMUX_
	AE_REMUX_RUNNING,
	AE_REMUX_NOT_EXIST,
	AE_REMUX_INVALID_INOUT,

	// AE_WGC_
  AE_WGC_CREATE_CAPTURER_FAILED,

	AE_MAX
};

static const char *ERRORS_STR[] = {
	"no error",                         //AE_NO
	"error",                            //AE_ERROR
	"not support for now",              //AE_UNSUPPORT
	"invalid context",                  //AE_INVALID_CONTEXT
	"need init first",                  //AE_NEED_INIT
	"operation timeout",                //AE_TIMEOUT
	"allocate memory failed",           //AE_ALLOCATE_FAILED,

	"com init failed",                  //AE_CO_INITED_FAILED
	"com create instance failed",       //AE_CO_CREATE_FAILED
	"com get endpoint failed",          //AE_CO_GETENDPOINT_FAILED
	"com active device failed",         //AE_CO_ACTIVE_DEVICE_FAILED
	"com get wave formatex failed",     //AE_CO_GET_FORMAT_FAILED
	"com audio client init failed",     //AE_CO_AUDIOCLIENT_INIT_FAILED
	"com audio get capture failed",     //AE_CO_GET_CAPTURE_FAILED
	"com audio create event failed",    //AE_CO_CREATE_EVENT_FAILED
	"com set ready event failed",       //AE_CO_SET_EVENT_FAILED
	"com start to record failed",       //AE_CO_START_FAILED
	"com enum audio endpoints failed",  //AE_CO_ENUMENDPOINT_FAILED
	"com get endpoints count failed",   //AE_CO_GET_ENDPOINT_COUNT_FAILED
	"com get endpoint id failed",       //AE_CO_GET_ENDPOINT_ID_FAILED
	"com open endpoint property failed", //AE_CO_OPEN_PROPERTY_FAILED
	"com get property value failed",    //AE_CO_GET_VALUE_FAILED
	"com get buffer failed",            //AE_CO_GET_BUFFER_FAILED
	"com release buffer failed",        //AE_CO_RELEASE_BUFFER_FAILED
	"com get packet size failed",       //AE_CO_GET_PACKET_FAILED
	"com get padding size unexpected",  //AE_CO_PADDING_UNEXPECTED

	"ffmpeg open input failed",         //AE_FFMPEG_OPEN_INPUT_FAILED
	"ffmpeg find stream info failed",   //AE_FFMPEG_FIND_STREAM_FAILED
	"ffmpeg find decoder failed",       //AE_FFMPEG_FIND_DECODER_FAILED
	"ffmpeg open codec failed",         //AE_FFMPEG_OPEN_CODEC_FAILED
	"ffmpeg read frame failed",         //AE_FFMPEG_READ_FRAME_FAILED
	"ffmpeg read packet failed",        //AE_FFMPEG_READ_PACKET_FAILED
	"ffmpeg decode frame failed",       //AE_FFMPEG_DECODE_FRAME_FAILED
	"ffmpeg create swscale failed",     //AE_FFMPEG_NEW_SWSCALE_FAILED

	"ffmpeg find encoder failed",       //AE_FFMPEG_FIND_ENCODER_FAILED
	"ffmpeg alloc context failed",      //AE_FFMPEG_ALLOC_CONTEXT_FAILED
	"ffmpeg encode frame failed",       //AE_FFMPEG_ENCODE_FRAME_FAILED
	"ffmpeg alloc frame failed",        //AE_FFMPEG_ALLOC_FRAME_FAILED
	
	"ffmpeg open io ctx failed",        //AE_FFMPEG_OPEN_IO_FAILED
	"ffmpeg new stream failed",         //AE_FFMPEG_CREATE_STREAM_FAILED
	"ffmpeg copy parameters failed",    //AE_FFMPEG_COPY_PARAMS_FAILED
	"resampler init failed",            //AE_RESAMPLE_INIT_FAILED
	"ffmpeg new out stream failed",     //AE_FFMPEG_NEW_STREAM_FAILED
	"ffmpeg find input format failed",  //AE_FFMPEG_FIND_INPUT_FMT_FAILED
	"ffmpeg write file header failed",  //AE_FFMPEG_WRITE_HEADER_FAILED
	"ffmpeg write file trailer failed", //AE_FFMPEG_WRITE_TRAILER_FAILED
	"ffmpeg write frame failed",        //AE_FFMPEG_WRITE_FRAME_FAILED

	"avfilter alloc avfilter failed",   //AE_FILTER_ALLOC_GRAPH_FAILED
	"avfilter create graph failed",     //AE_FILTER_CREATE_FILTER_FAILED
	"avfilter parse ptr failed",        //AE_FILTER_PARSE_PTR_FAILED
	"avfilter config graph failed",     //AE_FILTER_CONFIG_FAILED
	"avfilter invalid ctx index",       //AE_FILTER_INVALID_CTX_INDEX
	"avfilter add frame failed",        //AE_FILTER_ADD_FRAME_FAILED

	"gdi get dc failed",                //AE_GDI_GET_DC_FAILED
	"gdi create dc failed",             //AE_GDI_CREATE_DC_FAILED
	"gdi create bmp failed",            //AE_GDI_CREATE_BMP_FAILED
	"gdi bitblt failed",                //AE_GDI_BITBLT_FAILED
	"gid geet dibbits failed",          //AE_GDI_GET_DIBITS_FAILED

	"d3d11 library load failed",        //AE_D3D_LOAD_FAILED
	"d3d11 proc get failed",            //AE_D3D_GET_PROC_FAILED
	"d3d11 create device failed",       //AE_D3D_CREATE_DEVICE_FAILED
	"d3d11 query interface failed",     //AE_D3D_QUERYINTERFACE_FAILED
	"d3d11 create vertex shader failed",//AE_D3D_CREATE_VERTEX_SHADER_FAILED
	"d3d11 create input layout failed", //AE_D3D_CREATE_INLAYOUT_FAILED
	"d3d11 create pixel shader failed", //AE_D3D_CREATE_PIXEL_SHADER_FAILED
	"d3d11 create sampler state failed",//AE_D3D_CREATE_SAMPLERSTATE_FAILED

	"dxgi get proc address failed",     //AE_DXGI_GET_PROC_FAILED
	"dxgi get adapter failed",          //AE_DXGI_GET_ADAPTER_FAILED
	"dxgi get factory failed",          //AE_DXGI_GET_FACTORY_FAILED
	"dxgi specified adapter not found", //AE_DXGI_FOUND_ADAPTER_FAILED

	"duplication attatch desktop failed", //AE_DUP_ATTATCH_FAILED
	"duplication query interface failed", //AE_DUP_QI_FAILED
	"duplication get parent failed",      //AE_DUP_GET_PARENT_FAILED
	"duplication enum ouput failed",      //AE_DUP_ENUM_OUTPUT_FAILED
	"duplication duplicate unavailable",  //AE_DUP_DUPLICATE_MAX_FAILED
	"duplication duplicate failed",       //AE_DUP_DUPLICATE_FAILED
	"duplication release frame failed",   //AE_DUP_RELEASE_FRAME_FAILED
	"duplication acquire frame failed",   //AE_DUP_ACQUIRE_FRAME_FAILED
	"duplication qi frame failed",        //AE_DUP_QI_FRAME_FAILED
	"duplication create texture failed",  //AE_DUP_CREATE_TEXTURE_FAILED
	"duplication dxgi qi failed",         //AE_DUP_QI_DXGI_FAILED
	"duplication map rects failed",       //AE_DUP_MAP_FAILED
	"duplication get cursor shape failed",//AE_DUP_GET_CURSORSHAPE_FAILED

	"remux is already running",           //AE_REMUX_RUNNING
	"remux input file do not exist",      //AE_REMUX_NOT_EXIST
	"remux input or output file invalid", //AE_REMUX_INVALID_INOUT
};

#define err2str(e) e < AE_MAX ? ERRORS_STR[e] : "unknown"

#define AMERROR_CHECK(err) if(err != AE_NO) return err

#endif // !ERROR_DEFINE