//https://www.khronos.org/registry/webgl/extensions/WEBGL_draw_buffers/
//For drawbuffers in glsl of webgl you need to use GL_EXT_draw_buffers instead of WEBGL_draw_buffers

#ifdef ENABLE_DEFERRED
#ifdef GL_EXT_draw_buffers
    #extension GL_EXT_draw_buffers: require
    #extension OES_texture_float_linear : enable
    #define drawBuffersIsSupported 1
#endif
#endif

#ifdef COMPILING_VS
/* vertex shader code */
precision highp float;
in vec3 aPosition;
in vec3 aNormal;
in vec2 aTexCoord;
in vec2 aTexCoord2;
in vec2 aTexCoord3;
in vec4 aColor;
in vec4 aColor2;

layout(std140) uniform sceneWideBlockVSPS {
    mat4 uLookAtMat;
    mat4 uPMatrix;
};

layout(std140) uniform modelWideBlockVS {
    mat4 uPlacementMat;
};

layout(std140) uniform meshWideBlockVS {
    ivec4 VertexShader_UseLitColor;
};

out VertexData {
vec2 vTexCoord;
vec2 vTexCoord2;
vec2 vTexCoord3;
vec4 vColor;
vec4 vColor2;
vec4 vPosition;
vec3 vNormal;
} vs_out;

#ifdef drawBuffersIsSupported
//vec3 vNormal;
varying float fs_Depth;
#endif

vec2 posToTexCoord(vec3 cameraPoint, vec3 normal){
//    vec3 normPos = -normalize(cameraPoint.xyz);
    vec3 normPos = cameraPoint.xyz;
    vec3 reflection = reflect(normPos, normal);
    return (normalize(vec3(reflection.r, reflection.g, reflection.b + 1.0)).rg * 0.5) + vec2(0.5);

//    vec3 normPos_495 = normPos;
//    vec3 temp_500 = (normPos_495 - (normal * (2.0 * dot(normPos_495, normal))));
//    vec3 temp_657 = vec3(temp_500.x, temp_500.y, (temp_500.z + 1.0));
//
//    return ((normalize(temp_657).xy * 0.5) + vec2(0.5));
}

//mat3 blizzTranspose(mat4 value) {
//    return mat3(
//        value[0].xyz,
//        value[1].xyz,
//        value[2].xyz
//    );
//};

void main() {
    vec2 vTexCoord;
    vec2 vTexCoord2;
    vec2 vTexCoord3;
    vec4 vColor;
    vec4 vColor2;
    vec4 vPosition;
    vec3 vNormal;


    vec4 worldPoint = uPlacementMat * vec4(aPosition, 1);

    vec4 cameraPoint = uLookAtMat * worldPoint;


    mat4 viewModelMat = uLookAtMat * uPlacementMat;
    mat3 viewModelMatTransposed = mat3(
                viewModelMat[0].xyz,
                viewModelMat[1].xyz,
                viewModelMat[2].xyz
            );

#ifndef drawBuffersIsSupported
    gl_Position = uPMatrix * cameraPoint;
    vPosition = vec4(cameraPoint.xyz, aColor.w);
    vNormal = normalize(viewModelMatTransposed * aNormal);
#else
    gl_Position = uPMatrix * cameraPoint;
    fs_Depth = gl_Position.z / gl_Position.w;

    vNormal = normalize(viewModelMatTransposed * aNormal);
    vPosition = vec4(cameraPoint.xyz, aColor.w);
#endif //drawBuffersIsSupported

    vColor.rgba = vec4(vec3(0.5, 0.499989986, 0.5), 1.0);
    vColor2 = vec4((aColor.bgr * 2.0), aColor2.a);
    int uVertexShader = VertexShader_UseLitColor.x;
    if ( uVertexShader == -1 ) {
        vTexCoord = aTexCoord;
        vTexCoord2 = aTexCoord2;
        vTexCoord3 = aTexCoord3;
    } else if (uVertexShader == 0) { //MapObjDiffuse_T1
        vTexCoord = aTexCoord;
        vTexCoord2 = aTexCoord2; //not used
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 1) { //MapObjDiffuse_T1_Refl
        vTexCoord = aTexCoord;
        vTexCoord2 = reflect(normalize(cameraPoint.xyz), vNormal).xy;
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 2) { //MapObjDiffuse_T1_Env_T2
        vTexCoord = aTexCoord;

        vTexCoord2 = posToTexCoord(vPosition.xyz, vNormal);;
        vTexCoord3 = aTexCoord3;
    } else if (uVertexShader == 3) { //MapObjSpecular_T1
        vTexCoord = aTexCoord;
        vTexCoord2 = aTexCoord2; //not used
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 4) { //MapObjDiffuse_Comp
        vTexCoord = aTexCoord;
        vTexCoord2 = aTexCoord2; //not used
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 5) { //MapObjDiffuse_Comp_Refl
        vTexCoord = aTexCoord;
        vTexCoord2 = aTexCoord2;
        vTexCoord3 = reflect(normalize(cameraPoint.xyz), vNormal).xy;
    } else if (uVertexShader == 6) { //MapObjDiffuse_Comp_Terrain
        vTexCoord = aTexCoord;
        vTexCoord2 = vPosition.xy * -0.239999995;
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 7) { //MapObjDiffuse_CompAlpha
        vTexCoord = aTexCoord;
        vTexCoord2 = vPosition.xy * -0.239999995;
        vTexCoord3 = aTexCoord3; //not used
    } else if (uVertexShader == 8) { //MapObjParallax
        vTexCoord = aTexCoord;
        vTexCoord2 = vPosition.xy * -0.239999995;
        vTexCoord3 = aTexCoord3; //not used
    }


    vs_out.vTexCoord = vTexCoord;
    vs_out.vTexCoord2 = vTexCoord2;
    vs_out.vTexCoord3 = vTexCoord3;
    vs_out.vColor = vColor;
    vs_out.vColor2 = vColor2;
    vs_out.vPosition = vPosition;
    vs_out.vNormal = vNormal;
}
#endif //COMPILING_VS

#ifdef COMPILING_GS
layout (triangles) in;
layout (triangle_strip, max_vertices=3) out;

in VertexData {
vec2 vTexCoord;
vec2 vTexCoord2;
vec2 vTexCoord3;
vec4 vColor;
vec4 vColor2;
vec4 vPosition;
vec3 vNormal;
} VertexIn[];

out vec2 vTexCoord;
out vec2 vTexCoord2;
out vec2 vTexCoord3;
out vec4 vColor;
out vec4 vColor2;
out vec4 vPosition;
out vec3 vNormal;
out vec3 vBaryCentric;

 void main()
{
  for(int i = 0; i < gl_in.length(); i++)
  {
     // copy attributes
    gl_Position = gl_in[i].gl_Position;
    vTexCoord = VertexIn[i].vTexCoord;
    vTexCoord2 = VertexIn[i].vTexCoord2;
    vTexCoord3 = VertexIn[i].vTexCoord3;
    vColor = VertexIn[i].vColor;
    vColor2 = VertexIn[i].vColor2;
    vPosition = VertexIn[i].vPosition;
    vNormal = VertexIn[i].vNormal;
    vBaryCentric = vec3(
//    0, 0, 0
        ((i & 3) == 0) ? 1.0f : 0,
        ((i & 3) == 1) ? 1.0f : 0,
        ((i & 3) == 2) ? 1.0f : 0
    );

    // done with the vertex
    EmitVertex();
  }
  EndPrimitive();
}
#endif

#ifdef COMPILING_FS

precision highp float;
in vec2 vTexCoord;
in vec2 vTexCoord2;
in vec2 vTexCoord3;
in vec4 vColor;
in vec4 vColor2;
in vec4 vPosition;
in vec3 vNormal;
in vec3 vBaryCentric;

layout(std140) uniform meshWideBlockPS {
    vec4 uViewUp;
    vec4 uSunDir_FogStart;
    vec4 uSunColor_uFogEnd;
    vec4 uAmbientLight;
    vec4 uAmbientLight2AndIsBatchA;
    ivec4 UseLitColor_EnableAlpha_PixelShader;
    vec4 FogColor_AlphaTest;
};

uniform sampler2D uTexture;
uniform sampler2D uTexture2;
uniform sampler2D uTexture3;

out vec4 outputColor;

#ifdef drawBuffersIsSupported
varying float fs_Depth;
#endif

vec3 makeDiffTerm(vec3 matDiffuse) {
    vec3 currColor;
    vec3 lDiffuse = vec3(0.0, 0.0, 0.0);
    if (UseLitColor_EnableAlpha_PixelShader.x == 1) {
        //vec3 viewUp = normalize(vec3(0, 0.9, 0.1));
        vec3 normalizedN = normalize(vNormal);
        float nDotL = dot(normalizedN, -(uSunDir_FogStart.xyz));
        float nDotUp = dot(normalizedN, uViewUp.xyz);

        vec3 precomputed = vColor2.rgb;

        vec3 ambientColor = uAmbientLight.rgb;
        if (uAmbientLight2AndIsBatchA.w > 0.0) {
            ambientColor = mix(uAmbientLight.rgb, uAmbientLight2AndIsBatchA.rgb, vec3(vPosition.w));
        }

        vec3 adjAmbient = (ambientColor.rgb + precomputed);
        vec3 adjHorizAmbient = (ambientColor.rgb + precomputed);
        vec3 adjGroundAmbient = (ambientColor.rgb + precomputed);

        if ((nDotUp >= 0.0))
        {
            currColor = mix(adjHorizAmbient, adjAmbient, vec3(nDotUp));
        }
        else
        {
            currColor= mix(adjHorizAmbient, adjGroundAmbient, vec3(-(nDotUp)));
        }

        vec3 skyColor = (currColor * 1.10000002);
        vec3 groundColor = (currColor* 0.699999988);
        currColor = mix(groundColor, skyColor, vec3((0.5 + (0.5 * nDotL))));
        lDiffuse = (uSunColor_uFogEnd.xyz * clamp(nDotL, 0.0, 1.0));
    } else {
        currColor = vec3 (1.0, 1.0, 1.0) * uAmbientLight.rgb;
    }

    vec3 gammaDiffTerm = matDiffuse * (currColor + lDiffuse);
    vec3 linearDiffTerm = (matDiffuse * matDiffuse) * vec3(0.0);
    return sqrt(gammaDiffTerm*gammaDiffTerm + linearDiffTerm) ;

//    return matDiffuse * currColor.rgb ;
}

void main() {


    vec4 tex = texture2D(uTexture, vTexCoord).rgba ;
    vec4 tex2 = texture2D(uTexture2, vTexCoord2).rgba;
    vec4 tex3 = texture2D(uTexture3, vTexCoord3).rgba;

    if (UseLitColor_EnableAlpha_PixelShader.y == 1) {
        if ((tex.a - 0.501960814) < 0.0) {
            discard;
        }
    }

    int uPixelShader = UseLitColor_EnableAlpha_PixelShader.z;
    vec4 finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    if ( uPixelShader == -1 ) {
        finalColor = vec4(makeDiffTerm(tex.rgb * vColor.rgb + tex2.rgb*vColor2.bgr), tex.a);
    } else if (uPixelShader == 0) { //MapObjDiffuse

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 1) { //MapObjSpecular

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 2) { //MapObjMetal

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 3) { //MapObjEnv

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        vec3 env = tex2.rgb * tex.a;

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);

    } else if (uPixelShader == 4) { //MapObjOpaque

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 5) { //MapObjEnvMetal

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        vec3 env = (tex.rgb * tex.a) * tex2.rgb;

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);

    } else if (uPixelShader == 6) { //MapObjTwoLayerDiffuse

        vec3 layer1 = tex.rgb;
        vec3 layer2 = mix(layer1, tex2.rgb, tex2.a);
        vec3 matDiffuse = (vColor.rgb * 2.0) * mix(layer2, layer1, vColor2.a);

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), 1.0);
    } else if (uPixelShader == 7) { //MapObjTwoLayerEnvMetal

        vec4 colorMix = mix(tex2, tex, vColor2.a);
        vec3 env = (colorMix.rgb * colorMix.a) * tex3.rgb;
        vec3 matDiffuse = colorMix.rgb * (2.0 * vColor.rgb);

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);

    } else if (uPixelShader == 8) { //MapObjTwoLayerTerrain

        vec3 layer1 = tex.rgb;
        vec3 layer2 = tex2.rgb;

        vec3 matDiffuse = ((vColor.rgb * 2.0) * mix(layer2, layer1, vColor2.a));
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 9) { //MapObjDiffuseEmissive

        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        vec3 env = tex2.rgb * tex2.a * vColor2.a;

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);

    } else if (uPixelShader == 10) { //MapObjMaskedEnvMetal

        float mixFactor = clamp((tex3.a * vColor2.a), 0.0, 1.0);
        vec3 matDiffuse =
            (vColor.rgb * 2.0) *
            mix(mix(((tex.rgb * tex2.rgb) * 2.0), tex3.rgb, mixFactor), tex.rgb, tex.a);

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);

    } else if (uPixelShader == 11) { //MapObjEnvMetalEmissive
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        vec3 env =
            (
                ((tex.rgb * tex.a) * tex2.rgb) +
                ((tex3.rgb * tex3.a) * vColor2.a)
            );

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);
    } else if (uPixelShader == 12) { //MapObjTwoLayerDiffuseOpaque
        vec3 matDiffuse =
            (vColor.rgb * 2.0) *
            mix(tex2.rgb, tex.rgb, vColor2.a);

        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } else if (uPixelShader == 13) { //MapObjTwoLayerDiffuseEmissive
        vec3 t1diffuse = (tex2.rgb * (1.0 - tex2.a));

        vec3 matDiffuse =
            ((vColor.rgb * 2.0) *
            mix(t1diffuse, tex.rgb, vColor2.a));

        //TODO: there is env missing here
        vec3 env = ((tex2.rgb * tex2.a) * (1.0 - vColor2.a));
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse)+env, vColor.a);
    } else if (uPixelShader == 13) { //MapObjTwoLayerDiffuseEmissive
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } else if (uPixelShader == 14) { //MapObjAdditiveMaskedEnvMetal
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } else if (uPixelShader == 15) { //MapObjTwoLayerDiffuseMod2x
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } if (uPixelShader == 16) { //MapObjTwoLayerDiffuseMod2xNA
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } if (uPixelShader == 17) { //MapObjTwoLayerDiffuseAlpha
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } if (uPixelShader == 18) { //MapObjLod
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    } if (uPixelShader == 19) { //MapObjParallax
        vec3 matDiffuse = tex.rgb * (2.0 * vColor.rgb);
        finalColor.rgba = vec4(makeDiffTerm(matDiffuse), vColor.a);
    }

    //finalColor.rgb *= 4.0;

    if(finalColor.a < FogColor_AlphaTest.w)
        discard;

    vec3 fogColor = FogColor_AlphaTest.xyz;
    float fog_start = uSunDir_FogStart.w;
    float fog_end = uSunColor_uFogEnd.w;
    float fog_rate = 1.5;
    float fog_bias = 0.01;

    //vec4 fogHeightPlane = pc_fog.heightPlane;
    //float heightRate = pc_fog.color_and_heightRate.w;

    float distanceToCamera = length(vPosition.xyz);
    float z_depth = (distanceToCamera - fog_bias);
    float expFog = 1.0 / (exp((max(0.0, (z_depth - fog_start)) * fog_rate)));
    //float height = (dot(fogHeightPlane.xyz, vPosition.xyz) + fogHeightPlane.w);
    //float heightFog = clamp((height * heightRate), 0, 1);
    float heightFog = 1.0;
    expFog = (expFog + heightFog);
    float endFadeFog = clamp(((fog_end - distanceToCamera) / (0.699999988 * fog_end)), 0.0, 1.0);

    finalColor.rgb = mix(fogColor.rgb, finalColor.rgb, vec3(min(expFog, endFadeFog)));

    finalColor.a = 1.0; //do I really need it now?




//#ifndef drawBuffersIsSupported
    //Forward rendering without lights
    outputColor = finalColor;
//    if(!any(lessThan(vBaryCentric, vec3(0.02)))){
//        discard;
//    }

//#else
    //Deferred rendering
    //gl_FragColor = finalColor;
//    gl_FragData[0] = vec4(vec3(fs_Depth), 1.0);
//    gl_FragData[1] = vec4(vPosition.xyz,0);
//    gl_FragData[2] = vec4(vNormal.xyz,0);
//    gl_FragData[3] = finalColor;
//#endif //drawBuffersIsSupported
}

#endif //COMPILING_FS
