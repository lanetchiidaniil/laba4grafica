#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform sampler2D texture1;
uniform bool useTexture;

void main()
{
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float diff = max(dot(norm, lightDir), 0.0);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);

    vec3 ambient = vec3(0.2, 0.2, 0.2) * objectColor;
    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0) * objectColor;
    vec3 specular = spec * vec3(0.6, 0.6, 0.6);

    vec3 result = ambient + diffuse + specular;

    if (useTexture)
    {
        vec3 texColor = texture(texture1, TexCoord).rgb;
        result *= texColor;
    }

    FragColor = vec4(result, 1.0);
}
