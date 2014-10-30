/* Copyright (c) 2012-2014 The Linux Foundataion. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are
* met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of The Linux Foundation nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
* ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
* BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
* OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#define LOG_TAG "QCameraStream"

#include <utils/Errors.h>
#include "QCamera2HWI.h"
#include "QCameraStream.h"

#define CAMERA_MIN_ALLOCATED_BUFFERS     3

namespace qcamera {
/*===========================================================================
 * FUNCTION   : get_bufs
 *
 * DESCRIPTION: static function entry to allocate stream buffers
 *
 * PARAMETERS :
 *   @offset     : offset info of stream buffers
 *   @num_bufs   : number of buffers allocated
 *   @initial_reg_flag: flag to indicate if buffer needs to be registered
 *                      at kernel initially
 *   @bufs       : output of allocated buffers
 *   @ops_tbl    : ptr to buf mapping/unmapping ops
 *   @user_data  : user data ptr of ops_tbl
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStream::get_bufs(
                     cam_frame_len_offset_t *offset,
                     uint8_t *num_bufs,
                     uint8_t **initial_reg_flag,
                     mm_camera_buf_def_t **bufs,
                     mm_camera_map_unmap_ops_tbl_t *ops_tbl,
                     void *user_data)
{
    QCameraStream *stream = reinterpret_cast<QCameraStream *>(user_data);
    if (!stream) {
        ALOGE("getBufs invalid stream pointer");
        return NO_MEMORY;
    }
    return stream->getBufs(offset, num_bufs, initial_reg_flag, bufs, ops_tbl);
}

/*===========================================================================
 * FUNCTION   : QCameraStream
 *
 * DESCRIPTION: constructor of QCameraStream
 *
 * PARAMETERS :
 *   @allocator  : memory allocator obj
 *   @camHandle  : camera handle
 *   @chId       : channel handle
 *   @camOps     : ptr to camera ops table
 *   @paddingInfo: ptr to padding info
 *
 * RETURN     : None
 *==========================================================================*/
QCameraStream::QCameraStream(QCameraAllocator &allocator,
                             uint32_t camHandle,
                             uint32_t chId,
                             mm_camera_ops_t *camOps,
                             cam_padding_info_t *paddingInfo,
                             bool deffered) :
        mDumpFrame(0),
        mDumpMetaFrame(0),
        mDumpSkipCnt(0),
        mCamHandle(camHandle),
        mChannelHandle(chId),
        mHandle(0),
        mCamOps(camOps),
        mStreamInfo(NULL),
        mNumBufs(0),
        mNumBufsNeedAlloc(0),
        mDataCB(NULL),
        mUserData(NULL),
        mDataQ(releaseFrameData, this),
        mStreamInfoBuf(NULL),
        mStreamBufs(NULL),
        mAllocator(allocator),
        mBufDefs(NULL),
        mStreamBufsAcquired(false),
        m_bActive(false),
        mDynBufAlloc(false),
        mBufAllocPid(0),
        mDefferedAllocation(deffered),
        wait_for_cond(false)
{
    mMemVtbl.user_data = this;
    if ( !deffered ) {
        mMemVtbl.get_bufs = get_bufs;
        mMemVtbl.put_bufs = put_bufs;
    } else {
        mMemVtbl.get_bufs = get_bufs_deffered;
        mMemVtbl.put_bufs = put_bufs_deffered;
    }
    mMemVtbl.invalidate_buf = invalidate_buf;
    mMemVtbl.clean_invalidate_buf = clean_invalidate_buf;
    memset(&mFrameLenOffset, 0, sizeof(mFrameLenOffset));
    memcpy(&mPaddingInfo, paddingInfo, sizeof(cam_padding_info_t));
    memset(&mCropInfo, 0, sizeof(cam_rect_t));
    memset(&m_MemOpsTbl, 0, sizeof(mm_camera_map_unmap_ops_tbl_t));
    memset(&m_OutputCrop, 0, sizeof(cam_stream_parm_buffer_t));
    memset(&m_ImgProp, 0, sizeof(cam_stream_parm_buffer_t));
    pthread_mutex_init(&mCropLock, NULL);
    pthread_mutex_init(&mParameterLock, NULL);
}

/*===========================================================================
 * FUNCTION   : getBufs
 *
 * DESCRIPTION: allocate stream buffers
 *
 * PARAMETERS :
 *   @offset     : offset info of stream buffers
 *   @num_bufs   : number of buffers allocated
 *   @initial_reg_flag: flag to indicate if buffer needs to be registered
 *                      at kernel initially
 *   @bufs       : output of allocated buffers
 *   @ops_tbl    : ptr to buf mapping/unmapping ops
 *
 * RETURN     : int32_t type of status
 *              NO_ERROR  -- success
 *              none-zero failure code
 *==========================================================================*/
int32_t QCameraStream::getBufs(cam_frame_len_offset_t *offset,
        uint8_t *num_bufs,
        uint8_t **initial_reg_flag,
        mm_camera_buf_def_t **bufs,
        mm_camera_map_unmap_ops_tbl_t *ops_tbl)
{
    int rc = NO_ERROR;
    uint8_t *regFlags;

    if (!ops_tbl) {
        ALOGE("%s: ops_tbl is NULL", __func__);
        return INVALID_OPERATION;
    }

    mFrameLenOffset = *offset;

    uint8_t numBufAlloc = mNumBufs;
    mNumBufsNeedAlloc = 0;
    if (mDynBufAlloc) {
        numBufAlloc = CAMERA_MIN_ALLOCATED_BUFFERS;
        if (numBufAlloc > mNumBufs) {
            mDynBufAlloc = false;
            numBufAlloc = mNumBufs;
        } else {
            mNumBufsNeedAlloc = mNumBufs - numBufAlloc;
        }
    }

    //Allocate and map stream info buffer
    mStreamBufs = mAllocator.allocateStreamBuf(mStreamInfo->stream_type,
                                               mFrameLenOffset.frame_len,
                                               mFrameLenOffset.mp[0].stride,
                                               mFrameLenOffset.mp[0].scanline,
                                               numBufAlloc);
    mNumBufs = numBufAlloc + mNumBufsNeedAlloc;

    if (!mStreamBufs) {
        ALOGE("%s: Failed to allocate stream buffers", __func__);
        return NO_MEMORY;
    }

    for (int i = 0; i < numBufAlloc; i++) {
        rc = ops_tbl->map_ops(i, -1, mStreamBufs->getFd(i),
                mStreamBufs->getSize(i), ops_tbl->userdata);
        if (rc < 0) {
            ALOGE("%s: map_stream_buf failed: %d", __func__, rc);
            for (int j = 0; j < i; j++) {
                ops_tbl->unmap_ops(j, -1, ops_tbl->userdata);
            }
            mStreamBufs->deallocate();
            delete mStreamBufs;
            mStreamBufs = NULL;
            return INVALID_OPERATION;
        }
    }

    //regFlags array is allocated by us, but consumed and freed by mm-camera-interface
    regFlags = (uint8_t *)malloc(sizeof(uint8_t) * mNumBufs);
    if (!regFlags) {
        ALOGE("%s: Out of memory", __func__);
        for (int i = 0; i < numBufAlloc; i++) {
            ops_tbl->unmap_ops(i, -1, ops_tbl->userdata);
        }
        mStreamBufs->deallocate();
        delete mStreamBufs;
        mStreamBufs = NULL;
        return NO_MEMORY;
    }
    memset(regFlags, 0, sizeof(uint8_t) * mNumBufs);

    mBufDefs = (mm_camera_buf_def_t *)malloc(mNumBufs * sizeof(mm_camera_buf_def_t));
    if (mBufDefs == NULL) {
        ALOGE("%s: getRegFlags failed %d", __func__, rc);
        for (int i = 0; i < numBufAlloc; i++) {
            ops_tbl->unmap_ops(i, -1, ops_tbl->userdata);
        }
        mStreamBufs->deallocate();
        delete mStreamBufs;
        mStreamBufs = NULL;
        free(regFlags);
        regFlags = NULL;
        return INVALID_OPERATION;
    }
    memset(mBufDefs, 0, mNumBufs * sizeof(mm_camera_buf_def_t));
    for (int i = 0; i < numBufAlloc; i++) {
        mStreamBufs->getBufDef(mFrameLenOffset, mBufDefs[i], i);
    }

    rc = mStreamBufs->getRegFlags(regFlags);
    if (rc < 0) {
        ALOGE("%s: getRegFlags failed %d", __func__, rc);
        for (int i = 0; i < numBufAlloc; i++) {
            ops_tbl->unmap_ops(i, -1, ops_tbl->userdata);
        }
        mStreamBufs->deallocate();
        delete mStreamBufs;
        mStreamBufs = NULL;
        free(mBufDefs);
        mBufDefs = NULL;
        free(regFlags);
        regFlags = NULL;
        return INVALID_OPERATION;
    }

    *num_bufs = mNumBufs;
    *initial_reg_flag = regFlags;
    *bufs = mBufDefs;

    if (mNumBufsNeedAlloc > 0) {
        pthread_mutex_lock(&m_lock);
        wait_for_cond = TRUE;
        pthread_mutex_unlock(&m_lock);
        CDBG_HIGH("%s: Still need to allocate %d buffers",
              __func__, mNumBufsNeedAlloc);
        // remember memops table
        m_MemOpsTbl = *ops_tbl;
        // start another thread to allocate the rest of buffers
        pthread_create(&mBufAllocPid,
                       NULL,
                       BufAllocRoutine,
                       this);
    }

    return NO_ERROR;
}




}
