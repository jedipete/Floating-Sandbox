###VERTEX

#version 130

// Inputs
in vec2 waterPosition;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    gl_Position = paramOrthoMatrix * vec4(waterPosition.xy, -1.0, 1.0);
}

###FRAGMENT

#version 130

// Params
uniform vec4 paramMatteColor;

void main()
{
    gl_FragColor = paramMatteColor;
} 
