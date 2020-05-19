#include "cg_obj_loader.h"

inline Vec3f assimp_vector_to_vec3f(aiVector3D src) {
    Vec3f v = {};
    v.x = (f32)src.x;
    v.y = (f32)src.y;
    v.z = (f32)src.z;
    return v;
}

inline Vec2f assimp_vector_to_vec2f(aiVector3D src) {
    Vec2f v = {};
    v.x = (f32)src.x;
    v.y = (f32)src.y;
    return v;
}

inline bool load_obj_file(ConstString* filename, Vertex** vertex_buffer, u32* vertex_buffer_size, MemoryArena* storage) {
    const aiScene* scene = aiImportFile(filename->str,
                                        aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_SortByPType | aiProcess_GenSmoothNormals | aiProcess_FlipWindingOrder);
    
    if (scene == 0) {
        println("Error: %s", aiGetErrorString());
        return false;
    }
    
    if (!scene->HasMeshes()) {
        println("Error: no mesh found.");
        aiReleaseImport(scene);
        return false;
    }
    
    aiMesh* mesh = 0;
    for(u32 i = 0;i < scene->mNumMeshes;++i) {
        aiMesh* current = scene->mMeshes[i];
        if (current->mPrimitiveTypes == aiPrimitiveType_TRIANGLE) {
            mesh = current;
            break;
        }
    }
    
    if (mesh == 0) {
        println("Error: no mesh with triangle face found");
        aiReleaseImport(scene);
        return false;
    }
    
    if (!mesh->HasFaces()) {
        println("Error: mesh has no face.");
        aiReleaseImport(scene);
        return false;
    }
    
    *vertex_buffer_size = mesh->mNumFaces * 3;
    Vertex* vertices = (Vertex*)zero_allocate(storage, *vertex_buffer_size * sizeof(Vertex));
    
    if (vertices == 0) {
        println("Error: failed to allocate for the vertices.");
        aiReleaseImport(scene);
        return false;
    }
    
    Vertex* current_vertex = vertices;
    for (u32 i = 0;i < mesh->mNumFaces;++i) {
        aiFace* face = mesh->mFaces + i;
        unsigned int* indices = face->mIndices;
        Vec3f positions[3] = {};
        Vec2f uvs[3] = {};
        Vec3f normals[3] = {};
        if (mesh->HasNormals()) {
            normals[0] = assimp_vector_to_vec3f(mesh->mNormals[indices[0]]);
            normals[1] = assimp_vector_to_vec3f(mesh->mNormals[indices[1]]);
            normals[2] = assimp_vector_to_vec3f(mesh->mNormals[indices[2]]);
        }
        if (mesh->HasPositions()) {
            positions[0] = assimp_vector_to_vec3f(mesh->mVertices[indices[0]]);
            positions[1] = assimp_vector_to_vec3f(mesh->mVertices[indices[1]]);
            positions[2] = assimp_vector_to_vec3f(mesh->mVertices[indices[2]]);
        }
        if (mesh->HasTextureCoords(0)) {
            uvs[0] = assimp_vector_to_vec2f(mesh->mTextureCoords[0][indices[0]]);
            uvs[1] = assimp_vector_to_vec2f(mesh->mTextureCoords[0][indices[1]]);
            uvs[2] = assimp_vector_to_vec2f(mesh->mTextureCoords[0][indices[2]]);
        }
        *current_vertex++ = make_vertex(positions[0], uvs[0], normals[0]);
        *current_vertex++ = make_vertex(positions[1], uvs[1], normals[1]);
        *current_vertex++ = make_vertex(positions[2], uvs[2], normals[2]);
    }
    
    *vertex_buffer = vertices;
    
    aiReleaseImport(scene);
    return true;
}