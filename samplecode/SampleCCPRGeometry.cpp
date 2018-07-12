/*
 * Copyright 2017 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "SkTypes.h"

#if SK_SUPPORT_GPU

#include "GrContextPriv.h"
#include "GrPathUtils.h"
#include "GrRenderTargetContext.h"
#include "GrRenderTargetContextPriv.h"
#include "GrResourceProvider.h"
#include "SampleCode.h"
#include "SkCanvas.h"
#include "SkMakeUnique.h"
#include "SkPaint.h"
#include "SkPath.h"
#include "SkRectPriv.h"
#include "SkView.h"
#include "ccpr/GrCCCoverageProcessor.h"
#include "ccpr/GrCCGeometry.h"
#include "gl/GrGLGpu.cpp"
#include "glsl/GrGLSLFragmentProcessor.h"
#include "ops/GrDrawOp.h"

using TriPointInstance = GrCCCoverageProcessor::TriPointInstance;
using QuadPointInstance = GrCCCoverageProcessor::QuadPointInstance;
using PrimitiveType = GrCCCoverageProcessor::PrimitiveType;

static constexpr float kDebugBloat = 40;

/**
 * This sample visualizes the AA bloat geometry generated by the ccpr geometry shaders. It
 * increases the AA bloat by 50x and outputs color instead of coverage (coverage=+1 -> green,
 * coverage=0 -> black, coverage=-1 -> red). Use the keys 1-7 to cycle through the different
 * geometry processors.
 */
class CCPRGeometryView : public SampleView {
public:
    CCPRGeometryView() { this->updateGpuData(); }
    void onDrawContent(SkCanvas*) override;

    SkView::Click* onFindClickHandler(SkScalar x, SkScalar y, unsigned) override;
    bool onClick(SampleView::Click*) override;
    bool onQuery(SkEvent* evt) override;

private:
    class Click;
    class DrawCoverageCountOp;
    class VisualizeCoverageCountFP;

    void updateAndInval() { this->updateGpuData(); }

    void updateGpuData();

    PrimitiveType fPrimitiveType = PrimitiveType::kTriangles;
    SkCubicType fCubicType;
    SkMatrix fCubicKLM;

    SkPoint fPoints[4] = {
            {100.05f, 100.05f}, {400.75f, 100.05f}, {400.75f, 300.95f}, {100.05f, 300.95f}};

    float fConicWeight = .5;

    SkTArray<TriPointInstance> fTriPointInstances;
    SkTArray<QuadPointInstance> fQuadPointInstances;

    typedef SampleView INHERITED;
};

class CCPRGeometryView::DrawCoverageCountOp : public GrDrawOp {
    DEFINE_OP_CLASS_ID

public:
    DrawCoverageCountOp(CCPRGeometryView* view) : INHERITED(ClassID()), fView(view) {
        this->setBounds(SkRectPriv::MakeLargest(), GrOp::HasAABloat::kNo, GrOp::IsZeroArea::kNo);
    }

    const char* name() const override {
        return "[Testing/Sample code] CCPRGeometryView::DrawCoverageCountOp";
    }

private:
    FixedFunctionFlags fixedFunctionFlags() const override { return FixedFunctionFlags::kNone; }
    RequiresDstTexture finalize(const GrCaps&, const GrAppliedClip*) override {
        return RequiresDstTexture::kNo;
    }
    bool onCombineIfPossible(GrOp* other, const GrCaps& caps) override { return false; }
    void onPrepare(GrOpFlushState*) override {}
    void onExecute(GrOpFlushState*) override;

    CCPRGeometryView* fView;

    typedef GrDrawOp INHERITED;
};

class CCPRGeometryView::VisualizeCoverageCountFP : public GrFragmentProcessor {
public:
    VisualizeCoverageCountFP() : GrFragmentProcessor(kTestFP_ClassID, kNone_OptimizationFlags) {}

private:
    const char* name() const override {
        return "[Testing/Sample code] CCPRGeometryView::VisualizeCoverageCountFP";
    }
    std::unique_ptr<GrFragmentProcessor> clone() const override {
        return skstd::make_unique<VisualizeCoverageCountFP>();
    }
    void onGetGLSLProcessorKey(const GrShaderCaps&, GrProcessorKeyBuilder*) const override {}
    bool onIsEqual(const GrFragmentProcessor&) const override { return true; }

    class Impl : public GrGLSLFragmentProcessor {
        void emitCode(EmitArgs& args) override {
            GrGLSLFPFragmentBuilder* f = args.fFragBuilder;
            f->codeAppendf("half count = %s.a;", args.fInputColor);
            f->codeAppendf("%s = half4(clamp(-count, 0, 1), clamp(+count, 0, 1), 0, abs(count));",
                           args.fOutputColor);
        }
    };

    GrGLSLFragmentProcessor* onCreateGLSLInstance() const override { return new Impl; }
};

static void draw_klm_line(int w, int h, SkCanvas* canvas, const SkScalar line[3], SkColor color) {
    SkPoint p1, p2;
    if (SkScalarAbs(line[1]) > SkScalarAbs(line[0])) {
        // Draw from vertical edge to vertical edge.
        p1 = {0, -line[2] / line[1]};
        p2 = {(SkScalar)w, (-line[2] - w * line[0]) / line[1]};
    } else {
        // Draw from horizontal edge to horizontal edge.
        p1 = {-line[2] / line[0], 0};
        p2 = {(-line[2] - h * line[1]) / line[0], (SkScalar)h};
    }

    SkPaint linePaint;
    linePaint.setColor(color);
    linePaint.setAlpha(128);
    linePaint.setStyle(SkPaint::kStroke_Style);
    linePaint.setStrokeWidth(0);
    linePaint.setAntiAlias(true);
    canvas->drawLine(p1, p2, linePaint);
}

void CCPRGeometryView::onDrawContent(SkCanvas* canvas) {
    canvas->clear(SK_ColorBLACK);

    SkPath outline;
    outline.moveTo(fPoints[0]);
    switch (fPrimitiveType) {
        case PrimitiveType::kTriangles:
        case PrimitiveType::kWeightedTriangles:
            outline.lineTo(fPoints[1]);
            outline.lineTo(fPoints[3]);
            outline.close();
            break;
        case PrimitiveType::kQuadratics:
            outline.quadTo(fPoints[1], fPoints[3]);
            break;
        case PrimitiveType::kCubics:
            outline.cubicTo(fPoints[1], fPoints[2], fPoints[3]);
            break;
        case PrimitiveType::kConics:
            outline.conicTo(fPoints[1], fPoints[3], fConicWeight);
            break;
    }

    SkPaint outlinePaint;
    outlinePaint.setColor(0x80ffffff);
    outlinePaint.setStyle(SkPaint::kStroke_Style);
    outlinePaint.setStrokeWidth(0);
    outlinePaint.setAntiAlias(true);
    canvas->drawPath(outline, outlinePaint);

#if 0
    SkPaint gridPaint;
    gridPaint.setColor(0x10000000);
    gridPaint.setStyle(SkPaint::kStroke_Style);
    gridPaint.setStrokeWidth(0);
    gridPaint.setAntiAlias(true);
    for (int y = 0; y < this->height(); y += kDebugBloat) {
        canvas->drawLine(0, y, this->width(), y, gridPaint);
    }
    for (int x = 0; x < this->width(); x += kDebugBloat) {
        canvas->drawLine(x, 0, x, this->height(), outlinePaint);
    }
#endif

    SkString caption;
    if (GrRenderTargetContext* rtc = canvas->internal_private_accessTopLayerRenderTargetContext()) {
        // Render coverage count.
        GrContext* ctx = canvas->getGrContext();
        SkASSERT(ctx);
        sk_sp<GrRenderTargetContext> ccbuff =
                ctx->contextPriv().makeDeferredRenderTargetContext(SkBackingFit::kApprox,
                                                                   this->width(), this->height(),
                                                                   kAlpha_half_GrPixelConfig,
                                                                   nullptr);
        SkASSERT(ccbuff);
        ccbuff->clear(nullptr, 0, GrRenderTargetContext::CanClearFullscreen::kYes);
        ccbuff->priv().testingOnly_addDrawOp(skstd::make_unique<DrawCoverageCountOp>(this));

        // Visualize coverage count in main canvas.
        GrPaint paint;
        paint.addColorFragmentProcessor(
                GrSimpleTextureEffect::Make(sk_ref_sp(ccbuff->asTextureProxy()), SkMatrix::I()));
        paint.addColorFragmentProcessor(
                skstd::make_unique<VisualizeCoverageCountFP>());
        paint.setPorterDuffXPFactory(SkBlendMode::kSrcOver);
        rtc->drawRect(GrNoClip(), std::move(paint), GrAA::kNo, SkMatrix::I(),
                      SkRect::MakeIWH(this->width(), this->height()));

        // Add label.
        caption.appendf("PrimitiveType_%s",
                        GrCCCoverageProcessor::PrimitiveTypeName(fPrimitiveType));
        if (PrimitiveType::kCubics == fPrimitiveType) {
            caption.appendf(" (%s)", SkCubicTypeName(fCubicType));
        } else if (PrimitiveType::kConics == fPrimitiveType) {
            caption.appendf(" (w=%f)", fConicWeight);
        }
    } else {
        caption = "Use GPU backend to visualize geometry.";
    }

    SkPaint pointsPaint;
    pointsPaint.setColor(SK_ColorBLUE);
    pointsPaint.setStrokeWidth(8);
    pointsPaint.setAntiAlias(true);

    if (PrimitiveType::kCubics == fPrimitiveType) {
        int w = this->width(), h = this->height();
        canvas->drawPoints(SkCanvas::kPoints_PointMode, 4, fPoints, pointsPaint);
        draw_klm_line(w, h, canvas, &fCubicKLM[0], SK_ColorYELLOW);
        draw_klm_line(w, h, canvas, &fCubicKLM[3], SK_ColorBLUE);
        draw_klm_line(w, h, canvas, &fCubicKLM[6], SK_ColorRED);
    } else {
        canvas->drawPoints(SkCanvas::kPoints_PointMode, 2, fPoints, pointsPaint);
        canvas->drawPoints(SkCanvas::kPoints_PointMode, 1, fPoints + 3, pointsPaint);
    }

    SkPaint captionPaint;
    captionPaint.setTextSize(20);
    captionPaint.setColor(SK_ColorWHITE);
    captionPaint.setAntiAlias(true);
    canvas->drawText(caption.c_str(), caption.size(), 10, 30, captionPaint);
}

void CCPRGeometryView::updateGpuData() {
    fTriPointInstances.reset();
    fQuadPointInstances.reset();

    if (PrimitiveType::kCubics == fPrimitiveType) {
        double t[2], s[2];
        fCubicType = GrPathUtils::getCubicKLM(fPoints, &fCubicKLM, t, s);
        GrCCGeometry geometry;
        geometry.beginContour(fPoints[0]);
        geometry.cubicTo(fPoints, kDebugBloat / 2, kDebugBloat / 2);
        geometry.endContour();
        int ptsIdx = 0;
        for (GrCCGeometry::Verb verb : geometry.verbs()) {
            switch (verb) {
                case GrCCGeometry::Verb::kLineTo:
                    ++ptsIdx;
                    continue;
                case GrCCGeometry::Verb::kMonotonicQuadraticTo:
                    ptsIdx += 2;
                    continue;
                case GrCCGeometry::Verb::kMonotonicCubicTo:
                    fQuadPointInstances.push_back().set(&geometry.points()[ptsIdx], 0, 0);
                    ptsIdx += 3;
                    continue;
                default:
                    continue;
            }
        }
    } else if (PrimitiveType::kTriangles != fPrimitiveType) {
        SkPoint P3[3] = {fPoints[0], fPoints[1], fPoints[3]};
        GrCCGeometry geometry;
        geometry.beginContour(P3[0]);
        if (PrimitiveType::kQuadratics == fPrimitiveType) {
            geometry.quadraticTo(P3);
        } else {
            SkASSERT(PrimitiveType::kConics == fPrimitiveType);
            geometry.conicTo(P3, fConicWeight);
        }
        geometry.endContour();
        int ptsIdx = 0, conicWeightIdx = 0;
        for (GrCCGeometry::Verb verb : geometry.verbs()) {
            if (GrCCGeometry::Verb::kBeginContour == verb ||
                GrCCGeometry::Verb::kEndOpenContour == verb ||
                GrCCGeometry::Verb::kEndClosedContour == verb) {
                continue;
            }
            if (GrCCGeometry::Verb::kLineTo == verb) {
                ++ptsIdx;
                continue;
            }
            SkASSERT(GrCCGeometry::Verb::kMonotonicQuadraticTo == verb ||
                     GrCCGeometry::Verb::kMonotonicConicTo == verb);
            if (PrimitiveType::kQuadratics == fPrimitiveType &&
                GrCCGeometry::Verb::kMonotonicQuadraticTo == verb) {
                fTriPointInstances.push_back().set(&geometry.points()[ptsIdx], Sk2f(0, 0));
            } else if (PrimitiveType::kConics == fPrimitiveType &&
                       GrCCGeometry::Verb::kMonotonicConicTo == verb) {
                fQuadPointInstances.push_back().setW(&geometry.points()[ptsIdx], Sk2f(0, 0),
                                                     geometry.getConicWeight(conicWeightIdx++));
            }
            ptsIdx += 2;
        }
    } else {
        fTriPointInstances.push_back().set(fPoints[0], fPoints[1], fPoints[3], Sk2f(0, 0));
    }
}

void CCPRGeometryView::DrawCoverageCountOp::onExecute(GrOpFlushState* state) {
    GrResourceProvider* rp = state->resourceProvider();
    GrContext* context = state->gpu()->getContext();
    GrGLGpu* glGpu = kOpenGL_GrBackend == context->contextPriv().getBackend()
                             ? static_cast<GrGLGpu*>(state->gpu())
                             : nullptr;

    GrCCCoverageProcessor proc(rp, fView->fPrimitiveType);
    SkDEBUGCODE(proc.enableDebugBloat(kDebugBloat));

    SkSTArray<1, GrMesh> mesh;
    if (PrimitiveType::kCubics == fView->fPrimitiveType ||
        PrimitiveType::kConics == fView->fPrimitiveType) {
        sk_sp<GrBuffer> instBuff(rp->createBuffer(
                fView->fQuadPointInstances.count() * sizeof(QuadPointInstance),
                kVertex_GrBufferType, kDynamic_GrAccessPattern,
                GrResourceProvider::kNoPendingIO_Flag | GrResourceProvider::kRequireGpuMemory_Flag,
                fView->fQuadPointInstances.begin()));
        if (!fView->fQuadPointInstances.empty() && instBuff) {
            proc.appendMesh(instBuff.get(), fView->fQuadPointInstances.count(), 0, &mesh);
        }
    } else {
        sk_sp<GrBuffer> instBuff(rp->createBuffer(
                fView->fTriPointInstances.count() * sizeof(TriPointInstance), kVertex_GrBufferType,
                kDynamic_GrAccessPattern,
                GrResourceProvider::kNoPendingIO_Flag | GrResourceProvider::kRequireGpuMemory_Flag,
                fView->fTriPointInstances.begin()));
        if (!fView->fTriPointInstances.empty() && instBuff) {
            proc.appendMesh(instBuff.get(), fView->fTriPointInstances.count(), 0, &mesh);
        }
    }

    GrPipeline pipeline(state->drawOpArgs().fProxy, GrPipeline::ScissorState::kDisabled,
                        SkBlendMode::kPlus);

    if (glGpu) {
        glGpu->handleDirtyContext();
        // GR_GL_CALL(glGpu->glInterface(), PolygonMode(GR_GL_FRONT_AND_BACK, GR_GL_LINE));
        GR_GL_CALL(glGpu->glInterface(), Enable(GR_GL_LINE_SMOOTH));
    }

    if (!mesh.empty()) {
        SkASSERT(1 == mesh.count());
        proc.draw(state, pipeline, nullptr, mesh.begin(), 1, this->bounds());
    }

    if (glGpu) {
        context->resetContext(kMisc_GrGLBackendState);
    }
}

class CCPRGeometryView::Click : public SampleView::Click {
public:
    Click(SkView* target, int ptIdx) : SampleView::Click(target), fPtIdx(ptIdx) {}

    void doClick(SkPoint points[]) {
        if (fPtIdx >= 0) {
            this->dragPoint(points, fPtIdx);
        } else {
            for (int i = 0; i < 4; ++i) {
                this->dragPoint(points, i);
            }
        }
    }

private:
    void dragPoint(SkPoint points[], int idx) {
        SkIPoint delta = fICurr - fIPrev;
        points[idx] += SkPoint::Make(delta.x(), delta.y());
    }

    int fPtIdx;
};

SkView::Click* CCPRGeometryView::onFindClickHandler(SkScalar x, SkScalar y, unsigned) {
    for (int i = 0; i < 4; ++i) {
        if (PrimitiveType::kCubics != fPrimitiveType && 2 == i) {
            continue;
        }
        if (fabs(x - fPoints[i].x()) < 20 && fabsf(y - fPoints[i].y()) < 20) {
            return new Click(this, i);
        }
    }
    return new Click(this, -1);
}

bool CCPRGeometryView::onClick(SampleView::Click* click) {
    Click* myClick = (Click*)click;
    myClick->doClick(fPoints);
    this->updateAndInval();
    return true;
}

bool CCPRGeometryView::onQuery(SkEvent* evt) {
    if (SampleCode::TitleQ(*evt)) {
        SampleCode::TitleR(evt, "CCPRGeometry");
        return true;
    }
    SkUnichar unichar;
    if (SampleCode::CharQ(*evt, &unichar)) {
        if (unichar >= '1' && unichar <= '4') {
            fPrimitiveType = PrimitiveType(unichar - '1');
            if (fPrimitiveType >= PrimitiveType::kWeightedTriangles) {
                fPrimitiveType = (PrimitiveType) ((int)fPrimitiveType + 1);
            }
            this->updateAndInval();
            return true;
        }
        if (PrimitiveType::kConics == fPrimitiveType) {
            if (unichar == '+') {
                fConicWeight *= 2;
                this->updateAndInval();
                return true;
            }
            if (unichar == '+' || unichar == '=') {
                fConicWeight *= 5/4.f;
                this->updateAndInval();
                return true;
            }
            if (unichar == '-') {
                fConicWeight *= 4/5.f;
                this->updateAndInval();
                return true;
            }
            if (unichar == '_') {
                fConicWeight *= .5f;
                this->updateAndInval();
                return true;
            }
        }
        if (unichar == 'D') {
            SkDebugf("    SkPoint fPoints[4] = {\n");
            SkDebugf("        {%ff, %ff},\n", fPoints[0].x(), fPoints[0].y());
            SkDebugf("        {%ff, %ff},\n", fPoints[1].x(), fPoints[1].y());
            SkDebugf("        {%ff, %ff},\n", fPoints[2].x(), fPoints[2].y());
            SkDebugf("        {%ff, %ff}\n", fPoints[3].x(), fPoints[3].y());
            SkDebugf("    };\n");
            return true;
        }
    }
    return this->INHERITED::onQuery(evt);
}

DEF_SAMPLE(return new CCPRGeometryView;)

#endif  // SK_SUPPORT_GPU
