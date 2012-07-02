/*
**
** Copyright 2008, The Android Open Source Project
** Copyright 2010, Samsung Electronics Co. LTD
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
**
** @author  Taikyung, Yu(taikyung.yu@samsung.com)
** @date    2011-07-06
*/

#define LOG_TAG "SecTVOutService"

#include <binder/IServiceManager.h>
#include <utils/RefBase.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>
#include <utils/Log.h>
#include "SecTVOutService.h"
#include <linux/fb.h>

namespace android {
#define DEFAULT_LCD_WIDTH               800
#define DEFAULT_LCD_HEIGHT              480
    enum {
        SET_HDMI_STATUS = IBinder::FIRST_CALL_TRANSACTION,
        SET_HDMI_MODE,
        SET_HDMI_RESOLUTION,
        SET_HDMI_HDCP,
        BLIT_2_HDMI
    };

    int SecTVOutService::instantiate()
    {
        LOGD ("SecTVOutService instantiate");
        int r = defaultServiceManager()->addService(String16( "SecTVOutService"), new SecTVOutService ());
        LOGD ("SecTVOutService r=%d\n", r);

        return r;
    }

    SecTVOutService::SecTVOutService () {
        LOGV("SecTVOutService created");
        if (mSecHdmi.create() == false)
            LOGE("%s::mSecHdmi.create() fail \n", __func__);
        else {
            setLCDsize();
            setHdmiStatus(1);
        }
    }

    void SecTVOutService::setLCDsize(void) {
            char const * const device_template[] = {
                "/dev/graphics/fb%u",
                "/dev/fb%u",
                0 };

            int fd = -1;
            int i = 0;
            char name[64];

            while ((fd==-1) && device_template[i]) {
                snprintf(name, 64, device_template[i], 0);
                fd = open(name, O_RDWR, 0);
                i++;
            }
            if (fd > 0) {
                struct fb_var_screeninfo info;
                if (ioctl(fd, FBIOGET_VSCREENINFO, &info) != -1) {
                    mLCD_width  = info.xres;
                    mLCD_height = info.yres;
                } else {
                    mLCD_width  = DEFAULT_LCD_WIDTH;
                    mLCD_height = DEFAULT_LCD_HEIGHT;
                }
                close(fd);
            }
            return;
    }

    SecTVOutService::~SecTVOutService () {
        LOGV ("SecTVOutService destroyed");
    }

    status_t SecTVOutService::onTransact(uint32_t code, const Parcel & data, Parcel * reply, uint32_t flags)
    {
        switch (code) {
        case SET_HDMI_STATUS: {
            int status = data.readInt32();
            setHdmiStatus(status);
        } break;

        case SET_HDMI_MODE: {
            int mode = data.readInt32();
            setHdmiMode(mode);
        } break;

        case SET_HDMI_RESOLUTION: {
            int resolution = data.readInt32();
            setHdmiResolution(resolution);
        } break;

        case SET_HDMI_HDCP: {
            int enHdcp = data.readInt32();
            setHdmiHdcp(enHdcp);
        } break;

        case BLIT_2_HDMI: {
            uint32_t w = data.readInt32();
            uint32_t h = data.readInt32();
            uint32_t colorFormat = data.readInt32();
            uint32_t hdmiLayer   = data.readInt32();
            uint32_t physYAddr  = data.readInt32();
            uint32_t physCbAddr = data.readInt32();
            uint32_t num_of_hwc_layer = data.readInt32();

            blit2Hdmi(w, h, colorFormat, hdmiLayer, physYAddr, physCbAddr, num_of_hwc_layer);
        } break;

        default :
            LOGE ( "onTransact::default \n");
            return BBinder::onTransact (code, data, reply, flags);
        }

        return NO_ERROR;
    }

    void SecTVOutService::setHdmiStatus(uint32_t status)
    {

        LOGD("%s HDMI cable status = %d", __func__, status);
        {
            Mutex::Autolock _l(mLock);

            bool hdmiCableInserted = (bool)status;

            if (mHdmiCableInserted == hdmiCableInserted)
                return;

            if (hdmiCableInserted == true) {
                if (mSecHdmi.connect() == false) {
                    LOGE("%s::mSecHdmi.connect() fail\n", __func__);
                    hdmiCableInserted = false;
                }
            } else {
                if (mSecHdmi.disconnect() == false)
                    LOGE("%s::mSecHdmi.disconnect() fail\n", __func__);
            }

            mHdmiCableInserted = hdmiCableInserted;
        }

        if (hdmiCableInserted() == true)
            this->blit2Hdmi(mLCD_width, mLCD_height, HAL_PIXEL_FORMAT_BGRA_8888, HDMI_MODE_UI, 0, 0, 0);
    }

    void SecTVOutService::setHdmiMode(uint32_t mode)
    {
        LOGD("%s TV mode = %d", __func__, mode);

        if ((hdmiCableInserted() == true) && (mSecHdmi.setHdmiOutputMode(mode)) == false) {
            LOGE("%s::mSecHdmi.setHdmiOutputMode() fail\n", __func__);
            return;
        }
    }

    void SecTVOutService::setHdmiResolution(uint32_t resolution)
    {
        LOGD("%s TV resolution = %d", __func__, resolution);

        if ((hdmiCableInserted() == true) && (mSecHdmi.setHdmiResolution(resolution)) == false) {
            LOGE("%s::mSecHdmi.setHdmiResolution() fail\n", __func__);
            return;
        }
    }

    void SecTVOutService::setHdmiHdcp(uint32_t hdcp_en)
    {
        LOGD("%s TV HDCP = %d", __func__, hdcp_en);

        if ((hdmiCableInserted() == true) && (mSecHdmi.setHdcpMode(hdcp_en)) == false) {
            LOGE("%s::mSecHdmi.setHdcpMode() fail\n", __func__);
            return;
        }
    }

    void SecTVOutService::blit2Hdmi(uint32_t w, uint32_t h,
                                 uint32_t colorFormat, uint32_t hdmiMode,
                                 uint32_t pPhyYAddr,  uint32_t pPhyCAddr,
                                 uint32_t num_of_hwc_layer)
    {
        if (hdmiCableInserted() == false)
            return;

        int hdmiLayer = SecHdmi::HDMI_LAYER_VIDEO;

        switch (hdmiMode) {
        case HDMI_MODE_UI :
            if (num_of_hwc_layer >= 1) {
                hdmiLayer = SecHdmi::HDMI_LAYER_GRAPHIC_0;
            }
            else
                hdmiLayer = SecHdmi::HDMI_LAYER_VIDEO;

            if (mUILayerMode != hdmiLayer) {
                if (mSecHdmi.clear(mUILayerMode) == false)
                    LOGE("%s::mSecHdmi.clear(%d) fail\n", __func__, mUILayerMode);
            }

            mUILayerMode = hdmiLayer;

            if (mSecHdmi.flush(w, h, colorFormat, hdmiLayer,
                              pPhyYAddr, pPhyCAddr, num_of_hwc_layer) == false)
                LOGE("%s::mSecHdmi.flush() on HDMI_MODE_UI fail\n", __func__);
            break;

        case HDMI_MODE_VIDEO :
            hdmiLayer = SecHdmi::HDMI_LAYER_VIDEO;

            if (mSecHdmi.flush(w, h, colorFormat, hdmiLayer,
                              pPhyYAddr, pPhyCAddr, num_of_hwc_layer) == false)
                LOGE("%s::mSecHdmi.flush() on HDMI_MODE_VIDEO fail\n", __func__);
            break;

        default:
            LOGE("unmatched HDMI_MODE : %d", hdmiMode);
            break;
        }

        return;
    }

    bool SecTVOutService::hdmiCableInserted(void)
    {
        return mHdmiCableInserted;
    }

}
