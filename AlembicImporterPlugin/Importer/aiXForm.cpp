#include "pch.h"
#include "AlembicImporter.h"
#include "aiLogger.h"
#include "aiContext.h"
#include "aiObject.h"
#include "aiSchema.h"
#include "aiXForm.h"


aiXFormSample::aiXFormSample(aiXForm *schema)
    : super(schema)
{
}

void aiXFormSample::updateConfig(const aiConfig &config, bool &topoChanged, bool &dataChanged)
{
    DebugLog("aiXFormSample::updateConfig()");
    
    topoChanged = false;
    dataChanged = (config.swapHandedness != m_config.swapHandedness);
    m_config = config;
}


inline abcV3 aiExtractTranslation(const AbcGeom::XformSample &sample, bool swapHandedness)
{
    abcV3 ret;

    size_t n = sample.getNumOps();
    size_t n_trans_op = 0;
    for (size_t i = 0; i < n; ++i) {
        const auto &op = sample.getOp(i);
        if (op.getType() == AbcGeom::kTranslateOperation) {
            if (++n_trans_op == 1) {
                ret = op.getVector();
            }
        }
        else if (op.getType() == AbcGeom::kMatrixOperation) {
            n_trans_op = 2;
        }
    }

    if (n_trans_op != 1) {
        ret = sample.getTranslation();
    }

    if (swapHandedness)
    {
        ret.x *= -1.0f;
    }
    return ret;
}

// ret abcV4(axis.x, axis.y, axis.z, angle)
inline abcV4 aiExtractRotation(const AbcGeom::XformSample &sample, bool swapHandedness)
{
    // XformSample �͓����I�Ƀg�����X�t�H�[�����s��ŕێ����Ă���AgetAxis(), getScale() �Ȃǂ͂��̍s�񂩂�l�𕜌����悤�Ƃ���B
    // ���̂��߉�]�ƃX�P�[���������ɂ������Ă���ꍇ�Ԃ��Ă���l�͐��m�ł͂Ȃ��Ȃ��Ă��܂��B
    // ������Ȃ�Ƃ����邽�߁A�ێ�����Ă��� XformOp ������ kRotateOperation �� 1 �����������ꍇ�͂��� axis angle ��Ԃ��A
    // ����ȊO�̏ꍇ getAxis() �� fallback ����B

    const float Deg2Rad = (aiPI / 180.0f);
    abcV4 ret;

    size_t n = sample.getNumOps();
    size_t n_rot_op = 0;
    for (size_t i = 0; i < n; ++i) {
        const auto &op = sample.getOp(i);
        if (op.getType() == AbcGeom::kRotateOperation) {
            if (++n_rot_op == 1) {
                abcV3 ax = op.getAxis();
                ret = abcV4(ax.x, ax.y, ax.z, (float)op.getAngle() * Deg2Rad);
            }
        }
        else if (op.getType() == AbcGeom::kMatrixOperation) {
            n_rot_op = 2;
        }
    }

    if (n_rot_op != 1) {
        abcV3 ax = sample.getAxis();
        ret = abcV4(ax.x, ax.y, ax.z, (float)sample.getAngle() * Deg2Rad);
    }

    if (swapHandedness)
    {
        ret.x *= -1.0f;
        ret.w *= -1.0f;
    }
    return ret;
}

inline abcV3 aiExtractScale(const AbcGeom::XformSample &sample)
{
    abcV3 ret;

    size_t n = sample.getNumOps();
    size_t n_scale_op = 0;
    for (size_t i = 0; i < n; ++i) {
        const auto &op = sample.getOp(i);
        if (op.getType() == AbcGeom::kScaleOperation) {
            if (++n_scale_op == 1) {
                ret = op.getVector();
            }
        }
        else if (op.getType() == AbcGeom::kMatrixOperation) {
            n_scale_op = 2;
        }
    }

    if (n_scale_op != 1) {
        ret = sample.getScale();
    }
    return ret;
}

void aiXFormSample::getData(aiXFormData &outData) const
{
    DebugLog("aiXFormSample::getData()");

    abcV3 trans = aiExtractTranslation(m_sample, m_config.swapHandedness);
    abcV4 rot = aiExtractRotation(m_sample, m_config.swapHandedness);
    abcV3 scale = aiExtractScale(m_sample);

    outData.inherits = m_sample.getInheritsXforms();
    outData.translation = trans;
    outData.scale = scale;
    outData.rotation = abcV4(rot.x * std::sin(rot.w * 0.5f),
                             rot.y * std::sin(rot.w * 0.5f),
                             rot.z * std::sin(rot.w * 0.5f),
                             std::cos(rot.w * 0.5f) );

}



aiXForm::aiXForm(aiObject *obj)
    : super(obj)
{
}

aiXForm::Sample* aiXForm::newSample()
{
    Sample *sample = getSample();
    
    if (!sample)
    {
        sample = new Sample(this);
    }
    
    return sample;
}

aiXForm::Sample* aiXForm::readSample(float time, bool &topologyChanged)
{
    DebugLog("aiXForm::readSample(t=%f)", time);
    
    Sample *ret = newSample();

    m_schema.get(ret->m_sample, MakeSampleSelector(time));

    topologyChanged = false;

    return ret;
}
