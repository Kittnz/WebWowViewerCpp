//
// Created by Deamon on 8/27/2018.
//

#ifndef AWEBWOWVIEWERCPP_IMESH_H
#define AWEBWOWVIEWERCPP_IMESH_H

#include <vector>
#include "../IDevice.h"

enum class MeshType {
    eGeneralMesh = 0,
    eAdtMesh = 1,
    eWmoMesh = 2,
    eOccludingQuery = 3,
    eM2Mesh = 4,
    eParticleMesh = 5,
};

enum class EGxBlendEnum {
    GxBlend_UNDEFINED = -1,
    GxBlend_Opaque = 0,
    GxBlend_AlphaKey = 1,
    GxBlend_Alpha = 2,
    GxBlend_Add = 3,
    GxBlend_Mod = 4,
    GxBlend_Mod2x = 5,
    GxBlend_ModAdd = 6,
    GxBlend_InvSrcAlphaAdd = 7,
    GxBlend_InvSrcAlphaOpaque = 8,
    GxBlend_SrcAlphaOpaque = 9,
    GxBlend_NoAlphaAdd = 10,
    GxBlend_ConstantAlpha = 11,
    GxBlend_Screen = 12,
    GxBlend_BlendAdd = 13,
    GxBlend_MAX
};
struct BlendModeDesc {
    bool blendModeEnable;
    int SrcColor;
    int DestColor;
    int SrcAlpha;
    int DestAlpha;
};

extern BlendModeDesc blendModes[(int)EGxBlendEnum::GxBlend_MAX];

class gMeshTemplate {
public:
    gMeshTemplate(HGVertexBufferBindings bindings, HGShaderPermutation shader) : bindings(bindings), shader(shader) {}
    HGVertexBufferBindings bindings;
    HGShaderPermutation shader;
    MeshType meshType = MeshType::eGeneralMesh;

    int8_t triCCW = 1; //counter-clockwise
    bool depthWrite;
    bool depthCulling;
    bool backFaceCulling;
    EGxBlendEnum blendMode;

    uint8_t colorMask = 0xFF;

    int start;
    int end;
    int element;
    unsigned int textureCount;
    std::vector<HGTexture> texture = std::vector<HGTexture>(4, nullptr);
    HGUniformBuffer vertexBuffers[3] = {nullptr,nullptr,nullptr};
    HGUniformBuffer fragmentBuffers[3] = {nullptr,nullptr,nullptr};
};


class IMesh {
    friend class IDevice;

protected:
    int m_renderOrder = 0;

    float m_sortDistance = 0;
    int m_priorityPlane;
    int m_layer;
    void *m_m2Object = nullptr;

    HGVertexBufferBindings m_bindings;
    int m_start;
    int m_end;

    std::vector<HGTexture> m_texture;
    int m_textureCount;

public:
    virtual ~IMesh(){};
    virtual HGUniformBuffer getVertexUniformBuffer(int slot) = 0;
    virtual HGUniformBuffer getFragmentUniformBuffer(int slot) = 0;
    virtual EGxBlendEnum getGxBlendMode() = 0;
    virtual bool getIsTransparent() = 0;
    virtual MeshType getMeshType() = 0;
    virtual void setRenderOrder(int renderOrder) = 0;

    virtual void setEnd(int end)  = 0;
public:
    virtual void setM2Object(void * m2Object) = 0;
    virtual void setLayer(int layer)  = 0;
    virtual void setPriorityPlane(int priorityPlane) = 0;
    virtual void setQuery(const HGOcclusionQuery &query) = 0;
    virtual void setSortDistance(float distance) = 0;

};
#endif //AWEBWOWVIEWERCPP_IMESH_H
