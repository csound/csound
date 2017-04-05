declare name      "Fractal Noise";
declare author    "Tito Latini";
declare license   "GNU LGPL";
declare copyright "Tito Latini";
declare version   "1.0";

import("music.lib"); 

/* density of the poles */
h = 6.0;

amp = vslider("amp", 1.0, 0.0, 20.0, 0.01);

/*
 *   beta = 0,   white noise
 *   beta = 1,   pink noise
 *   beta = 2,   brownian noise
 */
beta = vslider("beta", 1.75, 0.0, 10.0, 0.01); 

c1 = pow(10.0, 1 / h);
c2 = pow(10.0, beta / (2.0 * h)); 
T = 1.0 / SR ; 

/* poles */
p1  = 50;  /* pole with lowest frequency */
p2  = p1  * c1;
p3  = p2  * c1;
p4  = p3  * c1;
p5  = p4  * c1;
p6  = p5  * c1;
p7  = p6  * c1;
p8  = p7  * c1;
p9  = p8  * c1;
p10 = p9  * c1;
p11 = p10 * c1;
p12 = p11 * c1;
p13 = p12 * c1;
p14 = p13 * c1;
p15 = p14 * c1;

coeffCalc(frq) = 0.0 - exp(0.0 - (2.0 * PI * frq * T));

/* coeff a */
a1  = coeffCalc(p1);
a2  = coeffCalc(p2);
a3  = coeffCalc(p3);
a4  = coeffCalc(p4);
a5  = coeffCalc(p5);
a6  = coeffCalc(p6);
a7  = coeffCalc(p7);
a8  = coeffCalc(p8);
a9  = coeffCalc(p9);
a10 = coeffCalc(p10);
a11 = coeffCalc(p11);
a12 = coeffCalc(p12);
a13 = coeffCalc(p13);
a14 = coeffCalc(p14);
a15 = coeffCalc(p15);

/* zeros */
z1  = p1  * c2;
z2  = p2  * c2;
z3  = p3  * c2;
z4  = p4  * c2;
z5  = p5  * c2;
z6  = p6  * c2;
z7  = p7  * c2;
z8  = p8  * c2;
z9  = p9  * c2;
z10 = p10 * c2;
z11 = p11 * c2;
z12 = p12 * c2;
z13 = p13 * c2;
z14 = p14 * c2;
z15 = p15 * c2;

/* coeff b */
b1  = coeffCalc(z1);
b2  = coeffCalc(z2);
b3  = coeffCalc(z3);
b4  = coeffCalc(z4);
b5  = coeffCalc(z5);
b6  = coeffCalc(z6);
b7  = coeffCalc(z7);
b8  = coeffCalc(z8);
b9  = coeffCalc(z9);
b10 = coeffCalc(z10);
b11 = coeffCalc(z11);
b12 = coeffCalc(z12);
b13 = coeffCalc(z13);
b14 = coeffCalc(z14);
b15 = coeffCalc(z15);

sezioni = TF2(1, b1+b2, b1*b2, a1+a2, a1*a2) 
    : TF2(1, b3+b4,   b3*b4,   a3+a4,   a3*a4) 
    : TF2(1, b5+b6,   b5*b6,   a5+a6,   a5*a6)
    : TF2(1, b7+b8,   b7*b8,   a7+a8,   a7*a8)
    : TF2(1, b9+b10,  b9*b10,  a9+a10,  a9*a10)
    : TF2(1, b11+b12, b11*b12, a11+a12, a11*a12)
    : TF2(1, b13+b14, b13*b14, a13+a14, a13*a14)
    : TF2(1, b15    , 0,       a15,     0) * amp;

fractalNoise = noise : sezioni;

process = fractalNoise;
