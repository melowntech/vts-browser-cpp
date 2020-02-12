
/*
uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texDepth;

#define KERNEL_SIZE 32

uniform vec3 kernel[ KERNEL_SIZE ];
uniform float tNoise[ 16 ];

uniform float cameraNear;
uniform float cameraFar;
uniform mat4 cameraProjectionMatrix;
uniform mat4 cameraInverseProjectionMatrix;

uniform float kernelRadius;
uniform float minDistance; 
uniform float maxDistance; 

float getTextValue(vec2 uv)
{
    vec2 uv2 = floor(mod(uv, 4));
    return tNoise[int(uv2.x)+int(uv2.y)*4];
}

in vec2 varUV;

out vec4 outColor;

void main()
{
    vec4 c = texture(texColor, varUV);
    vec4 n = texture(texNormal, varUV);
    float d = texture(texDepth, varUV).x;

    outColor.xyz = c.xyz * n.xyz * d;
    outColor.w = 1.0;
}
*/


#define KERNEL_SIZE 32

uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texDepth;

uniform vec3 kernel[ KERNEL_SIZE ];
uniform float tNoise[ 16 ];

uniform float cameraNear;
uniform float cameraFar;
uniform mat4 cameraProjectionMatrix;
uniform mat4 cameraInverseProjectionMatrix;

uniform float mixFactor;
uniform float kernelRadius;
uniform float minDistance; // avoid artifacts caused by neighbour fragments with minimal depth difference
uniform float maxDistance; // avoid the influence of fragments which are too far away

in vec2 varUV;

out vec4 outColor;


float getNoiseValue(vec2 uv)
{
    vec2 uv2 = floor(mod(uv, 4));
    return tNoise[int(uv2.x)+int(uv2.y)*4];
}

float getexDepth( const in vec2 screenPosition ) {
    return texture2D( texDepth, screenPosition ).x;
}

float perspectiveDepthToViewZ( const in float invClipZ, const in float near, const in float far ) {
    return ( near * far ) / ( ( far - near ) * invClipZ - far );
}

float viewZToOrthographicDepth( const in float viewZ, const in float near, const in float far ) {
    return ( viewZ + near ) / ( near - far );
}

float getLinearDepth( const in vec2 screenPosition ) {
    float fragCoordZ = texture2D( texDepth, screenPosition ).x;
    float viewZ = perspectiveDepthToViewZ( fragCoordZ, cameraNear, cameraFar );
    return viewZToOrthographicDepth( viewZ, cameraNear, cameraFar );
}

float getViewZ( const in float depth ) {
    return perspectiveDepthToViewZ( depth, cameraNear, cameraFar );
}

vec3 getViewPosition( const in vec2 screenPosition, const in float depth, const in float viewZ ) {
    float clipW = cameraProjectionMatrix[2][3] * viewZ + cameraProjectionMatrix[3][3];
    vec4 clipPosition = vec4( ( vec3( screenPosition, depth ) - 0.5 ) * 2.0, 1.0 );
    clipPosition *= clipW; // unprojection.
    return ( cameraInverseProjectionMatrix * clipPosition ).xyz;
}

vec3 unpackRGBToNormal( const in vec3 rgb ) {
    return 2.0 * rgb.xyz - 1.0;
}

vec3 getViewNormal( const in vec2 screenPosition ) {
    return unpackRGBToNormal( texture2D( texNormal, screenPosition ).xyz );
}

void main() {

    float depth = getexDepth( varUV );
    float viewZ = getViewZ( depth );

    vec3 viewPosition = getViewPosition( varUV, depth, viewZ );
    vec3 viewNormal = getViewNormal( varUV );

    vec2 resolution = vec2(textureSize(texColor, 0));

    vec2 noiseScale = vec2( resolution.x, resolution.y);
    vec3 random = vec3(getNoiseValue( varUV * noiseScale ));

    // compute matrix used to reorient a kernel vector

    vec3 tangent = normalize( random - viewNormal * dot( random, viewNormal ) );
    vec3 bitangent = cross( viewNormal, tangent );
    mat3 kernelMatrix = mat3( tangent, bitangent, viewNormal );

    float occlusion = 0.0;

    for ( int i = 0; i < KERNEL_SIZE; i ++ ) {

        vec3 sampleVector = kernelMatrix * kernel[ i ]; // reorient sample vector in view space
        vec3 samplePoint = viewPosition + ( sampleVector * kernelRadius ); // calculate sample point

        vec4 samplePointNDC = cameraProjectionMatrix * vec4( samplePoint, 1.0 ); // project point and calculate NDC
        samplePointNDC /= samplePointNDC.w;

        vec2 samplePointUv = samplePointNDC.xy * 0.5 + 0.5; // compute uv coordinates

        float realDepth = getLinearDepth( samplePointUv ); // get linear depth from depth texture
        float sampleDepth = viewZToOrthographicDepth( samplePoint.z, cameraNear, cameraFar ); // compute linear depth of the sample view Z value
        float delta = sampleDepth - realDepth;

        if ( delta > minDistance && delta < maxDistance ) { // if fragment is before sample point, increase occlusion
           occlusion += 1.0;
        }
    }

    occlusion = clamp( occlusion / float( KERNEL_SIZE ), 0.0, 1.0 );

    vec3 c = vec3( 1.0 - occlusion );
    vec3 c2 = texture(texColor, varUV).xyz * c;
    outColor = vec4( mix(c2, c, mixFactor), 1.0 );
}




