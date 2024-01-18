#version 330 core
out vec4 FragColor;
  
in vec2 outTexCoords;

uniform sampler2D godRaysTexture;
uniform sampler2D mainSceneTexture;
uniform float godRaysIntensity; // you can control this value from your application


void main()
{ 
    //FragColor = texture(screenTexture, outTexCoords);
    //float average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    //FragColor = vec4(average, average, average, 1.0);

    vec4 godRaysColor = texture(godRaysTexture, outTexCoords);
    vec4 mainSceneColor = texture(mainSceneTexture, outTexCoords);
    float godRaysAlpha = godRaysColor.a;

    // Calculate the brightness of the godRaysColor
    float brightness = length(godRaysColor.rgb);

    // Dynamically adjust the alpha of the godRaysColor based on its brightness
    float alpha = mix(0.0, 1.0, brightness);
    godRaysColor.a = alpha;

    vec4 blendedColor = mainSceneColor + (godRaysColor * godRaysIntensity);

    //Gamma correction
    float gamma = 2.2;
    vec3 gammaCorrectedColor = pow(blendedColor.rgb, vec3(1.0 / gamma));

    FragColor = vec4(blendedColor.rgb, 1.0);

}
