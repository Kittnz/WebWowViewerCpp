//
// Created by deamon on 01.08.17.
//

#ifndef WOWVIEWERLIB_ADTFILE_H
#define WOWVIEWERLIB_ADTFILE_H
#include "helper/ChunkFileReader.h"
#include "header/adtFileHeader.h"
#include "header/wdtFileHeader.h"

struct mcnkStruct_t {
    MCVT *mcvt = nullptr;
    MCLV *mclv = nullptr;
    MCCV *mccv = nullptr;
    SMNormal *mcnr = nullptr;
    SMLayer *mcly = nullptr;
    int mclyCnt = 0;

    struct {uint32_t *doodad_refs; uint32_t *object_refs; } mcrf;

    uint32_t *mcrd_doodad_refs = nullptr;
    int mcrd_doodad_refs_len = -1;

    uint32_t *mcrw_object_refs = nullptr;
    int mcrw_object_refs_len = -1;


    uint8_t *mcal;
};

class AdtFile {
public:
    AdtFile() {};

    std::vector<uint8_t> processTexture(const MPHDFlags &wdtObjFlags, int i);
    void process(std::vector<unsigned char> &adtFile, std::string &fileName);
    bool getIsLoaded() { return m_loaded; };
    void setIsMain(bool isMain) { m_mainAdt = isMain; };
public:
    SMMapHeader* mhdr;

    struct {
        SMChunkInfo chunkInfo[16][16];
    } *mcins;

    std::vector<std::string> textureNames;

    char *doodadNamesField;
    int doodadNamesFieldLen;

    char *wmoNamesField;
    int wmoNamesFieldLen;

    SMTextureParams *mtxp = 0;
    int mtxp_len = 0;

    SMDoodadDef * doodadDef = nullptr;
    int doodadDef_len = -1;

    SMMapObjDef * mapObjDef = nullptr;
    int mapObjDef_len = -1;

    SMDoodadDef * doodadDefObj1 = nullptr;
    int doodadDefObj1_len = -1;

    SMMapObjDefObj1 * mapObjDefObj1 = nullptr;
    int mapObjDefObj1_len = -1;

    uint32_t *mldd = nullptr;
    int mldd_len = -1;

    float *floatDataBlob;
    int floatDataBlob_len = -1;

    uint16_t *mvli_indicies = nullptr;
    int mvli_len = -1;

    uint16_t *mlsi_indicies = nullptr;
    int mlsi_len = -1;

    MLLL * mllls = nullptr;
    int mlll_len = -1;

    MLND * mlnds = nullptr;
    int mlnd_len = -1;

    SMLodLevelPerObject * lod_levels_for_objects = nullptr;

    uint32_t *mmid;
    int mmid_length;
    uint32_t *mwid;
    int mwid_length;

    int mcnkRead = -1;
    SMChunk mapTile[16*16];
    mcnkStruct_t mcnkStructs[16*16];
    int mcnkMap[16][16] = {{-1}};

    std::vector<int16_t> strips;
    std::vector<int> stripOffsets;
private:
    bool m_loaded = false;
    bool m_mainAdt = false;

    void createTriangleStrip();
    static chunkDef<AdtFile> adtFileTable;
    std::vector<unsigned char> m_adtFile;
};

#endif //WOWVIEWERLIB_ADTFILE_H
