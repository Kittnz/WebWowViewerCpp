//
// Created by Deamon on 7/23/2018.
//

#ifndef AWEBWOWVIEWERCPP_GTEXTURE_H
#define AWEBWOWVIEWERCPP_GTEXTURE_H

#include "../GDeviceGL33.h"
#include "../../interface/textures/ITexture.h"

class GTextureGL33 : public ITexture {
    friend class GDeviceGL33;
protected:
    explicit GTextureGL33(IDevice &device);
public:
    ~GTextureGL33() override;

    void loadData(int width, int height, void *data) override;
    bool getIsLoaded() override;
    void createGlTexture(TextureFormat textureFormat, const MipmapsVector &mipmaps) override {
        throw "Not Implemented in this class";
    }
private:
    void createBuffer();
    void destroyBuffer();
    virtual void bind(); //Should be called only by GDevice
    void unbind();
protected:
    void * pIdentifierBuffer;

    IDevice &m_device;

    bool m_loaded = false;
};


#endif //AWEBWOWVIEWERCPP_GTEXTURE_H
