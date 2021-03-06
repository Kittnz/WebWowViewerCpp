//
// Created by deamon on 26.06.17.
//

#include <cmath>
#include <iostream>
#include "animationManager.h"
#include "../algorithms/animate.h"
#include "../persistance/header/M2FileHeader.h"
#include "mathfu/glsl_mappings.h"

AnimationManager::AnimationManager(M2Data* m2File) {
    this->m_m2File = m2File;

    this->mainAnimationId = 0;
    this->mainAnimationIndex = 0;

    this->currentAnimationIndex = 0;
    this->currentAnimationTime = 0;
    this->currentAnimationPlayedTimes = 0;

    this->nextSubAnimationIndex = -1;
    this->nextSubAnimationTime = 0;

    this->firstCalc = true;

    this->initBonesIsCalc();
    this->initBlendMatrices();
    this->initGlobalSequenceTimes();
    this->calculateBoneTree();

    if (!this->setAnimationId(0, false)) { // try Stand(0) animation
        this->setAnimationId(147, false); // otherwise try Closed(147) animation
    }
}

void AnimationManager::initBonesIsCalc() {
    bonesIsCalculated = std::vector<bool>((unsigned long) m_m2File->bones.size);

    for (int i = 0; i <  m_m2File->bones.size; i++) {
        bonesIsCalculated[i] = false;
    }
}

void AnimationManager::initBlendMatrices() {
    unsigned long matCount = (unsigned long) std::max(m_m2File->bones.size, m_m2File->texture_transforms.size);
    blendMatrixArray = std::vector<mathfu::mat4>(matCount, mathfu::mat4::Identity());
}

void AnimationManager::initGlobalSequenceTimes() {
    globalSequenceTimes = std::vector<animTime_t>(
            (unsigned long) (m_m2File->global_loops.size > 0 ? m_m2File->global_loops.size : 0));

    for (int i = 0; i < globalSequenceTimes.size(); i++) {
        globalSequenceTimes[i] = 0;
    }
}

void AnimationManager::calculateBoneTree() {
    this->childBonesLookup = std::vector<std::vector<int>>(m_m2File->bones.size);
    for (int i = 0; i < m_m2File->bones.size; i++) {
        for (int j = 0; j < m_m2File->bones.size; j++) {
            if (m_m2File->bones[j]->parent_bone == i) {
                childBonesLookup[i].push_back(j);
            }
        }
    }
}

bool AnimationManager::setAnimationId(int animationId, bool reset) {
    int animationIndex = -1;
    if ((m_m2File->sequence_lookups.size == 0) && (m_m2File->sequences.size > 0)) {
        for (int i = 0; i < m_m2File->sequences.size; i++) {
            const M2Sequence* animationRecord = m_m2File->sequences[i];
            if (animationRecord->id == animationId) {
                animationIndex = i;
                break;
            }
        }
    } else if (animationId < m_m2File->sequence_lookups.size) {
        animationIndex = *m_m2File->sequence_lookups[animationId];
    }
    if ((animationIndex > - 1)&& (reset || (animationIndex != this->mainAnimationIndex) )) {
        //Reset animation
        this->mainAnimationId = animationId;
        this->mainAnimationIndex = animationIndex;

        this->currentAnimationIndex = animationIndex;
        this->currentAnimationTime = 0;
        this->currentAnimationPlayedTimes = 0;

        this->nextSubAnimationIndex = -1;
        this->nextSubAnimationTime = 0;
        this->nextSubAnimationActive = false;

        this->firstCalc = true;
    }
    return (animationIndex > -1);
}

void blendMatrices(std::vector<mathfu::mat4> &origMat, std::vector<mathfu::mat4> &blendMat, int count, float blendAlpha) {
//Actual blend
    for (int i = 0; i < count; i++) {
//        mathfu::mat4 &blendTransformMatrix = blendMat[i];
//        mathfu::mat4 &tranformMat = origMat[i];
        origMat[i] = ((blendMat[i] - origMat[i]) * (const float &) (1.0 - blendAlpha)) + origMat[i];
    }
}

template <typename T>
inline void calcAnimationTransform(
        mathfu::mat4 &tranformMat,
        mathfu::mat4 *billboardMatrix,
        mathfu::vec4 &pivotPoint,
        mathfu::vec4 &negatePivotPoint,
        std::vector<animTime_t> &globalSequenceTimes,
        bool &isAnimated,
        M2Track<C3Vector> &translationTrack,
        M2Track<T> &rotationTrack,
        M2Track<C3Vector> &scaleTrack,
        M2Data * m2Data,
        int animationIndex,
        animTime_t time
        ) {
    const M2Sequence* animationRecord = m2Data->sequences[animationIndex];
    tranformMat = tranformMat * mathfu::mat4::FromTranslationVector(pivotPoint.xyz());
//
    if (translationTrack.values.size > 0) {
        mathfu::vec4 defaultValue = mathfu::vec4(0,0,0,0);
        mathfu::vec4 transVec = animateTrack<C3Vector, mathfu::vec4>(
                time,
                animationRecord->duration,
                animationIndex,
                translationTrack,
                m2Data->global_loops,
                globalSequenceTimes,
                defaultValue
        );

        tranformMat = tranformMat * mathfu::mat4::FromTranslationVector(transVec.xyz());
        isAnimated = true;
    }

    if (billboardMatrix != nullptr) {
        tranformMat = tranformMat * *billboardMatrix;
    } else if (rotationTrack.values.size > 0) {
        mathfu::quat defaultValue = mathfu::quat(1,0,0,0);
        mathfu::quat quaternionResult = animateTrack<T, mathfu::quat>(
            time,
            animationRecord->duration,
            animationIndex,
            rotationTrack,
            m2Data->global_loops,
            globalSequenceTimes,
            defaultValue);

        tranformMat = tranformMat * quaternionResult.ToMatrix4();
        isAnimated = true;
    }

    if (scaleTrack.values.size > 0) {
        mathfu::vec4 defaultValue = mathfu::vec4(1,1,1,1);
        mathfu::vec4 scaleResult = animateTrack<C3Vector, mathfu::vec4>(
                time,
                animationRecord->duration,
                animationIndex,
                scaleTrack,
                m2Data->global_loops,
                globalSequenceTimes,
                defaultValue);

        tranformMat = tranformMat * mathfu::mat4::FromScaleVector(scaleResult.xyz());
        isAnimated = true;
    }
    tranformMat = tranformMat * mathfu::mat4::FromTranslationVector(negatePivotPoint.xyz());
}


void AnimationManager::calcAnimMatrixes (std::vector<mathfu::mat4> &textAnimMatrices, int animationIndex, animTime_t time) {

    mathfu::vec4 pivotPoint(0.5, 0.5, 0, 0);
    mathfu::vec4 negatePivotPoint = -pivotPoint;

    const M2Sequence* animationRecord = m_m2File->sequences[animationIndex];
    for (int i = 0; i < m_m2File->texture_transforms.size; i++) {
        M2TextureTransform *textAnimData = m_m2File->texture_transforms[i];

        textAnimMatrices[i] = mathfu::mat4::Identity();
        bool isAnimated;
        calcAnimationTransform(textAnimMatrices[i],
           nullptr,
           pivotPoint, negatePivotPoint,
           this->globalSequenceTimes,
           isAnimated,
           textAnimData->translation,
           textAnimData->rotation,
           textAnimData->scaling,
           m_m2File,
           animationIndex, time
        );
        this->isAnimated = isAnimated ? isAnimated : this->isAnimated;
    }
}

void calcBoneBillboardMatrix(
                mathfu::mat4 *billboardMatrix,
                std::vector<mathfu::mat4> &boneMatrices,
                M2CompBone *boneDefinition,
                int parentBone,
                mathfu::vec4 &pivotPoint,
                mathfu::vec3 &cameraPosInLocal) {

    mathfu::vec3 modelForward;
    mathfu::vec3 modelUp;
    mathfu::vec3 modelRight;

    mathfu::vec4 cameraPoint = mathfu::vec4(cameraPosInLocal, 0);
    if (parentBone >= 0) {
        mathfu::mat4 *parentMatrix = &boneMatrices[parentBone];
        cameraPoint = parentMatrix->Inverse() * cameraPoint;
    }

    cameraPoint = cameraPoint - pivotPoint;

    modelForward = cameraPoint.xyz().Normalized();

    if ((boneDefinition->flags.cylindrical_billboard_lock_z) > 0) {
        //Cylindrical billboard
        modelUp = mathfu::vec3(0, 0, 1);

        modelRight = mathfu::vec3::CrossProduct(modelUp, modelForward).Normalized();
        modelForward = mathfu::vec3::CrossProduct(modelRight, modelUp).Normalized();

        modelRight = mathfu::vec3::CrossProduct(modelUp, modelForward).Normalized();
    } else {
        //Spherical billboard
        modelRight = mathfu::vec3::CrossProduct(mathfu::vec3(0, 0, 1), modelForward).Normalized();
        modelUp = mathfu::vec3::CrossProduct(modelForward, modelRight).Normalized();
    }


    *billboardMatrix = mathfu::mat4(
            modelForward[0], modelForward[1], modelForward[2], 0,
            modelRight[0], modelRight[1], modelRight[2], 0,
            modelUp[0], modelUp[1], modelUp[2], 0,
            0, 0, 0, 1
    );
}

void
AnimationManager::calcBoneMatrix(std::vector<mathfu::mat4> &boneMatrices, int boneIndex, int animationIndex, animTime_t time,
                                 mathfu::vec3 cameraPosInLocal) {
    if (this->bonesIsCalculated[boneIndex]) return;

    const M2Sequence* animationRecord = m_m2File->sequences[animationIndex];
    M2CompBone *boneDefinition = m_m2File->bones[boneIndex];

    int parentBone = boneDefinition->parent_bone;

    /* 2. Prepare bone part of animation process */
    boneMatrices[boneIndex] = mathfu::mat4::Identity();

    if (parentBone >= 0) {
        this->calcBoneMatrix(boneMatrices, parentBone, animationIndex, time, cameraPosInLocal);
        boneMatrices[boneIndex] = boneMatrices[boneIndex] * boneMatrices[parentBone];
    }

    if ((boneDefinition->flags.transformed |
        boneDefinition->flags.spherical_billboard |
        boneDefinition->flags.cylindrical_billboard_lock_x |
        boneDefinition->flags.cylindrical_billboard_lock_y |
        boneDefinition->flags.cylindrical_billboard_lock_z) == 0) {
        this->bonesIsCalculated[boneIndex] = true;
        return;
    }

    C3Vector pP = boneDefinition->pivot;
    mathfu::vec4 pivotPoint = mathfu::vec4(pP.x, pP.y, pP.z, 0);
    mathfu::vec4 negatePivotPoint = -mathfu::vec4(pP.x, pP.y, pP.z, 0);


    /* 2.1 Calculate billboard matrix if needed */
    mathfu::mat4 *billboardMatrix = nullptr;

    if ((boneDefinition->flags.spherical_billboard) | (boneDefinition->flags.cylindrical_billboard_lock_z) != 0) {
        //From http://gamedev.stackexchange.com/questions/112270/calculating-rotation-matrix-for-an-object-relative-to-a-planets-surface-in-monog
        billboardMatrix = new mathfu::mat4();

        calcBoneBillboardMatrix(billboardMatrix, boneMatrices, boneDefinition, parentBone, pivotPoint,
                                                       cameraPosInLocal);
        this->isAnimated = true;
    }

    /* 3. Calculate matrix */
    calcAnimationTransform(boneMatrices[boneIndex],
           billboardMatrix,
           pivotPoint, negatePivotPoint,
           this->globalSequenceTimes,
           isAnimated,
           boneDefinition->translation,
           boneDefinition->rotation,
           boneDefinition->scaling,
           m_m2File,
           animationIndex, time);

    if (billboardMatrix != nullptr) {
        delete billboardMatrix;
    }

    this->bonesIsCalculated[boneIndex] = true;
}

void AnimationManager::calcChildBones(std::vector<mathfu::mat4> &boneMatrices, int boneIndex,
                                      int animationIndex, animTime_t time, mathfu::vec3 cameraPosInLocal) {
    std::vector<int> *childBones = &this->childBonesLookup[boneIndex];
    for (int i = 0; i < childBones->size(); i++) {
        int childBoneIndex = (*childBones)[i];
        this->bonesIsCalculated[childBoneIndex] = false;
        this->calcBoneMatrix(boneMatrices, childBoneIndex, animationIndex, time, cameraPosInLocal);
        this->calcChildBones(boneMatrices, childBoneIndex, animationIndex, time, cameraPosInLocal);
    }
}
std::string dumpMatrix(mathfu::mat4 &mat4) {
    return "[ "+
           std::to_string(mat4[0])+" "+std::to_string(mat4[1])+" "+std::to_string(mat4[2])+" "+std::to_string(mat4[3])+"\n"+
           std::to_string(mat4[4])+" "+std::to_string(mat4[5])+" "+std::to_string(mat4[6])+" "+std::to_string(mat4[7])+"\n"+
           std::to_string(mat4[8])+" "+std::to_string(mat4[9])+" "+std::to_string(mat4[10])+" "+std::to_string(mat4[11])+"\n"+
           std::to_string(mat4[12])+" "+std::to_string(mat4[13])+" "+std::to_string(mat4[14])+" "+std::to_string(mat4[15])+"\n";
}
void AnimationManager::calcBones (std::vector<mathfu::mat4> &boneMatrices, int animation, animTime_t time, mathfu::vec3 &cameraPosInLocal) {


    if (this->firstCalc || this->isAnimated) {
        //Animate everything with standard animation
        for (int i = 0; i < m_m2File->bones.size; i++) {
            this->calcBoneMatrix(boneMatrices, i, animation, time, cameraPosInLocal);
//            std::cout << "boneMatrices[" << std::to_string(i) << "] = " << dumpMatrix(boneMatrices[i]) << std::endl;
        }

        /* Animate mouth */
        /*
         if (m2File.keyBoneLookup[6] > -1) { // BONE_HEAD = 6
         var boneId = m2File.keyBoneLookup[6];
         this.calcBoneMatrix(boneId, this.bones[boneId], animation, time, cameraPos, invPlacementMat);
         }
         if (m2File.keyBoneLookup[7] > -1) { // BONE_JAW = 7
         var boneId = m2File.keyBoneLookup[7];
         this.calcBoneMatrix(boneId, this.bones[boneId], animation, time, cameraPos, invPlacementMat);
         }
         */

        int closedHandAnimation = -1;
        if (m_m2File->sequence_lookups.size > 15 && *m_m2File->sequence_lookups[15] > 0) { //ANIMATION_HANDSCLOSED = 15
            closedHandAnimation = *m_m2File->sequence_lookups[15];
        }

        if (closedHandAnimation >= 0) {
            if (this->leftHandClosed && m_m2File->key_bone_lookup.size > (13+5)) {
                for (int j = 0; j < 5; j++) {
                    if (*m_m2File->key_bone_lookup[13 + j] > -1) { // BONE_LFINGER1 = 13
                        int boneId = *m_m2File->key_bone_lookup[13 + j];
                        this->bonesIsCalculated[boneId] = false;
                        this->calcBoneMatrix(boneMatrices, boneId, closedHandAnimation, 1, cameraPosInLocal);
                        this->calcChildBones(boneMatrices, boneId, closedHandAnimation, 1, cameraPosInLocal);
                    }
                }
            }
            if (this->rightHandClosed && m_m2File->key_bone_lookup.size > (8+5)) {
                for (int j = 0; j < 5; j++) {
                    if (*m_m2File->key_bone_lookup[8 + j] > -1) { // BONE_RFINGER1 = 8
                        int boneId = *m_m2File->key_bone_lookup[8 + j];
                        this->bonesIsCalculated[boneId] = false;
                        this->calcBoneMatrix(boneMatrices, boneId, closedHandAnimation, 1, cameraPosInLocal);
                        this->calcChildBones(boneMatrices, boneId, closedHandAnimation, 1, cameraPosInLocal);
                    }
                }
            }
        }
    }
    this->firstCalc = false;
}


static bool dump = false;

void AnimationManager::update(animTime_t deltaTime, mathfu::vec3 cameraPosInLocal, std::vector<mathfu::mat4> &bonesMatrices,
                              std::vector<mathfu::mat4> &textAnimMatrices,
                              std::vector<mathfu::vec4> &subMeshColors,
                              std::vector<float> &transparencies,
                              std::vector<M2LightResult> &lights,
                              std::vector<ParticleEmitter *> &particleEmitters

        /*cameraDetails, , particleEmitters*/) {
    if (m_m2File->sequences.size <= 0) return;

    const M2Sequence* mainAnimationRecord = m_m2File->sequences[this->mainAnimationIndex];
    const M2Sequence* currentAnimationRecord = m_m2File->sequences[this->currentAnimationIndex];

    this->currentAnimationTime += deltaTime;
    //Update global sequences
    int minGlobal = 0; //12;
    int maxGlobal = (int) this->globalSequenceTimes.size();
//    int minGlobal = 0;
//    int maxGlobal = 1;

    for (int i = minGlobal; i < maxGlobal; i++) {
        if (m_m2File->global_loops[i]->timestamp > 0) { // Global sequence values can be 0's
            this->globalSequenceTimes[i] += deltaTime ;
            this->globalSequenceTimes[i] = fmod(this->globalSequenceTimes[i], (float)m_m2File->global_loops[i]->timestamp);
        }
    }

    /* Pick next animation if there is one and no next animation was picked before */
    const M2Sequence* subAnimRecord = nullptr;
    if (this->nextSubAnimationIndex < 0 && mainAnimationRecord->variationNext > -1) {
        //if (currentAnimationPlayedTimes)
        int probability = ((float)rand() / (float)RAND_MAX) * 0x7FFF;
        int calcProb = 0;

        /* First iteration is out of loop */
        auto currentSubAnimIndex = this->mainAnimationIndex;
        subAnimRecord = m_m2File->sequences[currentSubAnimIndex];
        calcProb += subAnimRecord->frequency;
        while ((calcProb < probability) && (subAnimRecord->variationNext > -1)) {
            currentSubAnimIndex = subAnimRecord->variationNext;
            subAnimRecord = m_m2File->sequences[currentSubAnimIndex];

            calcProb += subAnimRecord->frequency;
        }

        this->nextSubAnimationIndex = currentSubAnimIndex;
        this->nextSubAnimationTime = 0;
    }

    animTime_t currAnimLeft = currentAnimationRecord->duration - this->currentAnimationTime;

    /*if (this->nextSubAnimationActive) {
        this->nextSubAnimationTime += deltaTime;
    }
    */

    int subAnimBlendTime = 0;
    float blendAlpha = 1.0;
    if (this->nextSubAnimationIndex > -1) {
        subAnimRecord = m_m2File->sequences[this->nextSubAnimationIndex];
        subAnimBlendTime = subAnimRecord->blendtime;
    }

    int blendAnimationIndex = -1;
    if ((subAnimBlendTime > 0) && (currAnimLeft < subAnimBlendTime)) {
        this->firstCalc = true;
        this->nextSubAnimationTime = fmod((subAnimBlendTime - currAnimLeft) , (subAnimRecord->duration));
        blendAlpha = currAnimLeft / subAnimBlendTime;
        blendAnimationIndex = this->nextSubAnimationIndex;
    }

    if (this->currentAnimationTime >= currentAnimationRecord->duration) {
        if (this->nextSubAnimationIndex > -1) {
            this->currentAnimationIndex = this->nextSubAnimationIndex;
            this->currentAnimationTime = this->nextSubAnimationTime;

            this->firstCalc = true;

            this->nextSubAnimationIndex = -1;
            this->nextSubAnimationActive = false;
        } else if (currentAnimationRecord->duration > 0) {
            this->currentAnimationTime = fmod(this->currentAnimationTime , currentAnimationRecord->duration);
        }
    }


    /* Update animated values */

    this->calcAnimMatrixes(textAnimMatrices, this->currentAnimationIndex, this->currentAnimationTime);
    if (blendAnimationIndex > -1) {
        this->calcAnimMatrixes(this->blendMatrixArray, blendAnimationIndex, this->nextSubAnimationTime);
        blendMatrices(textAnimMatrices, this->blendMatrixArray, m_m2File->texture_transforms.size, blendAlpha);
    }

    for (int i = 0; i < m_m2File->bones.size; i++) {
        this->bonesIsCalculated[i] = false;
    }
    this->calcBones(bonesMatrices, this->currentAnimationIndex, this->currentAnimationTime, cameraPosInLocal);
    if (blendAnimationIndex > -1) {
        for (int i = 0; i < m_m2File->bones.size; i++) {
            this->bonesIsCalculated[i] = false;
            this->blendMatrixArray[i] = mathfu::mat4::Identity();

        }

        this->calcBones(this->blendMatrixArray, blendAnimationIndex, this->nextSubAnimationTime, cameraPosInLocal);
        blendMatrices(bonesMatrices, this->blendMatrixArray, m_m2File->bones.size, blendAlpha);
    }

    this->calcSubMeshColors(subMeshColors, this->currentAnimationIndex, this->currentAnimationTime, blendAnimationIndex,
                           this->nextSubAnimationTime, blendAlpha);
    this->calcTransparencies(transparencies, this->currentAnimationIndex, this->currentAnimationTime, blendAnimationIndex,
                            this->nextSubAnimationTime, blendAlpha);

//    this->calcCameras(cameraDetails, this->currentAnimationIndex, this->currentAnimationTime);
    this->calcLights(lights, bonesMatrices, this->currentAnimationIndex, this->currentAnimationTime);
    this->calcParticleEmitters(particleEmitters, bonesMatrices, this->currentAnimationIndex, this->currentAnimationTime);
}

void AnimationManager::calcSubMeshColors(std::vector<mathfu::vec4> &subMeshColors,
                                         int animationIndex,
                                         animTime_t time,
                                         int blendAnimationIndex,
                                         animTime_t blendAnimationTime,
                                         float blendAlpha) {
    M2Array<M2Color> &colors = this->m_m2File->colors;
    const M2Sequence* animationRecord = this->m_m2File->sequences[animationIndex];
    const M2Sequence* blendAnimationRecord = nullptr;
    if (blendAnimationIndex > -1) {
        blendAnimationRecord = this->m_m2File->sequences[blendAnimationIndex];
    }

    static mathfu::vec3 defaultVector(1.0, 1.0, 1.0);
    static float defaultAlpha = 1.0;

    for (int i = 0; i < colors.size; i++) {
        mathfu::vec3 colorResult1 = animateTrack<C3Vector, mathfu::vec3>(time,
                     animationRecord->duration,
                     animationIndex,
                     colors[i]->color,
                     this->m_m2File->global_loops,
                     this->globalSequenceTimes,
                     defaultVector
        );

        //Support for blend
        if (blendAnimationRecord != nullptr) {
            mathfu::vec3 colorResult2 = animateTrack<C3Vector, mathfu::vec3>(
                    blendAnimationTime,
                    blendAnimationRecord->duration,
                    blendAnimationIndex,
                    colors[i]->color,
                    this->m_m2File->global_loops,
                    this->globalSequenceTimes,
                    defaultVector
            );

            colorResult1 = colorResult1 * blendAlpha + (colorResult2 * (1.0f - (float)blendAlpha));
        }

        subMeshColors[i].x = colorResult1.x;
        subMeshColors[i].y = colorResult1.y;
        subMeshColors[i].z = colorResult1.z;

        float resultAlpha1 = animateTrack<fixed16, float>(time,
            animationRecord->duration,
            animationIndex,
            colors[i]->alpha,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultAlpha
        );

        // Support for blend
        if (blendAnimationRecord != nullptr) {
            float resultAlpha2 = animateTrack<fixed16, float>(
                    blendAnimationTime,
                    blendAnimationRecord->duration,
                    blendAnimationIndex,
                    colors[i]->alpha,
                    this->m_m2File->global_loops,
                    this->globalSequenceTimes,
                    defaultAlpha
            );

            resultAlpha1 = resultAlpha1 * blendAlpha + (resultAlpha2 * (1.0f - blendAlpha));
        }

        subMeshColors[i][3] = resultAlpha1;
    }
}

void AnimationManager::calcTransparencies(
        std::vector<float> &transparencies,
        int animationIndex,
        animTime_t time,
        int blendAnimationIndex,
        animTime_t blendAnimationTime,
        double blendAlpha) {

    M2Array<M2TextureWeight> &transparencyRecords = this->m_m2File->texture_weights;
    const M2Sequence* animationRecord = this->m_m2File->sequences[animationIndex];
    const M2Sequence* blendAnimationRecord = nullptr;
    if (blendAnimationIndex > -1) {
        blendAnimationRecord = this->m_m2File->sequences[blendAnimationIndex];
    }

    static float defaultAlpha = 1.0;
    for (int i = 0; i < transparencyRecords.size; i++) {
        float result1 = animateTrack<fixed16, float>(
            time,
            animationRecord->duration,
            animationIndex,
            transparencyRecords[i]->weight,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultAlpha
        );

        // Support for blend
        if (blendAnimationRecord != nullptr) {
            float result2 = animateTrack<fixed16, float>(
                blendAnimationTime,
                blendAnimationRecord->duration,
                blendAnimationIndex,
                transparencyRecords[i]->weight,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultAlpha
            );
            result1 = (result1 * blendAlpha) + ((1 - blendAlpha) * result2);
        }

        transparencies[i] = result1;
    }
}


void AnimationManager::calcLights(std::vector<M2LightResult> &lights, std::vector<mathfu::mat4> &bonesMatrices, int animationIndex, animTime_t animationTime) {
    auto &lightRecords = m_m2File->lights;
    if (lightRecords.size <= 0) return;
    static mathfu::vec3 defaultVector(1.0, 1.0, 1.0);
    static float defaultFloat = 1.0;

    auto animationRecord = m_m2File->sequences[animationIndex];

    for (int i = 0; i < lightRecords.size; i++) {

        M2Light * lightRecord = lightRecords.getElement(i);

        mathfu::vec4 ambient_color =
            mathfu::vec4(animateTrack<C3Vector, mathfu::vec3>(
                    animationTime,
                    animationRecord->duration,
                    animationIndex,
                    lightRecord->ambient_color,
                    this->m_m2File->global_loops,
                    this->globalSequenceTimes,
                    defaultVector
            ), 1.0);

        float ambient_intensity =
                animateTrack<float, float>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->ambient_intensity,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultFloat
                );
        mathfu::vec4 diffuse_color =
                mathfu::vec4(animateTrack<C3Vector, mathfu::vec3>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->diffuse_color,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultVector
                ), 1.0);

//        if (i == 1) {
//            diffuse_color = mathfu::vec4(diffuse_color.x, diffuse_color.z, diffuse_color.y, diffuse_color.w);
//        }
//        if (i == 0) {
////            diffuse_color = mathfu::vec4(diffuse_color.y, diffuse_color.y, diffuse_color.z, diffuse_color.w);
//        }


        float diffuse_intensity =
                animateTrack<float, float>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->diffuse_intensity,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultFloat
                );

        float attenuation_start =
                animateTrack<float, float>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->attenuation_start,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultFloat
                );
        float attenuation_end =
                animateTrack<float, float>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->attenuation_end,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultFloat
                );

        static unsigned char defaultChar = 0;
        unsigned char visibility =
                animateTrack<unsigned char, unsigned char>(
                        animationTime,
                        animationRecord->duration,
                        animationIndex,
                        lightRecord->visibility,
                        this->m_m2File->global_loops,
                        this->globalSequenceTimes,
                        defaultChar
                );

        mathfu::mat4 &boneMat = bonesMatrices[lightRecord->bone];
        C3Vector &pos_vec = lightRecord->position;

        mathfu::vec4 lightWorldPos = boneMat * mathfu::vec4(mathfu::vec3(pos_vec), 1.0);
        lightWorldPos.w = 1.0;

//        if (i == 0) {
//            diffuse_intensity = 1.0;
//        }

        lights[i].ambient_color = ambient_color;
        lights[i].ambient_intensity = ambient_intensity;
        lights[i].ambient_color = ambient_color * ambient_intensity;

        lights[i].diffuse_color = diffuse_color;
        lights[i].diffuse_intensity = diffuse_intensity;
        lights[i].attenuation_start = attenuation_start;
        lights[i].attenuation_end = attenuation_end;
        lights[i].position = lightWorldPos;
        lights[i].visibility = visibility;
    }
}

void AnimationManager::calcCamera(M2CameraResult &camera, int cameraId, mathfu::mat4 &placementMatrix) {
    int animationIndex = this->mainAnimationIndex;
    animTime_t animationTime = this->currentAnimationTime;

    auto &cameraRecords = m_m2File->cameras;
    if (cameraRecords.size <= 0) return;
    static mathfu::vec3 defaultVector(0.0, 0.0, 0.0);
    static float defaultFloat = 1.0;

    auto animationRecord = m_m2File->sequences[animationIndex];
    M2Camera * cameraRecord = cameraRecords.getElement(cameraId);

    mathfu::vec3 currentPosition =
        animateSplineTrack<C3Vector, mathfu::vec3>(
            animationTime,
            animationRecord->duration,
            animationIndex,
            cameraRecord->positions,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultVector
    );
    currentPosition = currentPosition + mathfu::vec3(cameraRecord->position_base);

    mathfu::vec3 currentTarget =
        animateSplineTrack<C3Vector, mathfu::vec3>(
            animationTime,
            animationRecord->duration,
            animationIndex,
            cameraRecord->target_position,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultVector
        );

    currentTarget = currentTarget + mathfu::vec3(cameraRecord->target_position_base);

    float fov =
        animateSplineTrack<float, float>(
            animationTime,
            animationRecord->duration,
            animationIndex,
            cameraRecord->FoV,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultFloat
        );

    float roll =
        animateSplineTrack<float, float>(
            animationTime,
            animationRecord->duration,
            animationIndex,
            cameraRecord->roll,
            this->m_m2File->global_loops,
            this->globalSequenceTimes,
            defaultFloat
        );

    //Write values
    camera.position = placementMatrix*mathfu::vec4(currentPosition, 1.0);
    camera.target_position = placementMatrix*mathfu::vec4(currentTarget, 1.0);
    camera.far_clip = cameraRecord->far_clip;
    camera.near_clip = cameraRecord->near_clip;
    camera.roll = roll;
    camera.diagFov = fov;
}

void AnimationManager::calcParticleEmitters(std::vector<ParticleEmitter *> &particleEmitters,
                                            std::vector<mathfu::mat4> &bonesMatrices, int animationIndex,
                                            animTime_t animationTime) {
    auto &peRecords = m_m2File->particle_emitters;
    if (peRecords.size <= 0) return;
    static mathfu::vec3 defaultVector(1.0, 1.0, 1.0);
    static float defaultFloat = 1.0;
    static unsigned char defaultChar = 1;

//    check_offset<offsetof(M2Particle, old.geometry_model_filename), 24>();
//    check_offset<offsetof(M2Particle, old.blendingType), 40>();
    check_offset<offsetof(M2Particle, old.textureDimensions_rows), 48>();
    check_size<M2ParticleOld, 476>();
    check_size<M2Particle, 492>();

    auto animationRecord = m_m2File->sequences[animationIndex];

    for (int i = 0; i < peRecords.size; i++) {
        auto &peRecord = *peRecords.getElement(i);
        auto &particleEmitter = particleEmitters[i];
        if (particleEmitter->getGenerator() == nullptr) continue;
        CGeneratorAniProp *aniProp = particleEmitters[i]->getGenerator()->getAniProp();

        unsigned char enabledIn =
            animateTrack<unsigned char, unsigned char>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.enabledIn,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultChar
            );

        particleEmitter->isEnabled = enabledIn;

        aniProp->emissionSpeed =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.emissionSpeed,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->speedVariation =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.speedVariation,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->verticalRange =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.verticalRange,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->horizontalRange =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.horizontalRange,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        if (peRecord.old.flags & 0x800000) {
            aniProp->gravity = animateTrack<CompressedParticleGravity, mathfu::vec3>(
                            animationTime,
                            animationRecord->duration,
                            animationIndex,
                            peRecord.old.gravityCompr,
                            this->m_m2File->global_loops,
                            this->globalSequenceTimes,
                            defaultVector
                    );
        } else {
            aniProp->gravity = mathfu::vec3(
                    0,
                    0,
                    -animateTrack<float, float>(
                            animationTime,
                            animationRecord->duration,
                            animationIndex,
                            peRecord.old.gravity,
                            this->m_m2File->global_loops,
                            this->globalSequenceTimes,
                            defaultFloat
                    ));
        }



        aniProp->lifespan =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.lifespan,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->emissionRate =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.emissionRate,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->emissionAreaY =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.emissionAreaLength,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->emissionAreaX =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.emissionAreaWidth,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );
        aniProp->zSource =
            animateTrack<float, float>(
                animationTime,
                animationRecord->duration,
                animationIndex,
                peRecord.old.zSource,
                this->m_m2File->global_loops,
                this->globalSequenceTimes,
                defaultFloat
            );

        bool enabled = enabledIn != 0;
        bool emitterEnabled = enabled && 0 != (particleEmitter->flags & 2);
        bool shouldUpdate = enabled;
        if (peRecord.old.flags & 0x8000) {
            if (!enabled) {
                particleEmitter->emittingLastFrame = false;
            }
            else {
                if (aniProp->emissionRate > 0 && !particleEmitter->emittingLastFrame) {
                    particleEmitter->flags |= 4;
                    particleEmitter->emittingLastFrame = true;
                }
            }
        }
        else {
            if (emitterEnabled) {
                particleEmitter->flags |= 1;
            }
            else {
                particleEmitter->flags &= ~1;
            }
        }

        if (!emitterEnabled) {
            aniProp->emissionRate = 0;
        }
    }

}
