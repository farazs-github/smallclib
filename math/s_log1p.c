/*******************************************************************************
 *
 * Copyright (C) 2014-2018 Wave Computing, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************/

/* From Newlib 2.0, remove NaN/Inf handling for tiny version
Modified constants loads, removed specific branch code which was
identical under IEEE754 arithmetic to the alternative branch. */

/* @(#)s_log1p.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice 
 * is preserved.
 * ====================================================
 */

/*
FUNCTION
<<log1p>>, <<log1pf>>---log of <<1 + <[x]>>>

INDEX
	log1p
INDEX
	log1pf

ANSI_SYNOPSIS
	#include <math.h>
	double log1p(double <[x]>);
	float log1pf(float <[x]>);

TRAD_SYNOPSIS
	#include <math.h>
	double log1p(<[x]>)
	double <[x]>;

	float log1pf(<[x]>)
	float <[x]>;

DESCRIPTION
<<log1p>> calculates 
@tex
$ln(1+x)$, 
@end tex
the natural logarithm of <<1+<[x]>>>.  You can use <<log1p>> rather
than `<<log(1+<[x]>)>>' for greater precision when <[x]> is very
small.

<<log1pf>> calculates the same thing, but accepts and returns
<<float>> values rather than <<double>>.

RETURNS
<<log1p>> returns a <<double>>, the natural log of <<1+<[x]>>>.
<<log1pf>> returns a <<float>>, the natural log of <<1+<[x]>>>.

PORTABILITY
Neither <<log1p>> nor <<log1pf>> is required by ANSI C or by the System V
Interface Definition (Issue 2).

*/

/* double log1p(double x)
 *
 * Method :                  
 *   1. Argument Reduction: find k and f such that 
 *			1+x = 2^k * (1+f), 
 *	   where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *      Note. If k=0, then f=x is exact. However, if k!=0, then f
 *	may not be representable exactly. In that case, a correction
 *	term is need. Let u=1+x rounded. Let c = (1+x)-u, then
 *	log(1+x) - log(u) ~ c/u. Thus, we proceed to compute log(u),
 *	and add back the correction term c/u.
 *	(Note: when x > 2**53, one can simply return log(x))
 *
 *   2. Approximation of log1p(f).
 *	Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *		 = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *	     	 = 2s + s*R
 *      We use a special Reme algorithm on [0,0.1716] to generate 
 * 	a polynomial of degree 14 to approximate R The maximum error 
 *	of this polynomial approximation is bounded by 2**-58.45. In
 *	other words,
 *		        2      4      6      8      10      12      14
 *	    R(z) ~ Lp1*s +Lp2*s +Lp3*s +Lp4*s +Lp5*s  +Lp6*s  +Lp7*s
 *  	(the values of Lp1 to Lp7 are listed in the program)
 *	and
 *	    |      2          14          |     -58.45
 *	    | Lp1*s +...+Lp7*s    -  R(z) | <= 2 
 *	    |                             |
 *	Note that 2s = f - s*f = f - hfsq + s*hfsq, where hfsq = f*f/2.
 *	In order to guarantee error in log below 1ulp, we compute log
 *	by
 *		log1p(f) = f - (hfsq - s*(hfsq+R)).
 *	
 *	3. Finally, log1p(x) = k*ln2 + log1p(f).  
 *		 	     = k*ln2_hi+(f-(hfsq-(s*(hfsq+R)+k*ln2_lo)))
 *	   Here ln2 is split into two floating point number: 
 *			ln2_hi + ln2_lo,
 *	   where n*ln2_hi is always exact for |n| < 2000.
 *
 * Special cases:
 *	log1p(x) is NaN with signal if x < -1 (including -INF) ; 
 *	log1p(+INF) is +INF; log1p(-1) is -INF with signal;
 *	log1p(NaN) is that NaN with no signal.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following 
 * constants. The decimal values may be used, provided that the 
 * compiler will convert from decimal to binary accurately enough 
 * to produce the hexadecimal values shown.
 *
 * Note: Assuming log() return accurate answer, the following
 * 	 algorithm can be used to compute log1p(x) to within a few ULP:
 *	
 *		u = 1+x;
 *		if(u==1.0) return x ; else
 *			   return log(u)*(x/(u-1.0));
 *
 *	 See HP-15C Advanced Functions Handbook, p.193.
 */

#include <errno.h>
#include "low/_math.h"
#include "low/_fpuinst.h"

#ifndef _DOUBLE_IS_32BITS

#ifdef __STDC__
static const double
#else
static double
#endif
ln2_hi  =  6.93147180369123816490e-01,	/* 3fe62e42 fee00000 */
ln2_lo  =  1.90821492927058770002e-10;	/* 3dea39ef 35793c76 */

#ifdef __MATH_FORCE_EXPAND_POLY__
static const double
Lp1 = 6.666666666666735130e-01,  /* 3FE55555 55555593 */
Lp2 = 3.999999999940941908e-01,  /* 3FD99999 9997FA04 */
Lp3 = 2.857142874366239149e-01,  /* 3FD24924 94229359 */
Lp4 = 2.222219843214978396e-01,  /* 3FCC71C5 1D8E78AF */
Lp5 = 1.818357216161805012e-01,  /* 3FC74664 96CB03DE */
Lp6 = 1.531383769920937332e-01,  /* 3FC39A09 D078C69F */
Lp7 = 1.479819860511658591e-01;  /* 3FC2F112 DF3E5244 */
#else /* __MATH_FORCE_EXPAND_POLY__ */
static const double Lp[] = {
6.666666666666735130e-01,  /* Lp1 = 3FE55555 55555593 */
3.999999999940941908e-01,  /* Lp2 = 3FD99999 9997FA04 */
2.857142874366239149e-01,  /* Lp3 = 3FD24924 94229359 */
2.222219843214978396e-01,  /* Lp4 = 3FCC71C5 1D8E78AF */
1.818357216161805012e-01,  /* Lp5 = 3FC74664 96CB03DE */
1.531383769920937332e-01  /* Lp6 = 3FC39A09 D078C69F */
},
Lp7 = 1.479819860511658591e-01;   /* Lp7 = 3FC2F112 DF3E5244 */

static double poly(double w, const double cf[], double retval)
{
	int i=5;
	while (i >= 0)
	    retval=w*retval+cf[i--];
	return retval;
};
#endif /* __MATH_FORCE_EXPAND_POLY__ */

#ifdef __STDC__
static const double zero = 0.0;
#else
static double zero = 0.0;
#endif

#ifdef __STDC__
	double log1p(double x)
#else
	double log1p(x)
	double x;
#endif
{
	double hfsq,f,c,s,z,R,u;
	__int32_t k,hx,hu,ax;
	double one,two,half;
#ifdef __MATH_CONST_FROM_MEMORY__
	one=1.0;
	half=0.5;
#else  /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W (one, 1);
	__inst_ldi_D_H (half,0x3fe00000);
#endif /* __MATH_CONST_FROM_MEMORY__ */

	GET_HIGH_WORD(hx,x);
	ax = hx&0x7fffffff;

	k = 1;
	if (hx < 0x3FDA827A) {			/* x < 0.41422  */
	    if(ax>=0x3ff00000) {		/* x <= -1.0 */
#ifdef __MATH_MATCH_x86__
	/* Required as per ISO-C99, Section 7.12.6.9, even though
	   neither glibc nor newlib bother about it */
		errno=EDOM;
#endif /* __MATH_MATCH_x86__ */
		if(x==-one) 
		    return x/zero; /* log1p(-1)=+inf */
		else 
		    return (x-x)/(x-x);	/* log1p(x<-1)=NaN */
	    }
	    if(ax<0x3e200000) {			/* |x| < 2**-29 */
#ifdef __MATH_MATCH_NEWLIB_EXCEPTION__
		double two54; 
#ifdef __MATH_CONST_FROM_MEMORY__
		two54   =  1.80143985094819840000e+16;  /* 43500000 00000000 */
#else /* __MATH_CONST_FROM_MEMORY__ */
		__inst_ldi_D_H (two54, 0x43500000);
#endif /* __MATH_CONST_FROM_MEMORY__ */		   
		if(two54+x>zero			/* raise inexact */
		   &&ax<0x3c900000) 		/* |x| < 2**-54 */
#else /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		if(ax<0x24800000) 		/* |x| < 2**-54 */
#endif /* __MATH_MATCH_NEWLIB_EXCEPTION__ */
		    return x;
		else
		    return x - x*x*half;
	    }
	    if(hx>0||hx<=((__int32_t)0xbfd2bec3)) {
		k=0;f=x;hu=1;}	/* -0.2929<x<0.41422 */
	}
#ifdef __MATH_NONFINITE__
	if (hx >= 0x7ff00000) return x+x;
#endif /* __MATH_NONFINITE__ */
	if(k!=0) {
	    if(hx<0x43400000) {
		u  = one+x; 
		GET_HIGH_WORD(hu,u);
	        k  = (hu>>20)-1023;
	        c  = (k>0)? one-(u-x):x-(u-one);/* correction term */
		c /= u;
	    } else {
		u  = x;
		GET_HIGH_WORD(hu,u);
	        k  = (hu>>20)-1023;
		c  = 0;
	    }
	    hu &= 0x000fffff;
	    if(hu<0x6a09e) {
	        SET_HIGH_WORD(u,hu|0x3ff00000);	/* normalize u */
	    } else {
	        k += 1; 
		SET_HIGH_WORD(u,hu|0x3fe00000);	/* normalize u/2 */
	        hu = (0x00100000-hu)>>2;
	    }
	    f = u-one;
	}
	else
	    c=0;
	hfsq=half*f*f;
	R = hfsq*(one-0.66666666666666666*f);
	if(hu==0) {	/* |f| < 2**-20 */
		return k*ln2_hi-((R-(k*ln2_lo+c))-f);
	}
#ifdef __MATH_CONST_FROM_MEMORY__
	two=2.0;
#else /* __MATH_CONST_FROM_MEMORY__ */
	__inst_ldi_D_W (two, 2);
#endif /* __MATH_CONST_FROM_MEMORY__ */
 	s = f/(two+f);

	z = s*s;
#ifdef __MATH_FORCE_EXPAND_POLY__
	R = z*(Lp1+z*(Lp2+z*(Lp3+z*(Lp4+z*(Lp5+z*(Lp6+z*Lp7))))));
#else /* __MATH_FORCE_EXPAND_POLY__ */
	R = z*poly(z,Lp,Lp7);
#endif /* __MATH_FORCE_EXPAND_POLY__ */
	return k*ln2_hi-((hfsq-(s*(hfsq+R)+(k*ln2_lo+c)))-f);
}

#endif /* _DOUBLE_IS_32BITS */
