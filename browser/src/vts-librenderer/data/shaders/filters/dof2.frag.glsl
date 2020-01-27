
in vec2 varUV;

uniform sampler2D texColor;
uniform sampler2D texDepth;

uniform float maxblur;  // 1   // max blur amount
uniform float aperture; // 5    // aperture - bigger values for shallower depth of field

uniform float nearClip;
uniform float farClip;

uniform float focus;  // 500
uniform float aspect; 
uniform float vpass; 

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

float random(vec3 scale, float seed){
    return fract(sin(dot(gl_FragCoord.xyz + seed, scale)) * 43758.5453 + seed);
}


void main() {

    vec2 texelStep = 1.0 / vec2(textureSize(texColor, 0));

    vec2 aspectcorrect = vec2( 1.0, aspect );
    aspectcorrect = texelStep;

    float viewZ = getViewZ( getDepth( varUV ) );

    float factor = ( focus + viewZ ); // viewZ is <= 0, so this is a difference equation

    float radius = clamp( factor * aperture, -maxblur, maxblur );

    vec2 d = radius / textureSize(texColor, 0);

    if (vpass > 0.5)
        d.x = 0.0;
    else 
        d.y = 0.0;

    vec4 c3=vec4(0.0);
    
    float o=random(vec3(12.9898,78.233,151.7182),0.0)-0.5;
    float t=0.0;
    for(float tt=-30.0;tt<=30.0;tt++){
        float p=(tt+o)*(1.0/30.0);
        float w=1.0-abs(p);
        c3+=texture2D( texColor, varUV.xy+d*p)*w;
        t+=w;
    }

    c3/=t;

    outColor = c3;
    //outColor.r = 1.0;
    outColor.w = 1.0;
}

