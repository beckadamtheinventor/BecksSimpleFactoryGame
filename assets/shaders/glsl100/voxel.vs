#version 100

// Input vertex attributes
in uint vertexInfo;

// Uniform values
uniform mat4 mvp;
uniform vec3 chunkPosition;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out float lightLevel;

void main()
{
    // Unpack
    float x = float((vertexInfo & (0x1Fu << 22u)) >> 22u);
    float y = float((vertexInfo & (0x1Fu << 17u)) >> 17u);
    float z = float((vertexInfo & (0x1Fu << 12u)) >> 12u);
    x += chunkPosition.x;
    y += chunkPosition.y;
    z += chunkPosition.z;
    uint tx = uint(vertexInfo&0x3Fu);
    uint ty = uint((vertexInfo & (0x3Fu << 6u)) >> 6u);
    uint ll = uint(1u + ((vertexInfo & (0xFu << 27u)) >> 27u));
    // Calculate final vertex position
    gl_Position = mvp*vec4(x, y, z, 1.0);

    // Send vertex attributes to fragment shader
    fragTexCoord = vec2(tx/63.0f, ty/63.0f);
    lightLevel = ll/16.0f;

}