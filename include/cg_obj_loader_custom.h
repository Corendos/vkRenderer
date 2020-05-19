#ifndef __CG_OBJ_LOADER_CUSTOM_H__
#define __CG_OBJ_LOADER_CUSTOM_H__

enum ObjFaceType {
    OFT_Undefined = 0,
    OFT_Vertex,
    OFT_Texture,
    OFT_Normal,
    OFT_TextureNormal,
};

struct ObjParse {
    u32 vertex_count;
    u32 uv_count;
    u32 normal_count;
    u32 face_count;
    ObjFaceType face_type;
};

#endif //CG_OBJ_LOADER_CUSTOM_H
