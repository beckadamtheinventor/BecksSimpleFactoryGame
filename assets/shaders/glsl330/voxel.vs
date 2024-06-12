#version 330

// Input vertex attributes
in uint vertexInfo1;
in uint vertexInfo2;

// Uniform values
uniform mat4 mvp;
uniform vec3 chunkPosition;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 vertColor;
out float lightLevel;

void main()
{
    // Unpack
    float x = float((vertexInfo1 >> 12u) & 0x3Fu);
    float y = float((vertexInfo1 >>  6u) & 0x3Fu);
    float z = float((vertexInfo1 >>  0u) & 0x3Fu);
    x += chunkPosition.x;
    y += chunkPosition.y;
    z += chunkPosition.z;
    uint vno = (vertexInfo1 >> 26u) & 3u;
    float v = float(1u + ((vertexInfo1 >> 28u) & 0xFu));

    uint ri = (vertexInfo2 >> 16u) & 0xFu;
    uint gi = (vertexInfo2 >> 20u) & 0xFu;
    uint bi = (vertexInfo2 >> 24u) & 0xFu;

    uint tno = vertexInfo2 & 0xFFFFu;
    float tx = float((tno & 0xFFu) + (vno & 1u)) / 256.0;
    float ty = float(((tno >> 8) & 0xFFu) + (vno >> 1u)) / 256.0;
    // Calculate final vertex position
    gl_Position = mvp*vec4(x, y, z, 1.0);

    // Send vertex attributes to fragment shader
    fragTexCoord = vec2(tx, ty);
    lightLevel = v/16.0f;
    vertColor = vec4(float(ri)/15.0, float(gi)/15.0, float(bi)/15.0, 1.0);

}