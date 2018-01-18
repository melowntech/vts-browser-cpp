
uniform sampler2D texDepthSingle;
#ifndef GL_ES
uniform sampler2DMS texDepthMulti;
#endif
uniform sampler2D texDensity;

uniform vec4 uniAtmColorLow;
uniform vec4 uniAtmColorHigh;
uniform vec4 uniParams; // projected, multisampling, atmosphere
uniform mat4 uniInvViewProj;
uniform vec3 uniCameraPosition; // camera position
uniform vec3 uniCameraDirection; // normalize(uniCameraPosition)
uniform vec3 uniCornerDirs[4];

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
    float halfheight = 0.002; /* height where atmosphere reaches 50% thickness (planetary radius units) */
    float k = 1.0 / halfheight; /* atmosphere density = exp(-height*k) */
    float refHt = 1.0; /* height where density==1 */
    /* density=exp(-(height-refHt)*k) */

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
    return eScl*norm/sqrtKC*abs(erfDel);
}




float linearDepth(float depthSample)
{
    vec4 p4 = uniInvViewProj * vec4(varUv * 2.0 - 1.0, depthSample * 2.0 - 1.0, 1.0);
    vec3 p = p4.xyz / p4.w;
    float depthTrue = length(p - uniCameraPosition);
    return depthTrue;
}

vec3 cameraFragmentDirection()
{


    return normalize(mix(
        mix(uniCornerDirs[0], uniCornerDirs[1], varUv.x),
        mix(uniCornerDirs[2], uniCornerDirs[3], varUv.x), varUv.y));


    vec4 f4 = vec4(varUv * 2 - 1, 0, 1);
    f4 = uniInvViewProj * f4;
    vec3 f = f4.xyz / f4.w;
    vec3 dir2 = normalize(f - uniCameraPosition);

    //return abs(dir - dir2) * 10;

    /*
    vec4 n4 = vec4(varUv * 2 - 1, 0, 1);
    n4 = uniInvViewProj * n4;
    vec4 f4 = n4 + uniInvViewProj[2];
    vec3 n = n4.xyz / n4.w;
    vec3 f = f4.xyz / f4.w;
    return normalize(f - n);
    */
}





float sqr(float x)
{
    return x * x;
}


float erf(float x)
{
    float t = 1 / (1 + 0.5 * abs(x));

    float coefs[10];
    coefs[0] = -1.26551223;
    coefs[1] = +1.00002368;
    coefs[2] = +0.37409196;
    coefs[3] = +0.09678418;
    coefs[4] = -0.18628806;
    coefs[5] = +0.27886807;
    coefs[6] = -1.13520398;
    coefs[7] = +1.48851587;
    coefs[8] = -0.82215223;
    coefs[9] = +0.17087277;

    float sum = 0;
    for (int i = 0; i < 10; i++)
        sum = sum * t + coefs[9 - i];
    float tau = t * exp(-sqr(x) + sum);

    if (x < 0)
        return tau - 1;
    return 1 - tau;
}


float taylor(float polynom[4], float t)
{
    float ts[4];
    ts[0] = t;
    for (int i = 1; i < 4; i++)
        ts[i] = ts[i - 1] * t * t;
    float sum = 0;
    for (int i = 0; i < 4; i++)
        sum += polynom[i] * ts[i];
    return sum;
}


float decodeFloat(vec4 rgba)
{
    return dot(rgba, vec4(1.0, 1 / 255.0, 1 / 65025.0, 1 / 16581375.0));
}


float atmSample(float u, float v)
{
    return decodeFloat(texture(texDensity, vec2(u, v)));
}





float atmosphere(float t1)
{
    float l = length(uniCameraPosition);
    float x = dot(cameraFragmentDirection(), -uniCameraDirection) * l;
    //float y2 = abs(sqr(l) - sqr(x));
    float y2 = sqr(l) - sqr(x);
    float y = sqrt(y2);


    // fill holes in terrain
    if (y < 0.999 && x >= 0 && t1 > 99)
    {
        float z = sqrt(1 - y2);
        t1 = x - z;
    }



    //return dot(cameraFragmentDirection(), -uniCameraDirection);


    float atmHeight = 0.025;


    float atmRad = 1 + atmHeight;

    if (y > atmRad)
        return 0;

    float g = sqrt(sqr(atmRad) - y2);


    float t0 = max(0.0, x - g) - x;
    t1 = min(t1, x + g) - x;



    float tAtm = sqrt(sqr(atmRad) - y2);



    float v = y / atmRad;

    float d0 = atmSample(abs(t0) / tAtm, v);
    float d1 = atmSample(abs(t1) / tAtm, v);

    float sum;

    if (t0 * t1 < 0)
    {
        // ray crosses the symetry axis
        sum = d0 + d1;
    }
    else
    {
        sum = abs(d0 - d1);
    }

    return 1 - pow(10, -50 * sum);
    //return sum;
    //return atmSample(0.5, 0.5);






    /*
    float sum = 0;
    float step = 0.0001;
    for (float t = t0; t < t1; t += step)
    {
        float h = sqrt(sqr(t) + y2);
        h = (clamp(h, 1, atmRad) - 1) / atmHeight;
        float a = exp(-12 * h);
        sum += a;
    }
    sum *= step;
    return 1 - pow(10, -50 * sum);
    */





    /*
    float h = (clamp(y, 1, atmRad) - 1) / atmHeight;
    float atm = pow(t1 - t0, 0.8) * exp(-5 * h);
    return 3 * atm;
    */





    /*
    vec3 camDir = cameraFragmentDirection();
    float sum = atmosphereThickness(uniCameraPosition, camDir, t0 + x, t1 + x);
    return 1 - pow(10, -50 * sum);
    //return sum;
    */







    /*
    float t0 = 0;
    //float k = t1 > 99 ? 30 : 10;
    float k = 1000;
    */





    /*
    float int1 = -exp(-k * (t1 - x + y - 1));
    float int0 = -exp(-k * (t0 - x + y - 1));
    return (int1 - int0) / k;
    */


    /*
    float ekky = exp(k - k * y);
    float polynom[4];
    polynom[0] = +ekky;
    polynom[1] = -k * ekky / (6 * y);
    polynom[2] = +k * ekky * (k * y2 + y) / (40 * y2 * y2);
    polynom[3] = -k * ekky * (k * k * y * y * y + 3 * k * y2 + 3 * y) / (336 * y2 * y2 * y2);

    float int1 = taylor(polynom, t1 - x);
    float int0 = taylor(polynom, t0 - x);
    return int1 - int0;
    */


    /*
    if (t1 > 99)
    {
        float R = 10;
        float tr = sqrt(sqr(R) - sqr(y)) + x;
        float c = (sqrt(sqr(tr - x) + sqr(y)) - y) / sqr(tr - x);
        float ck = sqrt(c) * sqrt(k);
        float pir = sqrt(M_PI);
        float intC = pir * exp(k - k*y) / ck / 2;
        float int1 = erf(ck * (t1 - x));
        float int0 = erf(ck * (t0 - x));
        return k * intC * (int1 - int0);
    }
    else
    {
        float S = 1.15;
        float ts = sqrt(sqr(S) - sqr(y)) + x;
        float d = (sqrt(sqr(ts - x) + sqr(y)) - y) / abs(ts - x);
        float dk2 = d * k * 2;
        float edk2x = exp(dk2 * x);
        float edk2t0 = exp(dk2 * t0);
        float edk2t1 = exp(dk2 * t1);
        float int1 = -(sign(t1 - x) * (edk2t1 + edk2x) - edk2t1 + edk2x) * exp(-k * (d * (t1 + x) + y - 1)) / dk2;
        float int0 = -(sign(t0 - x) * (edk2t0 + edk2x) - edk2t0 + edk2x) * exp(-k * (d * (t0 + x) + y - 1)) / dk2;
        return k * (int1 - int0);
    }
    */


    /*
    //float R = 10;
    //float tr = sqrt(sqr(R) - sqr(y)) + x;
    float tr = t1 > 99 ? 10 : t1;
    float c = (sqrt(sqr(tr - x) + sqr(y)) - y) / sqr(tr - x);
    float ck = sqrt(c) * sqrt(k);
    float pir = sqrt(M_PI);
    float intC = pir * exp(k - k * y) / (ck * 2);
    float int1 = erf(ck * (t1 - x));
    float int0 = erf(ck * (t0 - x));
    return 100 * intC * (int1 - int0);
    */
}



void main()
{

    //outColor = vec4(cameraFragmentDirection(), 1);
    //return;



    // find the actual depth
    float depthSample = 0.0;
    #ifndef GL_ES
    int samples = int(uniParams[1] + 0.1);
    if (samples > 1)
    {
        for (int i = 0; i < samples; i++)
        {
            float d = texelFetch(texDepthMulti, ivec2(gl_FragCoord.xy), i).x;
            depthSample += d;
        }
        depthSample /= samples;
    }
    else
    {
    #endif
        depthSample = texelFetch(texDepthSingle, ivec2(gl_FragCoord.xy), 0).x;
    #ifndef GL_ES
    }
    #endif

    // max ray length
    float t1 = 100;
    if (depthSample < 1.0)
        t1 = linearDepth(depthSample);

    // integrate atmosphere along the ray
    //vec3 camDir = cameraFragmentDirection();
    //float atm = atmosphereThickness(uniCameraPosition, camDir, 0.0, t1);

    float atm = atmosphere(t1);
    //float atm = t1 / 10;

    //outColor = vec4(vec3(atm), 1);
    //return;

    //float atm = 1 - t1 / 5;
    //outColor = vec4(vec3(t1 * 0.2), 1);

    // compose atmosphere color
    atm = clamp(atm, 0.0, 1.0);
    outColor = mix(uniAtmColorLow, uniAtmColorHigh, 1.0 - atm);
    outColor.a *= atm;
}

