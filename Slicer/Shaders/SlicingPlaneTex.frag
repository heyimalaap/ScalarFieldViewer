#version 460 core

in vec3 vertexColor;
in vec3 texCoord;

uniform double data_min;
uniform double data_max;
uniform sampler1D colormapTexture;
uniform sampler3D dataTexture;

out vec4 fragColor;

void main() {
    // vec3 data_val = vec3((texture(dataTexture, texCoord) - data_min) / (data_max - data_min));
    vec3 data_val = vec3(texture(dataTexture, texCoord).r);
    vec3 color_out = texture(colormapTexture, data_val.r).rgb;
    fragColor = vec4(color_out, 1.0f);
}
