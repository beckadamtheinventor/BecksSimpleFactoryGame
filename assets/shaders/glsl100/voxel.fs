#version 100

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in float lightLevel;

// Input uniform values
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main()
{
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    // NOTE: Implement here your fragment shader code

    finalColor = texelColor*vec4(lightLevel, lightLevel, lightLevel, 0);
}

