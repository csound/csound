/*******************************************/
/*  One Zero Filter Class,                 */
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
/*  gain / (1.0 - fabs(alpha)).            */
/*******************************************/

#if !defined(__OneZero_h)
#define __OneZero_h

typedef struct OneZero {
    FLOAT gain;                 /* Filter subclass */
    FLOAT inputs;
    FLOAT lastOutput;           /* End of Filter */
    FLOAT zeroCoeff;
    FLOAT sgain;
} OneZero;


void make_OneZero(OneZero*);
void dest_OneZero(OneZero*);
void OneZero_clear(OneZero*);
void OneZero_setGain(OneZero*, FLOAT);
void OneZero_setCoeff(OneZero*, FLOAT);
FLOAT OneZero_tick(OneZero*, FLOAT);
void OneZero_print(OneZero *);

#endif
