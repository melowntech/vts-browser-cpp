

/*
uniform sampler2D texColor;

in vec2 varUV;

out vec4 outColor;

void main()
{
    outColor = texture(texColor, varUV); //vec4(1.0,0.0,0.0,1.0);
    outColor.xyz = vec3((outColor.x + outColor.y + outColor.z)*0.3333);
}
*/


in vec2 varUV;

uniform sampler2D texColor;
uniform sampler2D texDepth;

uniform float maxblur;  // 1   // max blur amount
uniform float aperture; // 5    // aperture - bigger values for shallower depth of field

uniform float nearClip;
uniform float farClip;

uniform float focus;  // 500
uniform float aspect; 

out vec4 outColor;

float perspectiveDepthToViewZ( const in float invClipZ, const in float near, const in float far ) {
    return ( near * far ) / ( ( far - near ) * invClipZ - far );
}

float getDepth( const in vec2 screenPosition ) {
    return texture2D( texDepth, screenPosition ).x;
}

float getViewZ( const in float depth ) {
    return perspectiveDepthToViewZ( depth, nearClip, farClip );
}

void main() {

    /*
    float viewZ2 = mod( getViewZ( getDepth( varUV ) ) , 1.0);
    //float viewZ2 = getDepth( varUV );

    //outColor.xyz = vec3(texture2D( texDepth, varUV ).x);
    outColor.xyz = vec3(viewZ2);
    outColor.a = 1.0;
    return;
    */

	vec2 texelStep = 1.0 / vec2(textureSize(texColor, 0));

    vec2 aspectcorrect = vec2( 1.0, aspect );
    aspectcorrect = texelStep;


    float viewZ = getViewZ( getDepth( varUV ) );

    float factor = ( focus + viewZ ); // viewZ is <= 0, so this is a difference equation

    /*outColor.x = abs(clamp( factor * aperture, -maxblur, maxblur )) / maxblur;
    outColor.y = 0.0;
    outColor.z = 0.0;
    outColor.a = 1.0;
    return;*/


    vec2 dofblur = vec2 ( clamp( factor * aperture, -maxblur, maxblur ) );

    vec2 dofblur9 = dofblur * 0.9;
    vec2 dofblur7 = dofblur * 0.7;
    vec2 dofblur4 = dofblur * 0.4;

    vec4 col = vec4( 0.0 );

    col += texture2D( texColor, varUV.xy );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,   0.4  ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.15,  0.37 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.29,  0.29 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.37,  0.15 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.40,  0.0  ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.37, -0.15 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.29, -0.29 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.15, -0.37 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,  -0.4  ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.15,  0.37 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29,  0.29 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.37,  0.15 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.4,   0.0  ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.37, -0.15 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29, -0.29 ) * aspectcorrect ) * dofblur );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.15, -0.37 ) * aspectcorrect ) * dofblur );

    col += texture2D( texColor, varUV.xy + ( vec2(  0.15,  0.37 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.37,  0.15 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.37, -0.15 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.15, -0.37 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.15,  0.37 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.37,  0.15 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.37, -0.15 ) * aspectcorrect ) * dofblur9 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.15, -0.37 ) * aspectcorrect ) * dofblur9 );

    col += texture2D( texColor, varUV.xy + ( vec2(  0.29,  0.29 ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.40,  0.0  ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.29, -0.29 ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,  -0.4  ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29,  0.29 ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.4,   0.0  ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29, -0.29 ) * aspectcorrect ) * dofblur7 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,   0.4  ) * aspectcorrect ) * dofblur7 );

    col += texture2D( texColor, varUV.xy + ( vec2(  0.29,  0.29 ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.4,   0.0  ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.29, -0.29 ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,  -0.4  ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29,  0.29 ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.4,   0.0  ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2( -0.29, -0.29 ) * aspectcorrect ) * dofblur4 );
    col += texture2D( texColor, varUV.xy + ( vec2(  0.0,   0.4  ) * aspectcorrect ) * dofblur4 );

    outColor = col / 41.0;
    outColor.a = 1.0;
    //outColor = vec4(1.0,0.0,0.0,1.0);

}

