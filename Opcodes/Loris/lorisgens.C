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
 *	lorisgens.C
 *
 *	Implementation of Csound unit generators supporting bandwidth-enhanced 
 *	synthesis using the Loris library.
 *
 *	This lorisplay module was originally written by Corbin Champion, 2002.
 *	
 *	The Loris generator code has been modified such that it can also
 *  be built as a dynamically loaded module. Thanks to Michael Gogins
 *	for telling me how to do this!
 *
 * Kelly Fitz, 9 May 2002
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
#include <cmath>
/* why is this here, you ask? Well, Csound's prototyp.h, as of
   version 4.21, declares modf, and does so differently from
   the math header. 

   double modf(double, double *);  /* not included in math.h * /

   The difference is in the throw() specification. Including
   cmath here seems to suppress the compiler errors that
   are the result of this bad idea.
 */
#include "cs.h"
#include "csound.h"
#include "lorisgens.h"
#include "string.h"

#include "Breakpoint.h"
#include "Envelope.h"
#include "Exception.h"
#include "Morpher.h"
#include "Oscillator.h"
#include "Partial.h"
#include "PartialUtils.h"
#include "SdifFile.h"

#include <algorithm>
#include <exception>
#include <iostream>
#include <map>
#include <string>
#include <memory>
#include <set>
#include <utility>
#include <vector>

using namespace Loris;
using namespace std;

typedef std::vector< Partial > PARTIALS;
typedef std::vector< Oscillator > OSCILS;

static MYFLT Lorisgens_Srate = 0.0;
static MYFLT Lorisgens_Krate = 0.0;
static MYFLT Lorisgens_Ksmps = 0.0;

#pragma mark -- static helpers --

static void setup_globals( ENVIRON * csound )
{
    if(csound->oparms_->odebug) {
        csound->Message(csound, "setup_globals: csound 0x%x.\n", csound);
    }
	Lorisgens_Srate = csound->GetSr( csound );
	Lorisgens_Krate = csound->GetKr( csound );
	Lorisgens_Ksmps = csound->GetKsmps( csound );
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "setup_globals: sr=%f kr=%f ksmps=%f.\n", 
        Lorisgens_Srate, Lorisgens_Krate, Lorisgens_Ksmps);
    }
}

static void import_partials( ENVIRON *csound, const std::string & sdiffilname, PARTIALS & part )
{
	try {
		//	clear the dstination:
		part.clear();		
		//	import:
		SdifFile f( sdiffilname );
		//	copy the Partials into the vector:
		part.reserve( f.partials().size() );
		part.insert( part.begin(), f.partials().begin(), f.partials().end() );
		//	just for grins, sort the vector:
		// std::sort( part.begin(), part.end(), PartialUtils::compare_label<>() );
	} catch(Exception ex) {
	    csound->Message(csound, "ERROR importing SDIF file: %s.\n", ex.what());
	} catch(std::exception ex) {
	    csound->Message(csound, "ERROR importing SDIF file: %s.\n", ex.what());
	}
}

//	Fade Partials in and out, if fadetime > 0.

static void apply_fadetime( PARTIALS & part, double fadetime )
{
	//	nothing to do if fadetime is zero:
	if (fadetime <= 0.)
		return;
		
	//	iterator over all Partials, adding Breakpoints at both ends:
	PARTIALS::iterator iter;
	for ( iter = part.begin(); iter != part.end(); ++iter )  
	{
		Partial & partial = *iter;
		
		double btime = partial.startTime();	// get start time of partial
	    double etime = partial.endTime();	// get end time of partial

		//	if a fadetime has been specified, introduce zero-amplitude
		//	Breakpoints to fade in and out over fadetime seconds:
		if( fadetime != 0 )
		{
			if ( partial.amplitudeAt(btime) > 0. )
			{
				//	only fade in if starting amplitude is non-zero:
				if( btime > 0. ) 
				{
					//	if the Partial begins after time 0, insert a Breakpoint
					//	of zero amplitude at a time fadetime before the beginning, 
					//	of the Partial, or at zero, whichever is later:
					double t = std::max(btime - fadetime, 0.);
					partial.insert( t, Breakpoint( partial.frequencyAt(t), partial.amplitudeAt(t), 
												   partial.bandwidthAt(t), partial.phaseAt(t)));
				}
				else 
				{
					//	if the Partial begins at time zero, insert the zero-amplitude
					//	Breakpoint at time zero, and make sure that the next Breakpoint
					//	in the Partial is no more than fadetime away from the beginning
					//	of the Partial:
					
					//	find the first Breakpoint later than time 0:
					Partial::iterator pit = partial.begin();
					while (pit.time() < 0.)
						++pit;
					if ( pit.time() > fadetime )
					{
						//	if first Breakpoint afer 0 is later than fadetime, 
						//	insert a Breakpoint at fadetime:
						double t = fadetime;
						partial.insert( t, Breakpoint( partial.frequencyAt(t), partial.amplitudeAt(t), 
													   partial.bandwidthAt(t), partial.phaseAt(t)));
					}
					
					//	insert the zero-amplitude Breakpoint at 0:
					partial.insert( 0, Breakpoint( partial.frequencyAt(0), 0, 
												   partial.bandwidthAt(0), partial.phaseAt(0)));

				}
			}
			
			//	add fadeout Breakpoint at end:
			double t = etime + fadetime;
			partial.insert( t, Breakpoint( partial.frequencyAt(t), 0, 
										   partial.bandwidthAt(t), partial.phaseAt(t)));
		}
	}
	
}

//	Compute radian frequency (used by Loris::Oscillator) from frequency in Hz.
	
static inline double radianFreq( double hz )
{
	//	only need to compute this once ever
	static const double TwoPiOverSR = TWOPI / Lorisgens_Srate;
	return hz * TwoPiOverSR;
}

static void accum_samples( Oscillator & oscil, Breakpoint & bp, double * bufbegin, int nsamps )
{
	if( bp.amplitude() > 0 || oscil.amplitude() > 0 ) {
		double radfreq = radianFreq( bp.frequency() );
		double amp = bp.amplitude();
		double bw = bp.bandwidth();
		//	initialize the oscillator if it is changing from zero
		//	to non-zero amplitude in this  control block:
		if ( oscil.amplitude() == 0. ) {
			//	don't initialize with bogus values, Oscillator
			//	only guards against out-of-range target values
			//	in generateSamples(), parameter mutators are 
			//	dangerous:
			if ( radfreq > PI )	//	don't alias
				amp = 0.;
			if ( bw > 1. )		//	clamp bandwidth
				bw = 1.;
			else if ( bw < 0. )
				bw = 0.;
			/*
			#ifdef DEBUG_LORISGENS
			std::cerr << "initializing oscillator " << std::endl;
			std::cerr << "parameters: " << bp.frequency() << "  ";
			std::cerr << amp << "  " << bw << std::endl;
			#endif
			*/
			//	initialize frequency, amplitude, and bandwidth to 
			//	their target values:
			/*
			oscil.setRadianFreq( radfreq );
			oscil.setAmplitude( amp );
			oscil.setBandwidth( bw );
			*/
			oscil.resetEnvelopes( bp, Lorisgens_Srate );
			//	roll back the phase:
			oscil.resetPhase( bp.phase() - ( radfreq * nsamps ) );
		}	
		//	accumulate samples into buffer:
		// oscil.generateSamples( bufbegin, bufbegin + nsamps, radfreq, amp, bw );
		oscil.oscillate( bufbegin, bufbegin + nsamps, bp, Lorisgens_Srate );
	}
}

static inline void clear_buffer( double * buf, int nsamps )
{
	std::fill( buf, buf + nsamps, 0. );
}

static inline void convert_samples( const double * src, MYFLT * tgt, int nn )
{
    for(int i = 0; i < nn; i++) {
        tgt[i] = src[i] * FL(32767.0);
    }
}

#pragma mark -- EnvelopeReader --

//	EnvelopeReader is a vector of Partial parameter envelope values sampled
//	at some time, represented as Breakpoints, though not necessarily
//	Breakpoints that are members of any Partial. Each set of parameters
//	(Breakpoint) is paired with the label of the corresponding Partial.
//
//	A static map of EnvelopeReader is maintained that allows EnvelopeReader to be
//	found by index and Csound owner-instrument. A EnvelopeReader can be added to 
//	this map, by is parent LorisReader (below), and subsequently found by
//	other generators having the same owner instrument. This is how lorisplay
//	and lorismorph access the data read by a LorisReader.

class EnvelopeReader
{
	std::vector< std::pair< Breakpoint, long > > _vec;
	ENVIRON *csound;
public:
	//	construction:
	explicit EnvelopeReader( ENVIRON *csound_, long n = 0 ) : _vec(n), csound(csound_) {}
	~EnvelopeReader( void ) {}	
	//	access:
	Breakpoint & valueAt( long idx ) { return _vec[idx].first; }
	const Breakpoint & valueAt( long idx ) const { return _vec[idx].first; }
	long & labelAt( long idx ) { return _vec[idx].second; }
	long labelAt( long idx ) const { return _vec[idx].second; }
	long size( void ) const { return _vec.size(); }
	void resize( long n ) { _vec.resize(n); }
	//	tagging:
	typedef std::pair< INSDS *, int > Tag;
	typedef std::map< Tag, EnvelopeReader * > TagMap;
	static TagMap & Tags( void );
	static const EnvelopeReader * Find( ENVIRON *csound, INSDS * owner, int idx );
};

//	Protect this map inside a function, because Csound has a C main() function,
//	and global C++ objects cannot be guaranteed to be instantiated properly.

EnvelopeReader::TagMap &EnvelopeReader::Tags( void )
{
	static TagMap readers;
	return readers;
}

//	May return NULL if no reader with the specified owner and index
//	is found.

const EnvelopeReader *EnvelopeReader::Find(ENVIRON *csound, INSDS * owner, int idx )
{
	TagMap & readers = Tags();
	TagMap::iterator it = readers.find( Tag( owner, idx ) );
	if ( it != readers.end() ) {
        if(csound->oparms_->odebug) {
            csound->Message(csound, "** found EnvelopeReader with owner 0x%x "
            "and index %d having %d envelopes.\n", owner, idx, it->second->size());
        } 
		return it->second;
	} else {
        if(csound->oparms_->odebug) {
            csound->Message(csound, "** could not find EnvelopeReader "
            "with owner 0x%x and ndex %d.\n", owner, idx);
        }
		return NULL;
	}
}

#pragma mark -- ImportedPartials --

//	ImportedPartials keeps track of a collection of imported Partials and the
//	fadetime, if any, that is applied to them, and the name of the file from
//	which they were imported. ImportedPartials instances can be compared using
//	equivalence so that they can be stored in an associative container. 
//	ImportedPartials are stored in a std::set accessed by the static member
//	GetPartials(), so that Partials from a particular file and using a 
//	particular fade time can be imported just once and reused.
//
//	The PARTIALS member is mutable, since it is not involve in the equivalence
//	test. Only const access to Partials is provided to clients, but the 
//	GetPartials() member needs to be able to import the Partials into a 
//	ImportedPartials that is already part of a std::set (and is thus immutable).
//	(The alternative is to copy, which is wasteful.)

class ImportedPartials
{
	mutable PARTIALS _partials;
	double _fadetime;
	std::string _fname;
	ENVIRON *csound;	
public:
	//	construction:
	ImportedPartials( ENVIRON *csound_ ) : csound(csound_) {}
	ImportedPartials( ENVIRON *csound_, const string & path, double fadetime ) :
		_fadetime( fadetime ), _fname( path ), csound(csound_) {}
	~ImportedPartials( void ) {}
	//	access:
	const Partial & operator [] ( long idx ) const { return _partials[idx]; }
	long size( void ) const { return _partials.size(); }
	//	comparison:
	friend bool operator < ( const ImportedPartials & lhs, const ImportedPartials & rhs )
	{
		return (lhs._fadetime < rhs._fadetime) || (lhs._fname < rhs._fname);
	}
	// 	static member for managing a permanent collection:
	static const ImportedPartials & GetPartials( ENVIRON *csound, const string & sdiffilname, double fadetime );
};

//	Return a reference to a collection of Partials from the specified file
//	with the specified fadetime applied. Import if necessary, reuse previously
//	imported Partials if possible. Store imported Partials in a permanent
//	set of imported Partials.

const ImportedPartials &ImportedPartials::GetPartials( ENVIRON *csound, 
    const string & sdiffilname, double fadetime )
{
	static std::set< ImportedPartials > AllPartials;
	ImportedPartials empty(csound, sdiffilname, fadetime );
	std::set< ImportedPartials >::iterator it = AllPartials.find( empty );
	if ( it == AllPartials.end() ) {
		it = AllPartials.insert( empty ).first;
		try {
			//	import Partials and apply fadetime:
            if(csound->oparms_->odebug) {
                csound->Message(csound, "** importing SDIF file %s.\n", 
                sdiffilname.c_str());
            }
			import_partials( csound, sdiffilname, it->_partials );
			apply_fadetime( it->_partials, fadetime );
		} catch( Exception & ex ) {
	        std::string s("Loris exception in ImportedPartials::GetPartials(): " );
	        s.append( ex.what() );
	        s.append("\n");
	        csound->Message(csound, s.c_str());
	    } catch( std::exception & ex ) {
	        std::string s("std C++ exception in ImportedPartials::GetPartials(): " );
	        s.append( ex.what() );
	        s.append("\n");
	        csound->Message(csound, s.c_str());
	    }
	} else  {
        if(csound->oparms_->odebug) {
            csound->Message(csound, "** reusing SDIF file %s.\n", 
            sdiffilname.c_str());
        }
	}	
	return *it;
}		

#pragma mark -- LorisReader --

//	LorisReader samples a ImportedPartials instance at a given time, updated by
//	calls to updateEnvelopePoints(). 

class LorisReader
{
	const ImportedPartials & _partials;
	EnvelopeReader _envelopes;
	EnvelopeReader::Tag _tag;
	ENVIRON *csound;
public:	
	//	construction:
	LorisReader( ENVIRON *csound_, const string & fname, double fadetime, INSDS * owner, int idx );
	~LorisReader( void );
	//	envelope parameter computation:
	//	(returns number of active Partials)
	long updateEnvelopePoints( double time, double fscale, double ascale, double bwscale );
}; 

LorisReader::LorisReader( ENVIRON *csound_, const string & fname, double fadetime, INSDS * owner, int idx ) :
	_partials( ImportedPartials::GetPartials( csound_, fname, fadetime ) ),
	_envelopes( csound, _partials.size() ),
	_tag( owner, idx ),
	csound(csound_)
{
	//	set the labels for the EnvelopeReader:
	for ( long i = 0; i < _partials.size(); ++i ) {
		_envelopes.labelAt(i) = _partials[i].label();
	}
	//	tag these envelopes:
	if(csound->oparms_->odebug) {
        csound->Message(csound, "** constructed new EnvelopeReader "
        "with owner 0x%x and index %d having %d envelopes.\n", owner, idx,
        _envelopes.size());
    }
	EnvelopeReader::Tags()[ _tag ] = &_envelopes;
}

LorisReader::~LorisReader( void )
{
	//	if this reader's envelopes are still in the tag map,
	//	remove them:
	EnvelopeReader::TagMap & tags = EnvelopeReader::Tags();
	EnvelopeReader::TagMap::iterator it = tags.find( _tag );
	if ( it != tags.end() && it->second == &_envelopes ) {
    	if(csound->oparms_->odebug) {
    	    csound->Message(csound, "** destroying EnvelopeReader with "
            "owner 0x%x and index %d having %d envelopes.\n", _tag.first, _tag.second, _envelopes.size());
        }
	    if(csound->oparms_->odebug) {
	        csound->Message(csound, "(Removing from Tag map.)\n");
        }
		tags.erase(it);
	}
}

long LorisReader::updateEnvelopePoints( double time, double fscale, double ascale, double bwscale )
{
	long countActive = 0;
	for (long i = 0; i < _partials.size(); ++i ) {
		const Partial & p = _partials[i];
		Breakpoint & bp = _envelopes.valueAt(i);
		//	update envelope paramters for this Partial:
	 	bp.setFrequency( fscale * p.frequencyAt( time ) );
		bp.setAmplitude( ascale * p.amplitudeAt( time ) );
		bp.setBandwidth( bwscale * p.bandwidthAt( time ) );
		bp.setPhase( p.phaseAt( time ) );
		//	update counter:
		if ( bp.amplitude() > 0 )
			++countActive;
	}
	//	tag these envelopes:
	EnvelopeReader::Tags()[ _tag ] = &_envelopes;
	return countActive;
}

#pragma mark -- lorisread generator functions --

extern "C" 
{
int lorisread_cleanup(void * p);

//	Runs at initialization time for lorisplay.

int lorisread_setup( void * p_)
{
    LORISREAD *p = (LORISREAD *)p_;
    ENVIRON *csound = p->h.insdshead->csound;
    if(csound->oparms_->odebug) {
        csound->Message(csound, "lorisread_setup, owner = 0x%x\n", 
        p->h.insdshead);
    }
	if (!Lorisgens_Srate)
		setup_globals( p->h.insdshead->csound );
	char sdiffilname[0x100];
	//	determine the name of the SDIF file to use:
	//	this code adapted from ugens8.c pvset()
	if ( *p->ifilnam == SSTRCOD ) {
		strcpy(sdiffilname, p->STRARG);
	} else {
		sprintf(sdiffilname,"loris.%d", (int)*p->ifilnam);
	}
	//	construct the implementation object:
	p->imp = new LorisReader( csound, sdiffilname, *p->fadetime, p->h.insdshead, int(*p->readerIdx) );
	// set lorisplay_cleanup as cleanup routine:
	p->h.dopadr = lorisread_cleanup;  
	return OK;
}

//	Control-rate generator function.

int lorisread( void * p_ )
{   
    LORISREAD *p  = (LORISREAD *)p_;
	//*(p->result) = 
	p->imp->updateEnvelopePoints( *p->time, *p->freqenv, *p->ampenv, *p->bwenv );
	return OK;
}

//	Cleans up after lorisread.

int lorisread_cleanup(void * p)
{
	LORISREAD * tp = (LORISREAD *)p;
	ENVIRON *csound = tp->h.insdshead->csound;
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "** Cleaning up lorisread "
        "(owner 0x%x).\n", tp->h.insdshead);
    }
    if(tp->imp) {
	    delete tp->imp;
	    tp->imp = 0;
    }
	return OK;
}

#pragma mark -- LorisPlayer --

};

// 	Define a structure holding private internal data.

struct LorisPlayer
{
    ENVIRON *csound;
	const EnvelopeReader * reader;
	OSCILS oscils;	
	std::vector< double > dblbuffer;
	LorisPlayer( LORISPLAY * params );
	~LorisPlayer( void ) {}
}; 

LorisPlayer::LorisPlayer( LORISPLAY * params ) :
	csound(params->h.insdshead->csound),
	reader(EnvelopeReader::Find(params->h.insdshead->csound, params->h.insdshead, (int)*(params->readerIdx) ) ),
	dblbuffer( params->h.insdshead->csound->GetKsmps(params->h.insdshead->csound), 0. )
{
	if ( reader != NULL )
		oscils.resize( reader->size() );
	else
	    csound->Message(csound, "** Could not find lorisplay source with "
        "index %d\n", (int)*(params->readerIdx));
}

#pragma mark -- lorisplay generator functions --

extern "C"
{
int lorisplay_cleanup(void * p);

//	Runs at initialization time for lorisplay.

int lorisplay_setup( void * p_ )
{
    LORISPLAY *p = (LORISPLAY *)p_;
    ENVIRON *csound = p->h.insdshead->csound;
	if (!Lorisgens_Srate)
		setup_globals( csound );
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "** Setting up lorisplay (owner 0x%x).\n", 
        p->h.insdshead);
    }
	p->imp = new LorisPlayer(p );
	p->h.dopadr = lorisplay_cleanup;  // set lorisplay_cleanup as cleanup routine
	return OK;
}

//	Audio-rate generator function.

int lorisplay( void * p_ )
{
    LORISPLAY *p = (LORISPLAY *)p_;
	LorisPlayer & player = *p->imp;
	OSCILS & oscils = player.oscils;
	//	clear the buffer first!
	double * bufbegin =  &(player.dblbuffer[0]);
	clear_buffer( bufbegin, p->h.insdshead->csound->GetKsmps(p->h.insdshead->csound) );
	//	now accumulate samples into the buffer:
	long numOscils = player.oscils.size();
	for( long i = 0; i < numOscils; ++i ) {
		const Breakpoint & bp = player.reader->valueAt(i);
		Breakpoint modifiedBp(  (*p->freqenv) * bp.frequency(),
								(*p->ampenv) * bp.amplitude(),
								(*p->bwenv) * bp.bandwidth(),
								bp.phase() );
		accum_samples( oscils[i], modifiedBp, bufbegin, p->h.insdshead->csound->GetKsmps(p->h.insdshead->csound) );
	} 
	//	transfer samples into the result buffer:
	convert_samples( bufbegin, p->result, p->h.insdshead->csound->GetKsmps(p->h.insdshead->csound) );
	return OK;
}

//	Cleans up after lorisplay.

int lorisplay_cleanup(void * p)
{
	LORISPLAY * tp = (LORISPLAY *)p;
	ENVIRON *csound = tp->h.insdshead->csound;
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "** Cleaning up lorisplay (owner 0x%x).\n", tp->h.insdshead);
    }
    if(tp->imp) {
	    delete tp->imp;
	    tp->imp = 0;
    }
	return OK;
}
};

#pragma mark -- LorisMorpher --

// 	Define a structure holding private internal data for lorismorph

class LorisMorpher
{
	ENVIRON *csound;
	Morpher morpher;
	const EnvelopeReader * src_reader;
	const EnvelopeReader * tgt_reader;
	EnvelopeReader morphed_envelopes;
	EnvelopeReader::Tag tag;
	typedef std::map< long, std::pair< long, long > > LabelMap;
	LabelMap labelMap;
	std::vector< long > src_unlabeled, tgt_unlabeled;
public:	
	//	construction:
	LorisMorpher( LORISMORPH * params );
	~LorisMorpher( void );	
	//	envelope update:
	long updateEnvelopes( void );
	//
	//	Define Envelope classes that can access the 
	//	morphing functions defined in the orchestra 
	//	at the control rate:
	struct GetFreqFunc : public Envelope
	{
		LORISMORPH * params;
		GetFreqFunc( LORISMORPH * p ) : params(p) {}
		GetFreqFunc( const GetFreqFunc & other ) : params( other.params ) {}
		~GetFreqFunc( void ) {}
		//	Envelope interface:
		GetFreqFunc * clone( void ) const { return new GetFreqFunc( *this ); }
		double valueAt( double /* time */ ) const {
			//	ignore time, can only access the current value:
			double val = *params->freqenv;
			if ( val > 1 )
				val = 1;
			else if ( val < 0 )
				val = 0; 
			return val;
		}
	};
	struct GetAmpFunc : public Envelope
	{
		LORISMORPH * params;
		GetAmpFunc( LORISMORPH * p ) : params(p) {}
		GetAmpFunc( const GetAmpFunc & other ) : params( other.params ) {}
		~GetAmpFunc( void ) {}	
		//	Envelope interface:
		GetAmpFunc * clone( void ) const { return new GetAmpFunc( *this ); }
		double valueAt( double /* time */ ) const
		{
			//	ignore time, can only access the current value:
			double val = *params->ampenv;
			if ( val > 1 )
				val = 1;
			else if ( val < 0 )
				val = 0; 
			return val;
		}
	};
	struct GetBwFunc : public Envelope
	{
		LORISMORPH * params;
		GetBwFunc( LORISMORPH * p ) : params(p) {}
		GetBwFunc( const GetBwFunc & other ) : params( other.params ) {}
		~GetBwFunc( void ) {}	
		//	Envelope interface:
		GetBwFunc * clone( void ) const { return new GetBwFunc( *this ); }
		double valueAt( double /* time */ ) const
		{
			//	ignore time, can only access the current value:
			double val = *params->bwenv;
			if ( val > 1 )
				val = 1;
			else if ( val < 0 )
				val = 0; 
			return val;
		}
	};
}; 

//	This is a very ugly piece of code to set up an index map that makes it
//	fast to associate the Breakpoints in the source and target readers with
//	the correct Oscillator. There should be a nicer way to do this, but
//	we cannot count on anything like unique labeling (though the results 
//	will be unpredictable if the labeling is not unique), so it seems
//	like an index map is the most efficient way.

LorisMorpher::LorisMorpher( LORISMORPH * params ) :
	csound(params->h.insdshead->csound),
	morpher( GetFreqFunc( params ), GetAmpFunc( params ), GetBwFunc( params ) ),
	src_reader( EnvelopeReader::Find( params->h.insdshead->csound, params->h.insdshead, (int)*(params->srcidx) ) ),
	tgt_reader( EnvelopeReader::Find( params->h.insdshead->csound, params->h.insdshead, (int)*(params->tgtidx) ) ),
	morphed_envelopes(params->h.insdshead->csound),
	tag( params->h.insdshead, (int)*(params->morphedidx) ) 
{
	if ( src_reader != NULL ) {
		//	map source Partial indices:
		//	(make a note of the largest label we see)
		src_unlabeled.reserve( src_reader->size() );
		long maxsrclabel = 0;
		for ( long i = 0; i < src_reader->size(); ++i ) {
			long label = src_reader->labelAt(i);
			if ( label != 0 )
				labelMap[ label ] = std::make_pair(i, long(-1));
			else
				src_unlabeled.push_back( i );
			if ( label > maxsrclabel )
				maxsrclabel = label;
		}
		if(csound->oparms_->odebug) {
		    csound->Message(csound, "** Largest source label is %d.\n", maxsrclabel);
        }
	} else {
	    csound->Message(csound, "** Could not find lorismorph source with "
        "index %d.\n", (int)*(params->srcidx));
	}	
	if ( tgt_reader != NULL ) {
		//	map target Partial indices:
		//	(make a note of the largest label we see)
		tgt_unlabeled.reserve( tgt_reader->size() );
		long maxtgtlabel = 0;
		for ( long i = 0; i < tgt_reader->size(); ++i ) {
			long label = tgt_reader->labelAt(i);
			if ( label != 0 ) {
				if ( labelMap.count( label ) > 0 )
					labelMap[ label ].second = i;
				else
					labelMap[ label ] = std::make_pair(long(-1), i);
			} else
				tgt_unlabeled.push_back( i );	
			if ( label > maxtgtlabel )
				maxtgtlabel = label;
		}
		if(csound->oparms_->odebug) {
		    csound->Message(csound, "** Largest target label is %d.\n", maxtgtlabel);
        }
	} else {
	    csound->Message(csound, "** Could not find lorismorph target with "
        "index %d.\n", (int)*(params->tgtidx));
	}
    if(csound->oparms_->odebug) {
        csound->Message(csound, "** Morph will use %d labeled partials, "
            "%d unlabeled source partial, and %d unlabeled target partials.\n",
            labelMap.size(), src_unlabeled.size(), tgt_unlabeled.size());
	}
	//	allocate and set the labels for the morphed envelopes:
	morphed_envelopes.resize( labelMap.size() + src_unlabeled.size() + tgt_unlabeled.size() );
	long envidx = 0;
	LabelMap::iterator it;
	for( it = labelMap.begin(); it != labelMap.end(); ++it, ++envidx ) {
		morphed_envelopes.labelAt(envidx) = it->first;
	}
	//	tag these envelopes:
	EnvelopeReader::Tags()[ tag ] = &morphed_envelopes;
}

LorisMorpher::~LorisMorpher( void )
{
	//	if the morphed envelopes are still in the tag map,
	//	remove them:
	EnvelopeReader::TagMap & tags = EnvelopeReader::Tags();
	EnvelopeReader::TagMap::iterator it = tags.find( tag );
	if ( it != tags.end() && it->second == &morphed_envelopes ) {
		tags.erase(it);
	}
}

//	Taking it on faith that the EnvelopeReaders will not be destroyed before
//	we are done using them!

long LorisMorpher::updateEnvelopes( void )
{
	//	first render all the labeled (morphed) Partials:
	// std::cerr << "** Morphing Partials labeled " << labelMap.begin()->first;
	// std::cerr << " to " << (--labelMap.end())->first << std::endl;
	Partial dummy;
	long envidx = 0;
	LabelMap::iterator it;
	for( it = labelMap.begin(); it != labelMap.end(); ++it, ++envidx ) {
		//long label = it->first;
		std::pair<long, long> & indices = it->second;
		Breakpoint & bp = morphed_envelopes.valueAt(envidx);
		long isrc = indices.first;
		long itgt = indices.second;
		//	this should not happen:
		if ( itgt < 0 && isrc < 0 ) {
		    if(csound->oparms_->odebug) {
		        csound->Message(csound, "HEY!!!! The labelMap had a pair of "
                "bogus indices in it at pos %d.\n", envidx);
            }
			continue;
		}		
		//	note: the time argument for all these morphParameters calls
		//	is irrelevant, since it is only used to index the morphing
		//	functions, which, as defined above, do not use the time
		//	argument, they can only return their current value.
		if ( itgt < 0 ) {
			//	morph from the source to a dummy:
			// std::cerr << "** Morphing source to dummy " << envidx << std::endl;
			morpher.morphParameters( src_reader->valueAt(isrc), dummy, 0, bp );
		} else if ( isrc < 0 ) {
			//	morph from a dummy to the target:
			// std::cerr << "** Morphing dummy to target " << envidx << std::endl;
			morpher.morphParameters( dummy, tgt_reader->valueAt(itgt), 0, bp );
		} else  {
			//	morph from the source to the target:
			// std::cerr << "** Morphing source to target " << envidx << std::endl;
			morpher.morphParameters( src_reader->valueAt(isrc), tgt_reader->valueAt(itgt), 0, bp );
		}	
	} 
	//	render unlabeled source Partials:
	// std::cerr << "** Crossfading " << src_unlabeled.size();
	// std::cerr << " unlabeled source Partials" << std::endl;
	for( size_t i = 0; i < src_unlabeled.size(); ++i, ++envidx ) {
		//	morph from the source to a dummy:
		Breakpoint & bp = morphed_envelopes.valueAt(envidx);
		morpher.morphParameters( src_reader->valueAt( src_unlabeled[i] ), dummy, 0, bp );
	}
	//	render unlabeled target Partials:
	// std::cerr << "** Crossfading " << tgt_unlabeled.size();
	// std::cerr << " unlabeled target Partials" << std::endl;
	for( size_t i = 0; i < tgt_unlabeled.size(); ++i, ++envidx ) {
		//	morph from a dummy to the target:
		Breakpoint & bp = morphed_envelopes.valueAt(envidx);
		morpher.morphParameters( dummy, tgt_reader->valueAt( tgt_unlabeled[i] ), 0, bp );
	}	
	//	tag these envelopes:
	EnvelopeReader::Tags()[ tag ] = &morphed_envelopes;
	return morphed_envelopes.size();
}

#pragma mark -- lorismorph generator functions --

extern "C" {
int lorismorph_cleanup(void * p);

//	Runs at initialization time for lorismorph.

int lorismorph_setup( void * p_ )
{
    LORISMORPH *p = (LORISMORPH *)p_;
    ENVIRON *csound = p->h.insdshead->csound;
	if (!Lorisgens_Srate)
		setup_globals( csound );
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "** Setting up lorismorph "
        "(owner 0x%x.\n", p->h.insdshead);
    }
	p->imp = new LorisMorpher( p );
	p->h.dopadr = lorismorph_cleanup;  // set lorismorph_cleanup as cleanup routine
	return OK;
}

//	Audio-rate generator function.

int lorismorph( void * p_ )
{
    LORISMORPH *p = (LORISMORPH *)p_;
	//*p->result = 
	p->imp->updateEnvelopes();
	return OK;
}

//	Cleans up after lorismorph.

int lorismorph_cleanup(void * p)
{
	LORISMORPH *tp = (LORISMORPH *)p;
	ENVIRON *csound = tp->h.insdshead->csound;
	if(csound->oparms_->odebug) {
	    csound->Message(csound, "** Cleaning up lorismorph "
        "(owner 0x%x\n", tp->h.insdshead);
    }
    if(tp->imp) {
        delete tp->imp;
        tp->imp = 0;
    }
	return OK;
}
};

// ---------------------------------------------------------------------------
//	Loris csound plugin
// ---------------------------------------------------------------------------
//	Added by Michael Gogins to make Loris generators work as a 
//	dynamically-loaded plugin to Csound.

extern "C"
{
	OENTRY lorisOentry[] = 
	{
		{"lorisread",  sizeof(LORISREAD),  3, "",  "kSikkko", &lorisread_setup,  &lorisread,  0 ,                0},
		{"lorisplay",  sizeof(LORISPLAY),  5, "a", "ikkk",    &lorisplay_setup,  0,                 &lorisplay , 0},
		{"lorismorph", sizeof(LORISMORPH), 3, "",  "iiikkk",  &lorismorph_setup, &lorismorph, 0,                 0}
	};

	/**
	 * Called by Csound to obtain the size of
	 * the table of OENTRY structures defined in this shared library.
	 */

	PUBLIC int opcode_size()
	{
		return sizeof(lorisOentry);
	}

	/**
	 * Called by Csound to obtain a pointer to
	 * the table of OENTRY structures defined in this shared library.
	 */

	PUBLIC OENTRY *opcode_init(ENVIRON *csound)
	{
		//	seems like I ought to be able to take this
		//	opportunity to initialize the global sample
		//	rate and k-rate:
		//
		//	This would be a good place to do this
		//	kind of initialization, but sadly, Csound
		//	calls this function when it loads the module,
		//	and _before_ it has read the orchestra, so
		//	the sample and control rates have not been
		//	set yet.
		/*
		Lorisgens_Srate = csound->GetSr( csound );
		Lorisgens_Krate = csound->GetKr( csound );
		Lorisgens_Ksmps = csound->GetKsmps( csound );
		std::cerr << "*** sample rate is " << Lorisgens_Srate << std::endl;
		std::cerr << "*** control rate is " << Lorisgens_Krate << std::endl;
		*/
		
		return lorisOentry;
	}
};

/*
This works:
 
../libtool --mode=compile g++ -DHAVE_CONFIG_H -I.. -I../src -I/usr/local/src/Csound-4.23 -g -O2 -c -o lorisgens.lo lorisgens.C

../libtool --mode=link g++ -o lorisgens.la -rpath /usr/local/lib -module -avoid-version lorisgens.lo ../src/libloris.la -lstdc++

 
 csound --opcode-lib=.libs/lorisgens.so tryit.csd
 */
 
