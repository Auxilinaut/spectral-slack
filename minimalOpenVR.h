#pragma once
/**
  Credit of original goes to Morgan McGuire

  This requires the bin, lib, and headers directories from the 
  OpenVR SDK: https://github.com/ValveSoftware/openvr

  The runtime for OpenVR is distributed with Steam. Ensure that 
  you've run Steam and let it update to the latest SteamVR before
  running an OpenVR program.
*/

#include <openvr.h>
#include <glm\glm.hpp>
#include <string>

#ifdef _WINDOWS
#   pragma comment(lib, "openvr_api")
#endif

/** Called by initOpenVR */
std::string getHMDString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError* peError = nullptr) {
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (unRequiredBufferLen == 0) {
	    return "";
    }

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}


/** Call immediately before initializing OpenGL 

    \param hmdWidth, hmdHeight recommended render target resolution
*/
vr::IVRSystem* initOpenVR(uint32_t& hmdWidth, uint32_t& hmdHeight) {
	vr::EVRInitError eError = vr::VRInitError_None;
	vr::IVRSystem* hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None) {
        fprintf(stderr, "OpenVR Initialization Error: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
        return nullptr;
	}
    
	const std::string& driver = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	const std::string& model  = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);
	const std::string& serial = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);
    const float freq = hmd->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);

    //get the proper resolution of the hmd
    hmd->GetRecommendedRenderTargetSize(&hmdWidth, &hmdHeight);

    fprintf(stderr, "HMD: %s '%s' #%s (%d x %d @ %g Hz)\n", driver.c_str(), model.c_str(), serial.c_str(), hmdWidth, hmdHeight, freq);

    // Initialize the compositor
    vr::IVRCompositor* compositor = vr::VRCompositor();
	if (! compositor) {
		fprintf(stderr, "OpenVR Compositor initialization failed. See log file for details\n");
        vr::VR_Shutdown();
        assert("VR failed" && false);
	}

    return hmd;
}

/*
GLM to VR 3x4 matrix helper
*/
inline vr::HmdMatrix34_t toOpenVr(const glm::mat4& m) {
	vr::HmdMatrix34_t result;
	for (uint8_t i = 0; i < 3; ++i) {
		for (uint8_t j = 0; j < 4; ++j) {
			result.m[i][j] = m[j][i];
		}
	}
	return result;
}

/*
VR to GLM 3x4 matrix helper
*/
inline glm::mat4 toGlm(const vr::HmdMatrix34_t& m) {
	glm::mat4 result = glm::mat4(
		m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
		m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
		m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
		m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
	return result;
}

/*
VR to GLM 4x4 matrix helper
*/
inline glm::mat4 toGlmMat4(const vr::HmdMatrix44_t& m) {
	glm::mat4 result = glm::mat4(
		m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0],
		m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1],
		m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2],
		m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3] );
	return result;
}

/**
 */
void getEyeTransformations
   (vr::IVRSystem*  hmd,
    vr::TrackedDevicePose_t* trackedDevicePose,
    float           nearPlaneZ,
    float           farPlaneZ,
    glm::mat4*          headToWorld3x4,
	glm::mat4*          ltEyeToHead3x4,
	glm::mat4*          rtEyeToHead3x4,
	glm::mat4*          ltProjectionMatrix4x4,
	glm::mat4*          rtProjectionMatrix4x4) {

    assert(nearPlaneZ > 0.0f && farPlaneZ > nearPlaneZ);

    vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

#   if defined(_DEBUG) && 0
        fprintf(stderr, "Devices tracked this frame: \n");
        int poseCount = 0;
	    for (int d = 0; d < vr::k_unMaxTrackedDeviceCount; ++d)	{
		    if (trackedDevicePose[d].bPoseIsValid) {
			    ++poseCount;
			    switch (hmd->GetTrackedDeviceClass(d)) {
                case vr::TrackedDeviceClass_Controller:        fprintf(stderr, "   Controller: ["); break;
                case vr::TrackedDeviceClass_HMD:               fprintf(stderr, "   HMD: ["); break;
                case vr::TrackedDeviceClass_Invalid:           fprintf(stderr, "   <invalid>: ["); break;
                case vr::TrackedDeviceClass_Other:             fprintf(stderr, "   Other: ["); break;
                case vr::TrackedDeviceClass_TrackingReference: fprintf(stderr, "   Reference: ["); break;
                default:                                       fprintf(stderr, "   ???: ["); break;
			    }
                /*for (int r = 0; r < 3; ++r) { 
                    for (int c = 0; c < 4; ++c) {
                        fprintf(stderr, "%g, ", trackedDevicePose[d].mDeviceToAbsoluteTracking.m[r][c]);
                    }
                }
                fprintf(stderr, "]\n");*/
		    }
	    }
        //fprintf(stderr, "\n");
#   endif

    assert(trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid);
    const vr::HmdMatrix34_t head = trackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].mDeviceToAbsoluteTracking;

    const vr::HmdMatrix34_t& ltMatrix = hmd->GetEyeToHeadTransform(vr::Eye_Left);
    const vr::HmdMatrix34_t& rtMatrix = hmd->GetEyeToHeadTransform(vr::Eye_Right);

	/*for (int r = 0; r < 3; ++r) {
		for (int c = 0; c < 4; ++c) {
			ltEyeToHead3x4[r * 4 + c] = ltMatrix.m[c][r];
			rtEyeToHead3x4[r * 4 + c] = rtMatrix.m[c][r];
			headToWorld3x4[r * 4 + c] = head.m[c][r];
			printf("r: %d, c: %d, head.m[c][r]: %d", r, c, head.m[c][r]);
			printf("\n");
		}
	}*/
	*ltEyeToHead3x4 = toGlm(ltMatrix);
	*rtEyeToHead3x4 = toGlm(rtMatrix);
	*headToWorld3x4 = toGlm(head);

    const vr::HmdMatrix44_t& ltProj = hmd->GetProjectionMatrix(vr::Eye_Left,  nearPlaneZ, farPlaneZ);
    const vr::HmdMatrix44_t& rtProj = hmd->GetProjectionMatrix(vr::Eye_Right, nearPlaneZ, farPlaneZ);

	/*for (int r = 0; r < 4; ++r) {
		for (int c = 0; c < 4; ++c) {
			ltProjectionMatrix4x4[r * 4 + c] = ltProj.m[r][c];
			rtProjectionMatrix4x4[r * 4 + c] = rtProj.m[r][c];
		}
	}*/

	*ltProjectionMatrix4x4 = toGlmMat4(ltProj);
	*rtProjectionMatrix4x4 = toGlmMat4(rtProj);
}


/** Call immediately before OpenGL swap buffers */
void submitToHMD(GLint ltEyeTexture, GLint rtEyeTexture, bool isGammaEncoded) {
    const vr::EColorSpace colorSpace = isGammaEncoded ? vr::ColorSpace_Gamma : vr::ColorSpace_Linear;

    const vr::Texture_t lt = { reinterpret_cast<void*>(intptr_t(ltEyeTexture))};
    vr::VRCompositor()->Submit(vr::Eye_Left, &lt);

    const vr::Texture_t rt = { reinterpret_cast<void*>(intptr_t(rtEyeTexture))};
    vr::VRCompositor()->Submit(vr::Eye_Right, &rt);

    // Tell the compositor to begin work immediately instead of waiting for the next WaitGetPoses() call
    vr::VRCompositor()->PostPresentHandoff();
}
