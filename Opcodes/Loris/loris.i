/*
 * This is the Loris C++ Class Library, implementing analysis, 
 * manipulation, and synthesis of digitized sounds using the Reassigned 
 * Bandwidth-Enhanced Additive Sound Model.
 *
 * Loris is Copyright (c) 1999-2004 by Kelly Fitz and Lippold Haken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY, without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 *	loris.i
 *
 *  SWIG interface file for building scripting language modules
 *  implementing Loris functionality. This interface has been 
 *  completely rewritten (23 Jan 2003) to support new versions
 *  of SWIG (current is 1.3.17) and take advantage of new features
 *  and bug fixes. This interface wraps many functions in the 
 *  Loris procedural interface, but also provides some Loris C++
 *  class wrappers, to provide enhanced functionality in the 
 *  context of laguages that perform garbage collection.
 *
 *	Also, several interface (.i) files were collapsed into one
 *	(not sure I did myself any favors).
 *
 * Kelly Fitz, 8 Nov 2000
 * rewrite: Kelly Fitz, 23 Jan 2003
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
//	perl defines list and screws us up,
//	undefine it so that we can use std::list
#if defined (SWIGPERL)
	%{
		#undef list
	%}
#endif
%module loris
// ----------------------------------------------------------------
//		notification and exception handlers
//
%{ 
/*	exception handling code for procedural interface calls

	Copied from the SWIG manual. Tastes great, less filling.
*/
static char error_message[256];
static int error_status = 0;

void throw_exception(const char *msg) {
        strncpy(error_message,msg,256);
        error_status = 1;
}

void clear_exception() {
        error_status = 0;
}
char *check_exception() {
        if (error_status) return error_message;
        else return NULL;
}

#include "loris.h"

//	import the entire Loris namespace, because
//	SWIG does not seem to like to wrap functions
//	with qualified names (like Loris::channelize),
//	they simply get ignored.
//
// (This has probably been fixed by now.)
using namespace Loris;

//	notification function for Loris debugging
//	and notifications, installed in initialization
//	block below:
static void printf_notifier( const char * s )
{
	printf("*\t%s\n", s);
}	

%}

//	Configure notification and debugging using a
//	in a SWIG initialization block. This code is
//	executed when the module is loaded by the 
//	host interpreter.
//
%init 
%{
	Loris::setNotifier( printf_notifier );
	Loris::setExceptionHandler( throw_exception );
%}

// ----------------------------------------------------------------
//		helper functions for creating vectors of doubles
//

%include "std_vector.i"

%{
	#include "Marker.h"	// for defining a vector of Markers
%}

namespace std {
   %template(DoubleVector) vector< double >;
   %template(MarkerVector) vector< Marker >;
};

%pythoncode 
%{
	SampleVector = DoubleVector
%}

%include exception.i 
%exception 
{
    char * err;
    clear_exception();
    $action
    if ((err = check_exception()))
    {
        SWIG_exception( SWIG_ValueError, err );
    }
}

// ----------------------------------------------------------------
//		wrap procedural interface
//
//	Not all functions in the procedural interface are trivially
//	wrapped, some are wrapped to return newly-allocated objects,
//	which we wouldn't do in the procedural interface, but we
//	can do, because SWIG and the scripting langauges take care of 
//	the memory management ambiguities.
//

void channelize( PartialList * partials, 
				 BreakpointEnvelope * refFreqEnvelope, int refLabel );
/*	Label Partials in a PartialList with the integer nearest to
	the amplitude-weighted average ratio of their frequency envelope
	to a reference frequency envelope. The frequency spectrum is 
	partitioned into non-overlapping channels whose time-varying 
	center frequencies track the reference frequency envelope. 
	The reference label indicates which channel's center frequency
	is exactly equal to the reference envelope frequency, and other
	channels' center frequencies are multiples of the reference 
	envelope frequency divided by the reference label. Each Partial 
	in the PartialList is labeled with the number of the channel
	that best fits its frequency envelope. The quality of the fit
	is evaluated at the breakpoints in the Partial envelope and
	weighted by the amplitude at each breakpoint, so that high-
	amplitude breakpoints contribute more to the channel decision.
	Partials are labeled, but otherwise unmodified. In particular, 
	their frequencies are not modified in any way.
 */


%newobject createFreqReference;
BreakpointEnvelope * 
createFreqReference( PartialList * partials, 
					 double minFreq, double maxFreq, long numSamps = 0 );
/*	Return a newly-constructed BreakpointEnvelope by sampling the 
	frequency envelope of the longest Partial in a PartialList. 
	Only Partials whose frequency at the Partial's loudest (highest 
	amplitude) breakpoint is within the given frequency range are 
	considered. 
	
	If the number of sample points is not specified, then the
	longest Partial's frequency envelope is sampled every 30 ms
	(No fewer than 10 samples are used, so the sampling maybe more
	dense for very short Partials.) 
	
	For very simple sounds, this frequency reference may be a 
	good first approximation to a reference envelope for
	channelization (see channelize()).
 */

%{
#include "Exception.h"
#include "Notifier.h"
#include <vector>

	void dilate_v( PartialList * partials, 
		   		   const std::vector<double> & ivec, 
				   const std::vector<double> & tvec )
	{
		Loris::debugger << ivec.size() << " initial points, " 
						<< tvec.size() << " target points" << Loris::endl;
						
		if ( ivec.size() != tvec.size() )
		{
			Throw( InvalidArgument, "Invalid arguments to dilate(): there must be as many target points as initial points" );
		}
		
		const double * initial = &(ivec[0]);
		const double * target = &(tvec[0]);
		int npts = ivec.size();
		dilate( partials, const_cast<double *>(initial), const_cast<double *>(target), npts );
	}
%}

%exception dilate_v
{
    char * err;
    clear_exception();
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
    if ((err = check_exception()))
    {
        SWIG_exception( SWIG_ValueError, err );
    }
}

%rename (dilate) dilate_v;
void dilate_v( PartialList * partials, 
			   const std::vector<double> & ivec, 
			   const std::vector<double> & tvec );

/*	Dilate Partials in a PartialList according to the given 
	initial and target time points. Partial envelopes are 
	stretched and compressed so that temporal features at
	the initial time points are aligned with the final time
	points. Time points are sorted, so Partial envelopes are 
	are only stretched and compressed, but breakpoints are not
	reordered. Duplicate time points are allowed. There must be
	the same number of initial and target time points.
	
	The time points are passed as strings; convert any native
	collection to a string representation, numerical elements
	will be extracted, other characters will be ignored.
 */


void distill( PartialList * partials );
/*	Distill labeled (channelized)  Partials in a PartialList into a 
	PartialList containing a single (labeled) Partial per label. 
	The distilled PartialList will contain as many Partials as
	there were non-zero labels (non-empty channels)
	in the original PartialList. Additionally, unlabeled (label 0) Partials are 
	"collated" into groups of temporally non-overlapping Partials,
	assigned an unused label, and fused into a single Partial per
	group.
 */
				 

%inline 
%{
	void exportAiff( const char * path, const std::vector< double > & samples,
					 double samplerate = 44100.0, int nchannels = 1, 
					 int bitsPerSamp = 16 )
	{
		exportAiff( path, &samples, samplerate, nchannels, bitsPerSamp );
	}
%}
/*	Export audio samples stored in a SampleVector to an AIFF file
	having the specified number of channels and sample rate at the 
	given file path (or name). The floating point samples in the 
	SampleVector are clamped to the range (-1.,1.) and converted 
	to integers having bitsPerSamp bits. The default values for the
	sample rate, number of channels, and sample size, if unspecified,
	are 44100 Hz (CD quality), 1 channel, and 16 bits per sample, 
	respectively.
 */


void exportSdif( const char * path, PartialList * partials );
/*	Export Partials in a PartialList to a SDIF file at the specified
	file path (or name). SDIF data is written in the 1TRC format.  
	For more information about SDIF, see the SDIF website at:
		www.ircam.fr/equipes/analyse-synthese/sdif/  
 */



void exportSpc( const char * path, PartialList * partials, double midiPitch, 
				int enhanced = true, double endApproachTime = 0. );
/*	Export Partials in a PartialList to a Spc file at the specified file
	path (or name). The fractional MIDI pitch must be specified. The optional
	enhanced parameter defaults to true (for bandwidth-enhanced spc files), 
	but an be specified false for pure-sines spc files. The optional 
	endApproachTime parameter is in seconds; its default value is zero (and 
	has no effect). A nonzero endApproachTime indicates that the PartialList does 
	not include a release, but rather ends in a static spectrum corresponding 
	to the final breakpoint values of the partials. The endApproachTime
	specifies how long before the end of the sound the amplitude, frequency, 
	and bandwidth values are to be modified to make a gradual transition to 
	the static spectrum.
 */


%newobject importSdif;
%inline %{
	PartialList * importSdif( const char * path )
	{
		PartialList * dst = createPartialList();
		importSdif( path, dst );

		// check for exception:
		if (check_exception())
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
%}
/*  Import Partials from an SDIF file at the given file path (or
	name), and return them in a PartialList.
	For more information about SDIF, see the SDIF website at:
		www.ircam.fr/equipes/analyse-synthese/sdif/
 */

%{
	#include "BreakpointEnvelope.h"
%}

%newobject importSpc;
%inline %{
	PartialList * importSpc( const char * path )
	{
		PartialList * dst = createPartialList();
		importSpc( path, dst );

		// check for exception:
		if (check_exception())
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
%}
/*  Import Partials from an Spc file at the given file path (or
	name), and return them in a PartialList.
 */

%newobject morph;
%inline %{
	PartialList * morph( const PartialList * src0, const PartialList * src1, 
						 const BreakpointEnvelope * ffreq, 
						 const BreakpointEnvelope * famp, 
						 const BreakpointEnvelope * fbw )
	{
		PartialList * dst = createPartialList();
		morph( src0, src1, ffreq, famp, fbw, dst );
		
		// check for exception:
		if ( check_exception() )
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
	PartialList * morph( const PartialList * src0, const PartialList * src1, 
						 double freqweight, 
						 double ampweight, 
						 double bwweight )
	{
		BreakpointEnvelope ffreq( freqweight ), famp( ampweight ), fbw( bwweight );
		
		PartialList * dst = createPartialList();
		morph( src0, src1, &ffreq, &famp, &fbw, dst );
		
		// check for exception:
		if ( check_exception() )
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
%}
/*  Morph labeled Partials in two PartialLists according to the
	given frequency, amplitude, and bandwidth (noisiness) morphing
	envelopes, and return the morphed Partials in a PartialList.
	Loris morphs Partials by interpolating frequency, amplitude,
	and bandwidth envelopes of corresponding Partials in the
	source PartialLists. For more information about the Loris
	morphing algorithm, see the Loris website:
	www.cerlsoundgroup.org/Loris/
 */


%newobject synthesize;
%inline %{
	std::vector<double> synthesize( const PartialList * partials, double srate = 44100.0 )
	{
		std::vector<double> dst;
		synthesize( partials, &dst, srate );
		return dst;
	}
%}
/*  Synthesize Partials in a PartialList at the given sample
	rate, and return the (floating point) samples in a SampleVector.
	The SampleVector is sized to hold as many samples as are needed
	for the complete synthesis of all the Partials in the PartialList.
	If the sample rate is unspecified, the deault value of 44100 Hz
	(CD quality) is used.
 */

void crop( PartialList * partials, double t1, double t2 );
/*	Trim Partials by removing Breakpoints outside a specified time span.
	Insert a Breakpoint at the boundary when cropping occurs.
 */

%newobject copyLabeled;
%inline %{
	PartialList * copyLabeled( PartialList * partials, long label )
	{
		PartialList * dst = createPartialList();
		copyLabeled( partials, label, dst );
		
		// check for exception:
		if ( check_exception() )
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
%}
/*  Copy Partials in the source PartialList having the specified
    label into a new PartialList. The source PartialList is
    unmodified.
 */

%newobject extractLabeled;
%inline %{
	PartialList * extractLabeled( PartialList * partials, long label )
	{
		PartialList * dst = createPartialList();
		extractLabeled( partials, label, dst );
		
		// check for exception:
		if ( check_exception() )
		{
			destroyPartialList( dst );
			dst = NULL;
		}
		return dst;
	}
%}
/*  Extract Partials in the source PartialList having the specified
    label and return them in a new PartialList.
 */

void removeLabeled( PartialList * partials, long label );
/*  Remove from a PartialList all Partials having the specified label.
 */

void resample( PartialList * partials, double interval );
/*  Resample all Partials in a PartialList using the specified
	sampling interval, so that the Breakpoints in the Partial 
	envelopes will all lie on a common temporal grid.
	The Breakpoint times in resampled Partials will comprise a  
	contiguous sequence of integer multiples of the sampling interval,
	beginning with the multiple nearest to the Partial's start time and
	ending with the multiple nearest to the Partial's end time. Resampling
	is performed in-place. 

 */
 
void scaleAmp( PartialList * partials, BreakpointEnvelope * ampEnv );
/*	Scale the amplitude of the Partials in a PartialList according 
	to an envelope representing a time-varying amplitude scale value.
 */
				 
void scaleBandwidth( PartialList * partials, BreakpointEnvelope * bwEnv );
/*	Scale the bandwidth of the Partials in a PartialList according 
	to an envelope representing a time-varying bandwidth scale value.
 */
				 
void scaleFrequency( PartialList * partials, BreakpointEnvelope * freqEnv );
/*	Scale the frequency of the Partials in a PartialList according 
	to an envelope representing a time-varying frequency scale value.
 */
				 
void scaleNoiseRatio( PartialList * partials, BreakpointEnvelope * noiseEnv );
/*	Scale the relative noise content of the Partials in a PartialList 
	according to an envelope representing a (time-varying) noise energy 
	scale value.
 */

void shiftPitch( PartialList * partials, BreakpointEnvelope * pitchEnv );
/*	Shift the pitch of all Partials in a PartialList according to 
	the given pitch envelope. The pitch envelope is assumed to have 
	units of cents (1/100 of a halfstep).
 */

%inline %{	
	void scaleAmp( PartialList * partials, double w )
	{
		BreakpointEnvelope e( w );
		scaleAmp( partials, &e );
	}
	
	
	void scaleBandwidth( PartialList * partials, double w )
	{
		BreakpointEnvelope e( w );
		scaleBandwidth( partials, &e );
	}
	
	
	void scaleFrequency( PartialList * partials, double w )
	{
		BreakpointEnvelope e( w );
		scaleFrequency( partials, &e );
	}
	
	
	void scaleNoiseRatio( PartialList * partials, double w )
	{
		BreakpointEnvelope e( w );
		scaleNoiseRatio( partials, &e );
	}
	
	
	void shiftPitch( PartialList * partials, double w )
	{
		BreakpointEnvelope e( w );
		shiftPitch( partials, &e );
	}
%}	
	


void shiftTime( PartialList * partials, double offset );
/*	Shift the time of all the Breakpoints in a Partial by a 
	constant amount.
 */

void sift( PartialList * partials );
/*  Eliminate overlapping Partials having the same label
	(except zero). If any two partials with same label
   	overlap in time, keep only the longer of the two.
   	Set the label of the shorter duration partial to zero.
 */

void sortByLabel( PartialList * partials );
/*	Sort the Partials in a PartialList in order of increasing label.
 	The sort is stable; Partials having the same label are not 
 	reordered.
 */

%inline %{
	const char * version( void )
	{
		static const char * vstr = LORIS_VERSION_STR;
		return vstr;
	}
%}
 
// ----------------------------------------------------------------
//		wrap Loris classes

//	Wrap all calls into the Loris library with exception
//	handlers to prevent exceptions from leaking out of the
//	C++ code, wherein they can be handled, and into the
//	interpreter, where they will surely cause an immediate
//	halt. Only std::exceptions and Loris::Exceptions (and 
//	subclasses) can be thrown.
//
//	Don't use procedural interface calls here, because this 
//	exception handler doesn't check for exceptions raised in
//	the procedural interface!
//
%{
	#include "Exception.h"
	#include <stdexcept>
%}

//	These should probably not all report UnknownError, could
//	make an effort to raise the right kind of (SWIG) exception.
//
%exception {
	try
	{	
		$action
	}
	catch( Loris::Exception & ex ) 
	{
		//	catch Loris::Exceptions:
		std::string s("Loris exception: " );
		s.append( ex.what() );
		SWIG_exception( SWIG_UnknownError, (char *) s.c_str() );
	}
	catch( std::exception & ex ) 
	{
		//	catch std::exceptions:
		std::string s("std C++ exception: " );
		s.append( ex.what() );
		SWIG_exception( SWIG_UnknownError, (char *) s.c_str() );
	}
}

// several classes define a copy member that
// returns a new object:
//
%newobject *::copy;

// ---------------------------------------------------------------------------
//	class Marker
//
//	Class Marker represents a labeled time point in a set of Partials
//	or a vector of samples. Collections of Markers (see the MarkerContainer
//	definition below) are held by the File I/O classes in Loris (AiffFile,
//	SdifFile, and SpcFile) to identify temporal features in imported
//	and exported data.
//
%{
	#include "Marker.h"
%}

class Marker
{
public:
//	-- construction --
	Marker( void );
	/*	Default constructor - initialize a Marker at time zero with no label.
	 */
	 
	Marker( double t, const char * s );
	/*	Initialize a Marker with the specified time (in seconds) and name.
	 */
	 
	Marker( const Marker & other );
	/*	Initialize a Marker that is an exact copy of another Marker, that is,
		having the same time and name.
	 */
		 
//	-- access --
	%extend 
	{
		const char * name( void ) { return self->name().c_str(); }
	}
	/*	Return a reference (or const reference) to the name string
		for this Marker.
	 */
	 
	double time( void );
	/*	Return the time (in seconds) associated with this Marker.
	 */
	 
//	-- mutation --
	void setName( const char * s );
	/*	Set the name of the Marker.
	 */
	 
	void setTime( double t );
	/* 	Set the time (in seconds) associated with this Marker.
	 */

	
};	//	end of class Marker

// ---------------------------------------------------------------------------
//	class AiffFile
//	
//	An AiffFile represents a sample file (on disk) in the Audio Interchange
//	File Format. The file is read from disk and the samples stored in memory
//	upon construction of an AiffFile instance. The samples are accessed by 
//	the samples() method, which converts them to double precision floats and
//	returns them in a SampleVector.
//
%{
	#include "AiffFile.h"
%}

%newobject AiffFile::samples;
%newobject AiffFile::getMarker;
%exception AiffFile::getMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}
%exception AiffFile::removeMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}


class AiffFile
{
public:
	AiffFile( const char * filename );
	AiffFile( const std::vector< double > & vec, double samplerate = 44100 );

	~AiffFile( void );
	
	double sampleRate( void ) const;
	double midiNoteNumber( void ) const;

	//	this has been renamed
	%rename( sampleFrames ) numFrames;
	unsigned long numFrames( void ) const;
		
	// int sampleSize( void ) const;
	//	cannot implement sampleSize anymore, that
	//	has only to do with writing, samples are
	//	converted as they are read in now.

	void addPartial( const Loris::Partial & p, double fadeTime = .001 /* 1 ms */ );
	/*	Render the specified Partial using the (optionally) specified
		Partial fade time, and accumulate the resulting samples into
		the sample vector for this AiffFile.
	 */

	void setMidiNoteNumber( double nn );
	/*	Set the fractional MIDI note number assigned to this AiffFile. 
		If the sound has no definable pitch, use note number 60.0 (the default).
	 */
		 
	void write( const char * filename, unsigned int bps = 16 );
	/*	Export the sample data represented by this AiffFile to
		the file having the specified filename or path. Export
		signed integer samples of the specified size, in bits
		(8, 16, 24, or 32).
	*/
	
	%extend 
	{
		AiffFile( PartialList * l, double sampleRate = 44100, double fadeTime = .001 ) 
		{
			return new AiffFile( l->begin(), l->end(), sampleRate, fadeTime );
		}
		/*	Initialize an instance of AiffFile having the specified sample 
			rate, accumulating samples rendered at that sample rate from
			all Partials on the specified half-open (STL-style) range with
			the (optionally) specified Partial fade time (see Synthesizer.h
			for an examplanation of fade time). 
		 */
	
		//	return a copy of the sample vector 
		//	from this AiffFile
		std::vector< double > samples( void )
		{
			return self->samples();
		}
		/*	Return a SampleVector containing the AIFF samples from this AIFF 
			file as double precision floats on the range -1,1.
		 */
		 
		//	Loris only deals in mono AiffFiles
		int channels( void ) { return 1; }

		//	add a PartialList of Partials:
		void addPartials( PartialList * l, double fadeTime = 0.001/* 1ms */ )
		{
			self->addPartials( l->begin(), l->end(), fadeTime );
		}
		/*	Render all Partials on the specified half-open (STL-style) range
			with the (optionally) specified Partial fade time (see Synthesizer.h
			for an examplanation of fade time), and accumulate the resulting 
			samples. 
		 */
		 
		//	add members to access Markers
		// 	now much improved to take advantage of 
		// 	SWIG support for std::vector.
		
		int numMarkers( void ) { return self->markers().size(); }
		
		Marker * getMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			return new Marker( self->markers()[i] );
		}
		
		void removeMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			self->markers().erase( self->markers().begin() + i );
		}
		
		void addMarker( Marker m )
		{
			self->markers().push_back( m );
		}
		
		void clearMarkers( void )
		{
			self->markers().clear();
		}
		
		std::vector< Marker > markers( void )
		{
			return self->markers();
		}
		
		void addMarkers( const std::vector< Marker > & markers )
		{
			self->markers().insert( self->markers().end(), 
									markers.begin(), markers.end() );
		}
	}
};

// ---------------------------------------------------------------------------
//	class Analyzer
//	
//	An Analyzer represents a configuration of parameters for
//	performing Reassigned Bandwidth-Enhanced Additive Analysis
//	of sampled waveforms. This analysis process yields a collection 
//	of Partials, each having a trio of synchronous, non-uniformly-
//	sampled breakpoint envelopes representing the time-varying 
//	frequency, amplitude, and noisiness of a single bandwidth-
//	enhanced sinusoid. 
//
//	For more information about Reassigned Bandwidth-Enhanced 
//	Analysis and the Reassigned Bandwidth-Enhanced Additive Sound 
//	Model, refer to the Loris website: www.cerlsoundgroup.org/Loris/
//

%{
	#include "Analyzer.h"
	#include "Partial.h"
%}

%newobject Analyzer::analyze;
			
class Analyzer
{
public:
	%extend 
	{
		//	construction:
		Analyzer( double resolutionHz, double windowWidthHz = 0. )
		{
			if ( windowWidthHz == 0. )
				windowWidthHz = resolutionHz;
			return new Analyzer( resolutionHz, windowWidthHz );
		}
		/*	Construct and return a new Analyzer configured with the given	
			frequency resolution (minimum instantaneous frequency	
			difference between Partials) and analysis window main 
			lobe width (between zeros). All other Analyzer parameters 	
			are computed from the specified resolution and window
			width. If the window width is not specified, or is 0,
			then it is assumed to be equal to the resolution. 			
		 */
	
		Analyzer * copy( void )
		{
			return new Analyzer( *self );
		}
		/*	Construct and return a new Analyzer having identical
			parameter configuration to another Analyzer.			
		 */
	
		//	analysis:
		PartialList * analyze( const std::vector< double > & vec, double srate )
		{
			PartialList * partials = new PartialList();
			if ( ! vec.empty() )
			{
				self->analyze( vec, srate );
			}
			partials->splice( partials->end(), self->partials() );
			return partials;
		}
		/*	Analyze a SampleVector of (mono) samples at the given sample rate 	  	
			(in Hz) and return the resulting Partials in a PartialList. 												
		 */
		 
		PartialList * analyze( const std::vector< double > & vec, double srate, 
							   BreakpointEnvelope * env )
		{
			PartialList * partials = new PartialList();
			if ( ! vec.empty() )
			{
				self->analyze( vec, srate, *env );
			}
			partials->splice( partials->end(), self->partials() );
			return partials;
		}
		/*	Analyze a SampleVector of (mono) samples at the given sample rate 	  	
			(in Hz) and return the resulting Partials in a PartialList. Use 
			the specified frequency envelope as a fundamental reference for
			Partial formation.
		 */
	}
	
	//	parameter access:
	double freqResolution( void ) const;
	double ampFloor( void ) const;
 	double windowWidth( void ) const;
 	double sidelobeLevel( void ) const;
 	double freqFloor( void ) const;
	double hopTime( void ) const;
 	double freqDrift( void ) const;
 	double cropTime( void ) const;
	double bwRegionWidth( void ) const;
	
	//	parameter mutation:
	void setFreqResolution( double x );
	void setAmpFloor( double x );
	void setWindowWidth( double x );
	void setSidelobeLevel( double x );
	void setFreqFloor( double x );
	void setFreqDrift( double x );
 	void setHopTime( double x );
 	void setCropTime( double x );
 	void setBwRegionWidth( double x );

};	//	end of class Analyzer
			
// ---------------------------------------------------------------------------
//	class BreakpointEnvelope
//
//	A BreakpointEnvelope represents a linear segment breakpoint 
//	function with infinite extension at each end (that is, the 
//	values past either end of the breakpoint function have the 
//	values at the nearest end).
//
%{
	#include "BreakpointEnvelope.h"
%}

class BreakpointEnvelope
{
public:
	//	construction:
	BreakpointEnvelope( void );
	BreakpointEnvelope( double initialValue );
	~BreakpointEnvelope( void );
	
	%extend 
	{
		BreakpointEnvelope * copy( void )
		{
			return new BreakpointEnvelope( *self );
		}
		/*	Construct and return a new BreakpointEnvelope that is
			a copy of this BreapointEnvelope (has the same value
			as this BreakpointEnvelope everywhere).			
		 */
	}

	//	envelope access and mutation:
	void insertBreakpoint( double time, double value );
	double valueAt( double x ) const;		
	 
};	//	end of class BreakpointEnvelope

%newobject BreakpointEnvelopeWithValue;

%inline %{
	BreakpointEnvelope *
	BreakpointEnvelopeWithValue( double initialValue )
	{
		return new BreakpointEnvelope( initialValue );
	}
%}

// ---------------------------------------------------------------------------
//	class SdifFile
//
//	Class SdifFile represents reassigned bandwidth-enhanced Partial 
//	data in a SDIF-format data file. Construction of an SdifFile 
//	from a stream or filename automatically imports the Partial
//	data. 
%{
	#include "SdifFile.h"
%}


%newobject SdifFile::partials;
%newobject SdifFile::getMarker;
%exception SdifFile::getMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}
%exception SdifFile::removeMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}

class SdifFile
{
public:
 	SdifFile( const char * filename );
	/*	Initialize an instance of SdifFile by importing Partial data from
		the file having the specified filename or path.
	*/
  
	SdifFile( void );
	/*	Initialize an empty instance of SdifFile having no Partials.
	 */
	 
	 ~SdifFile( void );
		
	void write( const char * path );
	/*	Export the envelope Partials represented by this SdifFile to
		the file having the specified filename or path.
	*/

	void write1TRC( const char * path );
	/*	Export the envelope Partials represented by this SdifFile to
		the file having the specified filename or path in the 1TRC
		format, resampled, and without phase or bandwidth information.
	*/
	
	%extend 
	{
		SdifFile( PartialList * l ) 
		{
			return new SdifFile( l->begin(), l->end() );
		}
	
		//	return a copy of the Partials represented by this SdifFile.
		PartialList * partials( void )
		{
			PartialList * plist = new PartialList( self->partials() );
			return plist;
		}
		 
		//	add a PartialList of Partials:
		void addPartials( PartialList * l )
		{
			self->addPartials( l->begin(), l->end() );
		}
		 
		//	add members to access Markers
		// 	now much improved to take advantage of 
		// 	SWIG support for std::vector.
		
		int numMarkers( void ) { return self->markers().size(); }
		
		Marker * getMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			return new Marker( self->markers()[i] );
		}
		
		void removeMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			self->markers().erase( self->markers().begin() + i );
		}
		
		void addMarker( Marker m )
		{
			self->markers().push_back( m );
		}
		
		void clearMarkers( void )
		{
			self->markers().clear();
		}
		
		std::vector< Marker > markers( void )
		{
			return self->markers();
		}
		
		void addMarkers( const std::vector< Marker > & markers )
		{
			self->markers().insert( self->markers().end(), 
									markers.begin(), markers.end() );
		}
	}	
		 
};	//	end of class SdifFile

// ---------------------------------------------------------------------------
//	class SpcFile
//
//	Class SpcFile represents a collection of reassigned bandwidth-enhanced
//	Partial data in a SPC-format envelope stream data file, used by the
//	real-time bandwidth-enhanced additive synthesizer implemented on the
//	Symbolic Sound Kyma Sound Design Workstation. Class SpcFile manages 
//	file I/O and conversion between Partials and envelope parameter streams.
//	
%{
	#include "SpcFile.h"
%}

%newobject SpcFile::partials;
%newobject SpcFile::getMarker;
%exception SpcFile::getMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}
%exception SpcFile::removeMarker
{
	try
	{
		$action
	}
	catch ( InvalidArgument & ex )
	{
		SWIG_exception(SWIG_ValueError, (char *)ex.what() );
	}
}


class SpcFile
{
public:
	SpcFile( const char * filename );
	/*	Initialize an instance of SpcFile by importing envelope parameter 
		streams from the file having the specified filename or path.
	*/
	
	SpcFile( double midiNoteNum = 60 );
	/*	Initialize an instance of SpcFile having the specified fractional
		MIDI note number, and no Partials (or envelope parameter streams). 
	*/

	~SpcFile( void );
	
	double sampleRate( void ) const;
	double midiNoteNumber( void ) const;

	void addPartial( const Loris::Partial & p );
	/*	Add the specified Partial to the enevelope parameter streams
		represented by this SpcFile. 
		
		A SpcFile can contain only one Partial having any given (non-zero) 
		label, so an added Partial will replace a Partial having the 
		same label, if such a Partial exists.

		This may throw an InvalidArgument exception if an attempt is made
		to add unlabeled Partials, or Partials labeled higher than the
		allowable maximum.
	 */
	 
	void addPartial( const Loris::Partial & p, int label );
	/*	Add a Partial, assigning it the specified label (and position in the
		Spc data).
		
		A SpcFile can contain only one Partial having any given (non-zero) 
		label, so an added Partial will replace a Partial having the 
		same label, if such a Partial exists.

		This may throw an InvalidArgument exception if an attempt is made
		to add unlabeled Partials, or Partials labeled higher than the
		allowable maximum.
	 */

	void setMidiNoteNumber( double nn );
	/*	Set the fractional MIDI note number assigned to this SpcFile. 
		If the sound has no definable pitch, use note number 60.0 (the default).
	 */
	 
	void setSampleRate( double rate );
	/*	Set the sampling freqency in Hz for the spc data in this
		SpcFile. This is the rate at which Kyma must be running to ensure
		proper playback of bandwidth-enhanced Spc data.
		
		The default sample rate is 44100 Hz.
	*/
			 
	void write( const char * filename, bool enhanced = true,
				double endApproachTime = 0 );
	/*	Export the envelope parameter streams represented by this SpcFile to
		the file having the specified filename or path. Export phase-correct 
		bandwidth-enhanced envelope parameter streams if enhanced is true 
		(the default), or pure sinsoidal streams otherwise.
	
		A nonzero endApproachTime indicates that the Partials do not include a
		release or decay, but rather end in a static spectrum corresponding to the
		final Breakpoint values of the partials. The endApproachTime specifies how
		long before the end of the sound the amplitude, frequency, and bandwidth
		values are to be modified to make a gradual transition to the static spectrum.
		
		If the endApproachTime is not specified, it is assumed to be zero, 
		corresponding to Partials that decay or release normally.
	*/
	
	%extend 
	{
		SpcFile( PartialList * l, double midiNoteNum = 60 ) 
		{
			return new SpcFile( l->begin(), l->end(), midiNoteNum );
		}
		/*	Initialize an instance of SpcFile with copies of the Partials
			on the specified half-open (STL-style) range.
	
			If compiled with NO_TEMPLATE_MEMBERS defined, this member accepts
			only PartialList::const_iterator arguments.
		*/
	
		//	return a copy of the Partials represented by this SdifFile.
		PartialList * partials( void )
		{
			PartialList * plist = new PartialList( self->partials().begin(), self->partials().end() );
			return plist;
		}

		//	add a PartialList of Partials:
		void addPartials( PartialList * l )
		{
			self->addPartials( l->begin(), l->end() );
		}
		/*	Add all Partials on the specified half-open (STL-style) range
			to the enevelope parameter streams represented by this SpcFile. 
			
			A SpcFile can contain only one Partial having any given (non-zero) 
			label, so an added Partial will replace a Partial having the 
			same label, if such a Partial exists.
	
			This may throw an InvalidArgument exception if an attempt is made
			to add unlabeled Partials, or Partials labeled higher than the
			allowable maximum.
		 */
		 
		//	add members to access Markers
		// 	now much improved to take advantage of 
		// 	SWIG support for std::vector.
		
		int numMarkers( void ) { return self->markers().size(); }
		
		Marker * getMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			return new Marker( self->markers()[i] );
		}
		
		void removeMarker( int i )
		{
			if ( i < 0 || i >= self->markers().size() )
			{
				Throw( InvalidArgument, "Marker index out of range." );
			}
			self->markers().erase( self->markers().begin() + i );
		}
		
		void addMarker( Marker m )
		{
			self->markers().push_back( m );
		}
		
		void clearMarkers( void )
		{
			self->markers().clear();
		}
		
		std::vector< Marker > markers( void )
		{
			return self->markers();
		}
		
		void addMarkers( const std::vector< Marker > & markers )
		{
			self->markers().insert( self->markers().end(), 
									markers.begin(), markers.end() );
		}
	}
	
};	//	end of class SpcFile

// ----------------------------------------------------------------
//		wrap PartialList classes
//
//	(PartialList, PartialListIterator, Partial, PartialIterator, 
//	and Breakpoint)
//
//	This stuff is kind of big, so it lives in its own interface
//	file.
%include lorisPartialList.i

