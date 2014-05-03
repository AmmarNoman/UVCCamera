/*********************************************************************
 *********************************************************************/
/*********************************************************************
 * added and modified some function for support and help for Android
 * Copyright (C) 2014 saki@serenegiant All rights reserved.
 * add:
 * 	added some helper functions for supporting rgb565 and rgbx8888
 * modified:
 * 	modified for optimization with gcc
 * 	modified macros that convert pixel format to reduce cpu cycles
 * 	added boundary check of pointer in the converting function to avoid crash
 *********************************************************************/

/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (C) 2010-2012 Ken Tossell
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the author nor other contributors may be
 *     used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/
/**
 * @defgroup frame Frame processing
 */
#include "libuvc/libuvc.h"
#include "libuvc/libuvc_internal.h"

/** @internal */
uvc_error_t uvc_ensure_frame_size(uvc_frame_t *frame, size_t need_bytes) {
	if (frame->library_owns_data) {
		if (!frame->data || frame->data_bytes != need_bytes) {	// XXX
			frame->data_bytes = need_bytes;
			frame->data = realloc(frame->data, frame->data_bytes);
		}
		if (UNLIKELY(!frame->data || !need_bytes))	// XXX
			return UVC_ERROR_NO_MEM;
		return UVC_SUCCESS;
	} else {
		if (UNLIKELY(!frame->data || frame->data_bytes < need_bytes))	// XXX
			return UVC_ERROR_NO_MEM;
		return UVC_SUCCESS;
	}
}

/** @brief Allocate a frame structure
 * @ingroup frame
 *
 * @param data_bytes Number of bytes to allocate, or zero
 * @return New frame, or NULL on error
 */
uvc_frame_t *uvc_allocate_frame(size_t data_bytes) {
	uvc_frame_t *frame = malloc(sizeof(*frame));

	if (UNLIKELY(!frame))
		return NULL;

	memset(frame, 0, sizeof(*frame));	// bzero(frame, sizeof(*frame)); // bzero is deprecated

//	frame->library_owns_data = 1;	// XXX moved to lower

	if (LIKELY(data_bytes > 0)) {	// XXX add
		frame->library_owns_data = 1;
		frame->data_bytes = data_bytes;
		frame->data = malloc(data_bytes);

		if (UNLIKELY(!frame->data)) {
			free(frame);
			return NULL ;
		}
	}

	return frame;
}

/** @brief Free a frame structure
 * @ingroup frame
 *
 * @param frame Frame to destroy
 */
void uvc_free_frame(uvc_frame_t *frame) {
	if ((frame->data_bytes > 0) && frame->library_owns_data)
		free(frame->data);

	free(frame);
}

static inline unsigned char sat(int i) {
	return (unsigned char) (i >= 255 ? 255 : (i < 0 ? 0 : i));
}

/** @brief Duplicate a frame, preserving color format
 * @ingroup frame
 *
 * @param in Original frame
 * @param out Duplicate frame
 */
uvc_error_t uvc_duplicate_frame(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(uvc_ensure_frame_size(out, in->data_bytes) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = in->frame_format;
	out->step = in->step;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	memcpy(out->data, in->data, in->data_bytes);	// XXX crash

	return UVC_SUCCESS;
}

#define PIXEL_RGB565		2
#define PIXEL_UYVY			2
#define PIXEL_YUYV			2
#define PIXEL_RGB			3
#define PIXEL_BGR			3
#define PIXEL_RGBX			4

#define PIXEL2_RGB565		PIXEL_RGB565 * 2
#define PIXEL2_UYVY			PIXEL_UYVY * 2
#define PIXEL2_YUYV			PIXEL_YUYV * 2
#define PIXEL2_RGB			PIXEL_RGB * 2
#define PIXEL2_BGR			PIXEL_BGR * 2
#define PIXEL2_RGBX			PIXEL_RGBX * 2

#define PIXEL4_RGB565		PIXEL_RGB565 * 4
#define PIXEL4_UYVY			PIXEL_UYVY * 4
#define PIXEL4_YUYV			PIXEL_YUYV * 4
#define PIXEL4_RGB			PIXEL_RGB * 4
#define PIXEL4_BGR			PIXEL_BGR * 4
#define PIXEL4_RGBX			PIXEL_RGBX * 4

#define PIXEL8_RGB565		PIXEL_RGB565 * 8
#define PIXEL8_UYVY			PIXEL_UYVY * 8
#define PIXEL8_YUYV			PIXEL_YUYV * 8
#define PIXEL8_RGB			PIXEL_RGB * 8
#define PIXEL8_BGR			PIXEL_BGR * 8
#define PIXEL8_RGBX			PIXEL_RGBX * 8

#define PIXEL16_RGB565		PIXEL_RGB565 * 16
#define PIXEL16_UYVY		PIXEL_UYVY * 16
#define PIXEL16_YUYV		PIXEL_YUYV * 16
#define PIXEL16_RGB			PIXEL_RGB * 16
#define PIXEL16_BGR			PIXEL_BGR * 16
#define PIXEL16_RGBX		PIXEL_RGBX * 16

#define RGB2RGBX_2(prgb, prgbx, ax, bx) { \
		(prgbx)[bx+0] = (prgb)[ax+0]; \
		(prgbx)[bx+1] = (prgb)[ax+1]; \
		(prgbx)[bx+2] = (prgb)[ax+2]; \
		(prgbx)[bx+3] = 0xff; \
		(prgbx)[bx+4] = (prgb)[ax+3]; \
		(prgbx)[bx+5] = (prgb)[ax+4]; \
		(prgbx)[bx+6] = (prgb)[ax+5]; \
		(prgbx)[bx+7] = 0xff; \
	}
#define RGB2RGBX_16(prgb, prgbx, ax, bx) \
	RGB2RGBX_8(prgb, prgbx, ax, bx) \
	RGB2RGBX_8(prgb, prgbx, ax + PIXEL8_RGB, bx +PIXEL8_RGBX);
#define RGB2RGBX_8(prgb, prgbx, ax, bx) \
	RGB2RGBX_4(prgb, prgbx, ax, bx) \
	RGB2RGBX_4(prgb, prgbx, ax + PIXEL4_RGB, bx + PIXEL4_RGBX);
#define RGB2RGBX_4(prgb, prgbx, ax, bx) \
	RGB2RGBX_2(prgb, prgbx, ax, bx) \
	RGB2RGBX_2(prgb, prgbx, ax + PIXEL2_RGB, bx + PIXEL2_RGBX);

/** @brief Convert a frame from RGB888 to RGBX8888
 * @ingroup frame
 * @param ini RGB888 frame
 * @param out RGBX8888 frame
 */
uvc_error_t uvc_rgb2rgbx(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_RGB))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGBX) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGBX;
	out->step = in->width * PIXEL_RGBX;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *prgb = in->data;
	const uint8_t *prgb_end = prgb + in->data_bytes - PIXEL8_RGB;
	uint8_t *prgbx = out->data;
	const uint8_t *prgbx_end = prgbx + out->data_bytes - PIXEL8_RGBX;

	// RGB888 to RGBX8888
	int r, g, b;
	while (LIKELY((prgbx <= prgbx_end) && (prgb <= prgb_end))) {
		RGB2RGBX_8(prgb, prgbx, 0, 0);

		prgb += PIXEL8_RGB;
		prgbx += PIXEL8_RGBX;
	}

	return UVC_SUCCESS;
}

// prgb565[0] = ((g << 3) & 0b11100000) | ((b >> 3) & 0b00011111);	// low byte
// prgb565[1] = ((r & 0b11111000) | ((g >> 5) & 0b00000111)); 		// high byte
#define RGB2RGB565_2(prgb, prgb565, ax, bx) { \
		(prgb565)[bx+0] = (((prgb)[ax+1] << 3) & 0b11100000) | (((prgb)[ax+2] >> 3) & 0b00011111); \
		(prgb565)[bx+1] = (((prgb)[ax+0] & 0b11111000) | (((prgb)[ax+1] >> 5) & 0b00000111)); \
		(prgb565)[bx+2] = (((prgb)[ax+4] << 3) & 0b11100000) | (((prgb)[ax+5] >> 3) & 0b00011111); \
		(prgb565)[bx+3] = (((prgb)[ax+3] & 0b11111000) | (((prgb)[ax+4] >> 5) & 0b00000111)); \
    }
#define RGB2RGB565_16(prgb, prgb565, ax, bx) \
	RGB2RGB565_8(prgb, prgb565, ax, bx) \
	RGB2RGB565_8(prgb, prgb565, ax + PIXEL8_RGB, bx + PIXEL8_RGB565);
#define RGB2RGB565_8(prgb, prgb565, ax, bx) \
	RGB2RGB565_4(prgb, prgb565, ax, bx) \
	RGB2RGB565_4(prgb, prgb565, ax + PIXEL4_RGB, bx + PIXEL4_RGB565);
#define RGB2RGB565_4(prgb, prgb565, ax, bx) \
	RGB2RGB565_2(prgb, prgb565, ax, bx) \
	RGB2RGB565_2(prgb, prgb565, ax + PIXEL2_RGB, bx + PIXEL2_RGB565);

/** @brief Convert a frame from RGB888 to RGB565
 * @ingroup frame
 * @param ini RGB888 frame
 * @param out RGB565 frame
 */
uvc_error_t uvc_rgb2rgb565(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_RGB))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGB565) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB565;
	out->step = in->width * PIXEL_RGB565;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *prgb = in->data;
	const uint8_t *prgb_end = prgb + in->data_bytes - PIXEL8_RGB;
	uint8_t *prgb565 = out->data;
	const uint8_t *prgb565_end = prgb565 + out->data_bytes - PIXEL8_RGB565;

	// RGB888 to RGB565
	while (LIKELY((prgb565 <= prgb565_end) && (prgb <= prgb_end))) {
		RGB2RGB565_8(prgb, prgb565, 0, 0);

		prgb += PIXEL8_RGB;
		prgb565 += PIXEL8_RGB565;
	}

	return UVC_SUCCESS;
}
/*
 #define YUYV2RGB_2(pyuv, prgb) { \
    float r = 1.402f * ((pyuv)[3]-128); \
    float g = -0.34414f * ((pyuv)[1]-128) - 0.71414f * ((pyuv)[3]-128); \
    float b = 1.772f * ((pyuv)[1]-128); \
    (prgb)[0] = sat(pyuv[0] + r); \
    (prgb)[1] = sat(pyuv[0] + g); \
    (prgb)[2] = sat(pyuv[0] + b); \
    (prgb)[3] = sat(pyuv[2] + r); \
    (prgb)[4] = sat(pyuv[2] + g); \
    (prgb)[5] = sat(pyuv[2] + b); \
    }
*/

#define IYUYV2RGB_2(pyuv, prgb, ax, bx) { \
		const int d1 = (pyuv)[ax+1]; \
		const int d3 = (pyuv)[ax+3]; \
		const int r = (22987 * (d3/*(pyuv)[ax+3]*/ - 128)) >> 14; \
		const int g = (-5636 * (d1/*(pyuv)[ax+1]*/ - 128) - 11698 * (d3/*(pyuv)[ax+3]*/ - 128)) >> 14; \
		const int b = (29049 * (d1/*(pyuv)[ax+1]*/ - 128)) >> 14; \
		(prgb)[bx+0] = sat((pyuv)[ax+0] + r); \
		(prgb)[bx+1] = sat((pyuv)[ax+0] + g); \
		(prgb)[bx+2] = sat((pyuv)[ax+0] + b); \
		(prgb)[bx+3] = sat((pyuv)[ax+2] + r); \
		(prgb)[bx+4] = sat((pyuv)[ax+2] + g); \
		(prgb)[bx+5] = sat((pyuv)[ax+2] + b); \
    }
#define IYUYV2RGB_16(pyuv, prgb, ax, bx) \
	IYUYV2RGB_8(pyuv, prgb, ax, bx) \
	IYUYV2RGB_8(pyuv, prgb, ax + PIXEL8_YUYV, bx + PIXEL8_RGB)
#define IYUYV2RGB_8(pyuv, prgb, ax, bx) \
	IYUYV2RGB_4(pyuv, prgb, ax, bx) \
	IYUYV2RGB_4(pyuv, prgb, ax + PIXEL4_YUYV, bx + PIXEL4_RGB)
#define IYUYV2RGB_4(pyuv, prgb, ax, bx) \
	IYUYV2RGB_2(pyuv, prgb, ax, bx) \
	IYUYV2RGB_2(pyuv, prgb, ax + PIXEL2_YUYV, bx + PIXEL2_RGB)

/** @brief Convert a frame from YUYV to RGB888
 * @ingroup frame
 *
 * @param in YUYV frame
 * @param out RGB888 frame
 */
uvc_error_t uvc_yuyv2rgb(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_YUYV))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGB) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB;
	out->step = in->width * PIXEL_RGB;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_YUYV;	// XXX
	uint8_t *prgb = out->data;
	const uint8_t *prgb_end = prgb + out->data_bytes - PIXEL8_RGB;	// XXX

	// YUYV => RGB888
	while (LIKELY((prgb <= prgb_end) && (pyuv <= pyuv_end))) {	// XXX
		IYUYV2RGB_8(pyuv, prgb, 0, 0);

		prgb += PIXEL8_RGB;
		pyuv += PIXEL8_YUYV;
	}

	return UVC_SUCCESS;
}

/** @brief Convert a frame from YUYV to RGB565
 * @ingroup frame
 * @param ini YUYV frame
 * @param out RGB565 frame
 */
uvc_error_t uvc_yuyv2rgb565(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_YUYV))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGB565) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB565;
	out->step = in->width * PIXEL_RGB565;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_YUYV;
	uint8_t *prgb565 = out->data;
	const uint8_t *prgb565_end = prgb565 + out->data_bytes - PIXEL8_RGB565;

	uint8_t tmp[PIXEL8_RGB];	// for temporary rgb888 data(8pixel)

	// YUYV => RGB565
	while (LIKELY((prgb565 <= prgb565_end) && (pyuv <= pyuv_end))) {
		IYUYV2RGB_8(pyuv, tmp, 0, 0);
		RGB2RGB565_8(tmp, prgb565, 0, 0);

		prgb565 += PIXEL8_YUYV;
		pyuv += PIXEL8_RGB565;
	}

	return UVC_SUCCESS;
}

#define IYUYV2RGBX_2(pyuv, prgbx, ax, bx) { \
		const int d1 = (pyuv)[ax+1]; \
		const int d3 = (pyuv)[ax+3]; \
		const int r = (22987 * (d3/*(pyuv)[ax+3]*/ - 128)) >> 14; \
		const int g = (-5636 * (d1/*(pyuv)[ax+1]*/ - 128) - 11698 * (d3/*(pyuv)[ax+3]*/ - 128)) >> 14; \
		const int b = (29049 * (d1/*(pyuv)[ax+1]*/ - 128)) >> 14; \
		(prgbx)[bx+0] = sat((pyuv)[ax+0] + r); \
		(prgbx)[bx+1] = sat((pyuv)[ax+0] + g); \
		(prgbx)[bx+2] = sat((pyuv)[ax+0] + b); \
		(prgbx)[bx+3] = 0xff; \
		(prgbx)[bx+4] = sat((pyuv)[ax+2] + r); \
		(prgbx)[bx+5] = sat((pyuv)[ax+2] + g); \
		(prgbx)[bx+6] = sat((pyuv)[ax+2] + b); \
		(prgbx)[bx+7] = 0xff; \
    }
#define IYUYV2RGBX_16(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_8(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_8(pyuv, prgbx, ax + PIXEL8_YUYV, bx + PIXEL8_RGBX);
#define IYUYV2RGBX_8(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_4(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_4(pyuv, prgbx, ax + PIXEL4_YUYV, bx + PIXEL4_RGBX);
#define IYUYV2RGBX_4(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_2(pyuv, prgbx, ax, bx) \
	IYUYV2RGBX_2(pyuv, prgbx, ax + PIXEL2_YUYV, bx + PIXEL2_RGBX);

/** @brief Convert a frame from YUYV to RGBX8888
 * @ingroup frame
 * @param ini YUYV frame
 * @param out RGBX8888 frame
 */
uvc_error_t uvc_yuyv2rgbx(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_YUYV))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGBX) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGBX;
	out->step = in->width * PIXEL_RGBX;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_YUYV;
	uint8_t *prgbx = out->data;
	const uint8_t *prgbx_end = prgbx + out->data_bytes - PIXEL8_RGBX;

	// YUYV => RGBX8888
	while (LIKELY((prgbx <= prgbx_end) && (pyuv <= pyuv_end))) {
		IYUYV2RGBX_8(pyuv, prgbx, 0, 0);

		prgbx += PIXEL8_RGBX;
		pyuv += PIXEL8_YUYV;
	}

	return UVC_SUCCESS;
}

#define IYUYV2BGR_2(pyuv, pbgr, ax, bx) { \
		const int d1 = (pyuv)[1]; \
		const int d3 = (pyuv)[3]; \
	    const int r = (22987 * (d3/*(pyuv)[3]*/ - 128)) >> 14; \
	    const int g = (-5636 * (d1/*(pyuv)[1]*/ - 128) - 11698 * (d3/*(pyuv)[3]*/ - 128)) >> 14; \
	    const int b = (29049 * (d1/*(pyuv)[1]*/ - 128)) >> 14; \
		(pbgr)[bx+0] = sat((pyuv)[ax+0] + b); \
		(pbgr)[bx+1] = sat((pyuv)[ax+0] + g); \
		(pbgr)[bx+2] = sat((pyuv)[ax+0] + r); \
		(pbgr)[bx+3] = sat((pyuv)[ax+2] + b); \
		(pbgr)[bx+4] = sat((pyuv)[ax+2] + g); \
		(pbgr)[bx+5] = sat((pyuv)[ax+2] + r); \
    }
#define IYUYV2BGR_16(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_8(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_8(pyuv, pbgr, ax + PIXEL8_YUYV, bx + PIXEL8_BGR)
#define IYUYV2BGR_8(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_4(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_4(pyuv, pbgr, ax + PIXEL4_YUYV, bx + PIXEL4_BGR)
#define IYUYV2BGR_4(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_2(pyuv, pbgr, ax, bx) \
	IYUYV2BGR_2(pyuv, pbgr, ax + PIXEL2_YUYV, bx + PIXEL2_BGR)

/** @brief Convert a frame from YUYV to BGR888
 * @ingroup frame
 *
 * @param in YUYV frame
 * @param out BGR888 frame
 */
uvc_error_t uvc_yuyv2bgr(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_YUYV))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_BGR) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_BGR;
	out->step = in->width * PIXEL_BGR;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_YUYV;	// XXX
	uint8_t *pbgr = out->data;
	uint8_t *pbgr_end = pbgr + out->data_bytes - PIXEL8_BGR;	// XXX

	// YUYV => BGR888
	while (LIKELY(pbgr <= pbgr_end) && (pyuv <= pyuv_end)) {	// XXX
		IYUYV2BGR_8(pyuv, pbgr, 0, 0);

		pbgr += PIXEL8_BGR;
		pyuv += PIXEL8_YUYV;
	}

	return UVC_SUCCESS;
}

#define IUYVY2RGB_2(pyuv, prgb, ax, bx) { \
		const int d0 = (pyuv)[ax+0]; \
		const int d2 = (pyuv)[ax+2]; \
	    const int r = (22987 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int g = (-5636 * (d0/*(pyuv)[ax+0]*/ - 128) - 11698 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int b = (29049 * (d0/*(pyuv)[ax+0]*/ - 128)) >> 14; \
		(prgb)[bx+0] = sat((pyuv)[ax+1] + r); \
		(prgb)[bx+1] = sat((pyuv)[ax+1] + g); \
		(prgb)[bx+2] = sat((pyuv)[ax+1] + b); \
		(prgb)[bx+3] = sat((pyuv)[ax+3] + r); \
		(prgb)[bx+4] = sat((pyuv)[ax+3] + g); \
		(prgb)[bx+5] = sat((pyuv)[ax+3] + b); \
    }
#define IUYVY2RGB_16(pyuv, prgb, ax, bx) \
	IUYVY2RGB_8(pyuv, prgb, ax, bx) \
	IUYVY2RGB_8(pyuv, prgb, ax + 16, bx + 24)
#define IUYVY2RGB_8(pyuv, prgb, ax, bx) \
	IUYVY2RGB_4(pyuv, prgb, ax, bx) \
	IUYVY2RGB_4(pyuv, prgb, ax + 8, bx + 12)
#define IUYVY2RGB_4(pyuv, prgb, ax, bx) \
	IUYVY2RGB_2(pyuv, prgb, ax, bx) \
	IUYVY2RGB_2(pyuv, prgb, ax + 4, bx + 6)

/** @brief Convert a frame from UYVY to RGB888
 * @ingroup frame
 * @param ini UYVY frame
 * @param out RGB888 frame
 */
uvc_error_t uvc_uyvy2rgb(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_UYVY))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGB) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB;
	out->step = in->width * PIXEL_RGB;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_UYVY;	// XXX
	uint8_t *prgb = out->data;
	const uint8_t *prgb_end = prgb + out->data_bytes - PIXEL8_RGB;	// XXX

	// UYVY => RGB888
	while (LIKELY((prgb <= prgb_end) && (pyuv <= pyuv_end))) {	// XXX
		IUYVY2RGB_8(pyuv, prgb, 0, 0);

		prgb += PIXEL8_RGB;
		pyuv += PIXEL8_UYVY;
	}

	return UVC_SUCCESS;
}

/** @brief Convert a frame from UYVY to RGB565
 * @ingroup frame
 * @param ini UYVY frame
 * @param out RGB565 frame
 */
uvc_error_t uvc_uyvy2rgb565(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_UYVY))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGB565) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGB565;
	out->step = in->width * PIXEL_RGB565;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_UYVY;
	uint8_t *prgb565 = out->data;
	const uint8_t *prgb565_end = prgb565 + out->data_bytes - PIXEL8_RGB565;

	uint8_t tmp[PIXEL8_RGB];		// for temporary rgb888 data(8pixel)

	// UYVY => RGB565
	while (LIKELY((prgb565 <= prgb565_end) && (pyuv <= pyuv_end))) {
		IUYVY2RGB_8(pyuv, tmp, 0, 0);
		RGB2RGB565_8(tmp, prgb565, 0, 0);

		prgb565 += PIXEL8_RGB565;
		pyuv += PIXEL8_UYVY;
	}

	return UVC_SUCCESS;
}

#define IUYVY2RGBX_2(pyuv, prgbx, ax, bx) { \
		const int d0 = (pyuv)[ax+0]; \
		const int d2 = (pyuv)[ax+2]; \
	    const int r = (22987 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int g = (-5636 * (d0/*(pyuv)[ax+0]*/ - 128) - 11698 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int b = (29049 * (d0/*(pyuv)[ax+0]*/ - 128)) >> 14; \
		(prgbx)[bx+0] = sat((pyuv)[ax+1] + r); \
		(prgbx)[bx+1] = sat((pyuv)[ax+1] + g); \
		(prgbx)[bx+2] = sat((pyuv)[ax+1] + b); \
		(prgbx)[bx+3] = 0xff; \
		(prgbx)[bx+4] = sat((pyuv)[ax+3] + r); \
		(prgbx)[bx+5] = sat((pyuv)[ax+3] + g); \
		(prgbx)[bx+6] = sat((pyuv)[ax+3] + b); \
		(prgbx)[bx+7] = 0xff; \
    }
#define IUYVY2RGBX_16(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_8(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_8(pyuv, prgbx, ax + PIXEL8_UYVY, bx + PIXEL8_RGBX)
#define IUYVY2RGBX_8(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_4(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_4(pyuv, prgbx, ax + PIXEL4_UYVY, bx + PIXEL4_RGBX)
#define IUYVY2RGBX_4(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_2(pyuv, prgbx, ax, bx) \
	IUYVY2RGBX_2(pyuv, prgbx, ax + PIXEL2_UYVY, bx + PIXEL2_RGBX)

/** @brief Convert a frame from UYVY to RGBX8888
 * @ingroup frame
 * @param ini UYVY frame
 * @param out RGBX8888 frame
 */
uvc_error_t uvc_uyvy2rgbx(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_UYVY))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_RGBX) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_RGBX;
	out->step = in->width * PIXEL_RGBX;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_UYVY;
	uint8_t *prgbx = out->data;
	const uint8_t *prgbx_end = prgbx + out->data_bytes - PIXEL8_RGBX;

	// UYVY => RGBX8888
	while (LIKELY((prgbx <= prgbx_end) && (pyuv <= pyuv_end))) {
		IUYVY2RGBX_8(pyuv, prgbx, 0, 0);

		prgbx += PIXEL8_RGBX;
		pyuv += PIXEL8_UYVY;
	}

	return UVC_SUCCESS;
}

#define IUYVY2BGR_2(pyuv, pbgr, ax, bx) { \
		const int d0 = (pyuv)[ax+0]; \
		const int d2 = (pyuv)[ax+2]; \
	    const int r = (22987 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int g = (-5636 * (d0/*(pyuv)[ax+0]*/ - 128) - 11698 * (d2/*(pyuv)[ax+2]*/ - 128)) >> 14; \
	    const int b = (29049 * (d0/*(pyuv)[ax+0]*/ - 128)) >> 14; \
		(pbgr)[bx+0] = sat((pyuv)[ax+1] + b); \
		(pbgr)[bx+1] = sat((pyuv)[ax+1] + g); \
		(pbgr)[bx+2] = sat((pyuv)[ax+1] + r); \
		(pbgr)[bx+3] = sat((pyuv)[ax+3] + b); \
		(pbgr)[bx+4] = sat((pyuv)[ax+3] + g); \
		(pbgr)[bx+5] = sat((pyuv)[ax+3] + r); \
    }
#define IUYVY2BGR_16(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_8(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_8(pyuv, pbgr, ax + PIXEL8_UYVY, bx + PIXEL8_BGR)
#define IUYVY2BGR_8(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_4(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_4(pyuv, pbgr, ax + PIXEL4_UYVY, bx + PIXEL4_BGR)
#define IUYVY2BGR_4(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_2(pyuv, pbgr, ax, bx) \
	IUYVY2BGR_2(pyuv, pbgr, ax + PIXEL2_UYVY, bx + PIXEL2_BGR)

/** @brief Convert a frame from UYVY to BGR888
 * @ingroup frame
 * @param ini UYVY frame
 * @param out BGR888 frame
 */
uvc_error_t uvc_uyvy2bgr(uvc_frame_t *in, uvc_frame_t *out) {
	if (UNLIKELY(in->frame_format != UVC_FRAME_FORMAT_UYVY))
		return UVC_ERROR_INVALID_PARAM;

	if (UNLIKELY(uvc_ensure_frame_size(out, in->width * in->height * PIXEL_BGR) < 0))
		return UVC_ERROR_NO_MEM;

	out->width = in->width;
	out->height = in->height;
	out->frame_format = UVC_FRAME_FORMAT_BGR;
	out->step = in->width * PIXEL_BGR;
	out->sequence = in->sequence;
	out->capture_time = in->capture_time;
	out->source = in->source;

	uint8_t *pyuv = in->data;
	const uint8_t *pyuv_end = pyuv + in->data_bytes - PIXEL8_UYVY;	// XXX
	uint8_t *pbgr = out->data;
	const uint8_t *pbgr_end = pbgr + out->data_bytes - PIXEL8_BGR;	// XXX

	// UYVY => BGR888
	while (LIKELY((pbgr <= pbgr_end) && (pyuv <= pyuv_end))) {	// XXX
		IUYVY2BGR_8(pyuv, pbgr, 0, 0);

		pbgr += PIXEL8_BGR;
		pyuv += PIXEL8_UYVY;
	}

	return UVC_SUCCESS;
}

/** @brief Convert a frame to RGB565
 * @ingroup frame
 *
 * @param in non-RGB565 frame
 * @param out RGB565 frame
 */
uvc_error_t uvc_any2rgb565(uvc_frame_t *in, uvc_frame_t *out) {

	switch (in->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		return uvc_yuyv2rgb565(in, out);
	case UVC_FRAME_FORMAT_UYVY:
		return uvc_uyvy2rgb565(in, out);
	case UVC_FRAME_FORMAT_RGB565:
		return uvc_duplicate_frame(in, out);
	case UVC_FRAME_FORMAT_RGB:
		return uvc_rgb2rgb565(in, out);
	default:
		return UVC_ERROR_NOT_SUPPORTED;
	}
}

/** @brief Convert a frame to RGB888
 * @ingroup frame
 *
 * @param in non-RGB888 frame
 * @param out RGB888 frame
 */
uvc_error_t uvc_any2rgb(uvc_frame_t *in, uvc_frame_t *out) {

	switch (in->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		return uvc_yuyv2rgb(in, out);
	case UVC_FRAME_FORMAT_UYVY:
		return uvc_uyvy2rgb(in, out);
	case UVC_FRAME_FORMAT_RGB:
		return uvc_duplicate_frame(in, out);
	default:
		return UVC_ERROR_NOT_SUPPORTED;
	}
}

/** @brief Convert a frame to BGR888
 * @ingroup frame
 *
 * @param in non-BGR888 frame
 * @param out BGR888 frame
 */
uvc_error_t uvc_any2bgr(uvc_frame_t *in, uvc_frame_t *out) {

	switch (in->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		return uvc_yuyv2bgr(in, out);
	case UVC_FRAME_FORMAT_UYVY:
		return uvc_uyvy2bgr(in, out);
	case UVC_FRAME_FORMAT_BGR:
		return uvc_duplicate_frame(in, out);
	default:
		return UVC_ERROR_NOT_SUPPORTED;
	}
}

/** @brief Convert a frame to RGBX8888
 * @ingroup frame
 *
 * @param in non-RGB565 frame
 * @param out RGBX8888 frame
 */
uvc_error_t uvc_any2rgbx(uvc_frame_t *in, uvc_frame_t *out) {

	switch (in->frame_format) {
	case UVC_FRAME_FORMAT_YUYV:
		return uvc_yuyv2rgbx(in, out);
	case UVC_FRAME_FORMAT_UYVY:
		return uvc_uyvy2rgbx(in, out);
	case UVC_FRAME_FORMAT_RGBX:
		return uvc_duplicate_frame(in, out);
	case UVC_FRAME_FORMAT_RGB:
		return uvc_rgb2rgbx(in, out);
	default:
		return UVC_ERROR_NOT_SUPPORTED;
	}
}
