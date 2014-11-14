/*====================================================================
Copyright (c) 2013-2014 Qualcomm Technologies, Inc. All Rights Reserved
======================================================================*/

#ifdef __ANDROID__
#include <cutils/properties.h>    //Path:system/core/include/cutils/properties.h
#endif

/*=============================================================
 * Function   : isp_ch_util_dump_frame
 *
 * Description: dump isp frame by property system
 *
 * PARAMETERS :
 *
 * RETURN     : none
 *============================================================*/
static void isp_ch_util_dump_frame(int ion_fd, mct_stream_info_t *stream_info,
  isp_frame_buffer_t *image_buf, uint32_t frame_idx, boolean dump_to_fs)
{
  ISP_DBG(ISP_MOD_COM, "%s: E", __func__);
  int32_t enabled = 0;
  char value[PROPERTY_VALUE_MAX];
  char buf[64];
  int frm_num = 10;   //default frame number to be dumped
  static int sDumpFrmCnt = 0;   //static variable
  
  /* Usage: To enable dumps
    Preview: adb shell setprop persist.camera.isp.dump 2
      CAM_STREAM_TYPE_PREVIEW(1) 
    Snapshot: adb shell setprop persist.camera.isp.dump 8 
      CAM_STREAM_TYPE_SNAPSHOT(3) 
    Video: adb shell setprop persist.camera.isp.dump 16
      CAM_STREAM_TYPE_VIDEO(4) 
    To dump 10 frames again, just reset prop value to 0 and then set again
  */
  property_get("persist.camera.isp.dump", value, "0");
  enabled = atoi(value);
  ISP_DBG(ISP_MOD_COM, "%s:frame_idx %d enabled %d streamtype %d width %d height %d",
  __FUNCTION__, frame_idx, enabled, stream_info->stream_type,
  stream_info->dim.width, stream_info->dim.height);
  
  //  CAM_STREAM_TYPE_METADATA(7) special to metadata 
  if(stream_info->stream_type == CAM_STREAM_TYPE_METADATA){
    if(!dump_to_fs){
      return;
    }
    enabled = 1<<CAM_STREAM_TYPE_METADATA;
    frm_num = 1;
  }
  if(!enabled){
    sDumpFrmCnt = 0;
    return;
  }
  
  if((1<<(int)stream_info->stream_type) & enabled){
    CDBG_HIGH("%s: dump enabled for stream %d", __FUNCTION__,
      stream_info->stream_type);
  
    if(sDumpFrmCnt >= 0 && sDumpFrmCnt <= frm_num){
      int w, h;
      int file_fd;
      w = stream_info->dim.width;
      h = stream_info->dim.height;
      
      switch(stream_info->stream_type){
      case CAM_STREAM_TYPE_PREVIEW:
        snprintf(buf, sizeof(buf), "/data/isp_dump_%d_preview_%d_%d.yuv",
          frame_idx, w, h);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        break;
      
      case CAM_STREAM_TYPE_VIDEO:
        snprintf(buf, sizeof(buf), "/data/isp_dump_%d_video_%d_%d.yuv",
          frame_idx, w, h);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        break;
      
      case CAM_STREAM_TYPE_POSTVIEW:
        snprintf(buf, sizeof(buf), "/data/isp_dump_%d_postview_%d_%d.yuv",
          frame_idx, w, h);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        break;
        
      case CAM_STREAM_TYPE_SNAPSHOT:
        snprintf(buf, sizeof(buf), "/data/isp_dump_%d_snapshot_%d_%d.yuv",
          frame_idx, w, h);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        break;
        
      case CAM_STREAM_TYPE_METADATA:
        snprintf(buf, sizeof(buf), "/data/isp_dump_%d_embedded_%d_%d.yuv",
          frame_idx, w, h);
        file_fd = open(buf, O_RDWR | O_CREAT, 0777);
        break;
        
      case CAM_STREAM_TYPE_RAW:
      case CAM_STREAM_TYPE_DEFAULT:
      default:
        w = h = 0;
        file_fd = -1;
        break;
      }
    
      if(file_fd < 0){
        CDBG_ERROR("%s: cannot open file\n", __func__);
      }else{
        ISP_DBG(ISP_MOD_COM, "%s: num_planes %d", __FUNCTION__, image_buf->buffer.length);
        unsigned int i = 0;
        uint8_t * vaddr = (uint8_t *)image_buf->vaddr;
        
        for(i = 0; i < image_buf->buffer.length; i++){
        ISP_DBG(ISP_MOD_COM, "%s: file_fd %d vaddr %x, size %d \n", __FUNCTION__, file_fd,
        (unsigned int)(vaddr + image_buf->buffer.m.planes[i].data_offset),
        image_buf->buffer.m.planes[i].length);
        
        write(file_fd,
          (const void *)(vaddr + image_buf->buffer.m.planes[i].data_offset),
          image_buf->buffer.m.planes[i].length);
        }
        
        close(file_fd);
        /* buffer is invalidated from cache */
        isp_do_cache_inv_ion(ion_fd, image_buf);
      }
    }
    sDumpFrmCnt++;
  }
end:
  return;
}
