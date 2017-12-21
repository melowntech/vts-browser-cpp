
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif

uniform vec4 uniAtmColorLow;
uniform vec4 uniAtmColorHigh;
uniform vec4 uniBody; // major, minor, normalized atmosphere thickness
uniform vec4 uniPlanes; // near, far
uniform ivec4 uniParams; // projected, multisampling
uniform mat4 uniInvView;
uniform vec3 uniCameraPosition; // camera position
uniform vec3 uniCameraPosNorm; // camera up vector

in vec2 varUv;

layout(location = 0) out vec4 outColor;

const float M_PI = 3.1415926535;
float erf_guts(float x)
{
    const float a=8.0*(M_PI-3.0)/(3.0*M_PI*(4.0-M_PI));
    float x2=x*x;
    return exp(-x2 * (4.0/M_PI + a*x2) / (1.0+a*x2));
}
// "error function": integral of exp(-x*x)
float win_erf(float x)
{
    float sign=1.0;
    if (x<0.0)
        sign=-1.0;
    return sign*sqrt(1.0-erf_guts(x));
}
// erfc = 1.0-erf, but with less roundoff
float win_erfc(float x)
{
    if (x>3.0)
        return 0.5*erf_guts(x);
    else
        return 1.0-win_erf(x);
}

/**
   Compute the atmosphere's integrated thickness along this ray.
   https://www.cs.uaf.edu/2010/spring/cs481/section/1/lecture/02_23_planet.html
*/
float atmosphereThickness(vec3 start, vec3 dir, float tstart, float tend)
{
    float halfheight=0.05; /* height where atmosphere reaches 50% thickness (planetary radius units) */
    float k=1.0/halfheight; /* atmosphere density = exp(-height*k) */
    float refHt=1.0; /* height where density==refDen */
    float refDen=1.0; /* atmosphere opacity per planetary radius */
    /* density=refDen*exp(-(height-refHt)*k) */

    // Step 1: planarize problem from 3D to 2D
    // integral is along ray: tstart to tend along start + t*dir
    float a=dot(dir,dir),b=2.0*dot(dir,start),c=dot(start,start);
    float tc=-b/(2.0*a); //t value at ray/origin closest approach
    float y=sqrt(tc*tc*a+tc*b+c);
    float xL=tstart-tc;
    float xR=tend-tc;
    // 3D ray has been transformed to a line in 2D: from xL to xR at given y
    // x==0 is point of closest approach

    // Step 2: Find first matching radius r1-- smallest used radius
    float ySqr=y*y, xLSqr=xL*xL, xRSqr=xR*xR;
    float r1Sqr,r1;
    float isCross=0.0;
    if (xL*xR<0.0) //Span crosses origin-- use radius of closest approach
    {
        r1Sqr=ySqr;
        r1=y;
        isCross=1.0;
    }
    else
    { //Use either left or right endpoint-- whichever is closer to surface
        r1Sqr=xLSqr+ySqr;
        if (r1Sqr>xRSqr+ySqr)
            r1Sqr=xRSqr+ySqr;
        r1=sqrt(r1Sqr);
    }

    // Step 3: Find second matching radius r2
    float del=2.0/k;//This distance is 80% of atmosphere (at any height)
    float r2=r1+del;
    float r2Sqr=r2*r2;

    // Step 4: Find parameters for parabolic approximation to true hyperbolic distance
    // r(x)=sqrt(y^2+x^2), r'(x)=A+Cx^2; r1=r1', r2=r2'
    float x1Sqr=r1Sqr-ySqr; // rSqr = xSqr + ySqr, so xSqr = rSqr - ySqr
    float x2Sqr=r2Sqr-ySqr;

    float C=(r1-r2)/(x1Sqr-x2Sqr);
    float A=r1-x1Sqr*C-refHt;

    // Step 5: Compute the integral of exp(-k*(A+Cx^2)):
    float sqrtKC=sqrt(k*C); // change variables: z=sqrt(k*C)*x; exp(-z^2) needed for erf...
    float erfDel;
    if (isCross>0.0)
    { //xL and xR have opposite signs-- use erf normally
        erfDel=win_erf(sqrtKC*xR)-win_erf(sqrtKC*xL);
    }
    else
    { //xL and xR have same sign-- flip to positive half and use erfc
        if (xL<0.0)
        {
            xL=-xL;
            xR=-xR;
        }
        erfDel=win_erfc(sqrtKC*xR)-win_erfc(sqrtKC*xL);
    }
    const float norm=sqrt(M_PI)/2.0;
    float eScl=exp(-k*A); // from constant term of integral
    return refDen*eScl*norm/sqrtKC*abs(erfDel);
}

float sqr(float a)
{
    return a * a;
}

float linearDepth(float depthSample)
{
    vec4 p4 = uniInvView * vec4(varUv * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec3 p = p4.xyz / p4.w;
    float depthTrue = length(p - uniCameraPosition);
    return depthTrue;

    /*
    float zNear = uniPlanes[0];
    float zFar = uniPlanes[1];
    float z = 2 * zFar * zNear / (zFar + zNear - (zFar - zNear) * (2 * depthSample -1));
    return z / uniBody[0];
    */
}

vec3 cameraFragmentDirection()
{
    vec4 n4 = vec4(varUv * 2 - 1, 0, 1);
    n4 = uniInvView * n4;
    vec4 f4 = n4 + uniInvView[2];
    vec3 n = n4.xyz / n4.w;
    vec3 f = f4.xyz / f4.w;
    return normalize(f - n);
}

float erf(float x)
{
    return win_erf(x);
}

float atmosphere(float t1)
{
    float t0 = 0;
    float k = 100;
    float l = length(uniCameraPosition);
    float x = dot(cameraFragmentDirection(), -uniCameraPosNorm) * l;
    float y = sqrt(sqr(l) - sqr(x));
    float R = 1.01; // (normalized) height at which the approximation works best
    float tr = sqrt(sqr(R) - sqr(y)) + x;
    float c = (sqrt(sqr(tr - x) + sqr(y)) - y) / sqr(tr - x);
    float ck = sqrt(c) * sqrt(k);
    const float pir = sqrt(M_PI);
    float intC = pir * exp(k - k*y) / ck / 2;
    float int1 = erf(ck * (t1 - x));
    float int0 = erf(ck * (t0 - x));
    return intC * (int1 - int0);
}

float snoise(vec2 co)
{
    //http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

void main()
{
    // find the actual depth
    float depthSample = 0.0;
    #ifndef GL_ES
    if (uniParams[1] > 1)
    {
        for (int i = 0; i < uniParams[1]; i++)
        {
            float d = texelFetch(texDepthMulti, ivec2(gl_FragCoord.xy), i).x;
            depthSample += d;
        }
        depthSample /= uniParams[1];
    }
    else
    {
    #endif
        depthSample = texelFetch(texDepthSingle, ivec2(gl_FragCoord.xy), 0).x;
    #ifndef GL_ES
    }
    #endif

    // max ray length
    float t1 = 100; //length(uniCameraPosition) * 2;
    if (depthSample < 1.0)
        t1 = linearDepth(depthSample);

    // integrate atmosphere along the ray

    vec3 camDir = cameraFragmentDirection();
    float atm = atmosphereThickness(uniCameraPosition, camDir, 0.0, t1);

    //float atm = atmosphere(t1);

    // compose atmosphere color
    atm = clamp(atm, 0.0, 1.0);
    outColor = mix(uniAtmColorLow, uniAtmColorHigh, 1.0 - atm);
    outColor.a *= atm;
}

