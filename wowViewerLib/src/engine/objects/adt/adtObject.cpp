//
// Created by deamon on 10.07.17.
//

#include "adtObject.h"
#include "../../shader/ShaderDefinitions.h"
#include "../../algorithms/mathHelper.h"
#include "../../persistance/adtFile.h"
#include "../../persistance/wdtFile.h"
#include "../../../gapi/interface/IDevice.h"
#include "../../../gapi/UniformBufferStructures.h"


void AdtObject::loadingFinished() {
    createVBO();
    loadAlphaTextures();
    createMeshes();
//    createIndexVBO();
    m_loaded = true;
    calcBoundingBoxes();

    loadM2s();
    loadWmos();
}

inline int worldCoordinateToGlobalAdtChunk(float x) {
    return floor(( (32.0f*16.0f) - (x / (533.33333f / 16.0f)   )));
}

void AdtObject::loadM2s() {
    uint32_t offset = 0;
    int32_t length = m_adtFileObj->doodadDef_len;
    //1. Load non-lod
    objectLods[0].m2Objects = std::vector<M2Object *>(length, nullptr);
    for (int j = 0, i = offset; i < offset+length; i++, j++) {
        SMDoodadDef &doodadDef = m_adtFileObj->doodadDef[i];
        if (doodadDef.flags.mddf_entry_is_filedata_id) {
            //2. Get model
            int fileDataId = m_adtFileObj->mmid[doodadDef.nameId];
            objectLods[0].m2Objects[i] = m_mapApi->getM2Object(fileDataId, doodadDef);
        } else {
            std::string fileName = &m_adtFileObj->doodadNamesField[m_adtFileObj->mmid[doodadDef.nameId]];
            //2. Get model
            objectLods[0].m2Objects[i] = m_mapApi->getM2Object(fileName, doodadDef);
        }
    }

    //2. Load lod
    bool useLod1Version = m_adtFileObjLod->lod_levels_for_objects != nullptr;

    if (m_adtFileObjLod->lod_levels_for_objects != nullptr) {
        offset = m_adtFileObjLod->lod_levels_for_objects->m2LodOffset[2];
        length = m_adtFileObjLod->lod_levels_for_objects->m2LodLength[2];
    } else {
        length = 0;
    };
    objectLods[1].m2Objects = std::vector<M2Object *>(length, nullptr);
    for (int j = 0, i = offset; i < offset+length; i++, j++) {
        //1. Get filename
        SMDoodadDef &doodadDef = m_adtFileObjLod->doodadDefObj1[i];
        if (doodadDef.flags.mddf_entry_is_filedata_id == 1) {
            //2. Get model
            int fileDataId = doodadDef.nameId;
            objectLods[1].m2Objects[i] = m_mapApi->getM2Object(fileDataId, doodadDef);
        } else {
            std::string fileName = &m_adtFileObj->doodadNamesField[m_adtFileObj->mmid[doodadDef.nameId]];
            //2. Get model
            objectLods[1].m2Objects[i] = m_mapApi->getM2Object(fileName, doodadDef);
        }
    }
}
void AdtObject::loadWmos() {
    uint32_t offset = 0;
    int32_t length = m_adtFileObj->mapObjDef_len;

    //1. Load non lod
    objectLods[0].wmoObjects = std::vector<WmoObject *>(length, nullptr);

    for (int j = 0, i = offset; i < offset + length; i++, j++) {
        //1. Get filename
        std::string fileName;

        auto &mapDef = m_adtFileObj->mapObjDef[i];
        fileName = &m_adtFileObj->wmoNamesField[m_adtFileObj->mwid[mapDef.nameId]];
        objectLods[0].wmoObjects[j] = m_mapApi->getWmoObject(fileName, mapDef);
//        std::cout << "wmo filename = "<< fileName << std::endl;
    }

    //2. Load lod!
    offset = 0;
    length = m_adtFileObjLod->mapObjDefObj1_len;
    bool useLod1Version = m_adtFileObjLod->lod_levels_for_objects != nullptr;
    if (m_adtFileObjLod->lod_levels_for_objects != nullptr) {
        offset = m_adtFileObjLod->lod_levels_for_objects->wmoLodOffset[2];
        length = m_adtFileObjLod->lod_levels_for_objects->wmoLodLength[2];
    } else {
        length = 0;
    }
    objectLods[1].wmoObjects = std::vector<WmoObject *>(length, nullptr);
    for (int j = 0, i = offset; i < offset + length; i++, j++) {
        //Load Lods
        std::string fileName;
        auto &mapDefLod = m_adtFileObjLod->mapObjDefObj1[i];
        if (mapDefLod.flags.modf_entry_is_filedata_id == 0) {
            fileName = &m_adtFileObj->wmoNamesField[m_adtFileObj->mwid[mapDefLod.nameId]];
            objectLods[1].wmoObjects[j] = m_mapApi->getWmoObject(fileName, mapDefLod);
        } else {
            uint32_t fileDataId = mapDefLod.nameId;
            objectLods[1].wmoObjects[j] = m_mapApi->getWmoObject(fileDataId, mapDefLod);
        }
    }
}

void AdtObject::createVBO() {
    /* 1. help index + Heights + texCoords +  */
    std::vector<float> vboArray ;

    //DEBUG
//    //Interate MLLL
//    for (int i = 0; i < m_adtFileLod->mlll_len; i++) {
//        std::cout<< "MLLL index "<< i << std::endl << std::flush;
//        MLLL & mlll = m_adtFileLod->mllls[i];
//
//        int max_index = -999999;
//        int min_index = 999999;
//
//        for (int j = mlll.height_index; j < mlll.height_index+mlll.height_length; j++) {
//            int index = m_adtFileLod->mvli_indicies[j];
//            if (index < min_index) min_index = index;
//            if (index > max_index) max_index = index;
//        }
//
//        std::cout<< "min_index = "<< min_index << " max_index = " << max_index<< std::endl << std::flush;
//    }

//    int max_index = -999999;
//    int min_index = 999999;
//    for (int i = 0; i < m_adtFileLod->mlsi_len; i++) {
//        int index = m_adtFileLod->mlsi_indicies[i];
//        if (index < min_index) min_index = index;
//        if (index > max_index) max_index = index;
//    }
//    std::cout<< "mlsi: min_index = "<< min_index << " max_index = " << max_index<< std::endl << std::flush;

    /* 1.1 help index */
    this->indexOffset = vboArray.size();
    for (int i = 0; i <= m_adtFile->mcnkRead; i++) {
        for (int j = 0; j < 9 * 9 + 8 * 8; j++) {
            vboArray.push_back((float)j);
        }
    }

    /* 1.2 Heights */
    this->heightOffset = vboArray.size();
    for (int i = 0; i <= m_adtFile->mcnkRead; i++) {
        for (int j = 0; j < 145; j++) {
            vboArray.push_back(m_adtFile->mcnkStructs[i].mcvt->height[j]);
        }
    }

    /* 1.2 Normals */
    this->normalOffset = vboArray.size();
    for (int i = 0; i <= m_adtFile->mcnkRead; i++) {
        for (int j = 0; j < 145; j++) {
            for (int k = 0; k < 3; k++) {
                vboArray.push_back(m_adtFile->mcnkStructs[i].mcnr->entries[j].normal[k] / 127.0f);
            }
        }
    }

    /* 1.3 MCCV */
    this->colorOffset = vboArray.size();
    for (int i = 0; i <= m_adtFile->mcnkRead; i++) {
        if (m_adtFile->mcnkStructs[i].mccv != nullptr) {
            auto &mccv = m_adtFile->mcnkStructs[i].mccv;
            for (int j = 0; j < 145; j++) {
                vboArray.push_back(mccv->entries[j].red / 255.0f);
                vboArray.push_back(mccv->entries[j].green / 255.0f);
                vboArray.push_back(mccv->entries[j].blue / 255.0f);
                vboArray.push_back(mccv->entries[j].alpha / 255.0f);
            }
        } else {
            for (int j = 0; j < 145; j++) {
                // 0.5 to mitigate multiplication by 2 in shader
                vboArray.push_back(0.5f);
                vboArray.push_back(0.5f);
                vboArray.push_back(0.5f);
                vboArray.push_back(0.5f);
            }
        }
    }

    /* 1.4 MCLV */
    this->lightingOffset = vboArray.size();
    for (int i = 0; i <= m_adtFile->mcnkRead; i++) {
        if (m_adtFile->mcnkStructs[i].mclv != nullptr) {
            auto &mclv = m_adtFile->mcnkStructs[i].mclv;
            for (int j = 0; j < 145; j++) {
                vboArray.push_back(mclv->values[j].b / 255.0f);
                vboArray.push_back(mclv->values[j].g / 255.0f);
                vboArray.push_back(mclv->values[j].r / 255.0f);
                vboArray.push_back(mclv->values[j].a / 255.0f);
            }
        } else {
            for (int j = 0; j < 145; j++) {
                // 0.5 to mitigate multiplication by 2 in shader
                vboArray.push_back(0.0f);
                vboArray.push_back(0.0f);
                vboArray.push_back(0.0f);
                vboArray.push_back(0.0f);
            }
        }
    }

    /* 1.3 Make combinedVbo */
    IDevice *device = m_api->getDevice();
    combinedVbo = device->createVertexBuffer();
    combinedVbo->uploadData(&vboArray[0], vboArray.size()*sizeof(float));

    /* 2. Strips */
    stripVBO = device->createIndexBuffer();
    stripVBO->uploadData(&m_adtFile->strips[0], m_adtFile->strips.size() * sizeof(int16_t));

    adtVertexBindings = device->createVertexBufferBindings();
    adtVertexBindings->setIndexBuffer(stripVBO);

    GVertexBufferBinding vertexBinding;
    vertexBinding.vertexBuffer = combinedVbo;

	GBufferBinding bufferBinding = {(uint32_t)adtShader::Attribute::aIndex, 1, GL_FLOAT, false, 4, (this->indexOffset * 4)};
    vertexBinding.bindings.push_back(bufferBinding);
	bufferBinding = {(uint32_t)adtShader::Attribute::aHeight, 1, GL_FLOAT, false, 4, ((this->heightOffset) * 4)};
	vertexBinding.bindings.push_back(bufferBinding);
	bufferBinding = {(uint32_t)adtShader::Attribute::aColor, 4, GL_FLOAT, false, 16, ((this->colorOffset) * 4)};
	vertexBinding.bindings.push_back(bufferBinding);
	bufferBinding = { (uint32_t)adtShader::Attribute::aNormal, 3, GL_FLOAT, false, 12, ((this->normalOffset) * 4) };
	vertexBinding.bindings.push_back(bufferBinding);
	bufferBinding = { (uint32_t)adtShader::Attribute::aVertexLighting, 3, GL_FLOAT, false, 12, ((this->lightingOffset) * 4) };
	vertexBinding.bindings.push_back(bufferBinding);

    adtVertexBindings->addVertexBufferBinding(vertexBinding);
    adtVertexBindings->save();

    if (m_adtFileLod->getIsLoaded()) {
        //Generate MLLL buffers
        //Index buffer for lod
        std::vector<float> vboLod;

        for (int i = 0; i < m_adtFileLod->floatDataBlob_len ; i++) {
            vboLod.push_back(this->m_adtFileLod->floatDataBlob[i]);
        }
        uint32_t indexVBOLodOffset = vboLod.size();
        for (int i = 0; i < (129 * 129 + 128 * 128); i++) {
            vboLod.push_back((float) i);
        }

        heightVboLod = device->createVertexBuffer();
        heightVboLod->uploadData(&vboLod[0], vboLod.size()*sizeof(float));

        /* 2. Index buffer */
        stripVBOLod = device->createIndexBuffer();
        stripVBOLod->uploadData(m_adtFileLod->mvli_indicies,  m_adtFileLod->mvli_len * sizeof(int16_t));


        lodVertexBindings = device->createVertexBufferBindings();
        lodVertexBindings->setIndexBuffer(stripVBOLod);

        GVertexBufferBinding vertexBinding;
        vertexBinding.vertexBuffer = combinedVbo;

		GBufferBinding bufferBinding = { +adtLodShader::Attribute::aHeight, 1, GL_FLOAT, false, 4, 0 };
		vertexBinding.bindings.push_back(bufferBinding);
		bufferBinding = { +adtLodShader::Attribute::aIndex, 1, GL_FLOAT, false, 4, indexVBOLodOffset * sizeof(float) };
        vertexBinding.bindings.push_back(bufferBinding);

        lodVertexBindings->addVertexBufferBinding(vertexBinding);
        lodVertexBindings->save();
    }
}

void AdtObject::calcBoundingBoxes() {
    for (int i = 0; i < 256; i++) {
        mcnkStruct_t *mcnkContent = &this->m_adtFile->mcnkStructs[i];
        SMChunk *mcnkChunk = &m_adtFile->mapTile[i];

        //Loop over heights
        float minZ = 999999;
        float maxZ = -999999;
        for (int j = 0; j < 8*8+9*9; j++) {
            float heightVal = mcnkContent->mcvt->height[j];
            if (minZ > heightVal) minZ = heightVal;
            if (maxZ < heightVal) maxZ = heightVal;
        }

        float minX = mcnkChunk->position.x - (533.3433333 / 16.0);
        float maxX = mcnkChunk->position.x;
        float minY = mcnkChunk->position.y - (533.3433333 / 16.0);
        float maxY = mcnkChunk->position.y;
        minZ += mcnkChunk->position.z;
        maxZ += mcnkChunk->position.z;

        this->tileAabb[i] = CAaBox(
            C3Vector(mathfu::vec3(minX, minY, minZ)),
            C3Vector(mathfu::vec3(maxX, maxY, maxZ))
        );

        this->globIndexY[i] = worldCoordinateToGlobalAdtChunk((minX + maxX) / 2.0f);
        this->globIndexX[i] = worldCoordinateToGlobalAdtChunk((minY + maxY) / 2.0f);
    }
}

void AdtObject::createMeshes() {
    IDevice *device = m_api->getDevice();

    adtWideBlockPS = m_api->getDevice()->createUniformBuffer(sizeof(adtModelWideBlockPS));

    adtModelWideBlockPS &adtWideblockPS = adtWideBlockPS->getObject<adtModelWideBlockPS>();
    adtWideblockPS.uViewUp = mathfu::vec4_packed(mathfu::vec4(m_api->getViewUp(), 0.0));;
    adtWideblockPS.uSunDir_FogStart = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalSunDir(), m_api->getGlobalFogStart()));
    adtWideblockPS.uSunColor_uFogEnd = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalSunColor().xyz(), m_api->getGlobalFogEnd()));
    adtWideblockPS.uAmbientLight = m_api->getGlobalAmbientColor();
    adtWideblockPS.FogColor = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalFogColor().xyz(), 0));

    adtWideBlockPS->save(true);

    for (int i = 0; i < 256; i++) {
        //Cant be used only in Wotlk
        //if (m_adtFile->mapTile[i].nLayers <= 0) continue;
        if (m_adtFileTex->mcnkStructs[i].mclyCnt <= 0) continue;
        if (m_adtFileTex->mcnkStructs[i].mcly == nullptr) continue;

        HGShaderPermutation hgShaderPermutation = device->getShader("adtShader");
        gMeshTemplate aTemplate(adtVertexBindings, hgShaderPermutation);

        aTemplate.meshType = MeshType::eAdtMesh;
        aTemplate.triCCW = 0;
        aTemplate.depthWrite = 1;
        aTemplate.depthCulling = 1;
        aTemplate.backFaceCulling = 1;
        aTemplate.blendMode = EGxBlendEnum::GxBlend_Opaque;

        aTemplate.start = m_adtFile->stripOffsets[i] * 2;
        aTemplate.end = m_adtFile->stripOffsets[i + 1] - m_adtFile->stripOffsets[i];
        aTemplate.element = GL_TRIANGLE_STRIP;
        aTemplate.textureCount = 9;

        aTemplate.texture = std::vector<HGTexture>(aTemplate.textureCount, nullptr);
        aTemplate.texture[4] = alphaTextures[i];
        for (int j = 0; j < m_adtFileTex->mcnkStructs[i].mclyCnt; j++) {
            HGTexture layer_x = getAdtTexture(m_adtFileTex->mcnkStructs[i].mcly[j].textureId);
            HGTexture layer_height = getAdtHeightTexture(m_adtFileTex->mcnkStructs[i].mcly[j].textureId);
//            BlpTexture &layer_spec = getAdtSpecularTexture(m_adtFileTex->mcnkStructs[i].mcly[j].textureId);
            aTemplate.texture[j] = layer_x;
            aTemplate.texture[j + 5] = layer_height;
        }

        aTemplate.vertexBuffers[0] = m_api->getSceneWideUniformBuffer();
        aTemplate.vertexBuffers[1] = nullptr;
        aTemplate.vertexBuffers[2] = m_api->getDevice()->createUniformBuffer(sizeof(adtMeshWideBlockVS));

        aTemplate.fragmentBuffers[0] = m_api->getSceneWideUniformBuffer();
        aTemplate.fragmentBuffers[1] = adtWideBlockPS;
        aTemplate.fragmentBuffers[2] = m_api->getDevice()->createUniformBuffer(sizeof(adtMeshWideBlockPS));

        adtMeshWideBlockPS &blockPS = aTemplate.fragmentBuffers[2]->getObject<adtMeshWideBlockPS>();

        static const float heightScale [4] = {0.0, 0.0, 0.0, 0.0};
        static const float heightOffset [4] = {1.0, 1.0, 1.0, 1.0};

        for (int j = 0; j < 4; j++) {
            blockPS.uHeightOffset[j] = heightOffset[j];
            blockPS.uHeightScale[j] = heightScale[j];
        }
        if (m_adtFileTex->mtxp_len > 0) {
            for (int j = 0; j < m_adtFileTex->mcnkStructs[i].mclyCnt; j++) {
                blockPS.uHeightOffset[j] = m_adtFileTex->mtxp[m_adtFileTex->mcnkStructs[i].mcly[j].textureId].heightOffset;
                blockPS.uHeightScale[j] = m_adtFileTex->mtxp[m_adtFileTex->mcnkStructs[i].mcly[j].textureId].heightScale;
            }
        }
        aTemplate.fragmentBuffers[2]->save(true);

        adtMeshWideBlockVS &blockVS = aTemplate.vertexBuffers[2]->getObject<adtMeshWideBlockVS>();
        blockVS.uPos = mathfu::vec4(
            m_adtFile->mapTile[i].position.x,
            m_adtFile->mapTile[i].position.y,
            m_adtFile->mapTile[i].position.z,
            0
        );
        aTemplate.vertexBuffers[2]->save(true);

        HGMesh hgMesh = device->createMesh(aTemplate);
        adtMeshes.push_back(hgMesh);
    }
}

void AdtObject::loadAlphaTextures() {
    //int chunkCount = m_adtFile->mcnkRead+1;
    int chunkCount = m_adtFileTex->mcnkRead+1;
    int maxAlphaTexPerChunk = 4;
    int alphaTexSize = 64;

    int texWidth = alphaTexSize;
    int texHeight = alphaTexSize;

    int createdThisRun = 0;
    for (int i = 0; i < chunkCount; i++) {
        HGTexture alphaTexture = m_api->getDevice()->createTexture();
        std::vector<uint8_t> alphaTextureData = m_adtFileTex->processTexture(m_wdtFile->mphd->flags, i);

        alphaTexture->loadData(texWidth, texHeight, &alphaTextureData[0]);

        alphaTextures.push_back(alphaTexture);
    }
    this->alphaTexturesLoaded += createdThisRun;
}



void AdtObject::collectMeshes(std::vector<HGMesh> &renderedThisFrame, int renderOrder) {
    if (!m_loaded) return;

    size_t meshCount = adtMeshes.size();
    for (int i = 0; i < meshCount; i++) {
        if (!drawChunk[i]) continue;
        adtMeshes[i]->setRenderOrder(renderOrder);
        renderedThisFrame.push_back(adtMeshes[i]);
    }
}
void AdtObject::collectMeshesLod(std::vector<HGMesh> &renderedThisFrame) {
   /*
    if (!m_loaded) return;
    if (lodCommands.size() <= 0) return;

    GLuint blackPixelTexture = this->m_api->getBlackPixelTexture();
    ShaderRuntimeData *adtLodShader = this->m_api->getAdtLodShader();

    mathfu::vec3 adtPos = mathfu::vec3(m_adtFile->mapTile[m_adtFile->mcnkMap[0][0]].position);
    glUniform3f(adtLodShader->getUnf("uPos"),
                adtPos.x,
                adtPos.y,
                adtPos.z);

    glBindBuffer(GL_ARRAY_BUFFER, this->heightVboLod);
    glVertexAttribPointer(+adtLodShader::Attribute::aHeight, 1, GL_FLOAT, GL_FALSE, 4, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, this->indexVBOLod);
    glVertexAttribPointer(+adtLodShader::Attribute::aIndex, 1, GL_FLOAT, GL_FALSE, 4, nullptr);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->stripVBOLod);

    glActiveTexture(GL_TEXTURE0);
    if (lodDiffuseTexture->getIsLoaded()) {
        glBindTexture(GL_TEXTURE_2D, lodDiffuseTexture->getGlTexture());
    } else {
        glBindTexture(GL_TEXTURE_2D, blackPixelTexture);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE1);
    if (lodNormalTexture->getIsLoaded()) {
        glBindTexture(GL_TEXTURE_2D, lodNormalTexture->getGlTexture());
    } else {
        glBindTexture(GL_TEXTURE_2D, blackPixelTexture);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    for (int i = 0; i < lodCommands.size(); i++) {
        glDrawElements(GL_TRIANGLES, lodCommands[i].length, GL_UNSIGNED_SHORT, (void *)(lodCommands[i].index * 2));
    }
    */
}

void AdtObject::doPostLoad() {
    if (!m_loaded) {
        if (m_adtFile->getIsLoaded() &&
            m_adtFileObj->getIsLoaded() &&
            m_adtFileObjLod->getIsLoaded() &&
            (m_adtFileLod->getIsLoaded() || !m_wdtFile->mphd->flags.unk_0x0100) &&
            m_adtFileTex->getIsLoaded()) {
            this->loadingFinished();
            m_loaded = true;
        }
    }
}
void AdtObject::update() {
    if (!m_loaded) false;
    if (adtWideBlockPS == nullptr) return;

    adtModelWideBlockPS &adtWideblockPS = adtWideBlockPS->getObject<adtModelWideBlockPS>();
    adtWideblockPS.uViewUp = mathfu::vec4_packed(mathfu::vec4(m_api->getViewUp(), 0.0));;
    adtWideblockPS.uSunDir_FogStart = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalSunDir(), m_api->getGlobalFogStart()));
    adtWideblockPS.uSunColor_uFogEnd = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalSunColor().xyz(), m_api->getGlobalFogEnd()));
    adtWideblockPS.uAmbientLight = m_api->getGlobalAmbientColor();
    adtWideblockPS.FogColor = mathfu::vec4_packed(mathfu::vec4(m_api->getGlobalFogColor().xyz(), 0));

    adtWideBlockPS->save();
}

HGTexture AdtObject::getAdtTexture(int textureId) {
    auto item = m_requestedTextures.find(textureId);
    if (item != m_requestedTextures.end()) {
        return item->second;
    }

    std::string &materialTexture = m_adtFileTex->textureNames[textureId];
    HBlpTexture texture = m_api->getTextureCache()->get(materialTexture);
    HGTexture h_gblpTexture = m_api->getDevice()->createBlpTexture(texture, true, true);
    m_requestedTextures[textureId] = h_gblpTexture;

    return h_gblpTexture;
}

HGTexture AdtObject::getAdtHeightTexture(int textureId) {
    auto item = m_requestedTexturesHeight.find(textureId);
    if (item != m_requestedTexturesHeight.end()) {
        return item->second;
    }

    std::string &materialTexture = m_adtFileTex->textureNames[textureId];

    std::string matHeightText = materialTexture.substr(0, materialTexture.size() - 4) + "_h.blp";

    HBlpTexture texture = m_api->getTextureCache()->get(matHeightText);
    HGTexture h_gblpTexture = m_api->getDevice()->createBlpTexture(texture, true, true);
    m_requestedTexturesHeight[textureId] = h_gblpTexture;

    return h_gblpTexture;
}

HGTexture AdtObject::getAdtSpecularTexture(int textureId) {
    auto item = m_requestedTexturesSpec.find(textureId);
    if (item != m_requestedTexturesSpec.end()) {
        return item->second;
    }

    std::string &materialTexture = m_adtFileTex->textureNames[textureId];

    std::string matHeightText = materialTexture.substr(0, materialTexture.size() - 4) + "_s.blp";

    HBlpTexture texture = m_api->getTextureCache()->get(matHeightText);
    HGTexture h_gblpTexture = m_api->getDevice()->createBlpTexture(texture, true, true);
    m_requestedTexturesSpec[textureId] = h_gblpTexture;

    return h_gblpTexture;
}

const float dist = 533.0f/16;
static const float perLodDist[5] = {9999999999.99f,
                                    std::pow(dist*60, 2.0f),        //32
                                    std::pow(dist*40, 2.0f), //16
                                    std::pow(dist*20, 2.0f), //8
                                    std::pow(dist*10, 2.0f), //4
//                                  std::pow(dist / (16), 2) //2
};


bool AdtObject::iterateQuadTree(mathfu::vec4 &camera, const mathfu::vec3 &pos,
                                float x_offset, float y_offset, float cell_len,
                                int curentLod, int lastFoundLod,
                                const MLND *quadTree, int quadTreeInd,
                                std::vector<mathfu::vec4> &frustumPlanes,
                                std::vector<mathfu::vec3> &frustumPoints,
                                std::vector<mathfu::vec3> &hullLines,
                                mathfu::mat4 &lookAtMat4,
                                std::vector<M2Object *> &m2ObjectsCandidates,
                                std::vector<WmoObject *> &wmoCandidates) {



    bool drawLodBasedOnDist = false;
    const MLND *quadTreeNode = nullptr;
    int foundLod = lastFoundLod;

    if (quadTree == nullptr || quadTreeInd == -1 || curentLod == 4) {
        mathfu::vec2 aabb2D[2];
        aabb2D[0] = pos.xy() - mathfu::vec2(533.3333f * (x_offset + cell_len), 533.3333f*(y_offset + cell_len));
        aabb2D[1] = pos.xy() -  mathfu::vec2(533.3333f *x_offset, 533.3333f*y_offset);
        mathfu::vec2 point = camera.xy();

        //General frustum cull!
        bool atLeastOneIsDrawn = false;

        atLeastOneIsDrawn = checkNonLodChunkCulling(camera, frustumPlanes, frustumPoints, hullLines,
                                                         16.0f * x_offset, 16.0f * y_offset,
                                                         16.0f * cell_len, 16.0f * cell_len
        );

        checkReferences(camera, frustumPlanes, frustumPoints,
                        lookAtMat4, 5,
                        m2ObjectsCandidates, wmoCandidates,
                        16.1f * x_offset, 16.1f * y_offset,
                        16.1f * cell_len, 16.1f * cell_len);

        return atLeastOneIsDrawn;
    }
    quadTreeNode = &quadTree[quadTreeInd];

    if (foundLod == curentLod) {
        quadTreeNode = &quadTree[quadTreeInd];

        mathfu::vec2 aabb2D[2];
        aabb2D[0] = pos.xy() - mathfu::vec2(533.3333f * (x_offset + cell_len), 533.3333f*(y_offset + cell_len));
        aabb2D[1] = pos.xy() -  mathfu::vec2(533.3333f *x_offset, 533.3333f*y_offset);

        mathfu::vec2 point = camera.xy();
        float dist = MathHelper::distanceFromAABBToPoint2DSquared(aabb2D, point);

        while (dist < perLodDist[foundLod] && foundLod < 5) foundLod++;
    }



    if (quadTreeNode != nullptr && foundLod > curentLod) {
        //check all others
        float newCellLen = cell_len/2.0f;
        bool atLeastOneIsDrawn = false;

        //1. Node 1
        atLeastOneIsDrawn |= iterateQuadTree(camera, pos, x_offset, y_offset , newCellLen, curentLod + 1, foundLod, quadTree, quadTreeNode->indices[0],
                                            frustumPlanes, frustumPoints, hullLines, lookAtMat4, m2ObjectsCandidates, wmoCandidates);
        //2. Node 2
        atLeastOneIsDrawn |= iterateQuadTree(camera, pos, x_offset, y_offset + newCellLen, newCellLen, curentLod + 1, foundLod, quadTree, quadTreeNode->indices[1],
                                            frustumPlanes, frustumPoints, hullLines, lookAtMat4, m2ObjectsCandidates, wmoCandidates);
        //3. Node 3
        atLeastOneIsDrawn |= iterateQuadTree(camera, pos, x_offset + newCellLen, y_offset, newCellLen, curentLod + 1, foundLod, quadTree, quadTreeNode->indices[2],
                                            frustumPlanes, frustumPoints, hullLines, lookAtMat4, m2ObjectsCandidates, wmoCandidates);
        //4. Node 4
        atLeastOneIsDrawn |= iterateQuadTree(camera, pos, x_offset+ newCellLen, y_offset+ newCellLen, newCellLen, curentLod + 1, foundLod, quadTree, quadTreeNode->indices[3],
                                            frustumPlanes, frustumPoints, hullLines, lookAtMat4, m2ObjectsCandidates, wmoCandidates);

        return atLeastOneIsDrawn;
    } else {
            //Push the drawCall for this lod
            LodCommand command;
            command.index = quadTreeNode->index;
            command.length = quadTreeNode->length;

            lodCommands.push_back(command);

            checkReferences(camera, frustumPlanes, frustumPoints,
                            lookAtMat4, curentLod,
                            m2ObjectsCandidates, wmoCandidates,
                            16.0f * x_offset, 16.0f * y_offset,
                            16.0f * cell_len, 16.0f * cell_len);

            bool atLeastOneIsDrawn = true;

        return atLeastOneIsDrawn;
    }
}

bool AdtObject::checkNonLodChunkCulling(mathfu::vec4 &cameraPos,
                                        std::vector<mathfu::vec4> &frustumPlanes,
                                        std::vector<mathfu::vec3> &frustumPoints,
                                        std::vector<mathfu::vec3> &hullLines,

                                        int x, int y, int x_len, int y_len) {

    bool atLeastOneIsDrawn = false;
    for (int k = x; k < x + x_len; k++) {
        for (int l = y; l < y + y_len; l++) {
            int i = this->m_adtFile->mcnkMap[l][k];


            mcnkStruct_t &mcnk = this->m_adtFile->mcnkStructs[i];
            CAaBox &aabb = this->tileAabb[i];
            this->drawChunk[i] = false;

            //1. Check if camera position is inside Bounding Box
            bool cameraOnChunk =
                    (cameraPos[0] > aabb.min.x && cameraPos[0] < aabb.max.x &&
                     cameraPos[1] > aabb.min.y && cameraPos[1] < aabb.max.y);
            if (cameraOnChunk &&
                cameraPos[2] > aabb.min.z && cameraPos[2] < aabb.max.z
                    ) {
                this->drawChunk[i] = true;
                atLeastOneIsDrawn = true;
            }

            //2. Check aabb is inside camera frustum
            bool result = false;
            checkRefs[i] = this->drawChunk[i];
            if (!this->drawChunk[i]) {
                result = MathHelper::checkFrustum(frustumPlanes, aabb, frustumPoints);
                bool frustum2DRes = MathHelper::checkFrustum2D(hullLines, aabb);
                checkRefs[i] = result || frustum2DRes;

                this->drawChunk[i] = result;
                atLeastOneIsDrawn = atLeastOneIsDrawn || result;
            }
        }
    }

    return atLeastOneIsDrawn;
}

bool AdtObject::checkReferences(mathfu::vec4 &cameraPos,
                          std::vector<mathfu::vec4> &frustumPlanes,
                          std::vector<mathfu::vec3> &frustumPoints,
                          mathfu::mat4 &lookAtMat4,
                          int lodLevel,
                          std::vector<M2Object *> &m2ObjectsCandidates,
                          std::vector<WmoObject *> &wmoCandidates,
                          int x, int y, int x_len, int y_len) {
    if (!m_loaded) return false;

    for (int k = x; k < x+x_len; k++) {
        for (int l = y; l < y + y_len; l++) {
            int i = this->m_adtFile->mcnkMap[k][l];

            bool wotlk = false;
            float chunkDist = 1.0;
            if (wotlk) {
//                SMChunk *mapTile = &m_adtFile->mapTile[i];
//                mcnkStruct_t *mcnkContent = &m_adtFile->mcnkStructs[i];
//
//
//                if (mcnkContent != nullptr && mcnkContent->mcrf.doodad_refs != nullptr) {
//                    for (int j = 0; j < mapTile->nDoodadRefs; j++) {
//                        uint32_t m2Ref = mcnkContent->mcrf.doodad_refs[j];
//                        m2ObjectsCandidates.insert(this->m2Objects[m2Ref]);
//                    }
//                }
//                if (mcnkContent != nullptr && mcnkContent->mcrf.object_refs != nullptr) {
//                    for (int j = 0; j < mapTile->nMapObjRefs; j++) {
//                        uint32_t wmoRef = mcnkContent->mcrf.object_refs[j];
//                        wmoCandidates.insert(this->wmoObjects[wmoRef]);
//                    }
//                }
//            for (int j = 0; j < this->m2Objects.size(); j++) {
//                m2ObjectsCandidates.insert(this->m2Objects[j]);
//            }
//            for (int j = 0; j < wmoObjects.size(); j++) {
//                uint32_t wmoRef = mcnkContent->mcrf.object_refs[j];
//                wmoCandidates.insert(this->wmoObjects[j]);
//            }
            } else {
                SMChunk *mapTile = &m_adtFile->mapTile[i];
                mcnkStruct_t *mcnkContent = &m_adtFileObj->mcnkStructs[i];
                if (lodLevel >= 4) {
                    if (mcnkContent->mcrd_doodad_refs_len > 0) {
                        for (int j = 0; j < mcnkContent->mcrd_doodad_refs_len; j++) {
                            uint32_t m2Ref = mcnkContent->mcrd_doodad_refs[j];
                            m2ObjectsCandidates.push_back(this->objectLods[0].m2Objects[m2Ref]);
                        }
                    }
                } else {
                    for (auto m2Object : this->objectLods[1].m2Objects) {
                        m2ObjectsCandidates.push_back(m2Object);
                    }
                }

                if (lodLevel >= 4) {
                    if (mcnkContent->mcrw_object_refs_len > 0) {
                        for (int j = 0; j < mcnkContent->mcrw_object_refs_len; j++) {
                            uint32_t wmoRef = mcnkContent->mcrw_object_refs[j];
                            wmoCandidates.push_back(this->objectLods[0].wmoObjects[wmoRef]);
                        }
                    }
                } else {
                    for (auto wmoObject : this->objectLods[1].wmoObjects) {
                        wmoCandidates.push_back(wmoObject);
                    }
                }
            }
        }
    }
	return true;
}

bool AdtObject::checkFrustumCulling(mathfu::vec4 &cameraPos,
                                    int adt_glob_x,
                                    int adt_glob_y,
                                    std::vector<mathfu::vec4> &frustumPlanes,
                                    std::vector<mathfu::vec3> &frustumPoints,
                                    std::vector<mathfu::vec3> &hullLines,
                                    mathfu::mat4 &lookAtMat4,
                                    std::vector<M2Object *> &m2ObjectsCandidates,
                                    std::vector<WmoObject *> &wmoCandidates) {

    if (!this->m_loaded) return true;
    bool atLeastOneIsDrawn = false;

    mostDetailedLod = 5;
    leastDetiledLod = 0;

    for (int i = 0; i < 256; i++) {
        drawChunk[i] = false;
    }
    lodCommands.clear();

    //For new ADT with _lod.adt
    if (false) {
//    if (m_wdtFile->mphd->flags.unk_0x0100) {
        mathfu::vec3 adtPos = mathfu::vec3(m_adtFile->mapTile[m_adtFile->mcnkMap[0][0]].position);
        atLeastOneIsDrawn |= iterateQuadTree(cameraPos, adtPos,
                                             0, 0, 1.0,
                                             0, 0,
                                             m_adtFileLod->mlnds, 0,
                                             frustumPlanes, frustumPoints, hullLines, lookAtMat4, m2ObjectsCandidates,
                                             wmoCandidates);
    } else {
        //For old ADT without _lod.adt
        atLeastOneIsDrawn |= checkNonLodChunkCulling(cameraPos,
                                frustumPlanes, frustumPoints, hullLines,
                                0, 0, 16, 16);
        checkReferences(cameraPos, frustumPlanes, frustumPoints,
                        lookAtMat4, 5,
                        m2ObjectsCandidates, wmoCandidates,
                        0, 0,
                        16, 16);
    }


    return atLeastOneIsDrawn;
}

AdtObject::AdtObject(IWoWInnerApi *api, std::string &adtFileTemplate, std::string mapname, int adt_x, int adt_y, HWdtFile wdtFile) : alphaTextures(), adt_x(adt_x), adt_y(adt_y){
    m_api = api;
    tileAabb = std::vector<CAaBox>(256);
    globIndexX = std::vector<int>(256);
    globIndexY = std::vector<int>(256);
    adtFileTemplate = adtFileTemplate;

    m_wdtFile = wdtFile;

    m_adtFile = m_api->getAdtGeomCache()->get(adtFileTemplate+".adt");
    m_adtFile->setIsMain(true);
    m_adtFileTex = m_api->getAdtGeomCache()->get(adtFileTemplate+"_tex"+std::to_string(0)+".adt");
    m_adtFileObj = m_api->getAdtGeomCache()->get(adtFileTemplate+"_obj"+std::to_string(0)+".adt");
    m_adtFileObjLod = m_api->getAdtGeomCache()->get(adtFileTemplate+"_obj"+std::to_string(1)+".adt");
    m_adtFileLod = m_api->getAdtGeomCache()->get(adtFileTemplate+"_lod.adt");

    lodDiffuseTexture = m_api->getTextureCache()->get("world/maptextures/"+mapname+"/"
        +mapname+"_"+std::to_string(adt_x)+"_"+std::to_string(adt_y)+".blp");
    lodNormalTexture = m_api->getTextureCache()->get("world/maptextures/"+mapname+"/"
        +mapname+"_"+std::to_string(adt_x)+"_"+std::to_string(adt_y)+"_n.blp");

}
