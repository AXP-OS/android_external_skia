

/**************************************************************************************************
 *** This file was autogenerated from GrChildProcessorSampleMatrixConstant.fp; do not modify.
 **************************************************************************************************/
#include "GrChildProcessorSampleMatrixConstant.h"

#include "src/core/SkUtils.h"
#include "src/gpu/GrTexture.h"
#include "src/gpu/glsl/GrGLSLFragmentProcessor.h"
#include "src/gpu/glsl/GrGLSLFragmentShaderBuilder.h"
#include "src/gpu/glsl/GrGLSLProgramBuilder.h"
#include "src/sksl/SkSLCPP.h"
#include "src/sksl/SkSLUtil.h"
class GrGLSLChildProcessorSampleMatrixConstant : public GrGLSLFragmentProcessor {
public:
    GrGLSLChildProcessorSampleMatrixConstant() {}
    void emitCode(EmitArgs& args) override {
        GrGLSLFPFragmentBuilder* fragBuilder = args.fFragBuilder;
        const GrChildProcessorSampleMatrixConstant& _outer = args.fFp.cast<GrChildProcessorSampleMatrixConstant>();
        (void) _outer;
        SkString _sample68 = this->invokeChildWithMatrix(0, args);
        fragBuilder->codeAppendf(
R"SkSL(%s = %s;
)SkSL"
, args.fOutputColor, _sample68.c_str());
    }
private:
    void onSetData(const GrGLSLProgramDataManager& pdman, const GrFragmentProcessor& _proc) override {
    }
};
GrGLSLFragmentProcessor* GrChildProcessorSampleMatrixConstant::onCreateGLSLInstance() const {
    return new GrGLSLChildProcessorSampleMatrixConstant();
}
void GrChildProcessorSampleMatrixConstant::onGetGLSLProcessorKey(const GrShaderCaps& caps, GrProcessorKeyBuilder* b) const {
}
bool GrChildProcessorSampleMatrixConstant::onIsEqual(const GrFragmentProcessor& other) const {
    const GrChildProcessorSampleMatrixConstant& that = other.cast<GrChildProcessorSampleMatrixConstant>();
    (void) that;
    return true;
}
bool GrChildProcessorSampleMatrixConstant::usesExplicitReturn() const {
    return false;
}
GrChildProcessorSampleMatrixConstant::GrChildProcessorSampleMatrixConstant(const GrChildProcessorSampleMatrixConstant& src)
: INHERITED(kGrChildProcessorSampleMatrixConstant_ClassID, src.optimizationFlags()) {
        this->cloneAndRegisterAllChildProcessors(src);
}
std::unique_ptr<GrFragmentProcessor> GrChildProcessorSampleMatrixConstant::clone() const {
    return std::make_unique<GrChildProcessorSampleMatrixConstant>(*this);
}
#if GR_TEST_UTILS
SkString GrChildProcessorSampleMatrixConstant::onDumpInfo() const {
    return SkString();
}
#endif
