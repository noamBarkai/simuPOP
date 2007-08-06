/***************************************************************************
 *   Copyright (C) 2004 by Bo Peng                                         *
 *   bpeng@rice.edu
 *                                                                         *
 *   $LastChangedDate$
 *   $Rev$                                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _SELECTOR_H
#define _SELECTOR_H
/**
\file
\brief head file of class selector:public baseOperator
*/
#include "utility.h"
#include "operator.h"

#include "boost/tuple/tuple.hpp"
#include <numeric>
using std::min;

namespace simuPOP
{
    /// genetic selection
	/**
    Genetic selection is tricky to simulate since there are many different \em fitness
    values and many different ways to apply selection. \c simuPOP employs an
    \em 'ability-to-mate' approach. Namely, the probability that an individual will be
    chosen for mating is proportional to its fitness value. More specifically,
    \li \c PreMating selectors assign fitness values to each individual.
    \li During sexless mating (e.g. \c binomialSelection???), individuals are chosen
        at probabilities that are proportional to their fitness values. If there are
        \f$ N \f$ individuals with fitness values \f$ f_{i},i=1,...,N \f$, individual
        \f$ i \f$ will have probability \f$ \frac{f_{i}}{\sum_{j}f_{j}} \f$ to be chosen
        and passed to the next generation.
    \li During \c randomMating, males and females are separated. They are chosen from
        their respective groups in the same manner and mate.

    It is not very clear that our method agrees with the traditional
    <em>'average number of offspring'</em> definition of fitness. (Note that this concept
    is very difficult to simulate since we do not know who will determine the number of
    offspring if two parents are involved.) 

    All of the selection operators, when applied, will set a variable \c fitness and an
    indicator so that 'selector-aware' mating scheme can select individuals according to
    these values. Hence, two consequences are stated below:
    \li selector alone can not do selection! Only mating schemes can actually select individuals.
    \li selector has to be \c PreMating operator. This is not a problem when you use the
        operator form of the selectors since their default stage is \c PreMating. However,
        if you use the function form of these selectors in a \c pyOperator, make sure to
        set the stage of \c pyOperator to \c PreMating.
	*/
	class selector: public baseOperator
	{
		public:
			/// create a selector
			/**
			\param subPop a shortcut to <tt>subPops=[subPop]</tt>
			\param subPops subpopulations that the selector will apply to. Default to all.
			*/
			selector(const vectoru & subPops=vectoru(), int stage=PreMating, int begin=0, int end=-1, int step=1, vectorl at=vectorl(),
				int rep=REP_ALL, int grp=GRP_ALL, const vectorstr& infoFields=vectorstr(1, "fitness"))
				: baseOperator("","",stage, begin, end, step, at, rep, grp, infoFields), m_subPops(subPops)
			{
			}

			/// destructor
			virtual ~selector()
			{
			}

            /// deep copy of a selector
			virtual baseOperator * clone() const
			{
				return new selector(*this);
			}

			/// calculate/return the fitness value???
			virtual double indFitness(individual *, ULONG gen)
			{
				///
				throw ValueError("This selector is not supposed to be called directly");
				return 1.;
			}

			/// set fitness to all individuals???
			bool apply(population& pop);

            /// used by Python print function to print out the general information of the selector
			virtual string __repr__()
			{
				return "<simuPOP::selector>" ;
			}
			
		private:
			vectoru m_subPops;
	};

    /// selection according to the genotype at one locus
	/**
	This map selector implements selection at one locus. A user provided dictionary (map) of
    genotypes will be used in this selector to set each individual's fitness value.
    <funcForm>MapSelector</funcForm>
	*/
	class mapSelector: public selector
	{
		public:
            /// create a map selector
			/**
			\param locus the locus index. The genotype of this locus will be examed.???
			\param loci the locus indices. The genotypes of these loci will be examed.
			\param fitness a dictionary of fitness values. The genotype must be in the form of <tt>'a-b'</tt>
                for a single locus, and <tt>'a-b|c-d|e-f'</tt> for multi-loci.
			\param phase if \c True, genotypes \c a-b and \c b-a will have different fitness values. Default to false.
			\param output and other parameters please refer to help(baseOperator.__init__)???
			*/
			mapSelector( vectoru loci, const strDict& fitness, bool phase=false,
				const vectoru & subPops=vectoru(), int stage=PreMating, int begin=0, int end=-1, int step=1,
				vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(1, "fitness")):
			selector(subPops, stage, begin, end, step, at, rep, grp, infoFields),
				m_loci(loci), m_dict(fitness), m_phase(phase)
			{
			};

			virtual ~mapSelector()
			{
			}

            /// deep copy of a map selector
			virtual baseOperator * clone() const
			{
				return new mapSelector(*this);
			}

			/// calculate/return the fitness value, currently assuming diploid
			virtual double indFitness(individual * ind, ULONG gen);

            /// used by Python print function to print out the general information of the map selector
			virtual string __repr__()
			{
				return "<simuPOP::selector::map selector>" ;
			}

		private:
			/// one locus
			vectoru m_loci;

			/// fitness for each genotype
			strDict m_dict;

			///
			bool m_phase;
	};

    /// multiple allele selector (selection according to wildtype or diseased alleles)  
	/**
    This is called 'multiple-allele' selector. It separate alleles into two groups:
    wildtype and disease alleles. Wildtype alleles are specified by parameter
    \c wildtype and any other alleles are considered as diseased alleles.
    <funcForm>MaSelect</funcForm>
	*/
	class maSelector: public selector
	{
		public:

            /// create a multiple allele selector       
			/**
			\param fitness for the single locus case, fitness is an array of fitness of AA, Aa, aa.
                A is the wildtype group. In the case of multiple loci, fitness should be in the order of
				AABB, AABb, AAbb, AaBB, AaBb, Aabb, aaBB, aaBb, aabb.
			\param wildtype an array of alleles in the wildtype group. Any other alleles are
                considered to be diseased alleles. Default to <tt>[0]</tt>.
			\param output and other parameters please refer to help(baseOperator.__init__)???

            Please refer to \c mapSelector for other parameter descriptions.
                
            \note \li \c maSelector only works for diploid populations now.
                  \li \c wildtype at all loci are the same.
			*/
			maSelector( vectoru loci, const vectorf& fitness, const vectora& wildtype,
				const vectoru & subPops=vectoru(), int stage=PreMating, int begin=0, int end=-1, int step=1,
				vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(1, "fitness")):
			selector(subPops, stage, begin, end, step, at, rep, grp, infoFields),
				m_loci(loci), m_fitness(fitness), m_wildtype(wildtype)
			{
				DBG_ASSERT( m_fitness.size() == static_cast<UINT>(pow(static_cast<double>(3),
					static_cast<double>(loci.size()))),
					ValueError, "Please specify fitness for each combination of genotype.");
			};

			virtual ~maSelector()
			{
			}

            /// deep copy of a \c maSelector
			virtual baseOperator * clone() const
			{
				return new maSelector(*this);
			}

			/// calculate/return the fitness value, currently assuming diploid
			virtual double indFitness(individual * ind, ULONG gen);

            /// used by Python print function to print out the general information of the \c maSelector
			virtual string __repr__()
			{
				return "<simuPOP::selector::multiple-alleles selector>" ;
			}

		private:
			/// one locus
			vectoru m_loci;

			/// fitness for each genotype
			vectorf m_fitness;

			///
			vectora m_wildtype;
	};

    /// selection according to genotypes at multiple loci in a multiplicative model
	/**
    This selector is a 'multiple-loci model' selector. The selector takes a vector of
    selectors (can not be another \c mlSelector) and evaluate the fitness of an
    individual as the the product or sum of individual fitness values. The mode is
    determined by parameter \c mode, which takes the value
    \li \c SEL_Multiplicative: the fitness is calculated as \f$ f=\prod_{i}f_{i} \f$.
    \li \c SEL_Additive: the fitness is calculated as
        \f$ f=\max\left(0,1-\sum_{i}(1-f_{i})\right)=\max\left(0,1-\sum_{i}s_{i}\right) \f$.
        \f$ f \f$ will be set to \c 0 when \f$ f<0 \f$. In this case, \f$ s_{i} \f$ are added,
        not \f$ f_{i} \f$ directly. 

    <funcForm>MlSelect</funcForm>
	*/
	class mlSelector: public selector
	{
		public:

#define SEL_None 0
#define SEL_Multiplicative 1
#define SEL_Additive 2
#define SEL_Heterogeneity 3

			typedef std::vector< baseOperator * > vectorop;

		public:
            /// create a multi-loci selector
			/** 
			\param selectors a list of selectors

            Please refer to \c mapSelector for other parameter descriptions.        
			*/
			mlSelector( const vectorop selectors, int mode = SEL_Multiplicative,
				const vectoru & subPops=vectoru(), int stage=PreMating, int begin=0, int end=-1, int step=1,
				vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(1, "fitness")):
			selector(subPops, stage, begin, end, step, at, rep, grp, infoFields),
				m_selectors(0), m_mode(mode)
			{
				DBG_FAILIF( selectors.empty(), ValueError, "Please specify at least one selector.");
				for(vectorop::const_iterator s = selectors.begin(), sEnd=selectors.end(); s != sEnd; ++s)
				{
					DBG_ASSERT( (*s)->__repr__().substr(10,8) == "selector", ValueError,
						"Expecting a list of fitness calculator. Given " + (*s)->__repr__());
					m_selectors.push_back( (*s)->clone());
				}
			};

			virtual ~mlSelector()
			{
				for(vectorop::iterator s = m_selectors.begin(), sEnd=m_selectors.end(); s != sEnd; ++s)
					delete *s;
			}

            /// deep copy of a \c mlSelector
			virtual baseOperator * clone() const
			{
				throw ValueError("Multi-loci selector can not be nested.");
			}

			/// calculate/return the fitness value, currently assuming diploid
			virtual double indFitness(individual * ind, ULONG gen);

            /// used by Python print function to print out the general information of the \c mlSelector
			virtual string __repr__()
			{
				return "<simuPOP::selector::multiple-loci selector>" ;
			}

		private:
			/// a list of selectors
			vectorop m_selectors;

			/// mode
			int m_mode;
	};

    /// selection using user provided function
	/**
    \c pySelector assigns fitness values by calling a user provided function.
    It accepts a list of susceptibility loci and a Python function. For
    each individual, this operator will pass the genotypes at these loci (in the order
    of <tt>0-0,0-1,1-0,1-1</tt> etc. where X-Y represents locus X - ploidy Y, in the case
    of diploid population), generation number,??? and expect a returned fitness value.
    This, at least in theory, can accommodate all selection scenarios.

    <funcForm>PySelect</funcForm>    
	*/
	class pySelector: public selector
	{
		public:
            /// create a Python hybrid selector
			/**
            \param loci susceptibility loci. The genotype at these loci will be
                passed to \c func.
			\param func a Python function that accepts genotypes at susceptibility loci
				generation number, and return fitness value.???
			\param output and other parameters please refer to help(baseOperator.__init__)???
			*/
			// provide locus and fitness for 11, 12, 13 (in the form of dictionary)
			pySelector( vectoru loci, PyObject* func, const vectoru & subPops=vectoru(), 
				int stage=PreMating, int begin=0, int end=-1, int step=1,
				vectorl at=vectorl(), int rep=REP_ALL, int grp=GRP_ALL,
				const vectorstr& infoFields=vectorstr(1, "fitness")):
			selector(subPops, stage, begin, end, step, at, rep, grp, infoFields),
				m_loci(loci), m_alleles(0), m_len(0), m_numArray(NULL)
			{
				if( !PyCallable_Check(func))
					throw ValueError("Passed variable is not a callable python function.");

				Py_XINCREF(func);
				m_func = func;

				DBG_FAILIF( loci.empty(), ValueError,
					"Please specify susceptibility loci");
			};

			/// destructor
			virtual ~pySelector()
			{
				if( m_func != NULL)
					Py_DECREF(m_func);
			}

			/// CPPONLY
			pySelector(const pySelector& rhs):
			selector(rhs),
				m_loci(rhs.m_loci),
				m_func(rhs.m_func),
				m_alleles(rhs.m_alleles),
				m_len(rhs.m_len),
				m_numArray(NULL)
			{
				if( m_func != NULL)
					Py_INCREF(m_func);
			}

            /// deep copy of a \c pySelector
			virtual baseOperator * clone() const
			{
				return new pySelector(*this);
			}

			/// calculate/return the fitness value, currently assuming diploid
			virtual double indFitness(individual * ind, ULONG gen);

            /// used by Python print function to print out the general information of the \c pySelector
			virtual string __repr__()
			{
				return "<simuPOP::selector::python selector>" ;
			}

		private:

			/// susceptibility loci
			vectoru m_loci;

			/// user supplied python function
			PyObject* m_func;

			/// copy of alleles of each individual a time.
			vectora m_alleles;

			/// length of m_alleles
			int m_len;

			/// the object that passed to func
			PyObject * m_numArray;

	};
}
#endif
