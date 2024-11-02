#version 460 core

in vec3 fragPos;
in vec3 fragNormal;

uniform vec3 viewPos;

out vec4 fragColor;

vec3 lightPos = vec3(-10.0f, -10.0f, 0.0f);  // Light position
vec3 lightColor = vec3(1.0f, 1.0f, 1.0f); // Light color
vec3 objectColor = vec3(1.0f, 0.0f, 0.0f); // Object color

float ambientStrength = 0.3f;
float specularStrength = 0.5;
float shininess = 100;

void main() {
    vec3 norm = normalize(fragNormal);
    
    vec3 ambient = ambientStrength * lightColor;

    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    fragColor = vec4(result, 1.0);
}
