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
 *	lorisPartialList.i
 *
 *	SWIG interface file describing the PartialList class.
 *	A PartialList is a Loris::Handle<  >.
 *	Include this file in loris.i to include the PartialList class
 *	interface in the scripting module. (Can be used with the 
 *	-shadow option to SWIG to build a PartialList class in the 
 *	scripting interface.) This file does not support exactly the 
 *	public interface of the C++ std::list class, but has been 
 *	modified to better support SWIG and scripting languages.
 *
 *	--- CHANGES ---
 *	This interface has been modified (March 2001) to wrap the list
 *	access with Loris::Handles, and to include iterators on the lists,
 *	called Partial in the interface, and iterators on the Partials, 
 *	called Breakpoint in the interface. The iterators retain a counted
 *	reference to their collections (PartialList or Partial), and this 
 *	is the reason for wrapping those container class with Handles, so
 *	that an iterator that outlives its container in the interpreter
 *	doesn't wind up refering to an object that got garbage-collected.
 *	In this implementation, all references in the interpreter and all
 *	references in the interpreter to iterators have to be deleted 
 *	before the collection is finally released.
 *
 *	(April 2001) Guaranteeing the safety and validity of all these 
 *	iterators and collections is so costly and complicated that it
 *	cannot be worth the headaches and performance penalty. So for now,
 *	they come with no guarantees, and clients just have o be responsible,
 *	as in the C++ STL, with the added excitement of garbage collection. (!)
 *
 * Kelly Fitz, 17 Nov 2000
 * loris@cerlsoundgroup.org
 *
 * http://www.cerlsoundgroup.org/Loris/
 *
 */
 
/*  Define this symbol to keep old-style iterator behavior,
	even with the new iterators.
 */
#if defined (SWIGPYTHON)
%module loris
#endif
#define LEGACY_ITERATOR_BEHAVIOR
 
%include typemaps.i
%apply double * OUTPUT { double * tmin_out, double * tmax_out };

%newobject *::copy;
%newobject *::begin;
%newobject *::end;
%newobject *::insert;
%newobject NewPartialIterator::next;
	// but not NewPlistIterator::next
%newobject *::__iter__;
%newobject *::iterator;

%newobject *::findAfter;
%newobject *::findNearest;
	//  these aren't actually needed yet
	
#ifdef LEGACY_ITERATOR_BEHAVIOR
%newobject PartialListIterator::next;
%newobject PartialIterator::next;
%newobject *::prev;
#endif

/* ***************** inserted C++ code ***************** */
%{
#include "Partial.h"
#include "PartialList.h"
#include "PartialUtils.h"
#include "Notifier.h"
#include <list>

using Loris::debugger;
using Loris::Partial;
using Loris::PartialList;
using Loris::PartialListIterator;
using Loris::Breakpoint;

typedef Loris::Partial::iterator PartialIterator;

/*	new iterator definitions

	These are much better than the old things, more like the 
	iterators in Python 2.2 and later, very much simpler.
	The old iterators will be replaced entirely by the new
	kind soon, very soon.
	
	Note: the only reason I cannot merge the new functionality 
	into the old iterators is that the old iterators use the
	next() method to advance and return another iterator. Duh.
*/
struct NewPlistIterator
{
	PartialList & subject;
	PartialList::iterator it;

	NewPlistIterator( PartialList & l ) : subject( l ), it ( l.begin() ) {}
	NewPlistIterator( PartialList & l, PartialList::iterator i ) : subject( l ), it ( i ) {}
	
	bool atEnd( void ) { return it == subject.end(); }
	bool hasNext( void ) { return !atEnd(); }

	Partial * next( void )
	{
		if ( atEnd() )
		{
			throw_exception("end of PartialList");
			return 0;
		}
		Partial * ret = &(*it);
		++it;
		return ret;
	}
};

typedef Partial::iterator BreakpointPosition;

struct NewPartialIterator
{
	Partial & subject;
	Partial::iterator it;

	NewPartialIterator( Partial & p ) : subject( p ), it ( p.begin() ) {}
	NewPartialIterator( Partial & p, Partial::iterator i ) : subject( p ), it ( i ) {}
	
	bool atEnd( void ) { return it == subject.end(); }
	bool hasNext( void ) { return !atEnd(); }

	BreakpointPosition * next( void )
	{
		if ( atEnd() )
		{
			throw_exception("end of Partial");
			return 0;
		}
		BreakpointPosition * ret = new BreakpointPosition(it);
		++it;
		return ret;
	}
};

%}
/* ***************** end of inserted C++ code ***************** */

/* *********** exception handling for new iterators *********** */
/*	Exception handling code copied from the SWIG manual. 
	Tastes great, less filling.
	Defined in loris.i.
*/

%include exception.i
%exception next
{
    char * err;
    clear_exception();
    $action
    if ((err = check_exception()))
    {
#if defined(SWIGPYTHON)
		%#ifndef NO_PYTHON_EXC_STOPITER
		PyErr_SetString( PyExc_StopIteration, err );
		return NULL;
		%#else
		SWIG_exception( SWIG_ValueError, err );
		%#endif
#else
        SWIG_exception( SWIG_ValueError, err );
#endif
    }
}

%exception PartialList::erase
{
    char * err;
    clear_exception();
    $action
    if ((err = check_exception()))
    {
        SWIG_exception( SWIG_ValueError, err );
    }
}

#ifdef LEGACY_ITERATOR_BEHAVIOR
%exception NewPlistIterator::partial
{
    char * err;
    clear_exception();
    $action
    if ((err = check_exception()))
    {
        SWIG_exception( SWIG_ValueError, err );
    }
}
#endif
/* ******** end of exception handling for new iterators ******** */

/* ***************** new PartialList iterator ****************** */

%nodefault NewPlistIterator;
class NewPlistIterator
{
public:
	bool atEnd( void );
	Partial * next( void );
#ifdef SIWGPYTHON
    %extend
    {
        NewPlistIterator * __iter__( void )
        {
            return self;
        }

        NewPlistIterator * iterator( void )
        {
            return self;
        }
    }
#endif
#ifdef LEGACY_ITERATOR_BEHAVIOR
	%extend
	{
		Partial * partial( void )
		{
			if ( self->atEnd() )
			{
				throw_exception("end of PartialList");
				return 0;
			}			
			Partial & current = *(self->it);
			return &current;
		}
	}
#endif
};

/* ************** end of new PartialList iterator ************** */

/* ******************** new Partial iterator ******************* */

%nodefault NewPartialIterator;
class NewPartialIterator
{
public:
	bool atEnd( void );
	bool hasNext( void );
	BreakpointPosition * next( void );
#ifdef SIWGPYTHON
    %extend
    {
        NewPartialIterator * __iter__( void )
        {
            return self;
        }

        NewPartialIterator * iterator( void )
        {
            return self;
        }
    }
#endif
};

/* **************** end of new Partial iterator **************** */

/* ************************ PartialList ************************ */

/*	PartialList

	A PartialList represents a collection of Bandwidth-Enhanced 
	Partials, each having a trio of synchronous, non-uniformly-
	sampled breakpoint envelopes representing the time-varying 
	frequency, amplitude, and noisiness of a single bandwidth-
	enhanced sinusoid.

	For more information about Bandwidth-Enhanced Partials and the  
	Reassigned Bandwidth-Enhanced Additive Sound Model, refer to
	the Loris website: www.cerlsoundgroup.org/Loris/
*/
#ifdef LEGACY_ITERATOR_BEHAVIOR
class PartialListIterator;
#endif

class PartialList
{
public:
	//	PartialList construction:
	PartialList( void );
	PartialList( const PartialList & rhs );
	~PartialList( void );
	
	//	std::list methods:
	void clear( void );	
	unsigned long size( void );

	%extend
	{
		 //	wrapper for PartialUtils::timeSpan,
		 // Return the minimum start time and maximum end time
		 // of all Partials in this PartialList.
		 void timeSpan( double * tmin_out, double * tmax_out )
		 {
		 	std::pair<double, double> span = 
		 		Loris::PartialUtils::timeSpan( self->begin(), self->end() );
		 	*tmin_out = span.first;
		 	*tmax_out = span.second;
		 }
		  
		//	(new-style) iterator access:
		NewPlistIterator * iterator( void )
		{
			return new NewPlistIterator(*self);
		}
		#ifdef SWIGPYTHON
		NewPlistIterator * __iter__( void )
		{
			return new NewPlistIterator(*self);
		}
		#endif	
		
		//  append does not return position of inserted element:
		void append( Partial * partial )
		{
			self->insert( self->end(), *partial );
		}
	
		//  insert acts like STL insert, returning position
		//  of inserted element:
		NewPlistIterator * insert( NewPlistIterator * position, Partial * partial )
		{
			if ( self != &(position->subject) )
				return 0;
			return new NewPlistIterator(*self, self->insert( position->it, *partial ) );
		}

		//  implement erase using a linear search to find
		//  the Partial that should be removed -- slow and
		//  gross, but the only straightforward way to make
		//  erase play nice with the new iterator paradigm
		//  (especially in Python). Raise an exception if
		//  the specified Partial is not in the list.
		void erase( Partial * partial )
		{
			PartialList::iterator it = self->begin();
			while ( it != self->end() )
			{
				if ( &(*it) == partial )	// compare addresses
				{
					self->erase( it );
					return;
				}
				++it;
			}
			throw_exception( "PartialList.erase(p): p not in PartialList" );
		}
		 
		//  splice at end:
		void splice( PartialList * other )
		{
			self->splice( self->end(), *other );
		}

		Partial * first( void )
		{
			if ( self->empty() )
			{
				return 0;
			}
			else
			{
				return &( self->front() );
			}
		}

		Partial * last( void )
		{
			if ( self->empty() )
			{
				return 0;
			}
			else
			{
				return &( self->back() );
			}
		}

	}	//	end of added methods

#ifdef LEGACY_ITERATOR_BEHAVIOR
	//  old iterator behavior, keep this around to support 
	//  old code that uses the old iterators:
	PartialListIterator begin( void );
	PartialListIterator end( void );
	PartialListIterator insert( PartialListIterator position, const Partial & partial );
	void erase( PartialListIterator position );
	void splice( PartialListIterator position, PartialList & list );
	%extend
	{
		PartialListIterator insert( const Partial & partial )
		{
			return self->insert( self->end(), partial );
		}
	}

#endif	
	/*  Keep this around to support legacy code that
		might use it. Plain old copy constructor works
		now too.
	 */
	%extend
	{		
		PartialList * copy( void ) { return new PartialList( *self ); }
	}
};

/* ********************* end of PartialList ********************* */

/* ************************** Partial *************************** */

#ifdef LEGACY_ITERATOR_BEHAVIOR 
class PartialIterator;
#endif

class Partial
{
public:
	//	Partial construction:
	Partial( void );
	Partial( const Partial & );
	~Partial( void );
	
	//	Partial access and mutation:
	int label( void );
	double initialPhase( void );
	double startTime( void );
	double endTime( void );
	double duration( void );
	long numBreakpoints( void );
	// Breakpoint & first( void );
	// Breakpoint & last( void );
	
	void setLabel( int l );
		
	//	partial envelope interpolation/extrapolation:
	//	Return the interpolated value of a partial parameter at
	//	the specified time. At times beyond the ends of the
	//	Partial, frequency and bandwidth hold their boundary values,
	//	amplitude is zero, and phase is computed from frequency.
	//	There is of sensible definition for any of these for Partials
	//	having no Breakpoints, so they except (InvalidPartial) under 
	//	that condition.
	double frequencyAt( double time );
	double amplitudeAt( double time );
	double bandwidthAt( double time );
	double phaseAt( double time );

	%extend
	{
		//	new iterator access:
		NewPartialIterator * iterator( void )
		{
			return new NewPartialIterator(*self);
		}
		#ifdef SWIGPYTHON
		NewPartialIterator * __iter__( void )
		{
			return new NewPartialIterator(*self);
		}
		#endif	

		//	erase works nicely with the new iterators:
		void erase( BreakpointPosition * pos )
		{
			if ( *pos != self->end() )
			{
				*pos = self->erase( *pos );
			}
		}

        Breakpoint * first( void )
        {
            if ( self->numBreakpoints() == 0 )
            {
                return 0;
            }
            else
            {
                return &( self->first() );
            }
        }

        Breakpoint * last( void )
        {
            if ( self->numBreakpoints() == 0 )
            {
                return 0;
            }
            else
            {
                return &( self->last() );
            }
        }
#if !defined( LEGACY_ITERATOR_BEHAVIOR )
		//  cannot include these while the legacy versions
		//  are still supported!
		NewPartialIterator * insert( double time, const Breakpoint & bp )
		{
			return new NewPartialIterator(*self, self->insert( time, bp ));
		}
		
		NewPartialIterator * findAfter( double time )
		{
			return new NewPartialIterator(*self, self->findAfter( time ));
		}
	
		NewPartialIterator * findNearest( double time )
		{
			return new NewPartialIterator(*self, self->findNearest( time ));
		}
#endif
	}
	
#ifdef LEGACY_ITERATOR_BEHAVIOR
	PartialIterator begin( void );
	PartialIterator end( void );

	void erase( PartialIterator & pos );

	//	Partial access/mutation through iterators:
	//
	//  Dunno how I am going to migrate this functionality
	//  to the new iterators without breaking code...
	PartialIterator insert( double time, const Breakpoint & bp );
	/*	Make a copy of bp and insert it at time (seconds),
		return an iterator refering to the inserted Breakpoint.
	 */

	PartialIterator findAfter( double time );
	/*	Return the insertion position for a Breakpoint at
		the specified time (that is, the position of the first
		Breakpoint at a time later than the specified time).
	 */
	 
	PartialIterator findNearest( double time );
	/*	Return the insertion position for the Breakpoint nearest
		the specified time.
	 */
#endif

	/*  Keep this around to support legacy code that
		might use it. Plain old copy constructor works
		now too.
	 */
	%extend
	{
		Partial * copy( void ) { return new Partial( *self ); }
		 
		int equals( Partial * other )
		{
			return *self == *other;
		}
		/*	Return true (1) if this Partial is equal to the other. 
			Partials are equal is they have the same label and the
			same Breakpoint envelope.
		 */
	}	//	end of added methods
		
};
/* *********************** end of Partial *********************** */

/* ************************* Breakpoint ************************* */

/*	Breakpoint
	
	A Breakpoint represents a single breakpoint in the time-varying
	frequency, amplitude, and bandwidth envelope of a Reassigned 
	Bandwidth-Enhanced Partial.
	
	Instantaneous phase is also stored, but is only used at the onset of 
	a partial, or when it makes a transition from zero to nonzero amplitude.

	A Partial represents a Reassigned Bandwidth-Enhanced model component.
	For more information about Bandwidth-Enhanced Partials and the  
	Reassigned Bandwidth-Enhanced Additive Sound Model, refer to
	the Loris website: www.cerlsoundgroup.org/Loris/
 */
class Breakpoint
{
public:	
//	construction:
//
	Breakpoint( double f, double a, double b, double p = 0. );
	/*	Return a new Breakpoint having the specified frequency
		amplitude, bandwidth, and (optionally, defaults to zero)
		phase.
	 */

	Breakpoint( const Breakpoint & rhs );
	/*	Return a new Breakpoint that is a copy of this 
		Breakpoint (i.e. has identical parameter values).
	 */

	~Breakpoint( void );
	/*	Delete this Breakpoint.
	 */

//	attribute access:
//
	double frequency( void );
	/*	Return the frequency of this Breakpoint. 
	 */
	 
	double amplitude( void );
	/*	Return the amplitude of this Breakpoint. 
	 */
	 
	double bandwidth( void );
	/*	Return the bandwidth of this Breakpoint. 
	 */
	 
	double phase( void );
	/*	Return the phase of this Breakpoint. 
	 */
	 	
//	attribute mutation:
//
	void setFrequency( double x );
	/*	Assign the frequency of this Breakpoint. 
	 */
	 
	void setAmplitude( double x );
	/*	Assign the amplitude of this Breakpoint. 
	 */
	 
	void setBandwidth( double x );
	/*	Assign the bandwidth of this Breakpoint. 
	 */
	 
	void setPhase( double x );
	/*	Assign the phase of this Breakpoint. 
	 */
	 
	%extend
	{
		Breakpoint * copy( void )
		{
			return new Breakpoint( *self );
		}
		/*	Return a new Breakpoint that is a copy of this 
			Breakpoint (i.e. has identical parameter values).
		 */
		 
		int equals( Breakpoint * other )
		{
			return *self == *other;
		}
		/*	Return true (1) if this Breakpoint is equal to the
			other. Breakpoints are equal is they have identical 
			parameter values.
		 */

	}	//	end of added methods
	
};	//	//	end of SWIG interface class Breakpoint


%nodefault BreakpointPosition;
class BreakpointPosition
{
public:
	%extend
	{
		double time( void ) 
		{ 
			return self->time(); 
		}
		Breakpoint * breakpoint( void ) 
		{ 
			return &(self->breakpoint());
		}
		
		//	duplicate the Breakpoint interface:
		//	(not sure yet whether this is the right way)
		//
		
		double frequency( void ) { return self->breakpoint().frequency(); }
		/*	Return the frequency of this Breakpoint. 
		*/
		
		double amplitude( void ) { return self->breakpoint().amplitude(); }
		/*	Return the amplitude of this Breakpoint. 
		*/
		
		double bandwidth( void ) { return self->breakpoint().bandwidth(); }
		/*	Return the bandwidth of this Breakpoint. 
		*/
		
		double phase( void ) { return self->breakpoint().phase(); }
		/*	Return the phase of this Breakpoint. 
		*/
			
		void setFrequency( double x ) { self->breakpoint().setFrequency( x ); }
		/*	Assign the frequency of this Breakpoint. 
		*/
		
		void setAmplitude( double x ) { self->breakpoint().setAmplitude( x ); }
		/*	Assign the amplitude of this Breakpoint. 
		*/
		
		void setBandwidth( double x ) { self->breakpoint().setBandwidth( x ); }
		/*	Assign the bandwidth of this Breakpoint. 
		*/
		
		void setPhase( double x ) { self->breakpoint().setPhase( x ); }
		/*	Assign the phase of this Breakpoint. 
		*/
		
	}
};

/* ******************* legacy PartialIterator ******************* */
#ifdef LEGACY_ITERATOR_BEHAVIOR
/*	PartialIterator
	
	A PartialIterator represents an iterator on a Partial.
 */
class PartialIterator
{
public:
//	time and Breakpoint access:
//
	double time( void ) const { return _iter->first; }	
	/*	Return the time of the Breakpoint at the position of this
		PartialIterator.
	 */

	//	most of the pointer semantics of std C++ iterators
	//	are inappropriate for the scripting interface (those
	//	languages don't have pointers), so many methods in the 
	//	interface need to be added:
	%extend 
	{
		//  Breakpoint & breakpoint( void )
		//	this doesn't seem to swig correctly, Breakpoint
		//	winds up with ownership, try fixng it here:
		Breakpoint * breakpoint( void ) 
		{ 
			return &(self->breakpoint());
		}
		/*  Return (a reference to) the Breakpoint at the position of this
			PartialIterator.
		 */


		PartialIterator * copy( void )
		{
			return new PartialIterator( *self );
		}
		/*	Return a new PartialIterator that is a copy of this 
			PartialIterator (i.e. refers to the same position
			in the same Partial).
		 */
		 
		PartialIterator * next( void )
		{
			PartialIterator * next = new PartialIterator(*self);
			++(*next);
			return next;
		}
		/*	Return an iterator refering to the next position in the Partial.
		 */
		 
		PartialIterator * prev( void )
		{
			PartialIterator * prev = new PartialIterator(*self);
			--(*prev);
			return prev;
		}
		/*	Return an iterator refering to the previous position in the Partial.
		 */
		 
		int equals( PartialIterator * other )
		{
			return *self == *other;
		}
		/*	Return true (1) if this PartialIterator is equal to the
			other. PartialIterators are equal is they refer to the
			same position (Breakpoint) in the same Partial.
		 */

		 int isInRange( const PartialIterator * begin, const PartialIterator * end )
		 {	
		 	PartialIterator it;
		 	for ( it = *begin; it != *end; ++it )
		 	{
		 		if ( it == *self )
		 			return true;
		 	}
		 	return false;
		 }
		 /*	Return true (1) is this iterator is within the half-open iterator
		 	range bounded by begin and end, and false otherwise. This method
		 	can be used to check the validity of an iterator -- call with 
		 	begin and end methods of the Partial as arguments.
		  */

	}	//	end of added methods

};	
#endif
/* **************** end of legacy PartialIterator **************** */

/* ***************** legacy PartialListIterator ***************** */
#ifdef LEGACY_ITERATOR_BEHAVIOR
/*	PartialListIterator
	
	A PartialListIterator represents an iterator on a PartialList.
	Its interface reflects the interface of the underlying std::list
	iterator.
 */
class PartialListIterator
{
public:
	%extend 
	{
		PartialListIterator * copy( void )
		{
			return new PartialListIterator( *self );
		}
		/*	Return a new PartialListIterator that is a copy of this 
			PartialListIterator (i.e. refers to the same position
			in the same PartialList).
		 */
		 
		PartialListIterator * next( void )
		{
			PartialListIterator * next = new PartialListIterator(*self);
			++(*next);
			return next;
		}
		/*  Return an iterator refering to the next position in the
			PartialList. 
		 */
		 
		PartialListIterator * prev( void )
		{
			PartialListIterator * prev = new PartialListIterator(*self);
			--(*prev);
			return prev;
		}
		/*	Return an iterator refering to the previous position
			in the PartialList.
		 */
		 
		Partial * partial( void )
		{
			Partial & current = **self;
			return &current;
		}
		/*	Return (a reference to) the Partial at the position of this
			PartialListIterator.
		 */
		 
		int equals( PartialListIterator * other )
		{
			return *self == *other;
		}
		/*	Return true (1) if this PartialListIterator is equal to the
			other. PartialListIterators are equal if they refer to the
			same position in the same PartialList.
		 */
		 
		 int isInRange( const PartialListIterator * begin, const PartialListIterator * end )
		 {	
		 	PartialListIterator it;
		 	for ( it = *begin; it != *end; ++it )
		 	{
		 		if ( it == *self )
		 			return true;
		 	}
		 	return false;
		 }
		 /*	Return true (1) if this iterator is within the half-open iterator
		 	range bounded by begin and end, and false otherwise. This method
		 	can be used to check the validity of an iterator -- call with 
		 	begin and end methods of the PartialList as arguments.
		  */

	}	//	end of added methods

};
#endif
/* ************** end of legacy PartialListIterator ************** */
