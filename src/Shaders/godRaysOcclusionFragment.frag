#version 330 core

out vec4 FragColor;

uniform bool isSun;
uniform vec3 godRaysColor;

void main()
{
    if (isSun)
    {
        
        FragColor = vec4(godRaysColor, 1.0f); // scaled by intensity
    }
    else
    {
        FragColor = vec4(0.0f, 0.f, 0.f, 0.f);  // black
    }
}