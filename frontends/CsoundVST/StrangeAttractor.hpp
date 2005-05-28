/*
* C S O U N D   V S T
*
* A VST plugin version of Csound, with Python scripting.
*
* L I C E N S E
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef STRANGEATTRACTOR_H
#define STRANGEATTRACTOR_H

#ifdef SWIG
%module CsoundVST
%{
#include "Silence.hpp"
#include <string>
#include <vector>
#include <boost/random.hpp>
#include <boost/numeric/ublas/matrix.hpp>
        %}
%include "std_string.i"
%include "std_vector.i"
#else
#include "Silence.hpp"
#include <string>
#include <vector>
#include <boost/random.hpp>
#include <boost/numeric/ublas/matrix.hpp>
using namespace boost::numeric;
#endif

namespace csound
{
        /**
        * Generates notes by searching for a chaotic dynamical system defined by a
        * polynomial equation or partial differential equation using
        * Julien C. Sprott's Lyupanov exponent search, or by translating a known
        * chaotic dynamical system into music, by interpreting each iteration
        * of the system as a note. The time of the note can be represented either
        * as the order of iteration, or as a dimension of the attractor.
        * See Julien C. Sprott's book "Strange Attractors".
        */
        class StrangeAttractor :
                public ScoreNode
        {
        protected:
                std::string code;
                Random random;
                std::string filename;
                int scoreType;
                int NMAX;
                std::vector<double> A;
                double AL;
                double COSAL;
                int D;
                int DD;
                double D2;
                double D2MAX;
                double decibels;
                double DF;
                double DL2;
                double DLW;
                double DLX;
                double DLY;
                double DLZ;
                double DUM;
                double DW;
                double DX;
                double DY;
                double DZ;
                double EPS;
                double F;
                int I;
                double instrument;
                int I1;
                int I2;
                int I3;
                int I4;
                int I5;
                int J;
                double L;
                double duration;
                double LSUM;
                int M;
                double MX;
                double MY;
                int N;
                double N1;
                double N2;
                double NL;
                int O;
                double octave;
                int ODE;
                int OMAX;
                int P;
                double x;
                double pitchClassSet;
                int PREV;
                double PT;
                double RAN;
                double RS;
                double SH;
                double SINAL;
                double time;
                double SW;
                int T;
                double TIA;
                double TT;
                int TWOD;
                //double TWOPI;
                std::vector<double> V;
                double W;
                double WE;
                double WMAX;
                double WMIN;
                double WNEW;
                double WP;
                std::vector<double> WS;
                double WSAVE;
                double X;
                double XA;
                double XE;
                double XH;
                double XL;
                double XMAX;
                double XMIN;
                std::vector<double> XN;
                double XNEW;
                double XP;
                std::vector<double> XS;
                double XSAVE;
                double XW;
                std::vector<double> XY;
                double XZ;
                double Y;
                double YA;
                double YE;
                double YH;
                double YL;
                double YMAX;
                double YMIN;
                double YNEW;
                double YP;
                std::vector<double> YS;
                double YSAVE;
                double YW;
                double YZ;
                double Z;
                double ZA;
                double ZE;
                double ZMAX;
                double ZMIN;
                double ZNEW;
                double ZP;
                std::vector<double> ZS;
                double ZSAVE;
        public:
                StrangeAttractor(void);
                virtual ~StrangeAttractor(void);
                virtual void setCode(std::string code);
                virtual std::string getCode() const;
                virtual void setIterationCount(size_t iterationCount);
                virtual size_t getIterationCount() const;
                virtual void setIteration(size_t iteration);
                virtual size_t getIteration() const;
                virtual void setAttractorType(int attractorType);
                virtual int getAttractorType() const;
                virtual void setScoreType(int attractorType);
                virtual int getScoreType() const;
                virtual void initialize();
                virtual void reinitialize();
                virtual void reset();
                virtual void codeRandomize();
                virtual void specialFunctions();
                virtual void getDimensionAndOrder();
                virtual void getCoefficients();
                virtual void shuffleRandomNumbers();
                virtual void calculateLyupanovExponent();
                virtual void calculateFractalDimension();
                virtual double getFractalDimension() const;
                virtual double getLyupanovExponent() const;
                virtual void setX(double X);
                virtual double getX() const;
                virtual void setY(double X);
                virtual double getY() const;
                virtual void setZ(double X);
                virtual double getZ() const;
                virtual void setW(double X);
                virtual double getW() const;
                virtual bool searchForAttractor();
                virtual bool evaluateAttractor();
                virtual void iterate();
                virtual void generate();
                virtual void render (int N, double X, double Y, double Z, double W);
                virtual void setDimensionCount(int D);
                virtual int getDimensionCount() const;
        };
}

#endif
