//
// Created by Deamon on 7/1/2018.
//

#ifndef AWEBWOWVIEWERCPP_GSHADERPERMUTATION_4X_H
#define AWEBWOWVIEWERCPP_GSHADERPERMUTATION_4X_H

class GDeviceGL4x;

#include <string>
#include <unordered_map>
#include "GDeviceGL4x.h"
#include "../../engine/opengl/header.h"
#include "../../engine/algorithms/hashString.h"
#include "../interface/IShaderPermutation.h"

class GShaderPermutationGL4x : public IShaderPermutation {
    friend class GDeviceGL4x;

public:
    ~GShaderPermutationGL4x() override {};

protected:
    explicit GShaderPermutationGL4x(std::string &shaderName, IDevice *device);

    void compileShader() override;
    GLuint getUnf(const HashedString name) const {
        return m_uniformMap.at(name.Hash());
    }

    GLuint getUBOUnf(const HashedString name) const {
        return m_uniformMap.at(name.Hash());
    }

    void bindProgram();
    void unbindProgram();

    unsigned int m_programBuffer = 0;
private:
    IDevice *m_device;
private:
    std::unordered_map<size_t, unsigned int> m_uniformMap;
    int m_uboVertexBlockIndex[3];
    int m_uboFragmentBlockIndex[3];
    std::string m_shaderName;


    void setUnf(const std::string &name, GLuint index);
};


#endif //AWEBWOWVIEWERCPP_GSHADERPERMUTATION_4X_H
