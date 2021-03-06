//
// Created by deamon on 10.05.17.
//

#include "ShaderRuntimeData.h"
size_t hashFunction(const size_t & key){
    return key;
}

bool ShaderRuntimeData::hasUnf(const HashedString name) const {
    return m_uniformMap.find(name.Hash()) != m_uniformMap.end();
}
GLuint ShaderRuntimeData::getUnf(const HashedString name) const {
    return m_uniformMap.at(name.Hash());
}


GLuint ShaderRuntimeData::getUnfRT(const std::string &name) {
    const char * cstr = name.c_str();
    return m_uniformMap.at(CalculateFNV(cstr));
}

GLuint ShaderRuntimeData::getUnfHash(size_t hash) {
    return m_uniformMap.at(hash);
}


void ShaderRuntimeData::setUnf(const std::string &name, GLuint index) {
    const char * cstr = name.c_str();
    m_uniformMap[CalculateFNV(cstr)] = index;
}

GLuint ShaderRuntimeData::getProgram() {
    return m_program;
}
void ShaderRuntimeData::setProgram(GLuint program) {
    m_program = program;
}