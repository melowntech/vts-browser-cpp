
uniform sampler2D texColor;
uniform sampler2D texNormal;
uniform sampler2D texDepth;

uniform float cameraNear;
uniform float cameraFar;
uniform mat4 cameraProjectionMatrix;
uniform mat4 cameraInverseProjectionMatrix;

uniform float scale;
uniform float intensity;
uniform float bias;
uniform float kernelRadius;
uniform float minResolution;
uniform vec2 size;
uniform float randomSeed;

in vec2 varUV;

out vec4 outColor;

vec3 unpackRGBToNormal( const in vec3 rgb ) {
    return 2.0 * rgb.xyz - 1.0;
}

float perspectiveDepthToViewZ( const in float invClipZ, const in float near, const in float far ) {
    return ( near * far ) / ( ( far - near ) * invClipZ - far );
}

float viewZToOrthographicDepth( const in float viewZ, const in float near, const in float far ) {
    return ( viewZ + near ) / ( near - far );
}

float getDepth( const in vec2 screenPosition ) {
    return texture2D( texDepth, screenPosition ).x;
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

vec3 getViewNormal( const in vec3 viewPosition, const in vec2 screenPosition ) {
    return unpackRGBToNormal( texture2D( texNormal, screenPosition ).xyz );
}

float scaleDividedByCameraFar;
float minResolutionMultipliedByCameraFar;

float getOcclusion( const in vec3 centerViewPosition, const in vec3 centerViewNormal, const in vec3 sampleViewPosition ) {
    vec3 viewDelta = sampleViewPosition - centerViewPosition;
    float viewDistance = length( viewDelta );
    float scaledScreenDistance = scaleDividedByCameraFar * viewDistance;

    return max(0.0, (dot(centerViewNormal, viewDelta) - minResolutionMultipliedByCameraFar) / scaledScreenDistance - bias) / (1.0 + pow2( scaledScreenDistance ) );
}

// moving costly divides into consts
const float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
const float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

float getAmbientOcclusion( const in vec3 centerViewPosition ) {
    // precompute some variables require in getOcclusion.
    scaleDividedByCameraFar = scale / cameraFar;
    minResolutionMultipliedByCameraFar = minResolution * cameraFar;
    vec3 centerViewNormal = getViewNormal( centerViewPosition, varUV );

    // jsfiddle that shows sample pattern: https://jsfiddle.net/a16ff1p7/
    float angle = rand( varUV + randomSeed ) * PI2;
    vec2 radius = vec2( kernelRadius * INV_NUM_SAMPLES ) / size;
    vec2 radiusStep = radius;

    float occlusionSum = 0.0;
    float weightSum = 0.0;

    for( int i = 0; i < NUM_SAMPLES; i ++ ) {
        vec2 sampleUv = varUV + vec2( cos( angle ), sin( angle ) ) * radius;
        radius += radiusStep;
        angle += ANGLE_STEP;

        float sampleDepth = getDepth( sampleUv );
        if( sampleDepth >= ( 1.0 - EPSILON ) ) {
            continue;
        }

        float sampleViewZ = getViewZ( sampleDepth );
        vec3 sampleViewPosition = getViewPosition( sampleUv, sampleDepth, sampleViewZ );
        occlusionSum += getOcclusion( centerViewPosition, centerViewNormal, sampleViewPosition );
        weightSum += 1.0;
    }

    if( weightSum == 0.0 ) discard;

    return occlusionSum * ( intensity / weightSum );
}


void main() {
    float centerDepth = getDepth( varUV );
    if( centerDepth >= ( 1.0 - EPSILON ) ) {
        discard;
    }

    float centerViewZ = getViewZ( centerDepth );
    vec3 viewPosition = getViewPosition( varUV, centerDepth, centerViewZ );

    float ambientOcclusion = getAmbientOcclusion( viewPosition );

    outColor.xyz *=  1.0 - ambientOcclusion;
    outColor.w = 1.0;
}

