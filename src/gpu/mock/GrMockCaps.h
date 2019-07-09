/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GrMockCaps_DEFINED
#define GrMockCaps_DEFINED

#include "include/gpu/mock/GrMockTypes.h"
#include "src/gpu/GrCaps.h"
#include "src/gpu/SkGr.h"

class GrMockCaps : public GrCaps {
public:
    GrMockCaps(const GrContextOptions& contextOptions, const GrMockOptions& options)
            : INHERITED(contextOptions), fOptions(options) {
        fInstanceAttribSupport = options.fInstanceAttribSupport;
        fHalfFloatVertexAttributeSupport = options.fHalfFloatVertexAttributeSupport;
        fMapBufferFlags = options.fMapBufferFlags;
        fBufferMapThreshold = SK_MaxS32; // Overridable in GrContextOptions.
        fMaxTextureSize = options.fMaxTextureSize;
        fMaxRenderTargetSize = SkTMin(options.fMaxRenderTargetSize, fMaxTextureSize);
        fMaxPreferredRenderTargetSize = fMaxRenderTargetSize;
        fMaxVertexAttributes = options.fMaxVertexAttributes;

        fShaderCaps.reset(new GrShaderCaps(contextOptions));
        fShaderCaps->fGeometryShaderSupport = options.fGeometryShaderSupport;
        fShaderCaps->fIntegerSupport = options.fIntegerSupport;
        fShaderCaps->fFlatInterpolationSupport = options.fFlatInterpolationSupport;
        fShaderCaps->fMaxFragmentSamplers = options.fMaxFragmentSamplers;
        fShaderCaps->fShaderDerivativeSupport = options.fShaderDerivativeSupport;
        fShaderCaps->fDualSourceBlendingSupport = options.fDualSourceBlendingSupport;

        this->applyOptionsOverrides(contextOptions);
    }

    bool isFormatSRGB(const GrBackendFormat& format) const override {
        if (!format.getMockSRGBEncoded()) {
            return false;
        }

        return GrSRGBEncoded::kYes == *format.getMockSRGBEncoded();
    }

    bool isFormatTexturable(GrColorType, const GrBackendFormat& format) const override {
        if (!format.getMockColorType()) {
            return false;
        }

        // We are ignoring the srgb encoding here
        return fOptions.fConfigOptions[(int)*format.getMockColorType()].fTexturable;
    }

    bool isConfigTexturable(GrPixelConfig config) const override {
        GrColorType ct = GrPixelConfigToColorType(config);

        // We are ignoring the srgb encoding here
        return fOptions.fConfigOptions[(int)ct].fTexturable;
    }

    bool isFormatCopyable(SkColorType, const GrBackendFormat& format) const override {
        return false;
    }

    bool isConfigCopyable(GrPixelConfig config) const override {
        return false;
    }

    int getRenderTargetSampleCount(int requestCount, GrColorType ct) const {
        requestCount = SkTMax(requestCount, 1);

        switch (fOptions.fConfigOptions[(int)ct].fRenderability) {
            case GrMockOptions::ConfigOptions::Renderability::kNo:
                return 0;
            case GrMockOptions::ConfigOptions::Renderability::kNonMSAA:
                return requestCount > 1 ? 0 : 1;
            case GrMockOptions::ConfigOptions::Renderability::kMSAA:
                return requestCount > kMaxSampleCnt ? 0 : GrNextPow2(requestCount);
        }
        return 0;
    }

    int getRenderTargetSampleCount(int requestCount,
                                   SkColorType, const GrBackendFormat& format) const override {
        if (!format.getMockColorType()) {
            return 0;
        }

        // We are ignoring the srgb encoding here
        return this->getRenderTargetSampleCount(requestCount, *format.getMockColorType());
    }

    int getRenderTargetSampleCount(int requestCount, GrPixelConfig config) const override {
        GrColorType ct = GrPixelConfigToColorType(config);

        // We are ignoring the srgb encoding here
        return this->getRenderTargetSampleCount(requestCount, ct);
    }

    int maxRenderTargetSampleCount(GrColorType ct) const {
        switch (fOptions.fConfigOptions[(int)ct].fRenderability) {
            case GrMockOptions::ConfigOptions::Renderability::kNo:
                return 0;
            case GrMockOptions::ConfigOptions::Renderability::kNonMSAA:
                return 1;
            case GrMockOptions::ConfigOptions::Renderability::kMSAA:
                return kMaxSampleCnt;
        }
        return 0;
    }

    int maxRenderTargetSampleCount(SkColorType, const GrBackendFormat& format) const override {
        if (!format.getMockColorType()) {
            return 0;
        }

        // We are ignoring the srgb encoding here
        return this->maxRenderTargetSampleCount(*format.getMockColorType());
    }

    int maxRenderTargetSampleCount(GrPixelConfig config) const override {
        GrColorType ct = GrPixelConfigToColorType(config);

        // We are ignoring the srgb encoding here
        return this->maxRenderTargetSampleCount(ct);
    }

    SurfaceReadPixelsSupport surfaceSupportsReadPixels(const GrSurface*) const override {
        return SurfaceReadPixelsSupport::kSupported;
    }

    bool initDescForDstCopy(const GrRenderTargetProxy* src, GrSurfaceDesc* desc,
                            bool* rectsMustMatch, bool* disallowSubrect) const override {
        return false;
    }

    GrPixelConfig validateBackendRenderTarget(const GrBackendRenderTarget&,
                                              GrColorType) const override {
        return kUnknown_GrPixelConfig;
    }

    GrPixelConfig getYUVAConfigFromBackendFormat(const GrBackendFormat& format) const override {
        if (!format.getMockColorType() || !format.getMockSRGBEncoded()) {
            return kUnknown_GrPixelConfig;
        }

        return GrColorTypeToPixelConfig(*format.getMockColorType(),
                                        *format.getMockSRGBEncoded());
    }

    GrBackendFormat getBackendFormatFromColorType(GrColorType ct,
                                                  GrSRGBEncoded srgbEncoded) const override {
        return GrBackendFormat::MakeMock(ct, srgbEncoded);
    }

    GrBackendFormat getBackendFormatFromCompressionType(SkImage::CompressionType) const override {
        return {};
    }

    GrSwizzle getTextureSwizzle(const GrBackendFormat&, GrColorType) const override {
        return GrSwizzle();
    }
    GrSwizzle getOutputSwizzle(const GrBackendFormat&, GrColorType) const override {
        return GrSwizzle();
    }

private:
    bool onSurfaceSupportsWritePixels(const GrSurface*) const override { return true; }
    bool onCanCopySurface(const GrSurfaceProxy* dst, const GrSurfaceProxy* src,
                          const SkIRect& srcRect, const SkIPoint& dstPoint) const override {
        return true;
    }
    size_t onTransferFromOffsetAlignment(GrColorType bufferColorType) const override {
        // arbitrary
        return GrSizeAlignUp(GrColorTypeBytesPerPixel(bufferColorType), 4);
    }

    GrPixelConfig onGetConfigFromBackendFormat(const GrBackendFormat& format,
                                               GrColorType) const override {
        if (!format.getMockColorType() || !format.getMockSRGBEncoded()) {
            return kUnknown_GrPixelConfig;
        }

        return GrColorTypeToPixelConfig(*format.getMockColorType(),
                                        *format.getMockSRGBEncoded());

    }

    bool onAreColorTypeAndFormatCompatible(GrColorType ct,
                                           const GrBackendFormat& format) const override {
        if (GrColorType::kUnknown == ct) {
            return false;
        }

        const GrColorType* mockColorType = format.getMockColorType();
        if (!mockColorType) {
            return false;
        }

        return ct == *mockColorType;
    }

    static const int kMaxSampleCnt = 16;

    GrMockOptions fOptions;
    typedef GrCaps INHERITED;
};

#endif
