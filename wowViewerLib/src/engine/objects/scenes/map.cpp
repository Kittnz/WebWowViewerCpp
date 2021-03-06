//
// Created by Deamon on 7/16/2017.
//
#include <algorithm>
#include <iostream>
#include <set>
#include <cmath>
#include "map.h"
#include "../../algorithms/mathHelper.h"
#include "../../algorithms/grahamScan.h"
#include "../m2/m2Instancing/m2InstancingObject.h"
#include "../../persistance/wdtFile.h"
#include "../../persistance/db2/DB2WmoAreaTable.h"
#include "../../../gapi/interface/meshes/IM2Mesh.h"
#include "../../../gapi/interface/IDevice.h"
#include "../wowFrameData.h"


inline int worldCoordinateToAdtIndex(float x) {
    return floor((32.0f - (x / 533.33333f)));
}

inline int worldCoordinateToGlobalAdtChunk(float x) {
    return floor(( (32.0f*16.0f) - (x / (533.33333f / 16.0f)   )));
}

float AdtIndexToWorldCoordinate(int x) {
    return (32 - x) * 533.33333;
}

void Map::checkCulling(WoWFrameData *frameData) {
    if (!m_wdtfile->getIsLoaded()) return;

    mathfu::vec4 cameraPos = mathfu::vec4(frameData->m_cameraVec3, 1.0);
    mathfu::mat4 &frustumMat = frameData->m_perspectiveMatrixForCulling;
    mathfu::mat4 &lookAtMat4 = frameData->m_lookAtMat4;

    size_t adtRenderedThisFramePrev = frameData->adtArray.size();
    frameData->adtArray = std::vector<AdtObject*>();
    frameData->adtArray.reserve(adtRenderedThisFramePrev);

    size_t m2RenderedThisFramePrev = frameData->m2Array.size();
    frameData->m2Array = std::vector<M2Object*>();
    frameData->m2Array.reserve(m2RenderedThisFramePrev);

    size_t wmoRenderedThisFramePrev = frameData->wmoArray.size();
    frameData->wmoArray = std::vector<WmoObject*>();
    frameData->wmoArray.reserve(wmoRenderedThisFramePrev);


    mathfu::mat4 projectionModelMat = frustumMat*lookAtMat4;

    std::vector<mathfu::vec4> frustumPlanes = MathHelper::getFrustumClipsFromMatrix(projectionModelMat);
    MathHelper::fixNearPlane(frustumPlanes, cameraPos);

    std::vector<mathfu::vec3> frustumPoints = MathHelper::calculateFrustumPointsFromMat(projectionModelMat);
    std::vector<mathfu::vec3> hullines = MathHelper::getHullLines(frustumPoints);

    frameData->exteriorView = ExteriorView();
    frameData->interiorViews = std::vector<InteriorView>();
    m_viewRenderOrder = 0;

    if (!this->m_currentInteriorGroups.empty() && m_currentWMO != nullptr && this->m_currentWMO->isLoaded()) {
        this->m_currentWMO->resetTraversedWmoGroups();
        if (this->m_currentWMO->startTraversingWMOGroup(
            cameraPos,
            projectionModelMat,
            this->m_currentInteriorGroups[0].groupIndex,
            0,
            m_viewRenderOrder,
            true,
            frameData->interiorViews,
            frameData->exteriorView)) {

            frameData->wmoArray.push_back(this->m_currentWMO);
        }

        if (frameData->exteriorView.viewCreated) {
            checkExterior(cameraPos, frustumPoints, hullines, lookAtMat4, projectionModelMat, m_viewRenderOrder, frameData);
        }
    } else {
        //Cull exterior
        frameData->exteriorView.viewCreated = true;
        frameData->exteriorView.frustumPlanes.push_back(frustumPlanes);
        checkExterior(cameraPos, frustumPoints, hullines, lookAtMat4, projectionModelMat, m_viewRenderOrder, frameData);
    }

    //Fill M2 objects for views from WmoGroups
    for (auto &view : frameData->interiorViews) {
        view.addM2FromGroups(frustumMat, lookAtMat4, cameraPos);
    }
    frameData->exteriorView.addM2FromGroups(frustumMat, lookAtMat4, cameraPos);
    for (auto &adt : frameData->exteriorView.drawnADTs) {
        adt->collectMeshes(frameData->exteriorView.drawnChunks, frameData->exteriorView.renderOrder);
    }

    //Collect M2s for update
    frameData->m2Array.clear();
    auto inserter = std::back_inserter(frameData->m2Array);
    for (auto &view : frameData->interiorViews) {
        std::copy(view.drawnM2s.begin(), view.drawnM2s.end(), inserter);
    }
    std::copy(frameData->exteriorView.drawnM2s.begin(), frameData->exteriorView.drawnM2s.end(), inserter);

    //Sort and delete duplicates
    std::sort( frameData->m2Array.begin(), frameData->m2Array.end() );
    frameData->m2Array.erase( unique( frameData->m2Array.begin(), frameData->m2Array.end() ), frameData->m2Array.end() );
    frameData->m2Array = std::vector<M2Object*>(frameData->m2Array.begin(), frameData->m2Array.end());

    std::sort( frameData->wmoArray.begin(), frameData->wmoArray.end() );
    frameData->wmoArray.erase( unique( frameData->wmoArray.begin(), frameData->wmoArray.end() ), frameData->wmoArray.end() );
    frameData->wmoArray = std::vector<WmoObject*>(frameData->wmoArray.begin(), frameData->wmoArray.end());

    frameData->adtArray = std::vector<AdtObject*>(frameData->adtArray.begin(), frameData->adtArray.end());

//    //Limit M2 count based on distance/m2 height
//    for (auto it = this->m2RenderedThisFrameArr.begin();
//         it != this->m2RenderedThisFrameArr.end();) {
//        if ((*it)->getCurrentDistance() > ((*it)->getHeight() * 5)) {
//            it = this->m2RenderedThisFrameArr.erase(it);
//        } else {
//            ++it;
//        }
//    }
}

void Map::checkExterior(mathfu::vec4 &cameraPos,
                        std::vector<mathfu::vec3> &frustumPoints,
                        std::vector<mathfu::vec3> &hullLines,
                        mathfu::mat4 &lookAtMat4,
                        mathfu::mat4 &projectionModelMat,
                        int viewRenderOrder,
                        WoWFrameData *frameData
) {

    std::vector<M2Object *> m2ObjectsCandidates;
    std::vector<WmoObject *> wmoCandidates;

//    float adt_x = floor((32 - (cameraPos[1] / 533.33333)));
//    float adt_y = floor((32 - (cameraPos[0] / 533.33333)));

    //Get visible area that should be checked
    float minx = 99999, maxx = -99999;
    float miny = 99999, maxy = -99999;

    for (int i = 0; i < frustumPoints.size(); i++) {
        mathfu::vec3 &frustumPoint = frustumPoints[i];

        minx = std::min(frustumPoint.x, minx);
        maxx = std::max(frustumPoint.x, maxx);
        miny = std::min(frustumPoint.y, miny);
        maxy = std::max(frustumPoint.y, maxy);
    }
    int adt_x_min = worldCoordinateToAdtIndex(maxy);
    int adt_x_max = worldCoordinateToAdtIndex(miny);

    int adt_y_min = worldCoordinateToAdtIndex(maxx);
    int adt_y_max = worldCoordinateToAdtIndex(minx);

    int adt_global_x = worldCoordinateToGlobalAdtChunk(cameraPos.y);
    int adt_global_y = worldCoordinateToGlobalAdtChunk(cameraPos.x);

//    //FOR DEBUG
//    adt_x_min = worldCoordinateToAdtIndex(cameraPos.y) - 2;
//    adt_x_max = adt_x_min + 4;
//    adt_y_min = worldCoordinateToAdtIndex(cameraPos.x) - 2;
//    adt_y_max = adt_y_min + 4;

//    for (int i = 0; i < 64; i++)
//        for (int j = 0; j < 64; j++) {
//            if (this->m_wdtfile->mapTileTable->mainInfo[i][j].Flag_AllWater) {
//                std::cout << AdtIndexToWorldCoordinate(j) <<" "<<  AdtIndexToWorldCoordinate(i) << std::endl;
//            }
//        }

//    std::cout << AdtIndexToWorldCoordinate(adt_y_min) <<" "<<  AdtIndexToWorldCoordinate(adt_x_min) << std::endl;

    m_wdlObject->checkFrustumCulling(
        cameraPos, frameData->exteriorView.frustumPlanes[0],
        frustumPoints,
        hullLines,
        lookAtMat4, m2ObjectsCandidates, wmoCandidates);

    for (int i = adt_x_min; i <= adt_x_max; i++) {
        for (int j = adt_y_min; j <= adt_y_max; j++) {
            if ((i < 0) || (i > 64)) continue;
            if ((j < 0) || (j > 64)) continue;

            AdtObject *adtObject = this->mapTiles[i][j];
            if (adtObject != nullptr) {

                bool result = adtObject->checkFrustumCulling(
                    cameraPos,
                    adt_global_x,
                    adt_global_y,
                    frameData->exteriorView.frustumPlanes[0], //TODO:!
                    frustumPoints,
                    hullLines,
                    lookAtMat4, m2ObjectsCandidates, wmoCandidates);
                if (result) {
                    frameData->exteriorView.drawnADTs.push_back(adtObject);
                    frameData->adtArray.push_back(adtObject);
                }
            } else if (true){ //(m_wdtfile->mapTileTable->mainInfo[j][i].Flag_HasADT > 0) {
                std::string adtFileTemplate = "world/maps/"+mapName+"/"+mapName+"_"+std::to_string(i)+"_"+std::to_string(j);
                adtObject = new AdtObject(m_api, adtFileTemplate, mapName, i, j, m_wdtfile);

                adtObject->setMapApi(this);
                this->mapTiles[i][j] = adtObject;
            }
        }
    }

    //Sort and delete duplicates
    std::sort( wmoCandidates.begin(), wmoCandidates.end() );
    wmoCandidates.erase( unique( wmoCandidates.begin(), wmoCandidates.end() ), wmoCandidates.end() );

    //Frustum cull
    for (auto it = wmoCandidates.begin(); it != wmoCandidates.end(); ++it) {
        WmoObject *wmoCandidate = *it;

        if (!wmoCandidate->isLoaded()) {
            frameData->wmoArray.push_back(wmoCandidate);
            continue;
        }
        wmoCandidate->resetTraversedWmoGroups();
        if (wmoCandidate->startTraversingWMOGroup(
            cameraPos,
            projectionModelMat,
            -1,
            0,
            viewRenderOrder,
            false,
            frameData->interiorViews,
            frameData->exteriorView)) {

            frameData->wmoArray.push_back(wmoCandidate);
        }
    }

    //Sort and delete duplicates
    std::sort( m2ObjectsCandidates.begin(), m2ObjectsCandidates.end() );
    m2ObjectsCandidates.erase( unique( m2ObjectsCandidates.begin(), m2ObjectsCandidates.end() ), m2ObjectsCandidates.end() );

    //3.2 Iterate over all global WMOs and M2s (they have uniqueIds)
    for (auto &m2ObjectCandidate : m2ObjectsCandidates) {
        bool frustumResult = m2ObjectCandidate->checkFrustumCulling(
            cameraPos,
            frameData->exteriorView.frustumPlanes[0], //TODO:!
            frustumPoints) ;
//        bool frustumResult = true;
        if (frustumResult) {
            frameData->exteriorView.drawnM2s.push_back(m2ObjectCandidate);
            frameData->m2Array.push_back(m2ObjectCandidate);
        }
    }
}
void Map::doPostLoad(WoWFrameData *frameData){
//    if (m_api->getConfig()->getRenderM2()) {
        for (int i = 0; i < frameData->m2Array.size(); i++) {
            M2Object *m2Object = frameData->m2Array[i];
            if (m2Object == nullptr) continue;
            m2Object->doPostLoad();
        }
//    }

    for (auto &wmoObject : frameData->wmoArray) {
        wmoObject->doPostLoad();
    }

    for (auto &adtObject : frameData->adtArray) {
        adtObject->doPostLoad();
    }
};

void Map::update(WoWFrameData *frameData) {
    if (!m_wdtfile->getIsLoaded()) return;

    mathfu::vec3 &cameraVec3 = frameData->m_cameraVec3;
    mathfu::mat4 &frustumMat = frameData->m_perspectiveMatrixForCulling;
    mathfu::mat4 &lookAtMat = frameData->m_lookAtMat4;
    animTime_t deltaTime = frameData->deltaTime;

    Config* config = this->m_api->getConfig();
//    if (config->getRenderM2()) {
        std::for_each(frameData->m2Array.begin(), frameData->m2Array.end(), [deltaTime, &cameraVec3, &lookAtMat](M2Object *m2Object) {
            if (m2Object == nullptr) return;
            m2Object->update(deltaTime, cameraVec3, lookAtMat);
        });
//    }

    for (auto &wmoObject : frameData->wmoArray) {
        wmoObject->update();
    }

    for (auto &adtObject : frameData->adtArray) {
        adtObject->update();
    }

    //2. Calc distance every 100 ms
    std::for_each(frameData->m2Array.begin(), frameData->m2Array.end(), [&cameraVec3](M2Object *m2Object) {
        if (m2Object == nullptr) return;
        m2Object->calcDistance(cameraVec3);
    });

    //6. Check what WMO instance we're in
    this->m_currentInteriorGroups = {};
    this->m_currentWMO = nullptr;

    int bspNodeId = -1;
    int interiorGroupNum = -1;
    int currentWmoGroup = -1;

    //Get center of near plane
    mathfu::vec4 camera4 = mathfu::vec4(cameraVec3, 1.0);
    mathfu::mat4 viewPerspectiveMat = frustumMat*lookAtMat;
    std::vector<mathfu::vec3> frustumPoints = MathHelper::calculateFrustumPointsFromMat(viewPerspectiveMat);
    std::vector<mathfu::vec4> frustumPlanes = MathHelper::getFrustumClipsFromMatrix(viewPerspectiveMat);

    mathfu::vec3 nearPlaneCenter(0,0,0);
    nearPlaneCenter += frustumPoints[0];
    nearPlaneCenter += frustumPoints[1];
    nearPlaneCenter += frustumPoints[6];
    nearPlaneCenter += frustumPoints[7];
    nearPlaneCenter = nearPlaneCenter * 0.25f;

    //Get potential WMO
    std::vector<WmoObject*> potentialWmo;
    std::vector<M2Object*> potentialM2;

    int adt_x = worldCoordinateToAdtIndex(cameraVec3.y);
    int adt_y = worldCoordinateToAdtIndex(cameraVec3.x);
    AdtObject *adtObject = this->mapTiles[adt_x][adt_y];
    if (adtObject != nullptr) {
        adtObject->checkReferences(
            camera4,
            frustumPlanes,
            frustumPoints,
            lookAtMat,
            5,
            potentialM2,
            potentialWmo,
            0,
            0,
            16,
            16
        );
    }

    for (auto &checkingWmoObj : potentialWmo) {
        WmoGroupResult groupResult;
        bool result = checkingWmoObj->getGroupWmoThatCameraIsInside(mathfu::vec4(cameraVec3, 1), groupResult);

        if (result) {
            this->m_currentWMO = checkingWmoObj;
            currentWmoGroup = groupResult.groupIndex;
            if (checkingWmoObj->isGroupWmoInterior(groupResult.groupIndex)) {
                this->m_currentInteriorGroups.push_back(groupResult);
                interiorGroupNum = groupResult.groupIndex;
            } else {
            }

            if (m_api->getDB2WmoAreaTable()->getIsLoaded()) {
                DBWmoAreaTableRecord areaTableRecord;
                if (m_api->getDB2WmoAreaTable()->findRecord(
                    this->m_currentWMO->getWmoHeader()->wmoID,
                    this->m_currentWMO->getNameSet(),
                    groupResult.WMOGroupID,
                    areaTableRecord
                )) {
                    config->setAreaName(areaTableRecord.AreaName);
                }

            }

            bspNodeId = groupResult.nodeId;
            break;
        }
    }

//    //7. Get AreaId and Area Name
//    var currentAreaName = '';
//    var wmoAreaTableDBC = this.sceneApi.dbc.getWmoAreaTableDBC();
//    var areaTableDBC = this.sceneApi.dbc.getAreaTableDBC();
//    var areaRecord = null;
//    if (wmoAreaTableDBC && areaTableDBC) {
//        if (this.currentWMO) {
//            var wmoFile = this.currentWMO.wmoObj;
//            var wmoId = wmoFile.wmoId;
//            var wmoGroupId = this.currentWMO.wmoGroupArray[currentWmoGroup].wmoGeom.wmoGroupFile.mogp.groupID;
//            var nameSetId = this.currentWMO.nameSet;
//
//            var wmoAreaTableRecord = wmoAreaTableDBC.findRecord(wmoId, nameSetId, wmoGroupId);
//            if (wmoAreaTableRecord) {
//                var areaRecord = areaTableDBC[wmoAreaTableRecord.areaId];
//                if (wmoAreaTableRecord) {
//                    if (wmoAreaTableRecord.name == '') {
//                        var areaRecord = areaTableDBC[wmoAreaTableRecord.areaId];
//                        if (areaRecord) {
//                            currentAreaName = areaRecord.name
//                        }
//                    } else {
//                        currentAreaName = wmoAreaTableRecord.name;
//                    }
//                }
//            }
//        }
//        if (currentAreaName == '' && mcnkChunk) {
//            var areaRecord = areaTableDBC[mcnkChunk.areaId];
//            if (areaRecord) {
//                currentAreaName = areaRecord.name
//            }
//        }
//    }
//
    //8. Check fog color every 2 seconds
    bool fogRecordWasFound = false;
    if (this->m_currentTime + frameData->deltaTime - this->m_lastTimeLightCheck > 30) {
        mathfu::vec3 ambientColor = mathfu::vec3(0.0,0.0,0.0);
        mathfu::vec3 directColor = mathfu::vec3(0.0,0.0,0.0);
        mathfu::vec3 endFogColor = mathfu::vec3(0.0,0.0,0.0);

        if (this->m_currentWMO != nullptr) {
            CImVector sunFogColor;
            fogRecordWasFound = this->m_currentWMO->checkFog(cameraVec3, sunFogColor);
            if (fogRecordWasFound) {
                endFogColor =
                  mathfu::vec3((sunFogColor.r & 0xFF) / 255.0f,
                               ((sunFogColor.g ) & 0xFF) / 255.0f,
                               ((sunFogColor.b ) & 0xFF) / 255.0f);
            }
        }

        if (m_api->getDB2Light()->getIsLoaded() && m_api->getDB2LightData()->getIsLoaded()) {
            //Check areaRecord

            //Query Light Record
            std::vector<FoundLightRecord> result = m_api->getDB2Light()->findRecord(m_mapId, cameraVec3);
            float totalSummator = 0.0f;

            for (int i = 0; i < result.size() && totalSummator < 1.0f; i++) {
                DB2LightDataRecord lightData = m_api->getDB2LightData()->getRecord(result[i].lightRec.lightParamsID[0]);
                float blendPart = result[i].blendAlpha < 1.0f ? result[i].blendAlpha : 1.0f;
                blendPart = blendPart + totalSummator < 1.0f ? blendPart : 1.0f - totalSummator;

                ambientColor = ambientColor +
                    mathfu::vec3((lightData.ambientColor & 0xFF) / 255.0f,
                    ((lightData.ambientColor >> 8) & 0xFF) / 255.0f,
                    ((lightData.ambientColor >> 16) & 0xFF) / 255.0f) * blendPart ;
                directColor = directColor +
                    mathfu::vec3((lightData.directColor & 0xFF) / 255.0f,
                        ((lightData.directColor >> 8) & 0xFF) / 255.0f,
                        ((lightData.directColor >> 16) & 0xFF) / 255.0f) * blendPart ;

                if (!fogRecordWasFound) {
                    endFogColor = endFogColor +
                                  mathfu::vec3((lightData.sunFogColor & 0xFF) / 255.0f,
                                               ((lightData.sunFogColor >> 8) & 0xFF) / 255.0f,
                                               ((lightData.sunFogColor >> 16) & 0xFF) / 255.0f) * blendPart;
                }

                totalSummator += blendPart;
            }
//            DB2LightDataRecord lightData = m_api->getDB2LightData()->getRecord(20947);

//            for (int jk = 0; jk < 10000; jk++) {
//                DB2LightDataRecord lightData = m_api->getDB2LightData()->getRecord(20947+jk);
//                std::cout << "lightParamID = " << lightData.lightParamID << ", time = " << lightData.time << std::endl;
//                if (lightData.lightParamID == 379) {
//                    std::cout << "found :D" << std::endl;
//                }
//            }
            config->setAmbientColor(
                ambientColor.x,
                ambientColor.y,
                ambientColor.z,
                1.0
            );
            config->setSunColor(
                directColor.x,
                directColor.y,
                directColor.z,
                1.0
            );
        }

        config->setFogColor(
                endFogColor.x,
                endFogColor.y,
                endFogColor.z,
                1.0
        );

        this->m_lastTimeLightCheck = this->m_currentTime;
    }
    this->m_currentTime += frameData->deltaTime;
}

M2Object *Map::getM2Object(std::string fileName, SMDoodadDef &doodadDef) {
    M2Object * m2Object = m_m2MapObjects.get(doodadDef.uniqueId);
    if (m2Object == nullptr) {
        m2Object = new M2Object(m_api);
        m2Object->setLoadParams(0, {}, {});
        m2Object->setModelFileName(fileName);
        m2Object->createPlacementMatrix(doodadDef);
        m2Object->calcWorldPosition();
        m_m2MapObjects.put(doodadDef.uniqueId, m2Object);
    }
    return m2Object;
}

M2Object *Map::getM2Object(int fileDataId, SMDoodadDef &doodadDef) {
    M2Object * m2Object = m_m2MapObjects.get(doodadDef.uniqueId);
    if (m2Object == nullptr) {
        m2Object = new M2Object(m_api);
        m2Object->setLoadParams(0, {}, {});
        m2Object->setModelFileId(fileDataId);
        m2Object->createPlacementMatrix(doodadDef);
        m2Object->calcWorldPosition();
        m_m2MapObjects.put(doodadDef.uniqueId, m2Object);
    }
    return m2Object;
}


WmoObject *Map::getWmoObject(std::string fileName, SMMapObjDef &mapObjDef) {
    WmoObject * wmoObject = m_wmoMapObjects.get(mapObjDef.uniqueId);
    if (wmoObject == nullptr) {
        wmoObject = new WmoObject(m_api);
        wmoObject->setLoadingParam(mapObjDef);
        wmoObject->setModelFileName(fileName);
        m_wmoMapObjects.put(mapObjDef.uniqueId, wmoObject);
    }
    return wmoObject;
}

WmoObject *Map::getWmoObject(std::string fileName, SMMapObjDefObj1 &mapObjDef) {
    WmoObject * wmoObject = m_wmoMapObjects.get(mapObjDef.uniqueId);
    if (wmoObject == nullptr) {
        wmoObject = new WmoObject(m_api);
        wmoObject->setLoadingParam(mapObjDef);
        wmoObject->setModelFileName(fileName);
        m_wmoMapObjects.put(mapObjDef.uniqueId, wmoObject);
    }
    return wmoObject;
}

WmoObject *Map::getWmoObject(int fileDataId, SMMapObjDefObj1 &mapObjDef) {
    WmoObject * wmoObject = m_wmoMapObjects.get(mapObjDef.uniqueId);
    if (wmoObject == nullptr) {
        wmoObject = new WmoObject(m_api);
        wmoObject->setLoadingParam(mapObjDef);
        wmoObject->setModelFileId(fileDataId);
        m_wmoMapObjects.put(mapObjDef.uniqueId, wmoObject);
    }
    return wmoObject;
}

void Map::collectMeshes(WoWFrameData *frameData) {
    frameData->renderedThisFrame = std::vector<HGMesh>();

    // Put everything into one array and sort
    std::vector<GeneralView *> vector;
    for (auto & interiorView : frameData->interiorViews) {
        if (interiorView.viewCreated) {
            vector.push_back(&interiorView);
        }
    }
    if (frameData->exteriorView.viewCreated) {
        vector.push_back(&frameData->exteriorView);
    }

    if (m_api->getConfig()->getRenderWMO()) {
        for (auto &view : vector) {
            view->collectMeshes(frameData->renderedThisFrame);
        }
    }

    std::vector<M2Object *> m2ObjectsRendered;
    for (auto &view : vector) {
        std::copy(view->drawnM2s.begin(),view->drawnM2s.end(), std::back_inserter(m2ObjectsRendered));
    }
    std::sort( m2ObjectsRendered.begin(), m2ObjectsRendered.end() );
    m2ObjectsRendered.erase( unique( m2ObjectsRendered.begin(), m2ObjectsRendered.end() ), m2ObjectsRendered.end() );

    if (m_api->getConfig()->getRenderM2()) {
        for (auto &m2Object : m2ObjectsRendered) {
            if (m2Object == nullptr) continue;
            m2Object->collectMeshes(frameData->renderedThisFrame, m_viewRenderOrder);
            m2Object->drawParticles(frameData->renderedThisFrame, m_viewRenderOrder);
        }
    }

    std::sort(frameData->renderedThisFrame.begin(),
              frameData->renderedThisFrame.end(),
              IDevice::sortMeshes
    );
};

void Map::draw(WoWFrameData *frameData) {
    m_api->getDevice()->drawMeshes(frameData->renderedThisFrame);
}
