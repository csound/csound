/**
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
#include "StrangeAttractor.hpp"
#include "Conversions.hpp"
#include "Random.hpp"
#include "System.hpp"
#include <FL/Fl.H>
#include <cmath>

namespace csound
{
	boost::uniform_01<boost::mt19937> StrangeAttractor::uniform_01(Random::mersenneTwister);

	StrangeAttractor::StrangeAttractor () : scoreType(1)
	{
		initialize ();
		reset ();
	}

	StrangeAttractor::~StrangeAttractor(void)
	{
	}

	std::string StrangeAttractor::getCode() const
	{
		return code;
	}

	void StrangeAttractor::setCode(std::string code)
	{
		this->code = code;
	}

	void StrangeAttractor::initialize ()
	{
		N = 1;
		D = 2;
		EPS = .1;
		setIterationCount (4000);
		ODE = 0;
		OMAX = 5;
		PREV = 5;
		A.resize(505);
		V.resize(100);
		WS.resize(500);
		XN.resize(5);
		XS.resize(500);
		XY.resize(5);
		YS.resize(500);
		ZS.resize(500);
	}

	void StrangeAttractor::reinitialize ()
	{
		XMAX = XMIN = X = .05;
		YMAX = YMIN = Y = .05;
		ZMAX = ZMIN = Z = .05;
		WMAX = WMIN = W = .05;
		XE = X + .000001;
		YE = Y;
		ZE = Z;
		WE = W;
		LSUM = 0;
		N = 1;
		getCoefficients ();
		P = 0;
		LSUM = 0;
		NL = 0;
		N1 = 0;
		N2 = 0;
		TWOD = D << 1;
	}

	void StrangeAttractor::codeRandomize ()
	{
		O = 2 + (int) (std::floor (double(OMAX - 1)) * uniform_01());
		code.clear();
		code.push_back(59 + 4 * D + O + 8 * ODE);
		if (ODE > 1)
		{
			code[0] = (87 + ODE);
		}
		//      Get value of M.
		getDimensionAndOrder ();
		for (I = 1; I <= M; I++)
		{
			code[I] = (65 + ((int) std::floor (25 * uniform_01())));
		}
		static std::string copybuffer;
		copybuffer = code;
		Fl::copy(copybuffer.c_str(), copybuffer.length(), 1);
		// System::debug(code.c_str());
		//code.push_back(0);
	}

	void StrangeAttractor::reset ()
	{
		initialize ();
		codeRandomize ();
	}

	void StrangeAttractor::iterate ()
	{
		if (ODE > 1)
		{
			specialFunctions ();
		}
		else
		{
			M = 1;
			XY[1] = X;
			XY[2] = Y;
			XY[3] = Z;
			XY[4] = W;
			for (I = 1; I <= D; I++)
			{
				XN[I] = A[M];
				M = M + 1;
				for (I1 = 1; I1 <= D; I1++)
				{
					XN[I] = XN[I] + A[M] * XY[I1];
					M = M + 1;
					for (I2 = I1; I2 <= D; I2++)
					{
						XN[I] = XN[I] + A[M] * XY[I1] * XY[I2];
						M = M + 1;
						for (I3 = I2; O > 2 && I3 <= D; I3++)
						{
							XN[I] = XN[I] + A[M] * XY[I1] * XY[I2] * XY[I3];
							M = M + 1;
							for (I4 = I3; O > 3 && I4 <= D; I4++)
							{
								XN[I] =
									XN[I] +
									A[M] * XY[I1] * XY[I2] * XY[I3] * XY[I4];
								M = M + 1;
								for (I5 = I4; O > 4 && I5 <= D; I5++)
								{
									XN[I] =
										XN[I] +
										A[M] * XY[I1] * XY[I2] * XY[I3] * XY[I4] *
										XY[I5];
									M = M + 1;
								}
							}
						}
					}
				}
				if (ODE == 1)
				{
					XN[I] = XY[I] + EPS * XN[I];
				}
			}
			XNEW = XN[1];
			YNEW = XN[2];
			ZNEW = XN[3];
			WNEW = XN[4];
		}
		N = N + 1;
		M = M - 1;
		if (N >= 100 && N <= 1000)
		{
			if (X < XMIN)
			{
				XMIN = X;
			}
			if (X > XMAX)
			{
				XMAX = X;
			}
			if (Y < YMIN)
			{
				YMIN = Y;
			}
			if (Y > YMAX)
			{
				YMAX = Y;
			}
			if (Z < ZMIN)
			{
				ZMIN = Z;
			}
			if (Z > ZMAX)
			{
				ZMAX = Z;
			}
			if (W < WMIN)
			{
				WMIN = W;
			}
			if (W > WMAX)
			{
				WMAX = W;
			}
		}
		if (N == 1000)
		{
			if (D == 1)
			{
				YMIN = XMIN;
				YMAX = XMAX;
			}
		}
		XS[P] = X;
		YS[P] = Y;
		ZS[P] = Z;
		WS[P] = W;
		P = (P + 1) % 500;
		I = (P + 500 - PREV) % 500;
		if (D == 1)
		{
			XP = XS[I];
			YP = XNEW;
		}
		else
		{
			XP = X;
			YP = Y;
		}
	}

	void StrangeAttractor::specialFunctions ()
	{
		/*
		* Default 3rd and 4th dimension 
		*/ 
		ZNEW = X * X + Y * Y;
		WNEW = (N - 100) / 900;
		if (N > 1000)
		{
			WNEW = (N - 1000) / (NMAX - 1000);
		}
		switch (ODE)
		{
		case 2:
			M = 10;
			XNEW =
				A[1] + A[2] * X + A[3] * Y + A[4] * std::fabs (X) +
				A[5] * std::fabs (Y);
			YNEW =
				A[6] + A[7] * X + A[8] * Y + A[9] * std::fabs (X) +
				A[10] * std::fabs (Y);
			break;
		case 3:
			M = 14;
			XNEW =
				A[1] + A[2] * X + A[3] * Y +
				((int) (A[4] * X + .5) & (int) (A[5] * Y + .5)) +
				((int) (A[6] * X + .5) | (int) (A[7] * Y + .5));
			YNEW =
				A[8] + A[9] * X + A[10] * Y +
				((int) (A[11] * X + .5) & (int) (A[12] * Y + .5)) +
				((int) (A[13] * X + .5) | (int) (A[14] * Y + .5));
			break;
		case 4:
			M = 14;
			XNEW =
				A[1] + A[2] * X + A[3] * Y + A[4] * std::pow (std::fabs (X),
				A[5]) +
				A[6] * std::pow (std::fabs (Y), A[7]);
			YNEW =
				A[8] + A[9] * X + A[10] * Y + A[11] * std::pow (std::fabs (X),
				A[12]) +
				A[13] * std::pow (std::fabs (Y), A[14]);
			break;
		case 5:
			M = 18;
			XNEW =
				A[1] + A[2] * X + A[3] * Y + A[4] * std::sin (A[5] * X + A[6]) +
				A[7] * std::sin (A[8] * Y + A[9]);
			YNEW =
				A[10] + A[11] * X + A[12] * Y + A[13] * std::sin (A[14] * X +
				A[15]) +
				A[16] * std::sin (A[17] * Y + A[18]);
			break;
		case 6:
			M = 6;
			if (N < 2)
			{
				AL = Conversions::get2PI() / (13 + 10 * A[6]);
				SINAL = std::sin (AL);
				COSAL = std::cos (AL);
			}
			DUM = X + A[2] * std::sin (A[3] * Y + A[4]);
			XNEW = 10 * A[1] + DUM * COSAL + Y * SINAL;
			YNEW = 10 * A[5] - DUM * SINAL + Y * COSAL;
			break;
		case 7:
			M = 9;
			XNEW = X + EPS * A[1] * Y;
			YNEW =
				Y + EPS * (A[2] * X + A[3] * X * X * X + A[4] * X * X * Y +
				A[5] * X * Y * Y + A[6] * Y + A[7] * Y * Y * Y +
				A[8] * std::sin (Z));
			ZNEW = Z + EPS * (A[9] + 1.3);
			if (ZNEW > Conversions::get2PI())
			{
				ZNEW = ZNEW - Conversions::get2PI();
			}
		}
	}

	void StrangeAttractor::getDimensionAndOrder ()
	{
		D = 1 + (int) (std::floor (double((code[0] - 65)) / 4));
		if (D > 6)
		{
			ODE = code[0] - 87;
			D = 4;
			specialFunctions ();
		}
		else
		{
			if (D > 4)
			{
				D = D - 2;
				ODE = 1;
			}
			else
			{
				ODE = 0;
			}
			O = 2 + (code[0] - 65) % 4;
			M = 1;
			for (I = 1; I <= D; I++)
			{
				M = M * (O + I);
			}
			if (D > 2)
			{
				for (I = 3; I <= D; I++)
				{
					M = M / (I - 1);
				}
			}
		}
		if (code.length() != M + 1)
		{
			while (code.length() < (size_t) (M + 1))
			{
				code.append("M");
			}
		}
		code.resize(M + 1);
	}

	void StrangeAttractor::getCoefficients ()
	{
		getDimensionAndOrder ();
		for (I = 1; I <= M; I++)
		{
			A[I] = (code[I] - 77) / 10.0;
		}
	}

	size_t StrangeAttractor::getIterationCount () const
	{
		return NMAX - 1000;
	}

	void StrangeAttractor::setIterationCount (size_t newValue)
	{
		NMAX = int(newValue + 1000);
	}

	size_t StrangeAttractor::getIteration () const
	{
		return N;
	}

	void StrangeAttractor::setIteration (size_t newValue)
	{
		N = (int) newValue;
	}

	void StrangeAttractor::calculateLyupanovExponent ()
	{
		XSAVE = XNEW;
		YSAVE = YNEW;
		ZSAVE = ZNEW;
		WSAVE = WNEW;
		X = XE;
		Y = YE;
		Z = ZE;
		W = WE;
		N = N - 1;
		iterate ();
		DLX = XNEW - XSAVE;
		DLY = YNEW - YSAVE;
		DLZ = ZNEW - ZSAVE;
		DLW = WNEW - WSAVE;
		DL2 = DLX * DLX + DLY * DLY + DLZ * DLZ + DLW * DLW;
		if (DL2 > 0)
		{
			DF = 1E12 * DL2;
			RS = 1 / std::sqrt (DF);
			XE = XSAVE + RS * (XNEW - XSAVE);
			YE = YSAVE + RS * (YNEW - YSAVE);
			ZE = ZSAVE + RS * (ZNEW - ZSAVE);
			WE = WSAVE + RS * (WNEW - WSAVE);
			XNEW = XSAVE;
			YNEW = YSAVE;
			ZNEW = ZSAVE;
			WNEW = WSAVE;
			LSUM = LSUM + std::log (DF);
			NL = NL + 1;
			L = .721347 * LSUM / NL;
			if (ODE == 1 || ODE == 7)
			{
				L = L / EPS;
			}
		}
	}

	void StrangeAttractor::calculateFractalDimension ()
	{
		/*
		* Wait for transient to settle 
		*/ if (N >= 1000)
		{
			if (N == 1000)
			{
				D2MAX = std::pow (XMAX - XMIN, 2);
				D2MAX = D2MAX + std::pow (YMAX - YMIN, 2);
				D2MAX = D2MAX + std::pow (ZMAX - ZMIN, 2);
				D2MAX = D2MAX + std::pow (WMAX - WMIN, 2);
			}
			J = (P + 1 + ((int) (std::floor (480 * uniform_01())))) % 500;
			DX = XNEW - XS[J];
			DY = YNEW - YS[J];
			DZ = ZNEW - ZS[J];
			DW = WNEW - WS[J];
			D2 = DX * DX + DY * DY + DZ * DZ + DW * DW;
			if (D2 < .001 * TWOD * D2MAX)
			{
				N2 = N2 + 1;
			}
			if (D2 <= .00001 * TWOD * D2MAX)
			{
				N1 = N1 + 1;
				F = .434294 * std::log (N2 / (N1 - .5));
			}
		}
	}

	double StrangeAttractor::getX () const
	{
		return X;
	}

	void StrangeAttractor::setX (double newValue)
	{
		X = newValue;
	}

	double StrangeAttractor::getY () const
	{
		return Y;
	}

	void StrangeAttractor::setY (double newValue)
	{
		Y = newValue;
	}

	double StrangeAttractor::getZ () const
	{
		return Z;
	}

	void StrangeAttractor::setZ (double newValue)
	{
		Z = newValue;
	}

	double StrangeAttractor::getW () const
	{
		return W;
	}

	void StrangeAttractor::setW (double newValue)
	{
		W = newValue;
	}

	double StrangeAttractor::getFractalDimension () const
	{
		return F;
	}

	double StrangeAttractor::getLyupanovExponent () const
	{
		return L;
	}

	int StrangeAttractor::getScoreType () const
	{
		return scoreType;
	}

	void StrangeAttractor::setScoreType (int newValue)
	{
		scoreType = newValue;
	}

	void StrangeAttractor::render (int N, double X, double Y, double Z, double W)
	{
		pitchClassSet = W * 4095.0;
		switch (D)
		{
		case 1:
			instrument = 1.0;
			time = ((double) N) / 8.0;
			duration = 0.25;
			octave = X;
			decibels = 70.0;
			x = uniform_01() * 2.0 - 1.0;
			break;
		case 2:switch (scoreType)
					 {
		case 1:
			instrument = X;
			time = ((double) N) / 8.0;
			duration = 0.25;
			octave = Y;
			decibels = 70.0;
			x = uniform_01() * 2.0 - 1.0;
			break;
		case 0:
			instrument = 1.0;
			time = X;
			duration = 0.25;
			octave = Y;
			decibels = 70.0;
			x = uniform_01() * 2.0 - 1.0;
			break;
					 }
					 break;
		case 3:
			switch (scoreType)
			{
			case 1:
				instrument = X;
				time = ((double) N) / 8.0;
				duration = 0.25;
				octave = Y;
				decibels = Z;
				x = uniform_01() * 2.0 - 1.0;
				break;
			case 0:
				instrument = Z;
				time = X;
				duration = 0.25;
				octave = Y;
				decibels = 70.0;
				x = uniform_01() * 2.0 - 1.0;
				break;
			}
			break;
		case 4:
			switch (scoreType)
			{
			case 1:
				instrument = X;
				time = ((double) N) / 8.0;
				duration = W;
				octave = Y;
				decibels = ((double) Z);
				x = uniform_01() * 2.0 - 1.0;
				break;
			case 0:
				instrument = Z;
				time = X;
				duration = 0.25;
				octave = Y;
				decibels = W;
				x = uniform_01() * 2.0 - 1.0;
				break;
			}
			break;
		}
		score.append(time, duration, 144.0, instrument, Conversions::octaveToMidi(octave, false), decibels, 0.0, x, 0.0, 0.0, pitchClassSet);
	}

	void StrangeAttractor::shuffleRandomNumbers ()
	{
		if (V[0] == 0)
		{
			for (J = 0; J <= 99; J++)
			{
				V[J] = uniform_01();
			}
		}
		J = (int) std::floor (100 * RAN);
		RAN = V[J];
		V[J] = uniform_01();
	}

	int StrangeAttractor::getAttractorType () const
	{
		if (ODE == 0)
		{
			return D;
		}
		if (ODE == 1)
		{
			return D + 2;
		}
		return D + 1 + ODE;
	}

	void StrangeAttractor::setAttractorType (int newValue)
	{
		D = newValue;
		if (D > 6)
		{
			ODE = D - 5;
			D = 4;
		}
		else
		{
			if (D > 4)
			{
				ODE = 1;
				D = D - 2;
			}
			else
				ODE = 0;
		}
	}

	bool StrangeAttractor::evaluateAttractor ()
	{
		if (N == 1)
		{
			score.clear ();
			reinitialize ();
		}
		if (N >= NMAX)
		{
			return false;
		}
		iterate ();
		if (N >= 1000)
		{
			calculateFractalDimension ();
			calculateLyupanovExponent ();
		}
		X = XNEW;
		Y = YNEW;
		Z = ZNEW;
		W = WNEW;
		if (N >= 1000 && N < NMAX)
		{
			render (N, X, Y, Z, W);
		}
		return true;
	}

	bool StrangeAttractor::searchForAttractor ()
	{
		//      On the first iteration, generate a new code.
		if (N == 1)
		{
			codeRandomize ();
			reinitialize ();
		}
		iterate ();
		if (N >= 100)
		{
			calculateFractalDimension ();
			calculateLyupanovExponent ();
		}
		//      The attractor at infinity has been found.
		if ((std::fabs (XNEW) + std::fabs (YNEW) + std::fabs (ZNEW) + std::fabs (WNEW)) > 1000000)
		{
			//      It can't be musical, so force an immediate new search.
			N = 1;
			return true;
		}
		//      A strange attractor has been found.
		if (N >= NMAX)
		{
			//      End the search.
			return false;
		}
		//      The attractor at zero has been found.
		if ((std::fabs (XNEW - X) + std::fabs (YNEW - Y) + std::fabs (ZNEW - Z) + std::fabs (WNEW - W)) < .000001)
		{
			//      It can't be musical, so force an immediate new search.
			N = 1;
			return true;
		}
		//      A periodic attractor has been found.
		if ((N > 100) && (L < 0.005))
		{
			//      Force an immediate new search.
			N = 1;
			return true;
		}
		X = XNEW;
		Y = YNEW;
		Z = ZNEW;
		W = WNEW;
		//      Keep iterating.
		return true;
	}

	void StrangeAttractor::generate ()
	{
		for (N = 1; evaluateAttractor (); )
		{
		}
	}

	void StrangeAttractor::setDimensionCount(int D)
	{
		this->D = D;
	}
	
	int StrangeAttractor::getDimensionCount() const
	{
		return D;
	}

}
