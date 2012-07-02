/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
**
** @author Taikyung, Yu(taikyung.yu@samsung.com)
** @date   2010-09-10
**
*/

#define LOG_TAG "libhdmi"
#include <cutils/log.h>

#include "SecHdmi.h"

int hdmi_rotate = 0;
static unsigned int g2d_reserved_memory0 = 0;
static unsigned int g2d_reserved_memory1 = 0;
static unsigned int g2d_reserved_memory_size = 0;
static unsigned int prev_ui_top_y_address = 0;

/* I assume that s/w decoded video maximum width, height is (1280, 720)
 * this value is used for deciding fimc2 output buffer size.
 */
#define MAX_SW_DECODED_VIDEO_WIDTH      1280
#define MAX_SW_DECODED_VIDEO_HEIGHT     720

namespace android {

int tvout_v4l2_cropcap(int fp, struct v4l2_cropcap *a)
{
    struct v4l2_cropcap *cropcap = a;
    int ret;

    ret = ioctl(fp, VIDIOC_CROPCAP, cropcap);

    if (ret < 0) {
        LOGE("tvout_v4l2_cropcap" "VIDIOC_CROPCAP failed %d\n", errno);
        return ret;
    }

    LOGV("tvout_v4l2_cropcap" "bound width : %d, bound height : %d,\n",
		    cropcap->bounds.width, cropcap->bounds.height);
    return ret;
}

int tvout_v4l2_querycap(int fp)
{
    struct v4l2_capability cap;
    int ret;

    ret = ioctl(fp, VIDIOC_QUERYCAP, &cap);

    if (ret < 0) {
	    LOGE("tvout_v4l2_querycap" "VIDIOC_QUERYCAP failed %d\n", errno);
	    return ret;
    }

    LOGV("tvout_v4l2_querycap" "DRIVER : %s, CARD : %s, CAP.: 0x%08x\n",
		    cap.driver, cap.card, cap.capabilities);

    return ret;
}

/*
   ioctl VIDIOC_G_STD, VIDIOC_S_STD
   To query and select the current video standard applications use the VIDIOC_G_STD and
   VIDIOC_S_STD ioctls which take a pointer to a v4l2_std_id type as argument. VIDIOC_G_STD can
   return a single flag or a set of flags as in struct v4l2_standard field id
   */

int tvout_v4l2_g_std(int fp, v4l2_std_id *std_id)
{
    int ret;

    ret = ioctl(fp, VIDIOC_G_STD, std_id);
    if (ret < 0) {
	    LOGE("tvout_v4l2_g_std" "VIDIOC_G_STD failed %d\n", errno);
	    return ret;
    }

    return ret;
}

int tvout_v4l2_s_std(int fp, v4l2_std_id std_id)
{
    int ret;

    ret = ioctl(fp, VIDIOC_S_STD, &std_id);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_std" "VIDIOC_S_STD failed %d\n", errno);
	    return ret;
    }

    return ret;
}

/*
   ioctl VIDIOC_ENUMSTD
   To query the attributes of a video standard, especially a custom (driver defined) one, applications
   initialize the index field of struct v4l2_standard and call the VIDIOC_ENUMSTD ioctl with a pointer
   to this structure. Drivers fill the rest of the structure or return an EINVAL error code when the index
   is out of bounds.
   */
int tvout_v4l2_enum_std(int fp, struct v4l2_standard *std, v4l2_std_id std_id)
{
    std->index = 0;
    while (0 == ioctl (fp, VIDIOC_ENUMSTD, std))
    {
        if (std->id & std_id)
        {
	        LOGV("tvout_v4l2_enum_std" "Current video standard: %s\n", std->name);
        }
        std->index++;
    }

	return 0;
}

/*
   ioctl VIDIOC_ENUMOUTPUT
   To query the attributes of a video outputs applications initialize the index field of struct v4l2_output
   and call the VIDIOC_ENUMOUTPUT ioctl with a pointer to this structure. Drivers fill the rest of the
   structure or return an EINVAL error code when the index is out of bounds
   */
int tvout_v4l2_enum_output(int fp, struct v4l2_output *output)
{
    int ret;

    ret = ioctl(fp, VIDIOC_ENUMOUTPUT, output);

    if(ret >=0){
	    LOGV("tvout_v4l2_enum_output" "enum. output [index = %d] :: type : 0x%08x , name = %s\n",
			    output->index,output->type,output->name);
    }

    return ret;
}

/*
   ioctl VIDIOC_G_OUTPUT, VIDIOC_S_OUTPUT
   To query the current video output applications call the VIDIOC_G_OUTPUT ioctl with a pointer to an
   integer where the driver stores the number of the output, as in the struct v4l2_output index field.
   This ioctl will fail only when there are no video outputs, returning the EINVAL error code
   */
int tvout_v4l2_s_output(int fp, int index)
{
    int ret;

    ret = ioctl(fp, VIDIOC_S_OUTPUT, &index);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_output" "VIDIOC_S_OUTPUT failed %d\n", errno);
	    return ret;
    }

    return ret;
}

int tvout_v4l2_g_output(int fp, int *index)
{
    int ret;

    ret = ioctl(fp, VIDIOC_G_OUTPUT, index);
    if (ret < 0) {
	    LOGE("tvout_v4l2_g_output" "VIDIOC_G_OUTPUT failed %d\n", errno);
	    return ret;
    }else{
	    LOGV("tvout_v4l2_g_output" "Current output index %d\n", *index);
    }

    return ret;
}

/*
   ioctl VIDIOC_ENUM_FMT
   To enumerate image formats applications initialize the type and index field of struct v4l2_fmtdesc
   and call the VIDIOC_ENUM_FMT ioctl with a pointer to this structure. Drivers fill the rest of the
   structure or return an EINVAL error code. All formats are enumerable by beginning at index zero
   and incrementing by one until EINVAL is returned.
   */
int tvout_v4l2_enum_fmt(int fp, struct v4l2_fmtdesc *desc)
{
    desc->index = 0;
    while (0 == ioctl(fp, VIDIOC_ENUM_FMT, desc))
    {
	    LOGV("tvout_v4l2_enum_fmt" "enum. fmt [id : 0x%08x] :: type = 0x%08x, name = %s, pxlfmt = 0x%08x\n",
			    desc->index,
			    desc->type,
			    desc->description,
			    desc->pixelformat);
	    desc->index++;
    }

    return 0;
}

int tvout_v4l2_g_fmt(int fp, int buf_type, void* ptr)
{
    int ret;
    struct v4l2_format format;
    struct v4l2_pix_format_s5p_tvout *fmt_param = (struct v4l2_pix_format_s5p_tvout*)ptr;

    format.type = (enum v4l2_buf_type)buf_type;

    ret = ioctl(fp, VIDIOC_G_FMT, &format);
    if (ret < 0)
    {
	    LOGE("tvout_v4l2_g_fmt" "type : %d, VIDIOC_G_FMT failed %d\n", buf_type, errno);
	    return ret;
    }
    else
    {
	    memcpy(fmt_param, format.fmt.raw_data, sizeof(struct v4l2_pix_format_s5p_tvout));
	    LOGV("tvout_v4l2_g_fmt" "get. fmt [base_c : 0x%08x], [base_y : 0x%08x] type = 0x%08x, width = %d, height = %d\n",
			    fmt_param->base_c,
			    fmt_param->base_y,
			    fmt_param->pix_fmt.pixelformat,
			    fmt_param->pix_fmt.width,
			    fmt_param->pix_fmt.height);
    }

    return 0;
}

int tvout_v4l2_s_fmt(int fp, int buf_type, void *ptr)
{
    struct v4l2_format format;
    int ret;

    format.type = (enum v4l2_buf_type)buf_type;
    switch(buf_type)
    {
        case V4L2_BUF_TYPE_VIDEO_OUTPUT:
        {
            struct v4l2_pix_format_s5p_tvout *fmt_param = (struct v4l2_pix_format_s5p_tvout*)ptr;
            memcpy(format.fmt.raw_data, fmt_param, sizeof(struct v4l2_pix_format_s5p_tvout));
            break;
        }
        default:
            break;
    }

    ret = ioctl(fp, VIDIOC_S_FMT, &format);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_fmt" "[tvout_v4l2_s_fmt] : type : %d, VIDIOC_S_FMT failed %d\n", buf_type, errno);
	    return ret;
    }
    return 0;
}

int tvout_v4l2_g_parm(int fp, int buf_type, void *ptr)
{
    int ret;
    struct v4l2_streamparm parm;
    struct v4l2_window_s5p_tvout *vparm = (struct v4l2_window_s5p_tvout*)ptr;

    parm.type = (enum v4l2_buf_type)buf_type;

    ret = ioctl(fp, VIDIOC_G_PARM, &parm);

    if (ret < 0)
    {
	    LOGE("tvout_v4l2_g_parm" "type : %d, VIDIOC_G_PARM failed %d\n", buf_type, errno);
	    return ret;
    }
    else
    {
	    memcpy(vparm, parm.parm.raw_data, sizeof(struct v4l2_pix_format_s5p_tvout));
	    LOGV("tvout_v4l2_g_parm" "get. param : width  = %d, height = %d\n",
			    vparm->win.w.width,
			    vparm->win.w.height);
    }
    return 0;
}

int tvout_v4l2_s_parm(int fp, int buf_type, void *ptr)
{
    struct v4l2_streamparm parm;
    struct v4l2_window_s5p_tvout *vparm = (struct v4l2_window_s5p_tvout*)ptr;
    int ret;

    parm.type = (enum v4l2_buf_type)buf_type;
    memcpy(parm.parm.raw_data, vparm, sizeof(struct v4l2_window_s5p_tvout));

    ret = ioctl(fp, VIDIOC_S_PARM, &parm);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_parm" "VIDIOC_S_PARM failed %d\n", errno);
	    return ret;
    }

    return 0;
}

int tvout_v4l2_g_fbuf(int fp, struct v4l2_framebuffer *frame)
{
    int ret;

    ret = ioctl(fp, VIDIOC_G_FBUF, frame);
    if (ret < 0) {
	    LOGE("tvout_v4l2_g_fbuf" "VIDIOC_STREAMON failed %d\n", errno);
	    return ret;
    }

    LOGV("tvout_v4l2_g_fbuf" "get. fbuf: base = 0x%08X, pixel format = %d\n",
		    frame->base,
		    frame->fmt.pixelformat);
    return 0;
}

int tvout_v4l2_s_fbuf(int fp, struct v4l2_framebuffer *frame)
{
    int ret;

    ret = ioctl(fp, VIDIOC_S_FBUF, frame);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_fbuf" "VIDIOC_STREAMON failed %d\n", errno);
	    return ret;
    }
    return 0;
}

int tvout_v4l2_s_baseaddr(int fp, void *base_addr)
{
    int ret;

    ret = ioctl(fp, VIDIOC_TV_OUT_BASE_ADDR, base_addr);
    if (ret < 0) {
	    LOGE("tvout_v4l2_baseaddr" "VIDIOC_TV_OUT_BASE_ADDR failed %d\n", errno);
	    return ret;
    }
    return 0;
}

int tvout_v4l2_g_crop(int fp, unsigned int type, struct v4l2_rect *rect)
{
    int ret;
    struct v4l2_crop crop;
    crop.type = (enum v4l2_buf_type)type;
    ret = ioctl(fp, VIDIOC_G_CROP, &crop);
    if (ret < 0) {
	    LOGE("tvout_v4l2_g_crop" "VIDIOC_G_CROP failed %d\n", errno);
	    return ret;
    }

    rect->left	= crop.c.left;
    rect->top	= crop.c.top;
    rect->width	= crop.c.width;
    rect->height	= crop.c.height;

    LOGV("tvout_v4l2_g_crop" "get. crop : left = %d, top = %d, width  = %d, height = %d\n",
		    rect->left,
		    rect->top,
		    rect->width,
		    rect->height);
    return 0;
}

int tvout_v4l2_s_crop(int fp, unsigned int type, struct v4l2_rect *rect)
{
    struct v4l2_crop crop;
    int ret;

    crop.type 	= (enum v4l2_buf_type)type;

    crop.c.left 	= rect->left;
    crop.c.top 	    = rect->top;
    crop.c.width 	= rect->width;
    crop.c.height 	= rect->height;

    ret = ioctl(fp, VIDIOC_S_CROP, &crop);
    if (ret < 0) {
	    LOGE("tvout_v4l2_s_crop" "VIDIOC_S_CROP failed %d\n", errno);
	    return ret;
    }

    return 0;
}

int tvout_v4l2_streamon(int fp, unsigned int type)
{
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMON, &type);
    if (ret < 0) {
	    LOGE("tvout_v4l2_streamon" "VIDIOC_STREAMON failed %d\n", errno);
	    return ret;
    }

    LOGV("tvout_v4l2_streamon" "requested streamon buftype[id : 0x%08x]\n", type);
    return 0;
}

int tvout_v4l2_streamoff(int fp, unsigned int type)
{
    int ret;

    ret = ioctl(fp, VIDIOC_STREAMOFF, &type);
    if (ret < 0) {
	    LOGE("tvout_v4l2_streamoff""VIDIOC_STREAMOFF failed \n");
	    return ret;
    }

    LOGV("tvout_v4l2_streamoff" "requested streamon buftype[id : 0x%08x]\n", type);
    return 0;
}


int tvout_v4l2_start_overlay(int fp)
{
    int ret, start = 1;

    ret = ioctl(fp, VIDIOC_OVERLAY, &start);
    if (ret < 0) {
	    LOGE("tvout_v4l2_start_overlay" "VIDIOC_OVERLAY failed\n");
	    return ret;
    }

    return ret;
}

int tvout_v4l2_stop_overlay(int fp)
{
    int ret, stop =0;

    ret = ioctl(fp, VIDIOC_OVERLAY, &stop);
    if (ret < 0)
	    LOGE("tvout_v4l2_stop_overlay" "VIDIOC_OVERLAY failed\n");

    return ret;
}


////////////////////////////////////////////////////////////////////////

SecHdmi::SecHdmi()
:   mFlagCreate(false),
    mFlagConnected(false),
    mHdmiDstWidth(0),
    mHdmiDstHeight(0),
    mHdmiSrcYAddr(0),
    mHdmiSrcCbCrAddr(0),
    mHdmiOutputMode(HDMI_OUTPUT_MODE_YCBCR),
    mHdmiResolutionValue(DEFAULT_HDMI_RESOLUTION_VALUE),
    mHdmiStdId(DEFAULT_HDMI_STD_ID),
    mHdcpMode(false),
    mAudioMode(2),
    mCurrentHdmiOutputMode(-1),
    mCurrentHdmiResolutionValue(0), // 1080960
    mCurrentHdcpMode(false),
    mCurrentAudioMode(-1),
    mHdmiInfoChange(true),
    mFlagFimcStart(false),
    mFimcDstColorFormat(0),
    mFimcCurrentOutBufIndex(0),
    mFirstRef(false)
{
    for(int i = 0; i < HDMI_LAYER_MAX; i++)
    {
        mFlagHdmiStart[i] = false;

        mSrcWidth      [i] = 0;
        mSrcHeight     [i] = 0;
        mSrcColorFormat[i] = 0;
        mHdmiSrcWidth  [i] = 0;
        mHdmiSrcHeight [i] = 0;
    }

    mHdmiSizeOfResolutionValueList = 11;

    mHdmiResolutionValueList[0]  = 1080960;
    mHdmiResolutionValueList[1]  = 1080950;
    mHdmiResolutionValueList[2]  = 1080930;
    mHdmiResolutionValueList[3]  = 1080160;
    mHdmiResolutionValueList[4]  = 1080150;
    mHdmiResolutionValueList[5]  = 720960;
    mHdmiResolutionValueList[6]  = 720950;
    mHdmiResolutionValueList[7]  = 5769501;
    mHdmiResolutionValueList[8]  = 5769502;
    mHdmiResolutionValueList[9]  = 4809601;
    mHdmiResolutionValueList[10] = 4809602;

    mLCDFd = -1;
    mLCDWidth = 800;
    mLCDHeight = 480;

    mHdmiThread = 0;
    mFlagHdmiThreadRun = false;

    mV4LOutputType = V4L2_OUTPUT_TYPE_DIGITAL;

    mCecDeviceType = CEC_DEVICE_PLAYER;
    memset(mCecBuffer, 0, CEC_MAX_FRAME_SIZE);
    mCecLAddr = 0;
    mCecPAddr = CEC_NOT_VALID_PHYSICAL_ADDRESS;

    mFdTvout   = 0;
    mFdTvoutG0 = 0;
    mFdTvoutG1 = 0;

    cur_phy_g2d_addr = 0;
    memset(&mTvoutParam, 0, sizeof(tvout_param));
    memset(mFimcReservedMem, 0, sizeof(mFimcReservedMem));
}

SecHdmi::~SecHdmi()
{
    if(mFlagCreate == true)
        LOGE("%s::this is not Destroyed fail \n", __func__);
}

bool SecHdmi::create(void)
{
    Mutex::Autolock lock(mLock);

    bool flagCecOpen  = false;
    bool flagEdidOpen = false;

    if(mFlagCreate == true)
    {
        LOGE("%s::Already Created fail \n", __func__);
        return false;
    }

    if (mLCDFd < 0) {
        char const * const device_template[] = {
            "/dev/graphics/fb%u",
            "/dev/fb%u",
            0 };

        int i=0;
        char name[64];

        while ((mLCDFd == -1) && device_template[i]) {
            snprintf(name, 64, device_template[i], 0);
            mLCDFd = open(name, O_RDWR, 0);
            i++;
        }

        if (mLCDFd < 0) {
            LOGE("%s:open(%s) fail\n", __func__, name);
            return false;
        } else {
            struct fb_var_screeninfo info;
            if (ioctl(mLCDFd, FBIOGET_VSCREENINFO, &info) == -1) {
                LOGE("%s:ioctl(FBIOGET_VSCREENINFO) fail\n", __func__);
                return false;
            } else {
                mLCDWidth   =  info.xres;
                mLCDHeight  =  info.yres;
            }
        }
    }

    v4l2_std_id std_id;

    if(hdmi_resolution_2_std_id(mHdmiResolutionValue, &mHdmiDstWidth, &mHdmiDstHeight, &std_id) < 0)
    {
        LOGE("%s::hdmi_resolution_2_std_id(%d) fail\n", __func__, mHdmiResolutionValue);
        goto CREATE_FAIL;
    }

    for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
    {
        if(hdmi_initialize(layer, std_id) < 0)
        {
            LOGE("%s::hdmi_initialize(%d) fail \n", __func__, layer);
            goto CREATE_FAIL;
        }
    }

	#if defined(S5P_BOARD_USES_HDMI_SUBTITLES)
    {
        if(g2d_reserved_memory0 == 0)
        {
            int fd = open(SEC_G2D_DEV_NAME, O_RDWR);
            if(0 <= fd)
            {
                if(ioctl(fd, G2D_GET_MEMORY, &g2d_reserved_memory0) < 0)
                {
                    LOGE("%s::S3C_G2D_GET_MEMORY fail\n", __func__);
                    close(fd);
                    goto CREATE_FAIL;
                }

                if(ioctl(fd, G2D_GET_MEMORY_SIZE, &g2d_reserved_memory_size) < 0)
                {
                    LOGE("%s::S3C_G2D_GET_MEMORY_SIZE fail\n", __func__);
                    close(fd);
                    goto CREATE_FAIL;
                }
                close(fd);
            }
        }

        /* g2d reserved memory is allocated 4M for double buffering.
        * mG2dReservedMem0 is for front buffer.
        * g2d_reserved_meonry1 is for back buffer.
        */

        if(g2d_reserved_memory1 == 0)
            g2d_reserved_memory1 = g2d_reserved_memory0 + (g2d_reserved_memory_size >> 1);

    }
    #endif

    mFlagCreate = true;

    return true;

CREATE_FAIL :

    for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
    {
        if(hdmi_deinitialize(layer) < 0)
            LOGE("%s::hdmi_deinitialize(%d) fail \n", __func__, layer);
    }

    return false;
}

bool SecHdmi::destroy(void)
{
    Mutex::Autolock lock(mLock);

    bool ret = true;

    if(mFlagCreate == false)
    {
        LOGE("%s::Already Destroyed fail \n", __func__);
        return false;
    }

    if(   mFlagHdmiThreadRun == true
       && m_runEdid(false) == false)
    {
        LOGE("%s::m_runEdid(false) fail \n", __func__);
    }

    if(CECClose() == 0)
    {
        LOGE("CECClose() failed!\n");
        ret = false;
    }

    if(EDIDClose() == 0)
    {
        LOGE("EDIDClose() failed!\n");
        ret = false;
    }

    for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
    {
        if(mFlagHdmiStart[layer] == true && m_stopHdmi(layer) == false)
        {
            LOGE("%s::m_stopHdmi: layer[%d] fail \n", __func__, layer);
            ret = false;
        }

        if(hdmi_deinitialize(layer) < 0)
        {
            LOGE("%s::hdmi_deinitialize(%d) fail \n", __func__, layer);
            ret = false;
        }
    }

    if (mSecFimc.flagCreate() == true && mSecFimc.destroy() == false) {
        LOGE("%s::fimc destroy fail \n", __func__);
        ret = false;
    }

    if (0 < mLCDFd) {
        close(mLCDFd);
        mLCDFd = -1;
    }

    if (ret == true)
        mFlagCreate = false;

    return ret;
}

bool SecHdmi::connect(void)
{
    {
        Mutex::Autolock lock(mLock);
        unsigned int fimc_buf_size = (MAX_SW_DECODED_VIDEO_WIDTH*MAX_SW_DECODED_VIDEO_HEIGHT)*2;
        mFimcCurrentOutBufIndex = 0;

        if(mFlagCreate == false)
        {
            LOGE("%s::Not Yet Created \n", __func__);
            return false;
        }

        if(mFlagConnected == true)
        {
            LOGD("%s::Already Connected.. \n", __func__);
            return true;
        }

        if(m_flagHWConnected() == false)
        {
            LOGE("%s::m_flagHWConnected() fail \n", __func__);
            return false;
        }

        if(   mFlagHdmiThreadRun == false
           && m_runEdid(true) == false)
        {
            LOGE("%s::m_runEdid(true) fail \n", __func__);
            return false;
        }

        if(   mSecFimc.flagCreate() == false
        //   && mSecFimc.create(SecFimc::FIMC_DEV1, FIMC_OVLY_NONE_SINGLE_BUF, 1) == false)
           && mSecFimc.create(SecFimc::FIMC_DEV2, FIMC_OVLY_NONE_SINGLE_BUF, 1) == false)//fighter
        {
            LOGE("%s::SecFimc create fail \n", __func__);
            return false;
        }

        for(int i = 0; i < HDMI_LAYER_MAX; i++)
        {
            mSrcWidth      [i] = 0;
            mSrcHeight     [i] = 0;
            mSrcColorFormat[i] = 0;
            mHdmiSrcWidth  [i] = 0;
            mHdmiSrcHeight [i] = 0;
        }
        unsigned int fimcReservedMemStart;

        fimcReservedMemStart = mSecFimc.getFimcRsrvedPhysMemAddr();

        for(int i=0; i < HDMI_FIMC_OUTPUT_BUF_NUM; i++)
        {
            mFimcReservedMem[i] = fimcReservedMemStart + (fimc_buf_size * i);
        }
    }

    if(this->setHdmiOutputMode(mHdmiOutputMode, true) == false)
        LOGE("%s::setHdmiOutputMode(%d) fail \n", __func__, mHdmiOutputMode);

    if(this->setHdmiResolution(mHdmiResolutionValue, true) == false)
        LOGE("%s::setHdmiResolution(%d) fail \n", __func__, mHdmiResolutionValue);

    if(this->setHdcpMode(mHdcpMode, false) == false)
        LOGE("%s::setHdcpMode(%d) fail \n", __func__, mHdcpMode);

    if(this->m_setAudioMode(mAudioMode) == false)
        LOGE("%s::m_setAudioMode(%d) fail \n", __func__, mAudioMode);


    // show display..
    hdmi_show_info();

    mHdmiInfoChange = true;

    mFlagConnected = true;
    mFimcCurrentOutBufIndex = 0;

    return true;
}

bool SecHdmi::disconnect(void)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(mFlagConnected == false)
    {
        LOGE("%s::Already Disconnected.. \n", __func__);
        return true;
    }

    if(   mFlagHdmiThreadRun == true
       && m_runEdid(false) == false)
        LOGE("%s::m_runEdid(false) fail \n", __func__);

    if(   mSecFimc.flagCreate() == true
       && mSecFimc.destroy()    == false)
        LOGE("%s::fimc destroy fail \n", __func__);

    mFlagConnected = false;
    return true;
}


bool SecHdmi::flagConnected(void)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    return mFlagConnected;
}


bool SecHdmi::flush(int w, int h, int colorFormat,
                    int hdmiLayer,
                    unsigned int physYAddr, unsigned int physCbAddr,
                    int num_of_hwc_layer)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(mFlagConnected == false)
    {
        //LOGE("%s::Not Yet connected \n", __func__);
        //return false;
        return true;
    }

    if(mSecFimc.flagCreate() == false)
    {
        //if(mSecFimc.create(SecFimc::FIMC_DEV1, FIMC_OVLY_NONE_SINGLE_BUF, 1) == false)
        if(mSecFimc.create(SecFimc::FIMC_DEV2, FIMC_OVLY_NONE_SINGLE_BUF, 1) == false)	//fighter
        {
            LOGE("%s::SecFimc create fail \n", __func__);
            return false;
        }

        unsigned int fimcReservedMemStart;
        unsigned int fimc_buf_size = (MAX_SW_DECODED_VIDEO_WIDTH*MAX_SW_DECODED_VIDEO_HEIGHT)*2;

        fimcReservedMemStart = mSecFimc.getFimcRsrvedPhysMemAddr();

        for(int i=0; i < HDMI_FIMC_OUTPUT_BUF_NUM; i++)
        {
            mFimcReservedMem[i] = fimcReservedMemStart + (fimc_buf_size * i);
        }
    }

    if(    w               != mSrcWidth      [hdmiLayer]
        || h               != mSrcHeight     [hdmiLayer]
        || colorFormat     != mSrcColorFormat[hdmiLayer]
        || mHdmiInfoChange == true)
    {
        if(m_reset(w, h, colorFormat, hdmiLayer) == false)
        {
            LOGE("%s::m_reset(%d, %d, %d, %d) fail \n", __func__, w, h, colorFormat, hdmiLayer);
            return false;
        }
    }

    if (physYAddr == 0) {
        unsigned int phyLCDAddr = 0;

        // get physical framebuffer address for LCD
        if (ioctl(mLCDFd, S3CFB_GET_LCD_ADDR, &phyLCDAddr) == -1) {
            LOGE("%s:ioctl(S3CFB_GET_LCD_ADDR) fail\n", __func__);
            return false;
        }

        // when early suspend, FIMD IP off.
        // so physical framebuffer address for LCD is 0x00000000
        // so JUST RETURN.
        if (phyLCDAddr == 0) {
            LOGE("%s::S3CFB_GET_LCD_ADDR fail \n", __func__);
            return true;
        }

        physYAddr = phyLCDAddr;
    }

    if (hdmiLayer == HDMI_LAYER_VIDEO) {
        if (colorFormat == HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED) {
            hdmi_set_v_param(hdmiLayer,
                             w, h, mHdmiDstWidth, mHdmiDstHeight,
                             physYAddr, physCbAddr);
        } else if ((colorFormat == HAL_PIXEL_FORMAT_BGRA_8888) ||
                colorFormat == HAL_PIXEL_FORMAT_YCbCr_420_P) {
            if (mSecFimc.setSrcPhyAddr(physYAddr, physCbAddr) == false) {
                LOGE("%s::mSecFimc.setSrcPhyAddr(%d, %d) fail \n",
                     __func__, physYAddr, physCbAddr);
                return false;
            }

            unsigned int  y_size =  ALIGN(ALIGN(w, 128) * ALIGN(h, 32), SZ_8K);
            if (mSecFimc.setDstPhyAddr(mFimcReservedMem[mFimcCurrentOutBufIndex],
                        mFimcReservedMem[mFimcCurrentOutBufIndex] + y_size) == false) {
                LOGE("%s::mSecFimc.setDstPhyAddr(%d, %d) fail \n",
                        __func__, mFimcReservedMem[mFimcCurrentOutBufIndex], mFimcReservedMem[mFimcCurrentOutBufIndex] + y_size);
                return false;
            }

            mHdmiSrcYAddr    = mFimcReservedMem[mFimcCurrentOutBufIndex];
            mHdmiSrcCbCrAddr = mFimcReservedMem[mFimcCurrentOutBufIndex] + y_size;

            mFimcCurrentOutBufIndex++;

            if (mFimcCurrentOutBufIndex >= HDMI_FIMC_OUTPUT_BUF_NUM)
                mFimcCurrentOutBufIndex = 0;

            if (mSecFimc.handleOneShot() == false) {
                LOGE("%s::mSecFimc.handleOneshot() fail \n", __func__);
                return false;
            }

            hdmi_set_v_param(hdmiLayer, w, h, mHdmiDstWidth, mHdmiDstHeight,
                                                mHdmiSrcYAddr, mHdmiSrcCbCrAddr);
        }

    } else {
        if (hdmi_gl_set_param(hdmiLayer, w, h, mHdmiDstWidth, mHdmiDstHeight,
                          physYAddr, num_of_hwc_layer) < 0)
            return false;
    }

    if (mFlagHdmiStart[hdmiLayer] == false && m_startHdmi(hdmiLayer) == false) {
        LOGE("%s::m_startHdmi(%d) fail \n", __func__, hdmiLayer);
        return false;
    }

    return true;
}

bool SecHdmi::clear(int hdmiLayer)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(mFlagConnected == false)
    {
        //LOGE("%s::Not Yet connected \n", __func__);
        //return false;
        return true;
    }

    mSrcWidth[hdmiLayer] = 0;
    mSrcHeight[hdmiLayer] = 0;
    mSrcColorFormat[hdmiLayer] = 0;

    if(mFlagHdmiStart[hdmiLayer] == true && m_stopHdmi(hdmiLayer) == false)
    {
        LOGE("%s::m_stopHdmi: layer[%d] fail \n", __func__, hdmiLayer);
        return false;
    }

    return true;
}

bool SecHdmi::setHdmiOutputMode(int hdmiOutputMode, bool forceRun)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(forceRun == false && mHdmiOutputMode == hdmiOutputMode)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdmiOutputMode(%d) \n", __func__, hdmiOutputMode);
        #endif
        return true;
    }

    int newHdmiOutputMode = hdmiOutputMode;

    int v4l2OutputType = hdmi_outputmode_2_v4l2_output_type(hdmiOutputMode);
    if(v4l2OutputType < 0)
    {
        LOGD("%s::hdmi_outputmode_2_v4l2_output_type(%d) fail\n", __func__, hdmiOutputMode);
        return false;
    }

    int newV4l2OutputType = hdmi_check_output_mode(v4l2OutputType);
    if(newV4l2OutputType != v4l2OutputType)
    {
        newHdmiOutputMode = hdmi_v4l2_output_type_2_outputmode(newV4l2OutputType);
        if(newHdmiOutputMode < 0)
        {
            LOGD("%s::hdmi_v4l2_output_type_2_outputmode(%d) fail\n", __func__, newV4l2OutputType);
            return false;
        }

        LOGD("%s::calibration mode(%d -> %d)... \n", __func__, hdmiOutputMode, newHdmiOutputMode);
        mHdmiInfoChange = true;
    }

    if(mHdmiOutputMode != newHdmiOutputMode)
    {
        mHdmiOutputMode = newHdmiOutputMode;
        mHdmiInfoChange = true;
    }

    return true;
}

bool SecHdmi::setHdmiResolution(unsigned int hdmiResolutionValue, bool forceRun)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(forceRun == false && mHdmiResolutionValue == hdmiResolutionValue)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdmiResolutionValue(%d) \n", __func__, hdmiResolutionValue);
        #endif
        return true;
    }

    unsigned int newHdmiResolutionValue = hdmiResolutionValue;
    int w = 0;
    int h = 0;
    v4l2_std_id std_id;

    // find perfect resolutions..
    if(    hdmi_resolution_2_std_id(newHdmiResolutionValue, &w, &h, &std_id) < 0
        || hdmi_check_resolution(std_id) < 0)
    {
        bool flagFoundIndex = false;
        int resolutionValueIndex = m_resolutionValueIndex(newHdmiResolutionValue);

        for(int i = resolutionValueIndex + 1; i < mHdmiSizeOfResolutionValueList; i++)
        {
            if(    hdmi_resolution_2_std_id(mHdmiResolutionValueList[i], &w, &h, &std_id) == 0
               &&  hdmi_check_resolution(std_id) == 0)
            {
                newHdmiResolutionValue = mHdmiResolutionValueList[i];
                flagFoundIndex = true;
                break;
            }
        }

        if(flagFoundIndex == false)
        {
            LOGE("%s::hdmi cannot control this resolution(%d) fail \n", __func__, hdmiResolutionValue);
            return false;
        }
        else
            LOGD("%s::HDMI resolutions size is calibrated(%d -> %d)..\n", __func__, hdmiResolutionValue, newHdmiResolutionValue);
    }
    else
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::find resolutions(%d) at once\n", __func__, hdmiResolutionValue);
        #endif
    }

    if(mHdmiResolutionValue != newHdmiResolutionValue)
    {
        mHdmiResolutionValue = newHdmiResolutionValue;
        mHdmiInfoChange = true;
    }

    return true;
}


bool SecHdmi::setHdcpMode(bool hdcpMode, bool forceRun)
{
    Mutex::Autolock lock(mLock);

    if(mFlagCreate == false)
    {
        LOGE("%s::Not Yet Created \n", __func__);
        return false;
    }

    if(forceRun == false && mHdcpMode == hdcpMode)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdcpMode(%d) \n", __func__, hdcpMode);
        #endif
        return true;
    }

    mHdcpMode = hdcpMode;
    mHdmiInfoChange = true;

    return true;
}

bool SecHdmi::m_reset(int w, int h, int colorFormat, int hdmiLayer)
{
    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    // stop all..
    for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
    {
        if (mFlagHdmiStart[layer] == true && m_stopHdmi(layer) == false) {
            LOGE("%s::m_stopHdmi: layer[%d] fail \n", __func__, layer);
            return false;
        }
    }

    if (   w               != mSrcWidth      [hdmiLayer]
        || h               != mSrcHeight     [hdmiLayer]
        || colorFormat     != mSrcColorFormat[hdmiLayer]) {

        if (hdmiLayer == HDMI_LAYER_VIDEO) {
            if ( (colorFormat == HAL_PIXEL_FORMAT_BGRA_8888) ||
                    (colorFormat == HAL_PIXEL_FORMAT_YCbCr_420_P) ) {
                int aligned_w, aligned_h;

                aligned_w = (w + 15 ) & (~15);
                aligned_h = (h + 1 ) & (~1);

#ifdef DEBUG_HDMI_HW_LEVEL
                LOGD("### %s  call mSecFimc.setSrcParams\n", __func__);
#endif
                if (mSecFimc.setSrcParams((unsigned int)aligned_w, (unsigned int)aligned_h, 0, 0,
                            (unsigned int*)&aligned_w, (unsigned int*)&aligned_h, colorFormat, true) == false) {
                    LOGE("%s::mSecFimc.setSrcParams(%d, %d, %d) fail \n",
                            __func__, w, h, colorFormat);
                    return false;
                }

                mFimcDstColorFormat = HAL_PIXEL_FORMAT_CUSTOM_YCbCr_420_SP_TILED;

#ifdef DEBUG_HDMI_HW_LEVEL
                LOGD("### %s  call mSecFimc.setDstParams\n", __func__);
#endif

                if (mSecFimc.setDstParams((unsigned int)aligned_w, (unsigned int)aligned_h, 0, 0,
                            (unsigned int*)&aligned_w, (unsigned int*)&aligned_h, mFimcDstColorFormat, true) == false) {
                    LOGE("%s::mSecFimc.setDstParams(%d, %d, %d) fail \n",
                            __func__, w, h, mFimcDstColorFormat);
                    return false;
                }
            }
        }

        mSrcWidth      [hdmiLayer] = w;
        mSrcHeight     [hdmiLayer] = h;
        mSrcColorFormat[hdmiLayer] = colorFormat;

        mHdmiSrcWidth  [hdmiLayer] = w;
        mHdmiSrcHeight [hdmiLayer] = h;
    }

    if(mHdmiInfoChange == true)
    {
        if(m_setHdmiOutputMode(mHdmiOutputMode) == false)
        {
            LOGE("%s::m_setHdmiOutputMode() fail \n", __func__);
            return false;
        }

        if(m_setHdmiResolution(mHdmiResolutionValue) == false)
        {
            LOGE("%s::m_setHdmiResolution() fail \n", __func__);
            return false;
        }

        if(m_setHdcpMode(mHdcpMode) == false)
        {
            LOGE("%s::m_setHdcpMode() fail \n", __func__);
            return false;
        }

        for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
        {
            if(hdmi_deinitialize(layer) < 0)
                LOGE("%s::hdmi_deinitialize(%d) fail \n", __func__, layer);
        }

        for(int layer = HDMI_LAYER_BASE + 1; layer < HDMI_LAYER_MAX; layer++)
        {
            if(hdmi_initialize(layer, mHdmiStdId) < 0)
            {
                LOGE("%s::hdmi_initialize(%d) fail \n", __func__, layer);
            }
        }

        if(m_setAudioMode(mAudioMode) == false)
        {
            LOGE("%s::m_setAudioMode() fail \n", __func__);
        }
        mHdmiInfoChange = false;
    }
    return true;
}

bool SecHdmi::m_startHdmi(int hdmiLayer)
{
    if(mFlagHdmiStart[hdmiLayer] == true)
    {
        LOGD("%s::already HDMI(%d layer) started.. \n", __func__, hdmiLayer);
        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    if(hdmi_streamon(hdmiLayer) < 0)
    {
        LOGE("%s::hdmi_streamon(%d) fail \n", __func__, hdmiLayer);
        return false;
    }

    mFlagHdmiStart[hdmiLayer] = true;
    return true;
}

bool SecHdmi::m_stopHdmi(int hdmiLayer)
{
    if(mFlagHdmiStart[hdmiLayer] == false)
    {
        LOGD("%s::already HDMI(%d layer) stopped.. \n", __func__, hdmiLayer);
        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s : layer[%d] called\n", __func__, hdmiLayer);
    #endif

    if(hdmi_streamoff(hdmiLayer) < 0)
    {
        LOGE("%s::hdmi_streamoff(%d) fail \n", __func__, hdmiLayer);
        return false;
    }

    mFlagHdmiStart[hdmiLayer] = false;

    return true;
}

bool SecHdmi::m_setHdmiOutputMode(int hdmiOutputMode)
{
    if(hdmiOutputMode == mCurrentHdmiOutputMode)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdmiOutputMode(%d) \n", __func__, hdmiOutputMode);
        #endif
        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    int v4l2OutputType = hdmi_outputmode_2_v4l2_output_type(hdmiOutputMode);
    if(v4l2OutputType < 0)
    {
        LOGE("%s::hdmi_outputmode_2_v4l2_output_type(%d) fail\n", __func__, hdmiOutputMode);
        return false;
    }

    mV4LOutputType         = v4l2OutputType;
    mCurrentHdmiOutputMode = hdmiOutputMode;

    return true;

}

bool SecHdmi::m_setHdmiResolution(unsigned int hdmiResolutionValue)
{
    if(hdmiResolutionValue == mCurrentHdmiResolutionValue)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdmiResolutionValue(%d) \n", __func__, hdmiResolutionValue);
        #endif
        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    int w = 0;
    int h = 0;

    v4l2_std_id std_id;

    if(hdmi_resolution_2_std_id(hdmiResolutionValue, &w, &h, &std_id) < 0)
    {
        LOGE("%s::hdmi_resolution_2_std_id(%d) fail\n", __func__, hdmiResolutionValue);
        return false;
    }

    mHdmiStdId    = std_id;

    mHdmiDstWidth  = w;
    mHdmiDstHeight = h;

    mCurrentHdmiResolutionValue = hdmiResolutionValue;

    return true;
}

bool SecHdmi::m_setHdcpMode(bool hdcpMode)
{
    if(hdcpMode == mCurrentHdcpMode)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same hdcpMode(%d) \n", __func__, hdcpMode);
        #endif

        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    mCurrentHdcpMode = hdcpMode;

    return true;
}

bool SecHdmi::m_setAudioMode(int audioMode)
{
    if(audioMode == mCurrentAudioMode)
    {
        #ifdef DEBUG_HDMI_HW_LEVEL
            LOGD("%s::same audioMode(%d) \n", __func__, audioMode);
        #endif
        return true;
    }

    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    if(hdmi_check_audio() < 0)
    {
        LOGE("%s::hdmi_check_audio() fail \n", __func__);
        return false;
    }

    mCurrentAudioMode = audioMode;

    return true;
}

int SecHdmi::m_resolutionValueIndex(unsigned int ResolutionValue)
{
    int index = -1;

    for(int i = 0; i < mHdmiSizeOfResolutionValueList; i++)
    {
        if(mHdmiResolutionValueList[i] == ResolutionValue)
        {
            index = i;
            break;
        }
    }
    return index;
}


bool SecHdmi::m_flagHWConnected(void)
{
    #ifdef DEBUG_HDMI_HW_LEVEL
        LOGD("### %s called\n", __func__);
    #endif

    bool ret = true;
    int hdmiStatus = hdmi_cable_status();

    if(hdmiStatus <= 0)
    {
        if(hdmiStatus < 0)
            LOGE("%s::hdmi_cable_status() fail \n", __func__);

        ret = false;
    }
    else
        ret = true;

    return ret;
}


///////////////////////////////////////////////////////

int SecHdmi::hdmi_initialize(int layer, v4l2_std_id std_id)
{
    int ret = 0;

    switch(layer)
    {
        case HDMI_LAYER_VIDEO :
        {
            ret = hdmi_tvout_init(std_id);

            if(0 < ret)
                mFdTvout = ret;
            break;
        }
        case HDMI_LAYER_GRAPHIC_0 :
        {
            if(mFdTvoutG0 == 0)
            {
                ret = open(TVOUT_DEV_G0, O_RDWR);
                if(ret <= 0)
                    LOGE("%s::open(%s) failed\n", __func__, TVOUT_DEV_G0);
                else
                    mFdTvoutG0 = ret;
            }

            break;
        }
        case HDMI_LAYER_GRAPHIC_1 :
        {
            if(mFdTvoutG1 == 0)
            {
                ret = open(TVOUT_DEV_G1, O_RDWR);
                if(ret <= 0)
                    LOGE("%s::open(%s) failed\n", __func__, TVOUT_DEV_G1);
                else
                    mFdTvoutG1 = ret;
            }
            break;
        }
        default :
        {

            LOGE("%s::unmathced layer(%d) fail", __func__, layer);
            ret = -1;
            break;
        }
    }

    return ret;
}


int SecHdmi::hdmi_deinitialize(int layer)
{
    int ret = 0;

    switch(layer)
    {
        case HDMI_LAYER_VIDEO :
        {
            // sw5771.park(101005) :
            // If close on running time, when open, open has lock-up..
            if(0 < mFdTvout)
            {
	            close(mFdTvout);
                mFdTvout = 0;
            }
            break;
        }
        case HDMI_LAYER_GRAPHIC_0 :
        {
            if(0 < mFdTvoutG0)
            {
                close(mFdTvoutG0);
                mFdTvoutG0 = 0;
            }
            break;
        }
        case HDMI_LAYER_GRAPHIC_1 :
        {
            if(0 < mFdTvoutG1)
            {
                close(mFdTvoutG1);
                mFdTvoutG1 = 0;
            }
            break;
        }
        default :
        {

            LOGE("%s::unmathced layer(%d) fail", __func__, layer);
            ret = -1;
            break;
        }
    }

	return ret;
}


int SecHdmi::hdmi_streamon(int layer)
{
    int ret = 0;

    switch(layer)
    {
        case HDMI_LAYER_VIDEO :
        {
	        if(0 < mFdTvout)
            {
		        ret = tvout_v4l2_streamon(mFdTvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
	        }
            break;
        }
        case HDMI_LAYER_GRAPHIC_0 :
        {
            if(0 < mFdTvoutG0)
            {
                ret = tvout_v4l2_start_overlay(mFdTvoutG0);
            }
            break;
        }
        case HDMI_LAYER_GRAPHIC_1 :
        {
            if(0 < mFdTvoutG1)
            {
		        ret = tvout_v4l2_start_overlay(mFdTvoutG1);
            }
            break;
        }
        default :
        {

            LOGE("%s::unmathced layer(%d) fail", __func__, layer);
            ret = -1;
            break;
        }
    }

    return ret;
}

int SecHdmi::hdmi_streamoff(int layer)
{
    int ret = 0;

    switch(layer)
    {
        case HDMI_LAYER_VIDEO :
        {
            if(0 < mFdTvout)
            {
                ret = tvout_v4l2_streamoff(mFdTvout, V4L2_BUF_TYPE_VIDEO_OUTPUT);
            }
            break;
        }
        case HDMI_LAYER_GRAPHIC_0 :
        {
            if(0 < mFdTvoutG0)
            {
                cur_phy_g2d_addr = 0;
                ret = tvout_v4l2_stop_overlay(mFdTvoutG0);
            }
            break;
        }
        case HDMI_LAYER_GRAPHIC_1 :
        {
            if(0 < mFdTvoutG1)
            {
                ret = tvout_v4l2_stop_overlay(mFdTvoutG1);
            }
            break;
        }
        default :
            LOGE("%s::unmathced layer(%d) fail", __func__, layer);
            ret = -1;
            break;

    }

    return ret;
}

#define ROUND_UP(value, boundary) ((((uint32_t)(value)) + \
                                  (((uint32_t) boundary)-1)) & \
                                  (~(((uint32_t) boundary)-1)))

int SecHdmi::hdmi_set_v_param(int layer,
                     int src_w, int src_h, int dst_w, int dst_h,
                     unsigned int ui_top_y_address, unsigned int ui_top_c_address)
{
    if(mFdTvout <= 0)
    {
        LOGE("mFdTvout is <= 0 fail\n");
        return -1;
    }

    int round_up_src_w;
    int round_up_src_h;

    /* src_w, src_h round up to DWORD because of VP restriction */
    round_up_src_w = ROUND_UP( src_w, 2*sizeof(uint32_t) );
    round_up_src_h = ROUND_UP( src_h, 2*sizeof(uint32_t) );

    mTvoutParam.tvout_src.base_y         = (void *)ui_top_y_address;
    mTvoutParam.tvout_src.base_c         = (void *)ui_top_c_address;
    mTvoutParam.tvout_src.pix_fmt.width  = round_up_src_w;
    mTvoutParam.tvout_src.pix_fmt.height = round_up_src_h;
    mTvoutParam.tvout_src.pix_fmt.field  = V4L2_FIELD_NONE;
    //mTvoutParam.tvout_src.pix_fmt.pixelformat = VPROC_SRC_COLOR_TILE_NV12;
    mTvoutParam.tvout_src.pix_fmt.pixelformat = V4L2_PIX_FMT_NV12T;

    tvout_v4l2_s_fmt(mFdTvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &mTvoutParam.tvout_src);

    mTvoutParam.tvout_rect.win.w.width   = src_w;
    mTvoutParam.tvout_rect.win.w.height  = src_h;

    tvout_v4l2_s_parm(mFdTvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &mTvoutParam.tvout_rect);


    if(dst_w * src_h <= dst_h * src_w)
    {
	    mTvoutParam.tvout_dst.left   = 0;
	    mTvoutParam.tvout_dst.top    = (dst_h - ((dst_w * src_h) / src_w)) >> 1;
	    mTvoutParam.tvout_dst.width  = dst_w;
	    mTvoutParam.tvout_dst.height = ((dst_w * src_h) / src_w);
    }
    else
    {
	    mTvoutParam.tvout_dst.left   = (dst_w - ((dst_h * src_w) / src_h)) >> 1;
	    mTvoutParam.tvout_dst.top    = 0;
	    mTvoutParam.tvout_dst.width  = ((dst_h * src_w) / src_h);
	    mTvoutParam.tvout_dst.height = dst_h;
    }

    tvout_v4l2_s_crop(mFdTvout, V4L2_BUF_TYPE_VIDEO_OUTPUT, &mTvoutParam.tvout_dst);

    return 0;
}



int SecHdmi::hdmi_gl_set_param(int layer,
                      int src_w, int src_h, int dst_w, int dst_h,
                      unsigned int ui_top_y_address, int num_of_hwc_layer)
#if defined(S5P_BOARD_USES_HDMI_SUBTITLES)
{
    struct overlay_param ov_param;

    unsigned int hdmi_pixelformat;
    int          dst_color_format;
    int          ret = 0;

    switch (mHdmiStdId)
    {
        case V4L2_STD_1080P_60:
        case V4L2_STD_1080P_30:
        case V4L2_STD_1080I_60:
            hdmi_pixelformat = V4L2_PIX_FMT_RGB444;
            dst_color_format = SecG2d::COLOR_FORMAT_ARGB_4444;
            break;
        case V4L2_STD_720P_60:
        case V4L2_STD_576P_50_16_9:
        case V4L2_STD_480P_60_16_9:
        default:
            hdmi_pixelformat = V4L2_PIX_FMT_RGB32;
            dst_color_format = SecG2d::COLOR_FORMAT_ARGB_8888;
            break;
    }

    G2dRect srcRect = {0,     0,
                       src_w, src_h,
                       src_w, src_h,
                       SecG2d::COLOR_FORMAT_ARGB_8888,
                       ui_top_y_address,
                       NULL};


    G2dRect dstRect = {0,     0,
                       dst_w, dst_h,
                       dst_w, dst_h,
                       dst_color_format,
                       0,
                       NULL};

    /* if ui_top_y_address is not changed, we do not scale using g2d and JUST use
     * previously scaled image.
     */

    if (cur_phy_g2d_addr == 0 || ui_top_y_address != prev_ui_top_y_address) {
        if (cur_phy_g2d_addr == 0 || cur_phy_g2d_addr == g2d_reserved_memory1)
                dstRect.phys_addr = g2d_reserved_memory0;
        else
                dstRect.phys_addr = g2d_reserved_memory1;

        cur_phy_g2d_addr = dstRect.phys_addr;
        prev_ui_top_y_address = ui_top_y_address;

        G2dFlag flag = {SecG2d::ROTATE_0,
                        SecG2d::ALPHA_MAX,
                        0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

        if ( hdmi_rotate == 90)
            flag.rotate_val = SecG2d::ROTATE_90;
        else if ( hdmi_rotate == 180)
            flag.rotate_val = SecG2d::ROTATE_180;
        else if ( hdmi_rotate == 270)
            flag.rotate_val = SecG2d::ROTATE_270;
        else
            flag.rotate_val = SecG2d::ROTATE_0;


        flag.alpha_val = SecG2d::ALPHA_OPAQUE;  // no alpha..

        // scale and rotate with FIMG
        if (stretchSecG2d(&srcRect, &dstRect, NULL, &flag) < 0) {
            LOGE("%s::stretchSecG2d() 2 fail \n", __func__);
            return -1;
        }

        int fp_tvout_temp;

        if (layer == HDMI_LAYER_GRAPHIC_0)
            fp_tvout_temp = mFdTvoutG0;
        else
            fp_tvout_temp = mFdTvoutG1;

        tvout_v4l2_s_baseaddr(fp_tvout_temp, &cur_phy_g2d_addr);

        ov_param.overlay_frame.fmt.pixelformat = hdmi_pixelformat;
        ov_param.overlay_frame.base            =(void *)cur_phy_g2d_addr;

        ov_param.overlay_rect.flags            = 0;
        ov_param.overlay_rect.priority         = 0x02;
        ov_param.overlay_rect.win.w.left       = 0;
        ov_param.overlay_rect.win.w.top        = 0;
        ov_param.overlay_rect.win.w.width      = dst_w;
        ov_param.overlay_rect.win.w.height     = dst_h;

        /* for global alpha blending use */
        //	ov_param.overlay_rect.flags 		+= V4L2_FBUF_FLAG_GLOBAL_ALPHA;
        //	ov_param.overlay_rect.win.global_alpha 	= 0x00;

        /* for color key use */
        //	ov_param.overlay_rect.flags 		+= V4L2_FBUF_FLAG_CHROMAKEY;
        //	ov_param.overlay_rect.win.chromakey 	= 0xff000000;

        /* for per-pixel alpha blending use */
        ov_param.overlay_rect.flags            += V4L2_FBUF_FLAG_LOCAL_ALPHA;

        ov_param.overlay_dst.left   = 0;
        ov_param.overlay_dst.top    = 0;
        ov_param.overlay_dst.width  = dst_w;
        ov_param.overlay_dst.height = dst_h;

        ret = hdmi_set_overlay_param(layer, &ov_param);
    }
    return ret;
}
#else
{
    struct overlay_param ov_param;

    int fp_tvout_temp;
    if(layer == HDMI_LAYER_GRAPHIC_0)
        fp_tvout_temp = mFdTvoutG0;
    else
        fp_tvout_temp = mFdTvoutG1;

    tvout_v4l2_s_baseaddr(fp_tvout_temp, &ui_top_y_address);

    ov_param.overlay_frame.fmt.pixelformat = V4L2_PIX_FMT_RGB32;
    ov_param.overlay_frame.base            = (void *)ui_top_y_address;

    ov_param.overlay_rect.flags            = 0;
//    ov_param.overlay_rect.flags            += V4L2_FBUF_FLAG_LOCAL_ALPHA;
    ov_param.overlay_rect.priority         = 0x02;
    ov_param.overlay_rect.win.w.left       = 0;
    ov_param.overlay_rect.win.w.top        = 0;
    ov_param.overlay_rect.win.w.width      = src_w;
    ov_param.overlay_rect.win.w.height     = src_h;
    ov_param.overlay_rect.win.global_alpha = 0;

    if (src_w == dst_w) {
        ov_param.overlay_dst.left   = 0;
        ov_param.overlay_dst.top    = (dst_h - src_h) / 2;
    } else {
        ov_param.overlay_dst.left   = (dst_w - src_w) / 2;
        ov_param.overlay_dst.top    = 0;
    }

    ov_param.overlay_dst.width  = src_w;
    ov_param.overlay_dst.height = src_h;

    return hdmi_set_overlay_param(layer, &ov_param);
}
#endif

int SecHdmi::hdmi_cable_status()
{
    int cable_status;
    int ret = 0;

    if(mFdTvout == 0)
    {
        mFdTvout = open(TVOUT_DEV, O_RDWR);
        if (mFdTvout <= 0) {
            LOGE("%s::open(%s) failed\n", __func__, TVOUT_DEV);
            mFdTvout = 0;
            return -1;
        }
    }

    ret = ioctl(mFdTvout, VIDIOC_HDCP_STATUS, &cable_status);

    /*
    if(0 < mFdTvout)
    {
        close(mFdTvout);
        mFdTvout = 0;
    }
    */

    if(ret != 0)
        return -1;
    else
        return cable_status;
}

int SecHdmi::hdmi_outputmode_2_v4l2_output_type(int output_mode)
{
    int v4l2_output_type = -1;

    switch(output_mode)
    {
        case HDMI_OUTPUT_MODE_YCBCR:
            v4l2_output_type = V4L2_OUTPUT_TYPE_DIGITAL;
            break;
        case HDMI_OUTPUT_MODE_RGB:
            v4l2_output_type = V4L2_OUTPUT_TYPE_HDMI_RGB;
            break;
        case HDMI_OUTPUT_MODE_DVI:
            v4l2_output_type = V4L2_OUTPUT_TYPE_DVI;
            break;
        default:
            LOGE("%s::unmathced HDMI_mode(%d)", __func__, output_mode);
            v4l2_output_type = -1;
            break;
    }

    return v4l2_output_type;
}

int SecHdmi::hdmi_v4l2_output_type_2_outputmode(int v4l2_output_type)
{
    int outputMode = -1;

    switch(v4l2_output_type)
    {
        case V4L2_OUTPUT_TYPE_DIGITAL:
            outputMode = HDMI_OUTPUT_MODE_YCBCR;
            break;
        case V4L2_OUTPUT_TYPE_HDMI_RGB:
            outputMode = HDMI_OUTPUT_MODE_RGB;
            break;
        case V4L2_OUTPUT_TYPE_DVI:
            outputMode = HDMI_OUTPUT_MODE_DVI;
            break;
        default:
            LOGE("%s::unmathced v4l2_output_type(%d)", __func__, v4l2_output_type);
            outputMode = -1;
            break;
    }

    return outputMode;
}


int SecHdmi::hdmi_check_output_mode(int v4l2_output_type)
{
    struct HDMIVideoParameter video;
    struct HDMIAudioParameter audio;
    int    calbirate_v4l2_mode = v4l2_output_type;

    audio.formatCode = LPCM_FORMAT;
    audio.outPacket  = HDMI_ASP;
    audio.channelNum = CH_2;
    audio.sampleFreq = SF_44KHZ;

    switch(v4l2_output_type)
    {
        case V4L2_OUTPUT_TYPE_DIGITAL :

	        video.mode = HDMI;
	        if (!EDIDHDMIModeSupport(&video))
            {
                calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_DVI;
                //audio_state = NOT_SUPPORT;
                LOGI("Change mode into DVI\n");
                break;
	        }
            /*else
            {
                if (EDIDAudioModeSupport(&audio))
                    audio_state = ON;
	        }*/

	        video.colorSpace = HDMI_CS_YCBCR444;
	        if (!EDIDHDMIModeSupport(&video))
            {
		        calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_HDMI_RGB;
		        LOGI("Change mode into HDMI_RGB\n");
	        }
	        break;

        case V4L2_OUTPUT_TYPE_HDMI_RGB:

	        video.mode = HDMI;
	        if (!EDIDHDMIModeSupport(&video))
            {
                calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_DVI;
                //audio_state = NOT_SUPPORT;
                LOGI("Change mode into DVI\n");
                break;
	        }
            /*else
            {
                if (EDIDAudioModeSupport(&audio))
                    audio_state = ON;
	        }*/
	        video.colorSpace = HDMI_CS_RGB;
	        if (!EDIDColorSpaceSupport(&video))
            {
		        calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_DIGITAL;
                LOGI("Change mode into HDMI\n");
	        }
	        break;

        case V4L2_OUTPUT_TYPE_DVI:

	        video.mode = HDMI;
	        if (EDIDHDMIModeSupport(&video))
            {
		        video.colorSpace = HDMI_CS_YCBCR444;
		        if (EDIDColorSpaceSupport(&video))
                {
			        calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_DIGITAL;
			        LOGI("Change mode into HDMI_YCBCR\n");
		        }
                else
                {
			        calbirate_v4l2_mode = V4L2_OUTPUT_TYPE_HDMI_RGB;
			        LOGI("Change mode into HDMI_RGB\n");
		        }

		        /*if (EDIDAudioModeSupport(&audio))
			        audio_state = ON;*/
	        } /*else
		        audio_state = NOT_SUPPORT;*/
	        break;

        default:
	        break;
    }
    return calbirate_v4l2_mode;
}


int SecHdmi::hdmi_check_resolution(v4l2_std_id std_id)
{
    struct HDMIVideoParameter video;
    struct HDMIAudioParameter audio;

    switch (std_id)
    {
        case V4L2_STD_480P_60_16_9:
	        video.resolution = v720x480p_60Hz;
	        video.pixelAspectRatio = HDMI_PIXEL_RATIO_16_9;
	        break;
        case V4L2_STD_480P_60_4_3:
	        video.resolution = v720x480p_60Hz;
	        video.pixelAspectRatio = HDMI_PIXEL_RATIO_4_3;
	        break;
        case V4L2_STD_576P_50_16_9:
	        video.resolution = v720x576p_50Hz;
	        video.pixelAspectRatio = HDMI_PIXEL_RATIO_16_9;
	        break;
        case V4L2_STD_576P_50_4_3:
	        video.resolution = v720x576p_50Hz;
	        video.pixelAspectRatio = HDMI_PIXEL_RATIO_4_3;
	        break;
        case V4L2_STD_720P_60:
	        video.resolution = v1280x720p_60Hz;
	        break;
        case V4L2_STD_720P_50:
	        video.resolution = v1280x720p_50Hz;
	        break;
        case V4L2_STD_1080P_60:
	        video.resolution = v1920x1080p_60Hz;
	        break;
        case V4L2_STD_1080P_50:
	        video.resolution = v1920x1080p_50Hz;
	        break;
        case V4L2_STD_1080I_60:
	        video.resolution = v1920x1080i_60Hz;
	        break;
        case V4L2_STD_1080I_50:
	        video.resolution = v1920x1080i_50Hz;
	        break;
        case V4L2_STD_480P_59:
	        video.resolution = v720x480p_60Hz;
	        break;
        case V4L2_STD_720P_59:
	        video.resolution = v1280x720p_60Hz;
	        break;
        case V4L2_STD_1080I_59:
	        video.resolution = v1920x1080i_60Hz;
	        break;
        case V4L2_STD_1080P_59:
	        video.resolution = v1920x1080p_60Hz;
	        break;
        case V4L2_STD_1080P_30:
	        video.resolution = v1920x1080p_30Hz;
	        break;
        default:
            LOGE("%s::unmathced std_id(%lld)", __func__, std_id);
            return -1;
	        break;
    }

    if (!EDIDVideoResolutionSupport(&video))
    {
        LOGD("%s::EDIDVideoResolutionSupport(%llx) fail (not suppoted std_id) \n", __func__, std_id);

        return -1;
    }

    return 0;
}


int SecHdmi::hdmi_set_resolution(v4l2_std_id std_id)
{
    // set std
	if(tvout_v4l2_s_std(mFdTvout, std_id) < 0)
    {
        LOGD("%s::tvout_v4l2_s_std(%llx) fail\n", __func__, std_id);
        return -1;
    }

    return 0;
}

int SecHdmi::hdmi_resolution_2_std_id(unsigned int resolution, int * w, int * h, v4l2_std_id * std_id)
{
    int ret = 0;

    switch (resolution)
    {
        case 1080960:
            *std_id = V4L2_STD_1080P_60;
            *w      = 1920;
            *h      = 1080;
            break;
        case 1080950:
            *std_id = V4L2_STD_1080P_50;
            *w      = 1920;
            *h      = 1080;
            break;
        case 1080930:
            *std_id = V4L2_STD_1080P_30;
            *w      = 1920;
            *h      = 1080;
            break;
        case 1080160:
            *std_id = V4L2_STD_1080I_60;
            *w      = 1920;
            *h      = 1080;
            break;
        case 1080150:
            *std_id = V4L2_STD_1080I_50;
            *w      = 1920;
            *h      = 1080;
            break;
        case 720960:
            *std_id = V4L2_STD_720P_60;
            *w      = 1280;
            *h      = 720;
            break;
        case 720950:
            *std_id = V4L2_STD_720P_50;
            *w      = 1280;
            *h      = 720;
            break;
        case 5769501:
            *std_id = V4L2_STD_576P_50_16_9;
            *w      = 720;
            *h      = 576;
            break;
        case 5769502:
            *std_id = V4L2_STD_576P_50_4_3;
            *w      = 720;
            *h      = 576;
            break;
        case 4809601:
            *std_id = V4L2_STD_480P_60_16_9;
            *w      = 720;
            *h      = 480;
            break;
        case 4809602:
            *std_id = V4L2_STD_480P_60_4_3;
            *w     = 720;
            *h     = 480;
            break;
        default:
            LOGE("%s::unmathced resolution(%d)", __func__, resolution);
            ret = -1;
            break;
    }

    return ret;
}


//=============================
//
int SecHdmi::hdmi_set_overlay_param(int layer, void *param)
{
    int fp_tvout_temp;

    struct overlay_param * vo_param = (struct overlay_param *)param;
    struct overlay_param   vo_g_param;
    struct v4l2_cropcap cropcap;

    if(layer == HDMI_LAYER_GRAPHIC_0)
	    fp_tvout_temp = mFdTvoutG0;
    else
	    fp_tvout_temp = mFdTvoutG1;

    tvout_v4l2_s_fbuf(fp_tvout_temp, &(vo_param->overlay_frame));
    //tvout_v4l2_g_fbuf(fp_tvout_temp, &(vo_g_param.overlay_frame));

    tvout_v4l2_s_parm(fp_tvout_temp, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY, &(vo_param->overlay_rect));
    //tvout_v4l2_g_parm(fp_tvout_temp, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY, &(vo_g_param.overlay_rect));	//test get param

    cropcap.type = V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY;
    tvout_v4l2_cropcap(fp_tvout_temp, &cropcap);

    if(    vo_param->overlay_dst.width  <= cropcap.bounds.width
        && vo_param->overlay_dst.height <= cropcap.bounds.height)
    {
	    tvout_v4l2_s_crop(fp_tvout_temp, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,&(vo_param->overlay_dst));
	    tvout_v4l2_g_crop(fp_tvout_temp, V4L2_BUF_TYPE_VIDEO_OUTPUT_OVERLAY,&(vo_g_param.overlay_dst));
    }
    else
    {
	    LOGE("[%s] invalid crop size dst.w=%d dst.h=%d bounds.w=%d bounds.h=%d\n", __func__,
                vo_param->overlay_dst.width,
                vo_param->overlay_dst.height,
                cropcap.bounds.width,
                cropcap.bounds.height);
	    return -1;
    }

    return 0;
}


int SecHdmi::hdmi_enable_hdcp(unsigned int hdcp_en)
{
    if(ioctl(mFdTvout, VIDIOC_HDCP_ENABLE, &hdcp_en) < 0)
    {
        LOGD("%s::VIDIOC_HDCP_ENABLE(%d) fail \n", __func__, hdcp_en);
        return -1;
    }

    return 0;
}

int SecHdmi::hdmi_check_audio(void)
{
    struct HDMIAudioParameter audio;
    enum state audio_state = ON;
    int ret = 0;

    audio.formatCode = LPCM_FORMAT;
    audio.outPacket  = HDMI_ASP;
    audio.channelNum = CH_2;
    audio.sampleFreq = SF_44KHZ;

    if (!EDIDAudioModeSupport(&audio))
	    audio_state = NOT_SUPPORT;
    else
	    audio_state = ON;

    if (audio_state == ON)
	    ioctl(mFdTvout, VIDIOC_INIT_AUDIO, 1);
    else
        ioctl(mFdTvout, VIDIOC_INIT_AUDIO, 0);

    return ret;
}

void SecHdmi::hdmi_show_info(void)
{
    struct HDMIVideoParameter video;
    struct HDMIAudioParameter audio;

    audio.formatCode = LPCM_FORMAT;
    audio.outPacket  = HDMI_ASP;
    audio.channelNum = CH_2;
    audio.sampleFreq = SF_44KHZ;

    LOGI("=============== HDMI Audio  =============\n");

    if (EDIDAudioModeSupport(&audio))
	    LOGI("=  2CH_PCM 44100Hz audio supported      =\n");

    LOGI("========= HDMI Mode & Color Space =======\n");

    video.mode = HDMI;
    if (EDIDHDMIModeSupport(&video))
    {
	    video.colorSpace = HDMI_CS_YCBCR444;
	    if (EDIDHDMIModeSupport(&video))
		    LOGI("=  1. HDMI(YCbCr)                       =\n");

	    video.colorSpace = HDMI_CS_RGB;
	    if (EDIDHDMIModeSupport(&video))
		    LOGI("=  2. HDMI(RGB)                         =\n");
    }
    else
    {
	    video.mode = DVI;
	    if (EDIDHDMIModeSupport(&video)) {
		    LOGI("=  3. DVI                               =\n");
	    }
    }

    LOGI("===========    HDMI Rseolution   ========\n");

    /* 480P */
    video.resolution = v720x480p_60Hz;
    video.pixelAspectRatio = HDMI_PIXEL_RATIO_16_9 ;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  4. 480P_60_16_9 	(0x04000000)	=\n");

    video.pixelAspectRatio = HDMI_PIXEL_RATIO_4_3;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  5. 480P_60_4_3	(0x05000000)	=\n");


    /* 576P */
    video.resolution = v720x576p_50Hz;
    video.pixelAspectRatio = HDMI_PIXEL_RATIO_16_9;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  6. 576P_50_16_9 	(0x06000000)	=\n");

    video.pixelAspectRatio = HDMI_PIXEL_RATIO_4_3;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  7. 576P_50_4_3	(0x07000000)	=\n");

    /* 720P 60 */
    video.resolution = v1280x720p_60Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  8. 720P_60 		(0x08000000)	=\n");

    /* 720P_50 */
    video.resolution = v1280x720p_50Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  9. 720P_50 		(0x09000000)	=\n");

    /* 1080P_60 */
    video.resolution = v1920x1080p_60Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  a. 1080P_60 		(0x0a000000)	=\n");

    /* 1080P_50 */
    video.resolution = v1920x1080p_50Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  b. 1080P_50 		(0x0b000000)	=\n");

    /* 1080I_60 */
    video.resolution = v1920x1080i_60Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  c. 1080I_60		(0x0c000000)	=\n");

    /* 1080I_50 */
    video.resolution = v1920x1080i_50Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  d. 1080I_50		(0x0d000000)	=\n");

    /* 1080P_30 */
    video.resolution = v1920x1080p_30Hz;
    if (EDIDVideoResolutionSupport(&video))
	    LOGI("=  i. 1080P_30 		(0x12000000)	=\n");

    LOGI("=========================================\n");
}


int SecHdmi::hdmi_tvout_init(v4l2_std_id std_id)
{
    int ret;
    struct v4l2_output output;
    struct v4l2_standard std;
    //v4l2_std_id std_g_id;
    struct tvout_param tv_g_param;
    struct v4l2_fmtdesc desc;

    unsigned int matched=0, i=0;
    int output_index;

    // It was initialized already
    if(mFdTvout == 0)
    {
        mFdTvout = open(TVOUT_DEV, O_RDWR);
	    if (mFdTvout <= 0) {
		    LOGE("%s::open(%s) failed\n", __func__, TVOUT_DEV);
            mFdTvout = 0;
		    return -1;
	    }
    }

    unsigned int hdcp_en = 0;
    if(mCurrentHdcpMode == true)
        hdcp_en = 1;
    else
        hdcp_en = 0;

    ioctl(mFdTvout, VIDIOC_HDCP_ENABLE, &hdcp_en);

    /* ============== query capability============== */
    tvout_v4l2_querycap(mFdTvout);

    tvout_v4l2_enum_std(mFdTvout, &std, std_id);

    // set std
    tvout_v4l2_s_std(mFdTvout, std_id);
    tvout_v4l2_g_std(mFdTvout, &std_id);

    i = 0;

    do
    {
	    output.index = i;
	    ret = tvout_v4l2_enum_output(mFdTvout, &output);
	    if(output.type == mV4LOutputType)
	    {
		    matched = 1;
		    break;
	    }
	    i++;

    }while(ret >=0);

    if(!matched)
    {
	    LOGE("no matched output type [type : 0x%08x]\n", mV4LOutputType);
	    return -1;
    }

    // set output
    tvout_v4l2_s_output(mFdTvout, output.index);
    output_index = 0;
    tvout_v4l2_g_output(mFdTvout, &output_index);

    // initialize mTvoutParam data
    //set fmt param
    mTvoutParam.tvout_src.base_y         = (void *)0x0;
    mTvoutParam.tvout_src.base_c         = (void *)0x0;
    mTvoutParam.tvout_src.pix_fmt.width  = 0;
    mTvoutParam.tvout_src.pix_fmt.height = 0;
    mTvoutParam.tvout_src.pix_fmt.field  = V4L2_FIELD_NONE;
    //mTvoutParam.tvout_src.pix_fmt.pixelformat = VPROC_SRC_COLOR_TILE_NV12;
    mTvoutParam.tvout_src.pix_fmt.pixelformat = V4L2_PIX_FMT_NV12T;

    desc.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;

    tvout_v4l2_enum_fmt(mFdTvout, &desc);

    //set window param
    mTvoutParam.tvout_rect.flags         = 0;
    mTvoutParam.tvout_rect.priority      = 1;
    mTvoutParam.tvout_rect.win.w.left    = 0;
    mTvoutParam.tvout_rect.win.w.top     = 0;
    mTvoutParam.tvout_rect.win.w.width   = 0;
    mTvoutParam.tvout_rect.win.w.height  = 0;

    return mFdTvout;
}


bool SecHdmi::m_runEdid(bool flagInsert)
{
    bool ret = false;

    if(flagInsert == true)
    {
        Mutex::Autolock lock(mHdmiThreadLock);

        if (mFlagHdmiThreadRun == true) {
            // already running
            LOGE("%s::mFlagHdmiThreadRun == true fail \n", __func__);
            goto hdmi_run_edid_done;
        }

        if(initCEC() == false)
        {
            LOGE("%s::initCEC() fail \n", __func__);
            goto hdmi_run_edid_done;
        }

        mHdmiThread = new HdmiThread(this);
    }
    else //if(flagInsert == false)
    {
        if (mFlagHdmiThreadRun == false) {
            // already running
            LOGE("%s::mFlagHdmiThreadRun == false fail \n", __func__);
            goto hdmi_run_edid_done;
        }

        sp<HdmiThread> hdmiThread;

        { // scope for the lock
            Mutex::Autolock lock(mHdmiThreadLock);
            hdmiThread = mHdmiThread;
        }
 //fighter++
//        if (hdmiThread != 0)
//        {
//            hdmiThread->requestExitAndWait();
//        }
 //fighter--
        Mutex::Autolock lock(mHdmiThreadLock);
        hdmiThread.clear();
        mFirstRef = false;

        if(deinitCEC() == false)
        {
            LOGE("%s::deinitCEC() fail \n", __func__);
            goto hdmi_run_edid_done;
        }
    }

    mFlagHdmiThreadRun = flagInsert;
    ret = true;

hdmi_run_edid_done :

    return ret;
}

bool SecHdmi::initCEC(void)
{
    bool flagEdidOpen = false;

    if (!EDIDOpen())
    {
        LOGE("EDIDInit() failed!\n");
        goto initCEC_fail;
    }

    flagEdidOpen = true;

    if (!EDIDRead())
    {
        LOGE("EDIDRead() failed!\n");
        goto initCEC_fail;
    }

    return true;

initCEC_fail :

    if(flagEdidOpen == true)
    {
        if (!EDIDClose())
            LOGE("EDIDClose() failed!\n");
    }
    return false;
}

bool SecHdmi::deinitCEC(void)
{
    int ret = true;

    if (!CECClose())
    {
        LOGE("CECClose() failed!\n");
        ret = false;
    }

    if (!EDIDClose())
    {
        LOGE("EDIDClose() failed!\n");
        //LOGD("%s: EDID is closed!!!!", __func__);
        ret = false;
    }

    return ret;
}

bool SecHdmi::runCEC(void)
{
    int size;
    unsigned char lsrc, ldst, opcode;
    bool flagCecOpen  = false;

    if (mFirstRef == false) {

        // block signal
        sigset_t signal_set;
        /* add all signals */
        sigfillset(&signal_set);
        /* set signal mask */
        pthread_sigmask(SIG_BLOCK, &signal_set, 0);

        // set to not valid physical address
        mCecPAddr = CEC_NOT_VALID_PHYSICAL_ADDRESS;

        if (!EDIDGetCECPhysicalAddress(&mCecPAddr)) {
            LOGE("Error: EDIDGetCECPhysicalAddress() failed.\n");
            goto initCEC_fail;
        }

        LOGD("Device physical address is %X.%X.%X.%X\n",
                (mCecPAddr & 0xF000) >> 12, (mCecPAddr & 0x0F00) >> 8,
                (mCecPAddr & 0x00F0) >> 4, mCecPAddr & 0x000F);

        if (!CECOpen()) {
            LOGE("CECOpen() failed!!!\n");
            goto initCEC_fail;
        }

        flagCecOpen = true;

        // a logical address should only be allocated when a device
        // has a valid physical address, at all other times a device
        // should take the 'Unregistered' logical address (15)

        // if physical address is not valid device should take
        // the 'Unregistered' logical address (15)

            mCecLAddr = CECAllocLogicalAddress(mCecPAddr, mCecDeviceType);
        if (!mCecLAddr) {
            LOGE("CECAllocLogicalAddress() failed!!!\n");
            goto initCEC_fail;
        }
        mFirstRef = true;
    }

    size = CECReceiveMessage(mCecBuffer, CEC_MAX_FRAME_SIZE, 100000);

    if (!size) { // no data available or ctrl-c
        //LOGE("CECReceiveMessage() failed!\n");
        return true;
    }

    if (size == 1)
        return true;

    lsrc = mCecBuffer[0] >> 4;

    // ignore messages with src address == mCecLAddr
    if (lsrc == mCecLAddr)
        return true;

    opcode = mCecBuffer[1];

    if (CECIgnoreMessage(opcode, lsrc)) {
        LOGE("### ignore message coming from address 15 (unregistered)\n");
        return true;
    }

    if (!CECCheckMessageSize(opcode, size)) {
        LOGE("### invalid message size: %d(opcode: 0x%x) ###\n", size, opcode);
        return true;
    }

    // check if message broadcasted/directly addressed
    if (!CECCheckMessageMode(opcode, (mCecBuffer[0] & 0x0F) == CEC_MSG_BROADCAST ? 1 : 0)) {
        LOGE("### invalid message mode (directly addressed/broadcast) ###\n");
        return true;
    }

    ldst = lsrc;

    //TODO: macroses to extract src and dst logical addresses
    //TODO: macros to extract opcode

    switch (opcode)
    {
        case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
            // responce with "Report Physical Address"
            mCecBuffer[0] = (mCecLAddr << 4) | CEC_MSG_BROADCAST;
            mCecBuffer[1] = CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
            mCecBuffer[2] = (mCecPAddr >> 8) & 0xFF;
            mCecBuffer[3] = mCecPAddr & 0xFF;
            mCecBuffer[4] = mCecDeviceType;
            size = 5;
            break;

        case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
            LOGD("[CEC_OPCODE_REQUEST_ACTIVE_SOURCE]\n");
            // responce with "Active Source"
            mCecBuffer[0] = (mCecLAddr << 4) | CEC_MSG_BROADCAST;
            mCecBuffer[1] = CEC_OPCODE_ACTIVE_SOURCE;
            mCecBuffer[2] = (mCecPAddr >> 8) & 0xFF;
            mCecBuffer[3] = mCecPAddr & 0xFF;
            size = 4;
            LOGD("Tx : [CEC_OPCODE_ACTIVE_SOURCE]\n");
            break;

        case CEC_OPCODE_ABORT:
        case CEC_OPCODE_FEATURE_ABORT:
        default:
            // send "Feature Abort"
            mCecBuffer[0] = (mCecLAddr << 4) | ldst;
            mCecBuffer[1] = CEC_OPCODE_FEATURE_ABORT;
            mCecBuffer[2] = CEC_OPCODE_ABORT;
            mCecBuffer[3] = 0x04; // "refused"
            size = 4;
            break;
    }
//fighter++
//    if (CECSendMessage(mCecBuffer, size) != size)
//        LOGE("CECSendMessage() failed!!!\n");
//fighter--

    return true;

initCEC_fail :
    if(flagCecOpen == true)
    {
        if (!CECClose())
            LOGE("CECClose() failed!\n");
    }
    return false;
}
} // namespace android
