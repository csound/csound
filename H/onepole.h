/*******************************************/
/*  One Pole Filter Class,                 */
/*  by Perry R. Cook, 1995-96              */
/*  The parameter gain is an additional    */
/*  gain parameter applied to the filter   */
/*  on top of the normalization that takes */
/*  place automatically.  So the net max   */
/*  gain through the system equals the     */
/*  value of gain.  sgain is the combina-  */
/*  tion of gain and the normalization     */
/*  parameter, so if you set the poleCoeff */
/*  to alpha, sgain is always set to       */
/*  gain * (1.0 - fabs(alpha)).            */
/*******************************************/

#if !defined(__OnePole_h)
#define __OnePole_h

typedef struct OnePole {
    MYFLT gain;                 /* Start Filter subclass */
    MYFLT outputs;
    /*    MYFLT *inputs; */
    MYFLT lastOutput;           /* End */
    MYFLT poleCoeff;
    MYFLT sgain;
} OnePole;

void make_OnePole(OnePole*);
void OnePole_clear(OnePole*);
void OnePole_setPole(OnePole*, MYFLT aValue);
void OnePole_setGain(OnePole*, MYFLT aValue);
MYFLT OnePole_tick(OnePole*, MYFLT sample);
void OnePole_print(OnePole*);

#endif
