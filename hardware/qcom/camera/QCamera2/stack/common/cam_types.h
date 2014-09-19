typedef enum{
	/* applies to HAL 1 */
	CAM_STREAM_TYPE_DEFAULT,		/* 0 default stream type */
	CAM_STREAM_TYPE_PREVIEW,		/* 1 preview */
	CAM_STREAM_TYPE_POSTVIEW,		/* 2 postview */
	CAM_STREAM_TYPE_SNAPSHOT,		/* 3 snapshot */
	CAM_STREAM_TYPE_VIDEO,			/* 4 video */
	
	/* applies to HAL 3*/
	CAM_STREAM_TYPE_IMPL_DEFINED,	/* 5 opaque format: could be display, video enc, ZSL YUV */
	CAM_STREAM_TYPE_YUV,			/* 6 app requested callback stream type */
	
	/* applied to both HAL1 and HAL3*/
	CAM_STREAM_TYPE_METADATA,		/* 7 meta data */
	CAM_STREAM_TYPE_RAW,			/* 8 raw dump from camif */
	CAM_STREAM_TYPE_OFFLINE_PROC,	/* 9 offline process */
	CAM_STREAM_TYPE_MAX,
} cam_stream_type_t;
