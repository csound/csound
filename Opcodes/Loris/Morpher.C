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
 * Morpher.C
 *
 * Implementation of class Morpher.
 *
 * Kelly Fitz, 15 Oct 1999
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "Morpher.h"
#include "Breakpoint.h"
#include "Envelope.h"
#include "Exception.h"
#include "Notifier.h"
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"

#include <algorithm>
#include <memory>
#include <set>
#include <cmath>

#if defined(HAVE_M_PI) && (HAVE_M_PI)
const double Pi = M_PI;
#else
const double Pi = 3.14159265358979324;
#endif

//	begin namespace
namespace Loris {


  static const Partial::label_type DefaultReferenceLabel = 0;	
  //	by default, don't use reference Partial
  // (this is the traditional behavior or Loris)

  static const double DefaultFixThreshold = -90; 	// dB, very low by default
  static const double DefaultAmpShape = 1E-5; 	   // shaping parameter, see morphAmplitudes
  static const double DefaultBreakpointGap = 1E-4;// minimum time (sec) between Breakpoints in 
  // morphed Partials


	
#pragma mark -- construction --

  // ---------------------------------------------------------------------------
  //	Morpher constructor (single morph function)
  // ---------------------------------------------------------------------------
  //	Construct a new Morpher using the same morphing envelope for 
  //	frequency, amplitude, and bandwidth (noisiness).
  //
  Morpher::Morpher( const Envelope & f ) :
    _freqFunction( f.clone() ),
    _ampFunction( f.clone() ),
    _bwFunction( f.clone() ),
    _refLabel0( DefaultReferenceLabel ),
    _refLabel1( DefaultReferenceLabel ),
    _freqFixThresholdDb( DefaultFixThreshold ),
    _ampMorphShape( DefaultAmpShape ),
    _minBreakpointGapSec( DefaultBreakpointGap )
    {
    }

  // ---------------------------------------------------------------------------
  //	Morpher constructor (distinct morph functions)
  // ---------------------------------------------------------------------------
  //	Construct a new Morpher using the specified morphing envelopes for
  //	frequency, amplitude, and bandwidth (noisiness).
  //
  Morpher::Morpher( const Envelope & ff, const Envelope & af, const Envelope & bwf ) :
    _freqFunction( ff.clone() ),
    _ampFunction( af.clone() ),
    _bwFunction( bwf.clone() ),
    _refLabel0( DefaultReferenceLabel ),
    _refLabel1( DefaultReferenceLabel ),
    _freqFixThresholdDb( DefaultFixThreshold ),
    _ampMorphShape( DefaultAmpShape ),
    _minBreakpointGapSec( DefaultBreakpointGap )
    {
    }

  // ---------------------------------------------------------------------------
  //	Morpher copy constructor 
  // ---------------------------------------------------------------------------
  //!	Construct a new Morpher that is a duplicate of rhs.
  //!
  //! \param  rhs is the Morpher to duplicate
  Morpher::Morpher( const Morpher & rhs ) :
    _freqFunction( rhs._freqFunction->clone() ),
    _ampFunction( rhs._ampFunction->clone() ),
    _bwFunction( rhs._bwFunction->clone() ),
    _refLabel0( rhs._refLabel0 ),
    _refLabel1( rhs._refLabel1 ),
    _freqFixThresholdDb( rhs._freqFixThresholdDb ),
    _ampMorphShape( rhs._ampMorphShape ),
    _minBreakpointGapSec( rhs._minBreakpointGapSec )
    {
    }

  // ---------------------------------------------------------------------------
  //	Morpher destructor
  // ---------------------------------------------------------------------------
  //	Destroy this Morpher.
  //
  Morpher::~Morpher( void )
    {
    }

  // ---------------------------------------------------------------------------
  //	Morpher assignment operator
  // ---------------------------------------------------------------------------
  //! Make this Morpher a duplicate of rhs.
  //!
  //! \param  rhs is the Morpher to duplicate
  Morpher & 
    Morpher::operator= ( const Morpher & rhs )
    {
      if ( &rhs != this )
	{
	  _freqFunction.reset( rhs._freqFunction->clone() );
	  _ampFunction.reset( rhs._ampFunction->clone() );
	  _bwFunction.reset( rhs._bwFunction->clone() );
	    
	  _refLabel0 = rhs._refLabel0;
	  _refLabel1 = rhs._refLabel1;
	  _freqFixThresholdDb = rhs._freqFixThresholdDb;
	  _ampMorphShape = rhs._ampMorphShape;
	  _minBreakpointGapSec = rhs._minBreakpointGapSec;
	}
      return *this;
    }



#pragma mark -- Partial morphing --

  // ---------------------------------------------------------------------------
  //	morphPartial
  // ---------------------------------------------------------------------------
  //	Morph a pair of Partials to yield a new morphed Partial. 
  //	Dummy Partials (having no Breakpoints) don't contribute to the
  //	morph, except to cause their opposite to fade out. 
  //	Either (or neither) the source or target Partial may be a dummy 
  //	Partial (no Breakpoints), but not both. The morphed
  //	Partial has Breakpoints at times corresponding to every Breakpoint 
  //	in both source Partials. The morphed Partial is appended
  //	to the Morpher's PartialList, and a reference to it is returned.
  //	The morphed Partial is assigned the specified label.
  //
  Partial
    Morpher::morphPartial( const Partial & src, const Partial & tgt, int assignLabel )
    {
      if ( (src.numBreakpoints() == 0) && (tgt.numBreakpoints() == 0) )
	{
	  Throw( InvalidArgument, "Cannot morph two empty Partials," );
	}
	
      //	make a new Partial:
      Partial newp;
      newp.setLabel( assignLabel );


      Partial::const_iterator src_iter = src.begin();
      Partial::const_iterator tgt_iter = tgt.begin();
	
      //  Merge Breakpoints from the two Partials,
      //  loop until there are no more Breakpoints to
      //  consider in either Partial.
      while ( src_iter != src.end() || tgt_iter != tgt.end() )
	{
	  if ( ( tgt_iter == tgt.end() ) ||
	       ( src_iter != src.end() && src_iter.time() < tgt_iter.time() ) )
	    {
	      //  Ran out of tgt Breakpoints, or 
	      //  src Breakpoint is earlier, add it.
	      double t = src_iter.time();
	      if ( 0 < tgt.numBreakpoints() )
		{
		  morphSrcBreakpoint( src_iter.breakpoint(), tgt, t, newp );
		}
	      else
		{
		  fadeSrcBreakpoint( src_iter.breakpoint(), t, newp );
		}
	      ++src_iter;
	    }
	  else 
	    {
	      //  Ran out of src Breakpoints, or
	      //  tgt Breakpoint is earlier add it.
	      double t = tgt_iter.time();

	      if ( 0 < src.numBreakpoints() )
		{
		  morphTgtBreakpoint( tgt_iter.breakpoint(), src, t, newp );
		}
	      else
		{
		  fadeTgtBreakpoint( tgt_iter.breakpoint(), t, newp );
    		}
	      ++tgt_iter;
	    }  
	}

      return newp;
    }

  // ---------------------------------------------------------------------------
  //	crossfade
  // ---------------------------------------------------------------------------
  //	Crossfade Partials with no correspondences.
  //
  //	Unlabeled Partials (having label 0) are considered to 
  //	have no correspondences, so they are just faded out, and not 
  //	actually morphed. This is the same as morphing each with an 
  //	empty dummy Partial (having no Breakpoints). 
  //
  //	The Partials in the first range are treated as components of the 
  //	source sound, corresponding to a morph function value of 0, and  
  //	those in the second are treated as components of the target sound, 
  //	corresponding to a morph function value of 1.
  //
  //	The crossfaded Partials are stored in the Morpher's PartialList.
  //
  void 
    Morpher::crossfade( PartialList::const_iterator beginSrc, 
			PartialList::const_iterator endSrc,
			PartialList::const_iterator beginTgt, 
			PartialList::const_iterator endTgt,
			Partial::label_type label /* default 0 */ )
    {
      Partial nullPartial;
      debugger << "crossfading unlabeled (labeled 0) Partials" << endl;
	
      long debugCounter;

      //	crossfade Partials corresponding to a morph weight of 0:
      PartialList::const_iterator it;
      debugCounter = 0;
      for ( it = beginSrc; it != endSrc; ++it )
	{
	  if ( it->label() == label )
	    {		    
	      Partial newp;
	      newp.setLabel( label );

	      for ( Partial::const_iterator bpPos = it->begin(); 
		    bpPos != it->end(); 
		    ++bpPos )
		{	    
		  fadeSrcBreakpoint( bpPos.breakpoint(), bpPos.time(), newp );
		}
			
	      if ( newp.numBreakpoints() > 0 )
		{
		  ++debugCounter;
		  _partials.push_back( newp );
		}
	    }
	}
      debugger << "kept " << debugCounter << " from sound 1" << endl;

      //	crossfade Partials corresponding to a morph weight of 1:
      debugCounter = 0;
      for ( it = beginTgt; it != endTgt; ++it )
	{
	  if ( it->label() == label )
	    {
	      Partial newp;
	      newp.setLabel( label );

	                        
	      for ( Partial::const_iterator bpPos = it->begin(); 
		    bpPos != it->end(); 
		    ++bpPos )
		{
		  fadeTgtBreakpoint( bpPos.breakpoint(), bpPos.time(), newp );           
		}
			
	      if ( newp.numBreakpoints() > 0 )
		{
		  ++debugCounter;
		  _partials.push_back( newp );
		}
	    }
	}
      debugger << "kept " << debugCounter << " from sound 2" << endl;
    }

  // ---------------------------------------------------------------------------
  //	morph
  // ---------------------------------------------------------------------------
  //	Morph two sounds (collections of Partials labeled to indicate
  //	correspondences) into a single labeled collection of Partials.
  //	Unlabeled Partials (having label 0) are crossfaded. The morphed
  //	and crossfaded Partials are stored in the Morpher's PartialList.
  //
  //	The Partials in the first range are treated as components of the 
  //	source sound, corresponding to a morph function value of 0, and  
  //	those in the second are treated as components of the target sound, 
  //	corresponding to a morph function value of 1.
  //
  //	Throws InvalidArgument if either the source or target
  //	sequence is not distilled (contains more than one Partial having
  //	the same non-zero label).
  //
  //	Ugh! This ought to be a template function!
  // Ugh! But then crossfade needs to be a template function.
  // Maybe need to do something different with crossfade first.
  //
  void 
    Morpher::morph( PartialList::const_iterator beginSrc, 
		    PartialList::const_iterator endSrc,
		    PartialList::const_iterator beginTgt, 
		    PartialList::const_iterator endTgt )
    {
      //	build a PartialCorrespondence, a map of labels
      //	to pairs of pointers to Partials, by making every
      //	Partial in the source the first element of the
      //	pair at the corresponding label, and every Partial
      //	in the target the second element of the pair at
      //	the corresponding label. Pointers not assigned to
      //	point to a Partial in the source or target are 
      //	initialized to 0 in the correspondence map.
      PartialCorrespondence correspondence;
	
      //	add source Partials to the correspondence map:
      for ( PartialList::const_iterator it = beginSrc; it != endSrc; ++it ) 
	{
	  //	don't add the crossfade label to the set:
	  if ( it->label() != 0 )
	    {
	      PartialPtrPair & match = correspondence[ it->label() ];
	      if ( match.first != 0 )
		{
		  Throw( InvalidArgument, "Partials must be distilled before morphing." );
		}
	      match.first = &(*it);
	    }
	}
	
      //	add target Partials to the correspondence map:
      for ( PartialList::const_iterator it = beginTgt; it != endTgt; ++it ) 
	{
	  //	don't add the crossfade label to the set:
	  if ( it->label() != 0 )
	    {
	      PartialPtrPair & match = correspondence[ it->label() ];
	      if ( match.second != 0 )
		{
		  Throw( InvalidArgument, "Partials must be distilled before morphing." );
		}
	      match.second = &(*it);
	    }
	}
	
      //	morph corresponding labeled Partials:
      morph_aux( correspondence );
	
      //	crossfade the remaining unlabeled Partials:
      crossfade( beginSrc, endSrc, beginTgt, endTgt );
    }

#pragma mark -- morphing function access/mutation --

  // ---------------------------------------------------------------------------
  //	setFrequencyFunction
  // ---------------------------------------------------------------------------
  //	Assign a new frequency morphing envelope to this Morpher.
  //
  void
    Morpher::setFrequencyFunction( const Envelope & f )
    {
      _freqFunction.reset( f.clone() );
    }

  // ---------------------------------------------------------------------------
  //	setAmplitudeFunction
  // ---------------------------------------------------------------------------
  //	Assign a new amplitude morphing envelope to this Morpher.
  //
  void
    Morpher::setAmplitudeFunction( const Envelope & f )
    {
      _ampFunction.reset( f.clone() );
    }

  // ---------------------------------------------------------------------------
  //	setBandwidthFunction
  // ---------------------------------------------------------------------------
  //	Assign a new bandwidth morphing envelope to this Morpher.
  //
  void
    Morpher::setBandwidthFunction( const Envelope & f )
    {
      _bwFunction.reset( f.clone() );
    }

  // ---------------------------------------------------------------------------
  //	frequencyFunction
  // ---------------------------------------------------------------------------
  //	Return a reference to this Morpher's frequency morphing envelope.
  //
  const Envelope &
    Morpher::frequencyFunction( void ) const 
    {
      return * _freqFunction;
    }

  // ---------------------------------------------------------------------------
  //	amplitudeFunction
  // ---------------------------------------------------------------------------
  //	Return a reference to this Morpher's amplitude morphing envelope.
  //
  const Envelope &
    Morpher::amplitudeFunction( void ) const 
    {
      return * _ampFunction;
    }

  // ---------------------------------------------------------------------------
  //	bandwidthFunction
  // ---------------------------------------------------------------------------
  //	Return a reference to this Morpher's bandwidth morphing envelope.
  //
  const Envelope &
    Morpher::bandwidthFunction( void ) const 
    {
      return * _bwFunction;
    }


#pragma mark -- reference Partial label access/mutation --

  // ---------------------------------------------------------------------------
  //	sourceReferenceLabel
  // ---------------------------------------------------------------------------
  // Return the label of the Partial to be used as a reference
  //	Partial for the source sequence in a morph of two Partial
  //	sequences. The reference partial is used to compute 
  //	frequencies for very low-amplitude Partials whose frequency
  //	estimates are not considered reliable. The reference Partial
  //	is considered to have good frequency estimates throughout.
  //	The default label of 0 indicates that no reference Partial
  //	should be used for the source sequence.
  //
  Partial::label_type
    Morpher::sourceReferenceLabel( void ) const
    {
      return _refLabel0;
    }

  // ---------------------------------------------------------------------------
  //	targetReferenceLabel
  // ---------------------------------------------------------------------------
  // Return the label of the Partial to be used as a reference
  //	Partial for the target sequence in a morph of two Partial
  //	sequences. The reference partial is used to compute 
  //	frequencies for very low-amplitude Partials whose frequency
  //	estimates are not considered reliable. The reference Partial
  //	is considered to have good frequency estimates throughout.
  //	The default label of 0 indicates that no reference Partial
  //	should be used for the target sequence.
  //
  Partial::label_type
    Morpher::targetReferenceLabel( void ) const
    {
      return _refLabel1;
    }
	
  // ---------------------------------------------------------------------------
  //	setSourceReferenceLabel
  // ---------------------------------------------------------------------------
  // Set the label of the Partial to be used as a reference
  //	Partial for the source sequence in a morph of two Partial
  //	sequences. The reference partial is used to compute 
  //	frequencies for very low-amplitude Partials whose frequency
  //	estimates are not considered reliable. The reference Partial
  //	is considered to have good frequency estimates throughout.
  //	Setting the reference label to 0 indicates that no reference 
  //	Partial should be used for the source sequence.
  //
  void 
    Morpher::setSourceReferenceLabel( Partial::label_type l )
    {
      _refLabel0 = l;
    }

  // ---------------------------------------------------------------------------
  //	setTargetReferenceLabel
  // ---------------------------------------------------------------------------
  // Set the label of the Partial to be used as a reference
  //	Partial for the target sequence in a morph of two Partial
  //	sequences. The reference partial is used to compute 
  //	frequencies for very low-amplitude Partials whose frequency
  //	estimates are not considered reliable. The reference Partial
  //	is considered to have good frequency estimates throughout.
  //	Setting the reference label to 0 indicates that no reference 
  //	Partial should be used for the target sequence.
  //
  void 
    Morpher::setTargetReferenceLabel( Partial::label_type l )
    {
      _refLabel1 = l;
    }

  // ---------------------------------------------------------------------------
  //	amplitudeShape
  // ---------------------------------------------------------------------------
  //	Return the shaping parameter for the amplitude moprhing
  //	function (only used in new log-amplitude morphing).
  //	This shaping parameter controls the 
  //	slope of the amplitude morphing function,
  //	for values greater than 1, this function
  //	gets nearly linear (like the old amplitude
  //	morphing function), for values much less 
  //	than 1 (e.g. 1E-5) the slope is gently
  //	curved and sounds pretty "linear", for 
  //	very small values (e.g. 1E-12) the curve
  //	is very steep and sounds un-natural because
  //	of the huge jump from zero amplitude to
  //	very small amplitude.
  double Morpher::amplitudeShape( void ) const
    {
      return _ampMorphShape;
    }

  // ---------------------------------------------------------------------------
  //	setAmplitudeShape
  // ---------------------------------------------------------------------------
  //	Set the shaping parameter for the amplitude moprhing
  //	function (only used in new log-amplitude morphing).
  //	This shaping parameter controls the 
  //	slope of the amplitude morphing function,
  //	for values greater than 1, this function
  //	gets nearly linear (like the old amplitude
  //	morphing function), for values much less 
  //	than 1 (e.g. 1E-5) the slope is gently
  //	curved and sounds pretty "linear", for 
  //	very small values (e.g. 1E-12) the curve
  //	is very steep and sounds un-natural because
  //	of the huge jump from zero amplitude to
  //	very small amplitude.
  //
  //	x is the new shaping parameter, it must be positive.
  void Morpher::setAmplitudeShape( double x )
    {
      if ( x <= 0. )
	{
	  Throw( InvalidArgument, "the amplitude morph shaping parameter must be positive");
	}
      _ampMorphShape = x;
    }

  // ---------------------------------------------------------------------------
  //	minBreakpointGap
  // ---------------------------------------------------------------------------
  //	Return the minimum time gap (secs) between two Breakpoints
  //	in the morphed Partials. Morphing two
  //	Partials can generate a third Partial having
  //	Breakpoints arbitrarily close together in time,
  //	and this makes morphs huge. Raising this 
  //	threshold limits the Breakpoint density in
  //	the morphed Partials. Default is 1/10 ms.
  double Morpher::minBreakpointGap( void ) const
    {
      return _minBreakpointGapSec;
    }

  // ---------------------------------------------------------------------------
  //	setMinBreakpointGap
  // ---------------------------------------------------------------------------
  //	Set the minimum time gap (secs) between two Breakpoints
  //	in the morphed Partials. Morphing two
  //	Partials can generate a third Partial having
  //	Breakpoints arbitrarily close together in time,
  //	and this makes morphs huge. Raising this 
  //	threshold limits the Breakpoint density in
  //	the morphed Partials. Default is 1/10 ms.
  //
  //	x is the new minimum gap in seconds, it must be positive
  //	
  void Morpher::setMinBreakpointGap( double x )
    {
      if ( x <= 0. )
	{
	  Throw( InvalidArgument, "the minimum Breakpoint gap must be positive");
	}
      _minBreakpointGapSec = x;
    }

#pragma mark -- PartialList access --

  // ---------------------------------------------------------------------------
  //	partials
  // ---------------------------------------------------------------------------
  //	Return a reference to this Morpher's list of morphed Partials.
  //
  PartialList & 
    Morpher::partials( void )
    { 
      return _partials; 
    }

  // ---------------------------------------------------------------------------
  //	partials
  // ---------------------------------------------------------------------------
  //	Return a const reference to this Morpher's list of morphed Partials.
  //
  const PartialList & 
    Morpher::partials( void ) const 
    { 
      return _partials; 
    }

#pragma mark -- helpers: morphed parameter computation --

  // ---------------------------------------------------------------------------
  //	fix_frequencies
  // ---------------------------------------------------------------------------
  //	Local helper function to correct the frequencies of low-amplitude
  //	breakpoints using a reference Partial.
  //
  // This needs a new name, because when we improve phase morphing, we
  // will have another kind of frequency fixing as well.
  //
  static void fix_frequencies( Partial & fixme, const Partial & reference, 
			       double thresholdDb )
    {
      //	sanity
      if ( 0 == reference.size() )
	{
	  Throw( InvalidArgument, "reference Partial in fix_frequencies must not be empty" );
	}

      //	nothing to do if fixme is the reference Partial:
      if ( &fixme != &reference )
	{
	  //	compute absolute magnitude thresholds:
	  const double FadeRangeDB = 10;
	  const double Threshold = std::pow( 10., 0.05 * thresholdDb );
	  const double BeginFade = std::pow( 10., 0.05 * (thresholdDb+FadeRangeDB) );
	  const double OneOverFadeSpan = 1. / ( BeginFade - Threshold );

	  double fscale = (double)fixme.label() / reference.label();
		
	  //	fix the frequency of every Breakpoint in fixme
	  //	that has amplitude below the reference frequency
	  //	threshold (called BeginFade).
	  //
	  //	invariant:
	  //	bpPos is the position of a Breakpoint in fixme,
	  //	all Breakpoints earlier in fixme than bpPos have
	  //	amplitudes greater than the reference frequency
	  //	threshold, or their frequencies have been "fixed".
	  for ( Partial::iterator bpPos = fixme.begin(); bpPos != fixme.end(); ++bpPos )
	    {
	      if ( bpPos->amplitude() < BeginFade )
		{
		  //	the new frequency is computed from a weighted
		  //	average of the original analyzed frequency and
		  //	the reference frequency:
		  double alpha = std::max( ( BeginFade - bpPos->amplitude() ) * OneOverFadeSpan, 1. );
		  double fRef = reference.frequencyAt( bpPos.time() );
		  bpPos->setFrequency( ( alpha * ( fRef * fscale ) ) + 
				       ( (1 - alpha ) * bpPos->frequency() ) );
		}
	    }				
		
	  //	Breakpoints in the reference Partial that are before the
	  //	start of fixme should introduce zero-amplitude Breakpoints
	  //	in fixme.
	  //
	  //	invariant:
	  //	earlyBpPos is the position of a Breakpoint in the reference
	  //	Partial, all Breakpoints earlier than earlyBpPos have been 
	  //	added to fix me as zero amplitude Breakpoints at the 
	  //	appropriate frequency multiple (fscale, above)
	  //
	  //	Need to cache the start time of fixme, otherwise the
	  //	loop will always terminate after the fist iteration,
	  //	because the earliest Breakpoint is inserted first.
	  Partial::const_iterator earlyBpPos = reference.begin();
	  const double oldStartTime = fixme.startTime();
	  while ( ( earlyBpPos != reference.end() ) && ( earlyBpPos.time() < oldStartTime ) )
	    {
	      Breakpoint silentBreakpoint = earlyBpPos.breakpoint();
	      silentBreakpoint.setAmplitude( 0 );
	      silentBreakpoint.setFrequency( silentBreakpoint.frequency() * fscale );
	      fixme.insert( earlyBpPos.time(), silentBreakpoint );
			
	      /*
		debugger << "inserting silent Breakpoint at time " << earlyBpPos.time()
		<< " and frequency " << silentBreakpoint.frequency()
		<< " at front of Partial labeled " << fixme.label() << endl;
	      */
	      ++earlyBpPos;
	    }
		
	  //	Breakpoints in the reference Partial that are after the
	  //	end of fixme should introduce zero-amplitude Breakpoints
	  //	in fixme.
	  //
	  //	invariant:
	  //	lateBpPos is the position of a Breakpoint in the reference
	  //	Partial, all Breakpoints later than lateBpPos have been 
	  //	added to fix me as zero amplitude Breakpoints at the 
	  //	appropriate frequency multiple (fscale, above)
	  //
	  //	Need to cache the end time of fixme, otherwise the
	  //	loop will always terminate after the fist iteration,
	  //	because the latest Breakpoint is inserted first.
	  Partial::const_iterator lateBpPos = --( reference.end() );
	  const double oldEndTime = fixme.endTime();
	  while ( lateBpPos.time() > oldEndTime )
	    {
	      Breakpoint silentBreakpoint = lateBpPos.breakpoint();
	      silentBreakpoint.setAmplitude( 0 );
	      silentBreakpoint.setFrequency( silentBreakpoint.frequency() * fscale );
	      fixme.insert( lateBpPos.time(), silentBreakpoint );
			
	      /*
		debugger << "inserting silent Breakpoint at time " << lateBpPos.time()
		<< " and frequency " << silentBreakpoint.frequency()
		<< " at end of Partial labeled " << fixme.label() << endl;
	      */
	      if ( lateBpPos == reference.begin() )
		{
		  break;	// YIK! need to think of a better way
		}
	      --lateBpPos;
	    }
	}
    }

  // ---------------------------------------------------------------------------
  //	morph_aux
  // ---------------------------------------------------------------------------
  //	Helper function that performs the morph between corresponding pairs
  //	of Partials identified in a PartialCorrespondence. Called by the
  //	morph() implementation accepting two sequences of Partials.
  //
  //	PartialCorrespondence represents a map from non-zero Partial 
  //	labels to pairs of pointers to Partials that should be morphed 
  //	into a single Partial that is assigned that label. 
  //	PartialPtrPair is a pair of pointers to Partials that are
  //	initialized to zero, and it is the element type for the
  //	PartialCorrespondence map.
  //
  void Morpher::morph_aux( PartialCorrespondence & correspondence  )
    {
      const Partial * ref0 = 0;
      const Partial * ref1 = 0;
	
      //	find reference Partials:
      if ( _refLabel0 != 0 )
	{
	  PartialCorrespondence::iterator refPos = correspondence.find( _refLabel0 );
	  if ( refPos == correspondence.end() )
	    {
	      Throw( InvalidObject, "no Partial in the morph source has the specified reference label" );
	    }
	  ref0 = refPos->second.first;
	  Assert( ref0 != 0 );
	}
      if ( _refLabel1 != 0 )
	{
	  PartialCorrespondence::iterator refPos = correspondence.find( _refLabel1 );
	  if ( refPos == correspondence.end() )
	    {
	      Throw( InvalidObject, "no Partial in the morph target has the specified reference label" );
	    }
	  ref1 = refPos->second.second;
	  Assert( ref1 != 0 );
	}

      PartialCorrespondence::const_iterator it;
      for ( it = correspondence.begin(); it != correspondence.end(); ++it )
	{
	  Partial::label_type label = it->first;
	  PartialPtrPair match = it->second;
	  const Partial * p0 = match.first;
	  const Partial * p1 = match.second;

	  //	sanity check:
	  //	one of those Partials must exist
	  Assert( p0 != 0 || p1 != 0 );
					
	  // construct source Partial for morph:
	  Partial src;
	  if ( p0 != 0 )
	    {
	      //	use the Partial in the correspondence
	      src = *p0;
			
	      if ( ref0 != 0 )
		{
		  //	find quiet parts of the source Partial,
		  //	and use scaled reference frequencies for 
		  //	those Breakpoints:
		  fix_frequencies( src, *ref0, _freqFixThresholdDb );		
		}
	    }
	  else if ( ref0 != 0 )
	    {
	      //	fake it from the reference Partial:
	      double fscale = double(label) / ref0->label();
	      src = makePartialFromReference( *ref0, fscale );
	    }
	  //	else src is a dummy
		
	  // construct target Partial for morph:
	  Partial tgt;
	  if ( p1 != 0 )
	    {
	      //	use the Partial in the correspondence
	      tgt = *p1;
			
	      if ( ref1 != 0 )
		{
		  //	find quiet parts of the target Partial,
		  //	and use scaled reference frequencies for 
		  //	those Breakpoints:
		  fix_frequencies( tgt, *ref1, _freqFixThresholdDb );
		}
	    }
	  else if ( ref1 != 0 )
	    {
	      //	fake it from the reference Partial:
	      double fscale = double(label) / ref1->label();			
	      tgt = makePartialFromReference( *ref1, fscale );
	    }
	  //	else tgt is a dummy
		
	  debugger << "morphing " << ( ( 0 < src.size() )?( 1 ):( 0 ) )
		   << " and " << ( ( 0 < tgt.size() )?( 1 ):( 0 ) )
		   <<	" partials with label " <<	label << endl;
				   
	  Partial newp = morphPartial( src, tgt, label );
	  if ( 0 < newp.numBreakpoints() )
	    {
	      _partials.push_back( newp );
	    }
	}
    }

  // ---------------------------------------------------------------------------
  //	phaseTravel
  // ---------------------------------------------------------------------------
  //	Compute the sinusoidal phase travel due to sinusoidal oscillation
  // beginning at frequency f0 and changing linearly to frequency f1
  // over time dt. Return the total unwrapped phase travel.
  //
  static double phaseTravel( double f0, double f1, double dt )
    {
      double favg = .5 * ( f0 + f1 );
      return 2 * Pi * favg * dt;
    }
 
  // ---------------------------------------------------------------------------
  //	wrapPi
  // ---------------------------------------------------------------------------
  //	Wrap an unwrapped phase value to the range (-pi,pi].
  //
  static double wrapPi( double x )
    {
      x = std::fmod( x, 2*Pi );
	
      if ( x > Pi )
	{
	  x = x - ( 2*Pi );
	}
      else if ( x < -Pi )
	{
	  x = x + ( 2*Pi );
	}
      return x;
    }

  // ---------------------------------------------------------------------------
  //	makePartialFromReference
  // ---------------------------------------------------------------------------
  // Helper function to construct a Partial for morphing by scaling
  // the frequencies of the reference Partial. This is used when only
  // one of the sources in a morph has a Partial with a particular label.
  //
  Partial
    Morpher::makePartialFromReference( Partial scaleMe, double fscale )
    {
      Partial::iterator prev = scaleMe.begin();	
   
      for ( Partial::iterator it = scaleMe.begin(); it != scaleMe.end(); ++it )
	{
	  it.breakpoint().setAmplitude( 0 );
	  it.breakpoint().setFrequency( it.breakpoint().frequency() * fscale );
		
	  if ( it != prev )
	    {
	      double travel = phaseTravel( prev.breakpoint().frequency(), 
					   it.breakpoint().frequency(),
					   it.time() - prev.time() );
	      it.breakpoint().setPhase( wrapPi( prev.breakpoint().phase() + travel ) );
		
	      prev = it;
	    }
	}
      return scaleMe;
    }

  // ---------------------------------------------------------------------------
  //	Helper function for computing individual morphed frequency values
  //
  inline double 
    Morpher::morphFrequencies( double f0, double f1, double alpha )
    {
      return (alpha * f1) + ((1.-alpha) * f0);
    }
	
  // ---------------------------------------------------------------------------
  //	Helper function for computing individual morphed amplitude values
  //
  inline double 
    Morpher::morphAmplitudes( double a0, double a1, double alpha )
    {
      //	log-amplitude morphing:
      //	it is essential to add in a small Epsilon, so that 
      //	occasional zero amplitudes do not introduce artifacts
      //	(if amp is zero, then even if alpha is very small
      //	the effect is to multiply by zero, because 0^x = 0).
      //
      //	When Epsilon is very small, the curve representing the
      //	morphed amplitude is very steep, such that there is a 
      //	huge difference between zero amplitude and very small
      //	amplitude, and this causes audible artifacts. So instead
      //	use a larger value that shapes the curve more nicely. 
      //	Just have to subtract this value from the morphed 
      //	amplitude to avoid raising the noise floor a whole lot.
      using std::pow;
      static const double Epsilon = 1E-12;
      if ( ( a0 > Epsilon ) || ( a1 > Epsilon ) )
	{
	  double newamp = ( pow( a0 + _ampMorphShape, (1.-alpha) ) * 
			    pow( a1 + _ampMorphShape, alpha ) ) - _ampMorphShape;
	  return std::max( 0.0, newamp );		
		
	}
      else
	{
	  return 0;
	}
    }
	
  // ---------------------------------------------------------------------------
  //	Helper function for computing individual morphed bandwidth values
  //
  inline double 
    Morpher::morphBandwidths( double bw0, double bw1, double alpha )
    {
      return (alpha * bw1) + ((1.-alpha) * bw0);
    }
	
  // ---------------------------------------------------------------------------
  //	Helper function for computing individual morphed phase values
  //
  inline double 
    Morpher::morphPhases( double phi0, double phi1, double alpha )
    {
      //	try to wrap the phase so that they are
      //	as similar as possible:
      while ( ( phi0 - phi1 ) > Pi )
	{
	  phi0 -= 2 * Pi;
	}
      while ( ( phi1 - phi0 ) > Pi )
	{
	  phi0 += 2 * Pi;
	}

      return std::fmod( (alpha * phi1) + ((1.-alpha) * phi0), 2 * Pi );
    }

  // ---------------------------------------------------------------------------
  //	morphPhaseTravel
  // ---------------------------------------------------------------------------
  // Helper function to update the frequency and phase of a morphed Breakpoint
  // to achieve a desired weighted interpolation of phase travel. The desired
  // travel is computed as a weighted function of the phase travel that would
  // exactly match the phase of bp1 and the travel that would exactly match the 
  // frequencies of the two breakpoints. These two values of the phase travel
  // are weighted by the magnitude of the difference between the value of the
  // frequency morph function and one half, so that a a halfway morph uses
  // only the frequencies to determine the desired phase travel and a morph
  // near one of the boundaries (1 or 0) relies only on the interpolated absolute
  // phase to determine the desired phase travel. The frequency of bp1 is updated 
  // according to this morphed phase travel, but the frequency correction is 
  // clamped to at most MaxFixPct. Finally, the phase of bp1 is updated to 
  // match the phase travel between bp0 and bp1 with the updated frequency.
  //
  // bp0 is the Breakpoint that is already part of the morphed Partial
  // bp1 is the new morphed Breakpoint that is being computed
  // dt is the time between the two Breakpoints
  // alpha is the value of the frequency morphing function. 
  //
  // It doesn't matter whether the new Breakpoint is derived from a 
  // source or target Partial (0 or 1 value of morph function) since the 
  // only use of alpha is a measure of the magnitude of the difference 
  // from 0.5 (a halfway morph).
  // 
  static void
    morphPhaseTravel( const Breakpoint & bp0, Breakpoint & bp1, double dt, double alpha )
    {   
      // cannot do anything reasonable if dt == 0, since
      // there is no phase travel:
      if ( dt > 0 )
	{
	  // compute the sinusoidal phase travel between the two Breakpoints:
	  double travel = phaseTravel( bp0.frequency(), bp1.frequency(), dt ); 
   	
	  // compute the error between the predicted (due to phase travel) phase
	  // and the phase of the target Breakpoint:
	  double err = wrapPi( bp1.phase() - ( bp0.phase() + travel ) );
   	
	  // compute a morphed phase travel by weighting the error
	  // correct by the distance from a halfway morph, so that
	  // near halfway (alpha == .5) we rely on the frequencies
	  // to compute the morphed phase travel, and near the boundaries
	  // of the morph (alpha near 1 or 0) we rely on the interpolated
	  // absolute phases (might want to square this or something):
	  double errorWeight = 2. * std::fabs(  0.5 - alpha );
	  travel += errorWeight * err;
   	
	  // compute a new frequency for bp1 from the morphed
	  // phase travel:
	  double ftgt = ( travel / ( Pi * dt ) ) - bp0.frequency();
   	
	  //	the maximum amount by which frequency can be changed to 
	  //	attempt to achieve correct phases:
	  static const double MaxFixPct = .2;

	  //	if bp1 is not a null breakpoint, may need to 
	  //	clamp the amount of frequency correction:
	  if ( bp1.amplitude() != 0. )
	    {	
	      ftgt = std::min( ftgt, bp1.frequency() * ( 1 + (MaxFixPct*.01) ) );
	      ftgt = std::max( ftgt, bp1.frequency() * ( 1 - (MaxFixPct*.01) ) );
	    }
   	
	  // update the morphed frequency to achieve the desired
	  // morphed phase travel:
	  //ftgt = bp1.frequency();
	  bp1.setFrequency( ftgt );
   	
	  //	recompute the phase travel according to the new 
	  // frequency of bp1:
	  travel = phaseTravel( bp0.frequency(), bp1.frequency(), dt );
   	
	  // update the phase of bp1 to be correct according
	  // to the morphed frequencies, updated to achieve
	  // the desired morphed phase travel: 
	  bp1.setPhase( wrapPi( bp0.phase() + travel ) );
	}
    }

  // ---------------------------------------------------------------------------
  //	morphSrcBreakpoint
  // ---------------------------------------------------------------------------
  //!	Compute morphed parameter values at the specified time, using
  //!	the source Breakpoint (assumed to correspond exactly to the
  //!	specified time) and the target Partial (whose parameters are
  //!	examined at the specified time).
  //!
  //!	\pre    the target Partial may not be a dummy Partial (no Breakpoints).
  //!
  //!	\param 	srcBkpt is the Breakpoint corresponding to a morph function
  //!		   	value of 0.
  //!	\param 	tgtPartial is the Partial corresponding to a morph function
  //!		   	value of 1, evaluated at the specified time.
  //!	\param 	time is the time corresponding to srcBkpt (used
  //!			to evaluate the morphing functions and tgtPartial).
  //!	\param	newp is the morphed Partial under construction, the morphed
  //!			Breakpoint is added to this Partial.
  //
  void
    Morpher::morphSrcBreakpoint( const Breakpoint & srcBkpt, const Partial & tgtPartial, 
				 double time, Partial & newp  )
    {
      if ( 0 == tgtPartial.numBreakpoints() )
	{
	  Throw( InvalidArgument, "morphSrcBreakpoint cannot morph with empty Partial" );
	}
    
      //	Don't insert Breakpoints arbitrarily close together.
      if ( 0 == newp.numBreakpoints() || 
	   _minBreakpointGapSec <= ( time - newp.endTime() ) )
	{
	  double fweight = _freqFunction->valueAt( time );
	  double aweight = _ampFunction->valueAt( time );
	  double bweight = _bwFunction->valueAt( time );
    	
	  //	Don't insert Breakpoints at src times if all 
	  //	morph functions equal 1 (or > MaxMorphParam).
	  const double MaxMorphParam = .9;
	  if ( fweight < MaxMorphParam ||
	       aweight < MaxMorphParam ||
	       bweight < MaxMorphParam )
	    {
	      Breakpoint tgtBkpt = tgtPartial.parametersAt( time );
    	    
	      // compute interpolated Breakpoint parameters:
	      Breakpoint morphed;
	      morphed.setFrequency( morphFrequencies( srcBkpt.frequency(), tgtBkpt.frequency(),
						      fweight ) );
	      morphed.setAmplitude( morphAmplitudes( srcBkpt.amplitude(), tgtBkpt.amplitude(),
						     aweight ) );
	      morphed.setBandwidth( morphBandwidths( srcBkpt.bandwidth(), tgtBkpt.bandwidth(),
						     bweight ) );
	      morphed.setPhase( morphPhases( srcBkpt.phase(), tgtBkpt.phase(), fweight ) );

	      // correct phase travel:
	      if ( 0 != newp.numBreakpoints() )
		{
		  morphPhaseTravel( newp.last(), morphed, time - newp.endTime(), fweight );
		}

	      newp.insert( time, morphed );
	    }
	}
    }

  // ---------------------------------------------------------------------------
  //	morphTgtBreakpoint
  // ---------------------------------------------------------------------------
  //!	Compute morphed parameter values at the specified time, using
  //!	the target Breakpoint (assumed to correspond exactly to the
  //!	specified time) and the source Partial (whose parameters are
  //!	examined at the specified time).
  //!
  //!	\pre    the source Partial may not be a dummy Partial (no Breakpoints).
  //!
  //!	\param 	tgtBkpt is the Breakpoint corresponding to a morph function
  //!		   	value of 1.
  //!	\param 	srcPartial is the Partial corresponding to a morph function
  //!		   	value of 0, evaluated at the specified time.
  //!	\param 	time is the time corresponding to srcBkpt (used
  //!			to evaluate the morphing functions and srcPartial).
  //!	\param	newp is the morphed Partial under construction, the morphed
  //!			Breakpoint is added to this Partial.
  //
  void
    Morpher::morphTgtBreakpoint( const Breakpoint & tgtBkpt, const Partial & srcPartial, 
				 double time, Partial & newp  )
    {
      if ( 0 == srcPartial.numBreakpoints() )
	{
	  Throw( InvalidArgument, "morphTgtBreakpoint cannot morph with empty Partial" );
	}
    
      //	Don't insert Breakpoints arbitrarily close together.
      if ( 0 == newp.numBreakpoints() || 
	   _minBreakpointGapSec <= ( time - newp.endTime() ) )
	{
	  double fweight = _freqFunction->valueAt( time );
	  double aweight = _ampFunction->valueAt( time );
	  double bweight = _bwFunction->valueAt( time );
    	
	  //	Don't insert Breakpoints at tgt times if all 
	  //	morph functions equal 0 (or < MinMorphParam).
	  const double MinMorphParam = .1; // 1-MaxMorphparam, above
	  if ( fweight > MinMorphParam ||
	       aweight > MinMorphParam ||
	       bweight > MinMorphParam )
	    {
	      Breakpoint srcBkpt = srcPartial.parametersAt( time );
    	   
	      // compute interpolated Breakpoint parameters:    	   
	      Breakpoint morphed;
	      morphed.setFrequency( morphFrequencies( srcBkpt.frequency(), tgtBkpt.frequency(),
						      fweight ) );
	      morphed.setAmplitude( morphAmplitudes( srcBkpt.amplitude(), tgtBkpt.amplitude(),
						     aweight ) );
	      morphed.setBandwidth( morphBandwidths( srcBkpt.bandwidth(), tgtBkpt.bandwidth(),
						     bweight ) );
	      morphed.setPhase( morphPhases( srcBkpt.phase(), tgtBkpt.phase(), fweight ) );

	      // correct phase travel:
	      if ( 0 != newp.numBreakpoints() )
		{
		  morphPhaseTravel( newp.last(), morphed, time - newp.endTime(), fweight );
		}

	      newp.insert( time, morphed );
	    }
	}
    }

  // ---------------------------------------------------------------------------
  //	fadeTgtBreakpoint
  // ---------------------------------------------------------------------------
  //!	Compute morphed parameter values at the specified time, using
  //!	the source Breakpoint, assumed to correspond exactly to the
  //!	specified time, and assuming that there is no corresponding 
  //! target Partial, so the source Breakpoint should be simply faded.
  //!
  //!	\param 	bp is the Breakpoint corresponding to a morph function
  //!		   	value of 0.
  //!	\param 	time is the time corresponding to bp (used
  //!			to evaluate the morphing functions).
  //!	\param	newp is the morphed Partial under construction, the morphed
  //!			Breakpoint is added to this Partial.
  //
  void
    Morpher::fadeSrcBreakpoint( Breakpoint bp, double time, 
				Partial & newp )
    {
      //	Don't insert Breakpoints arbitrarily close together.
      //  (Need to be consistent with morphing behavior.)
      if ( 0 == newp.numBreakpoints() || 
	   _minBreakpointGapSec <= ( time - newp.endTime() ) )
	{
	  double alpha = _ampFunction->valueAt( time );
	  bp.setAmplitude( morphAmplitudes( bp.amplitude(), 0, alpha ) );
	  newp.insert( time, bp );
	}
    }

  // ---------------------------------------------------------------------------
  //	fadeTgtBreakpoint
  // ---------------------------------------------------------------------------
  //!	Compute morphed parameter values at the specified time, using
  //!	the target Breakpoint, assumed to correspond exactly to the
  //!	specified time, and assuming that there is not corresponding 
  //! source Partial, so the target Breakpoint should be simply faded.
  //!
  //!	\param 	bp is the Breakpoint corresponding to a morph function
  //!		   	value of 1.
  //!	\param 	time is the time corresponding to bp (used
  //!			to evaluate the morphing functions).
  //!	\param	newp is the morphed Partial under construction, the morphed
  //!			Breakpoint is added to this Partial.
  //
  void
    Morpher::fadeTgtBreakpoint( Breakpoint bp, double time, 
				Partial & newp )
    {
      //	Don't insert Breakpoints arbitrarily close together.
      //  (Need to be consistent with morphing behavior.)
      if ( 0 == newp.numBreakpoints() || 
	   _minBreakpointGapSec <= ( time - newp.endTime() ) )
	{
	  double alpha = _ampFunction->valueAt( time );
	  bp.setAmplitude( morphAmplitudes( 0, bp.amplitude(), alpha ) );
	  newp.insert( time, bp );
	}
    }

  // ---------------------------------------------------------------------------
  //	morphParameters - Breakpoint to Breakpoint
  // ---------------------------------------------------------------------------
  //	Compute morphed parameter values at the specified time, using
  //	the source and target Breakpoints (assumed to correspond exactly 
  //	to the specified time).
  void
    Morpher::morphParameters( const Breakpoint & srcBkpt, const Breakpoint & tgtBkpt, 
			      double time, Breakpoint & retBkpt )
    {
      //	evaluate morphing functions at time:
      double alphaA = _ampFunction->valueAt( time );
      double alphaF = _freqFunction->valueAt( time );
      double alphaBW = _bwFunction->valueAt( time );

      //	compute weighted average parameters for 
      //	the return Breakpoint:	
      retBkpt.setFrequency( (alphaF * tgtBkpt.frequency()) + ((1.-alphaF) * srcBkpt.frequency()) );
      retBkpt.setAmplitude( (alphaA * tgtBkpt.amplitude()) + ((1.-alphaA) * srcBkpt.amplitude()) );
      retBkpt.setBandwidth( (alphaBW * tgtBkpt.bandwidth()) + ((1.-alphaBW) * srcBkpt.bandwidth()) );
      retBkpt.setPhase( (alphaF * tgtBkpt.phase()) + ((1.-alphaF) * srcBkpt.phase()) );
    }

  // ---------------------------------------------------------------------------
  //	morphParameters - Breakpoint to Partial
  // ---------------------------------------------------------------------------
  //	Compute morphed parameter values at the specified time, using
  //	the source Breakpoint (assumed to correspond exactly to the
  //	specified time) and the target Partial (whose parameters are
  //	examined at the specified time).
  //
  //	The target Partial may be a dummy Partial (no Breakpoints).
  //
  void
    Morpher::morphParameters( const Breakpoint & srcBkpt, const Partial & tgtPartial, 
			      double time, Breakpoint & retBkpt )
    {
      //	if tgtPartial is a valid Partial, compute 
      //	weighted average parameters, otherwise just 
      //	fade the source:
      if ( tgtPartial.numBreakpoints() > 0 )
	{
	  //	evalute the target Partial at time:
	  Breakpoint tgtBkpt = tgtPartial.parametersAt( time );
	  //evaluateAt( tgtPartial, time, tgtBkpt );
		
	  //	compute weighted average parameters for 
	  //	the return Breakpoint:	
	  morphParameters( srcBkpt, tgtBkpt, time, retBkpt );
	}
      else
	{
	  //	its a dummy Partial, just fade (amplitude scale)
	  //	the source Breakpoint:
	  retBkpt = srcBkpt;
	  retBkpt.setAmplitude( (1.-_ampFunction->valueAt( time )) * srcBkpt.amplitude() );
	}
    }

  // ---------------------------------------------------------------------------
  //	morphParameters - Partial to Breakpoint
  // ---------------------------------------------------------------------------
  //	Compute morphed parameter values at the specified time, using
  //	the source Partial (whose parameters are examined at the specified 
  //	time) and the target Breakpoint (assumed to correspond exactly to 
  //	the specified time).
  //
  //	The source Partial may be a dummy Partial (no Breakpoints).
  //
  void
    Morpher::morphParameters( const Partial & srcPartial, const Breakpoint & tgtBkpt, 
			      double time, Breakpoint & retBkpt )
    {
      //	if srcPartial is a valid Partial, compute 
      //	weighted average parameters, otherwise just 
      //	fade the source:
      if ( srcPartial.numBreakpoints() > 0 )
	{
	  //	evalute the target Partial at time:
	  Breakpoint srcBkpt = srcPartial.parametersAt( time );
	  //evaluateAt( srcPartial, time, srcBkpt );
		
	  //	compute weighted average parameters for 
	  //	the return Breakpoint:	
	  morphParameters( srcBkpt, tgtBkpt, time, retBkpt );
	}
      else
	{
	  //	its a dummy Partial, just fade (amplitude scale)
	  //	the target Breakpoint:
	  retBkpt = tgtBkpt;
	  retBkpt.setAmplitude( _ampFunction->valueAt( time ) * tgtBkpt.amplitude() );
	}
    }

  // ---------------------------------------------------------------------------
  //	morphParameters - Partial to Partial
  // ---------------------------------------------------------------------------
  //	Compute morphed parameter values at the specified time, using
  //	the source  and target Partials, both of whose parameters are 
  //	examined at the specified time.
  //
  //	Either (or neither) the source or target Partial may be a dummy 
  //	Partial (no Breakpoints), but not both.
  //
  void
    Morpher::morphParameters( const Partial & srcPartial, const Partial & tgtPartial, 
			      double time, Breakpoint & retBkpt )
    {
      if ( srcPartial.numBreakpoints() == 0 && tgtPartial.numBreakpoints() == 0 )
	Throw( InvalidArgument, "Cannot morph a pair of dummy (empty) Partials." );
		
      // 	if the source Partial is valid, compute source parameters,
      //	otherwise compute target Partial parameters at the specified
      //	time, and scale the amplitude:
      Breakpoint srcBkpt;
      if ( srcPartial.numBreakpoints() > 0 )
	{
	  //	compute interpolated values for srcPartial:
	  srcBkpt  = srcPartial.parametersAt( time );
	  //evaluateAt( srcPartial, time, srcBkpt );
	}
      else
	{
	  //	srcPartial is a dummy Partial, just fade (amplitude scale)
	  //	the target Partial:
	  retBkpt = tgtPartial.parametersAt( time );
	  //evaluateAt( tgtPartial, time, retBkpt );
	  retBkpt.setAmplitude( _ampFunction->valueAt( time ) * retBkpt.amplitude() );
	  return;
	}
	
      //	if the target Partial is valid, compute target parameters,
      //	otherwise use the source parameter values, and scale the
      //	amplitude:
      Breakpoint tgtBkpt;
      if ( tgtPartial.numBreakpoints() > 0 )
	{
	  //	compute interpolated values for tgtPartial:
	  tgtBkpt = tgtPartial.parametersAt( time );
	  //evaluateAt( tgtPartial, time, tgtBkpt );
	}
      else
	{
	  //	tgtPartial is a dummy Partial, just fade (amplitude scale)
	  //	the source Partial:
	  retBkpt = srcPartial.parametersAt( time );
	  //evaluateAt( srcPartial, time, retBkpt );
	  retBkpt.setAmplitude( (1.-_ampFunction->valueAt( time )) * retBkpt.amplitude() );		
	  return;
	}
			
      //	both Partials were valid, compute weighted average parametersfor 
      //	the return Breakpoint:	
      morphParameters( srcBkpt, tgtBkpt, time, retBkpt );
	
    }


}	//	end of namespace Loris
