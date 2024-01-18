#version 330 core

in vec2 outTexCoords;
out vec4 FragColor;

uniform sampler2D GRtDiffuse;
uniform vec2 GRlightPosition;
uniform float GRExposure;
uniform float GRDecay;
uniform float GRDensity;
uniform float GRWeight;
uniform int GRSamples;
uniform vec3 GRColor;

void main() {
    vec2 texCoord = outTexCoords;
    
    /*Calculate vector from pixel to light source in screen space*/
    vec2 deltaTextCoord = texCoord - GRlightPosition;

    /*Divide by number of samples and scale by control factor*/
    deltaTextCoord *= 1.0 / float(GRSamples) * GRDensity;

    /*Store initial sample*/
    vec4 color = texture(GRtDiffuse, texCoord);

    /*Set up illumination decay factor*/
    float illuminationDecay = 1.0;

    /*Evaluate the summation for samples number of iterations up to n*/
    for (int i = 0; i < GRSamples; i++) {

        // Step sample location along ray
        texCoord -= deltaTextCoord;
        texCoord = clamp(texCoord, 0.0, 1.0);

        // Retrieve sample at new location
        vec4 sample = texture(GRtDiffuse, texCoord);

        // Apply sample attenuation scale/decay factors
        sample *= illuminationDecay * GRWeight;

        // Accumulate combined color
        color += sample;

        // Update exponential decay factor
        illuminationDecay *= GRDecay;
    }

    color.rgb *= GRColor;

    /*Output final color with a further scale control factor*/
    FragColor = color * GRExposure;
}