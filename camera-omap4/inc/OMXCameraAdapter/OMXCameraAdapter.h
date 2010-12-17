/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */


#ifndef OMX_CAMERA_ADAPTER_H
#define OMX_CAMERA_ADAPTER_H

#include "CameraHal.h"
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_CoreExt.h"
#include "OMX_IVCommon.h"
#include "OMX_Component.h"
#include "OMX_Index.h"
#include "OMX_IndexExt.h"
#include "OMX_TI_Index.h"
#include "OMX_TI_IVCommon.h"
#include "OMX_TI_Common.h"
#include "OMX_TI_Image.h"
#include "General3A_Settings.h"

#include "BaseCameraAdapter.h"
#include "DebugUtils.h"


extern "C"
{
#include "timm_osal_error.h"
#include "timm_osal_events.h"
#include "timm_osal_trace.h"
#include "timm_osal_semaphores.h"
}

namespace android {

#define FOCUS_THRESHOLD  5 //[s.]

#define MIN_JPEG_QUALITY 1
#define MAX_JPEG_QUALITY 100
#define EXP_BRACKET_RANGE 10

#define TOUCH_DATA_SIZE         200
#define DEFAULT_THUMB_WIDTH     160
#define DEFAULT_THUMB_HEIGHT    120
#define FRAME_RATE_FULL_HD      27
#define ZOOM_STAGES 61
#define FACE_DETECTION_BUFFER_SIZE  0x1000

/* Default portstartnumber of Camera component */
#define OMX_CAMERA_DEFAULT_START_PORT_NUM 0

/* Define number of ports for differt domains */
#define OMX_CAMERA_PORT_OTHER_NUM 1
#define OMX_CAMERA_PORT_VIDEO_NUM 4
#define OMX_CAMERA_PORT_IMAGE_NUM 1
#define OMX_CAMERA_PORT_AUDIO_NUM 0
#define OMX_CAMERA_NUM_PORTS (OMX_CAMERA_PORT_OTHER_NUM + OMX_CAMERA_PORT_VIDEO_NUM + OMX_CAMERA_PORT_IMAGE_NUM + OMX_CAMERA_PORT_AUDIO_NUM)

/* Define start port number for differt domains */
#define OMX_CAMERA_PORT_OTHER_START OMX_CAMERA_DEFAULT_START_PORT_NUM
#define OMX_CAMERA_PORT_VIDEO_START (OMX_CAMERA_PORT_OTHER_START + OMX_CAMERA_PORT_OTHER_NUM)
#define OMX_CAMERA_PORT_IMAGE_START (OMX_CAMERA_PORT_VIDEO_START + OMX_CAMERA_PORT_VIDEO_NUM)
#define OMX_CAMERA_PORT_AUDIO_START (OMX_CAMERA_PORT_IMAGE_START + OMX_CAMERA_PORT_IMAGE_NUM)

/* Port index for camera component */
#define OMX_CAMERA_PORT_OTHER_IN (OMX_CAMERA_PORT_OTHER_START + 0)
#define OMX_CAMERA_PORT_VIDEO_IN_VIDEO (OMX_CAMERA_PORT_VIDEO_START + 0)
#define OMX_CAMERA_PORT_VIDEO_OUT_PREVIEW (OMX_CAMERA_PORT_VIDEO_START + 1)
#define OMX_CAMERA_PORT_VIDEO_OUT_VIDEO (OMX_CAMERA_PORT_VIDEO_START + 2)
#define OMX_CAMERA_PORT_VIDEO_OUT_MEASUREMENT (OMX_CAMERA_PORT_VIDEO_START + 3)
#define OMX_CAMERA_PORT_IMAGE_OUT_IMAGE (OMX_CAMERA_PORT_IMAGE_START + 0)


#define OMX_INIT_STRUCT(_s_, _name_)	\
    memset(&(_s_), 0x0, sizeof(_name_));	\
    (_s_).nSize = sizeof(_name_);		\
    (_s_).nVersion.s.nVersionMajor = 0x1;	\
    (_s_).nVersion.s.nVersionMinor = 0x1;	\
    (_s_).nVersion.s.nRevision = 0x0;		\
    (_s_).nVersion.s.nStep = 0x0

#define OMX_INIT_STRUCT_PTR(_s_, _name_)   \
    memset((_s_), 0x0, sizeof(_name_));         \
    (_s_)->nSize = sizeof(_name_);              \
    (_s_)->nVersion.s.nVersionMajor = 0x1;      \
    (_s_)->nVersion.s.nVersionMinor = 0x1;      \
    (_s_)->nVersion.s.nRevision = 0x0;          \
    (_s_)->nVersion.s.nStep = 0x0

#define GOTO_EXIT_IF(_CONDITION,_ERROR) {                                       \
    if ((_CONDITION)) {                                                         \
        eError = (_ERROR);                                                      \
        goto EXIT;                                                              \
    }                                                                           \
}

///OMX Specific Functions
static OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_EVENTTYPE eEvent,
                                        OMX_IN OMX_U32 nData1,
                                        OMX_IN OMX_U32 nData2,
                                        OMX_IN OMX_PTR pEventData);

static OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

static OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                        OMX_IN OMX_PTR pAppData,
                                        OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);


/**
  * Class which completely abstracts the camera hardware interaction from camera hal
  * TODO: Need to list down here, all the message types that will be supported by this class
                Need to implement BufferProvider interface to use AllocateBuffer of OMX if needed
  */
class OMXCameraAdapter : public BaseCameraAdapter
{
public:

    /*--------------------Constant declarations----------------------------------------*/
    static const int32_t MAX_NO_BUFFERS = 20;

    ///@remarks OMX Camera has six ports - buffer input, time input, preview, image, video, and meta data
    static const int MAX_NO_PORTS = 6;

    ///Five second timeout
    static const int CAMERA_ADAPTER_TIMEOUT = 5000*1000;

    enum OMXCameraEvents
        {
        CAMERA_PORT_ENABLE  = 0x1,
        CAMERA_PORT_FLUSH   = 0x2,
        CAMERA_PORT_DISABLE = 0x4,
        };

    enum CaptureMode
        {
        HIGH_SPEED = 1,
        HIGH_QUALITY = 2,
        VIDEO_MODE = 3,
        };

    enum IPPMode
        {
        IPP_NONE = 0,
        IPP_NSF,
        IPP_LDC,
        IPP_LDCNSF,
        };

    enum CodingMode
        {
        CodingNone = 0,
        CodingJPS,
        CodingMPO,
        CodingRAWJPEG,
        CodingRAWMPO,
        };

    enum Algorithm3A
        {
        WHITE_BALANCE_ALGO = 0x1,
        EXPOSURE_ALGO = 0x2,
        FOCUS_ALGO = 0x4,
        };

    enum AlgoPriority
        {
        FACE_PRIORITY = 0,
        REGION_PRIORITY,
        };

    ///Parameters specific to any port of the OMX Camera component
    class OMXCameraPortParameters
    {
        public:
            OMX_U32                         mHostBufaddr[MAX_NO_BUFFERS];
            OMX_BUFFERHEADERTYPE           *mBufferHeader[MAX_NO_BUFFERS];
            OMX_U32                         mWidth;
            OMX_U32                         mHeight;
            OMX_U32                         mStride;
            OMX_U8                          mNumBufs;
            OMX_U32                         mBufSize;
            OMX_COLOR_FORMATTYPE            mColorFormat;
            OMX_PARAM_VIDEONOISEFILTERTYPE  mVNFMode;
            OMX_PARAM_VIDEOYUVRANGETYPE     mYUVRange;
            OMX_CONFIG_BOOLEANTYPE          mVidStabParam;
            OMX_CONFIG_FRAMESTABTYPE        mVidStabConfig;
            OMX_U32                         mCapFrame;
            OMX_U32                         mFrameRate;
            CameraFrame::FrameType mImageType;
    };

    ///Context of the OMX Camera component
    class OMXCameraAdapterComponentContext
    {
        public:
            OMX_HANDLETYPE              mHandleComp;
            OMX_U32                     mNumPorts;
            OMX_STATETYPE               mState ;
            OMX_U32                     mVideoPortIndex;
            OMX_U32                     mPrevPortIndex;
            OMX_U32                     mImagePortIndex;
            OMX_U32                     mMeasurementPortIndex;
            OMXCameraPortParameters     mCameraPortParams[MAX_NO_PORTS];
    };

public:

    OMXCameraAdapter();
    ~OMXCameraAdapter();


    ///Initialzes the camera adapter creates any resources required
    virtual status_t initialize(int sensor_index=0);

    //APIs to configure Camera adapter and get the current parameter set
    virtual status_t setParameters(const CameraParameters& params);
    virtual void getParameters(CameraParameters& params);

    virtual void returnFrame(void* frameBuf, CameraFrame::FrameType frameType);

    //API to get the caps
    virtual status_t getCaps();

    //API to give the buffers to Adapter
    virtual status_t useBuffers(CameraMode mode, void* bufArr, int num);

    // API
    virtual status_t UseBuffersPreview(void* bufArr, int num);

    //API to flush the buffers for preview
    status_t flushBuffers();

    //API to send a command to the camera
    virtual status_t sendCommand(int operation, int value1=0, int value2=0, int value3=0);

    //API to cancel a currently executing command
    virtual status_t cancelCommand(int operation);

    // API
    virtual status_t setFormat(OMX_U32 port, OMXCameraPortParameters &cap);

    //API to get the frame size required to be allocated. This size is used to override the size passed
    //by camera service when VSTAB/VNF is turned ON for example
    virtual void getFrameSize(int &width, int &height);

    virtual status_t getPictureBufferSize(size_t &length, size_t bufferCount);

    virtual status_t getFrameDataSize(size_t &dataFrameSize, size_t bufferCount);

 OMX_ERRORTYPE OMXCameraAdapterEventHandler(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_EVENTTYPE eEvent,
                                    OMX_IN OMX_U32 nData1,
                                    OMX_IN OMX_U32 nData2,
                                    OMX_IN OMX_PTR pEventData);

 OMX_ERRORTYPE OMXCameraAdapterEmptyBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_BUFFERHEADERTYPE* pBuffer);

 OMX_ERRORTYPE OMXCameraAdapterFillBufferDone(OMX_IN OMX_HANDLETYPE hComponent,
                                    OMX_IN OMX_BUFFERHEADERTYPE* pBuffHeader);


private:

    OMXCameraPortParameters *getPortParams(CameraFrame::FrameType frameType);
    void setFrameRefCount(void* frameBuf, CameraFrame::FrameType frameType, int refCount);
    int getFrameRefCount(void* frameBuf, CameraFrame::FrameType frameType);
    size_t getSubscriberCount(CameraFrame::FrameType frameType);
    status_t fillThisBuffer(void* frameBuf, OMXCameraPortParameters *port);

    OMX_ERRORTYPE SignalEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                                  OMX_IN OMX_EVENTTYPE eEvent,
                                                  OMX_IN OMX_U32 nData1,
                                                  OMX_IN OMX_U32 nData2,
                                                  OMX_IN OMX_PTR pEventData);

    status_t RegisterForEvent(OMX_IN OMX_HANDLETYPE hComponent,
                                          OMX_IN OMX_EVENTTYPE eEvent,
                                          OMX_IN OMX_U32 nData1,
                                          OMX_IN OMX_U32 nData2,
                                          OMX_IN Semaphore &semaphore,
                                          OMX_IN OMX_U32 timeout);

    //Instance timeout methods
    status_t setTimeOut(unsigned int sec);
    status_t cancelTimeOut();

    status_t setPictureRotation(unsigned int degree);
    status_t setImageQuality(unsigned int quality);
    status_t setThumbnailParams(unsigned int width, unsigned int height, unsigned int quality);

    //Focus functionality
    status_t doAutoFocus();
    status_t stopAutoFocus();
    status_t notifyFocusSubscribers(bool override, bool status);
    status_t checkFocus(OMX_PARAM_FOCUSSTATUSTYPE *eFocusStatus);

    //VSTAB and VNF Functionality
    status_t enableVideoNoiseFilter(bool enable);
    status_t enableVideoStabilization(bool enable);

    //Digital zoom
    status_t doZoom(int index);
    status_t notifyZoomSubscribers(int zoomIdx, bool targetReached);
    status_t advanceZoom();

    //Scenes
    OMX_ERRORTYPE setScene(Gen3A_settings& Gen3A);

    //Flash modes
    status_t setFlashMode(Gen3A_settings& Gen3A);

    //Exposure Modes
    OMX_ERRORTYPE setExposureMode(Gen3A_settings& Gen3A);

    //Manual Exposure
    status_t setManualExposure(Gen3A_settings& Gen3A);

    //Manual Gain
    status_t setManualGain(Gen3A_settings& Gen3A);

    //Noise filtering
    status_t setNSF(OMXCameraAdapter::IPPMode mode);

    //LDC
    status_t setLDC(OMXCameraAdapter::IPPMode mode);

    //Touch AF
    status_t parseTouchFocusPosition(const char *pos, unsigned int &posX, unsigned int &posY);
    status_t setTouchFocus(unsigned int posX, unsigned int posY, size_t width, size_t height);

    //Face detection
    status_t setFaceDetection(bool enable);
    status_t detectFaces(OMX_BUFFERHEADERTYPE* pBuffHeader);
    status_t encodeFaceCoordinates(const OMX_FACEDETECTIONTYPE *faceData, char *faceString, size_t faceStringSize);

    //3A Algorithms priority configuration
    status_t setAlgoPriority(AlgoPriority priority, Algorithm3A algo, bool enable);

    //Sensor overclocking
    status_t setSensorOverclock(bool enable);

    // Preview Service
    status_t startPreview();
    status_t stopPreview();

    //Video recording service
    status_t startVideoCapture();
    status_t stopVideoCapture();

    //Exposure Bracketing
    status_t setExposureBracketing(int *evValues, size_t evCount, size_t frameCount);
    status_t parseExpRange(const char *rangeStr, int * expRange, size_t count, size_t &validEntries);

    //Temporal Bracketing
    status_t stopBracketing();
    status_t startBracketing();
    status_t doBracketing(OMX_BUFFERHEADERTYPE *pBuffHeader, CameraFrame::FrameType typeOfFrame);
    status_t sendBracketFrames();

    // Image Capture Service
    status_t startImageCapture();
    status_t stopImageCapture();

    //Shutter callback notifications
    status_t notifyShutterSubscribers();
    status_t setShutterCallback(bool enabled);

    //Sets eithter HQ or HS mode and the frame count
    status_t setCaptureMode(OMXCameraAdapter::CaptureMode mode);
    status_t UseBuffersCapture(void* bufArr, int num);
    status_t UseBuffersPreviewData(void* bufArr, int num);

    //Used for calculation of the average frame rate during preview
    status_t recalculateFPS();

    ///Send the frame to subscribers
    status_t  sendFrameToSubscribers(OMX_IN OMX_BUFFERHEADERTYPE *pBuffHeader, int typeOfFrame,  OMXCameraPortParameters *port);

    const char* getLUTvalue_OMXtoHAL(int OMXValue, LUTtype LUT);
    int getLUTvalue_HALtoOMX(const char * HalValue, LUTtype LUT);
    OMX_ERRORTYPE apply3Asettings( Gen3A_settings& Gen3A );

    // AutoConvergence
    status_t setAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE pACMode, OMX_S32 pManualConverence);
    status_t getAutoConvergence(OMX_TI_AUTOCONVERGENCEMODETYPE *pACMode, OMX_S32 *pManualConverence);


public:

private:

    //AF callback
    status_t setFocusCallback(bool enabled);

    char mTouchCoords[TOUCH_DATA_SIZE];
    unsigned int mTouchFocusPosX;
    unsigned int mTouchFocusPosY;

    CaptureMode mCapMode;
    size_t mBurstFrames;
    size_t mCapturedFrames;

    bool mMeasurementEnabled;

#if PPM_INSTRUMENTATION || PPM_INSTRUMENTATION_ABS

    struct timeval mStartFocus;
    struct timeval mStartCapture;

#endif

    //Exposure Bracketing
    int mExposureBracketingValues[EXP_BRACKET_RANGE];
    size_t mExposureBracketingValidEntries;

    mutable Mutex mFaceDetectionLock;
    //Face detection status
    bool mFaceDetectionRunning;
    //Buffer for storing face detection results
    char mFaceDectionResult [FACE_DETECTION_BUFFER_SIZE];

    //Image post-processing
    IPPMode mIPP;

    //jpeg Picture Quality
    unsigned int mPictureQuality;

    //thumbnail resolution
    unsigned int mThumbWidth, mThumbHeight;

    //thumbnail quality
    unsigned int mThumbQuality;

    //variables holding the estimated framerate
    float mFPS, mLastFPS;

    //automatically disable AF after a given amount of frames
    unsigned int mFocusThreshold;

    //This is needed for the CTS tests. They falsely assume, that during
    //smooth zoom the current zoom stage will not change within the
    //zoom callback scope, which in a real world situation is not always the
    //case. This variable will "simulate" the expected behavior
    unsigned int mZoomParameterIdx;

    //current zoom
    Mutex mZoomLock;
    bool mSmoothZoomEnabled;
    unsigned int mCurrentZoomIdx, mTargetZoomIdx;
    int mZoomInc;
    bool mReturnZoomStatus;
    static const int32_t ZOOM_STEPS [];

     //local copy
    OMX_VERSIONTYPE mLocalVersionParam;

    unsigned int mPending3Asettings;
    Gen3A_settings mParameters3A;

    CameraParameters mParams;
    unsigned int mPictureRotation;
    bool mFocusStarted;
    bool mWaitingForSnapshot;
    int mSnapshotCount;
    int mPreviewBufferCount;
    int *mPreviewBuffers;
    KeyedVector<int, int> mPreviewBuffersAvailable;

    int *mCaptureBuffers;
    KeyedVector<int, bool> mCaptureBuffersAvailable;
    int mCaptureBuffersCount;
    size_t mCaptureBuffersLength;
    mutable Mutex mCaptureBufferLock;

    int *mPreviewDataBuffers;
    KeyedVector<int, bool> mPreviewDataBuffersAvailable;
    int mPreviewDataBuffersCount;
    size_t mPreviewDataBuffersLength;
    mutable Mutex mPreviewDataBufferLock;

    mutable Mutex mBracketingLock;
    bool *mBracketingBuffersQueued;
    int mBracketingBuffersQueuedCount;
    int mLastBracetingBufferIdx;
    bool mBracketingEnabled;
    int mBracketingRange;

    int *mVideoBuffers;
    KeyedVector<int, int> mVideoBuffersAvailable;
    int mVideoBuffersCount;
    size_t mVideoBuffersLength;
    mutable Mutex mVideoBufferLock;

    CameraParameters mParameters;
    OMXCameraAdapterComponentContext mCameraAdapterParameters;
    bool mFirstTimeInit;

    ///Semaphores used internally
    MessageQueue mEventSignalQ;
    Mutex mLock;
    bool mPreviewing;
    bool mCapturing;
    bool mRecording;
    bool mFlushBuffers;
    mutable Mutex mSubscriberLock;
    mutable Mutex mPreviewBufferLock;

    OMX_STATETYPE mComponentState;

    bool mVnfEnabled;
    bool mVstabEnabled;

    int mFrameCount;
    int mLastFrameCount;
    unsigned int mIter;
    nsecs_t mLastFPSTime;

    int mSensorIndex;
    CodingMode mCodingMode;
    Mutex mEventLock;
};
}; //// namespace
#endif //OMX_CAMERA_ADAPTER_H

