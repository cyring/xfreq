/*
 * xfreq-types.h by CyrIng
 *
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#define	TRUE	1
#define	FALSE	0

typedef unsigned long long int	Bool64;
typedef unsigned int		Bool32;

#define	ToStr(_inst)	_ToStr(_inst)
#define	_ToStr(_inst)	#_inst

#if defined(DEBUG)
	#define	tracerr(anystr)	fprintf(stderr, "%s\n", anystr);
#else
	#define	tracerr(anystr)
#endif

#if defined(Linux)
#define MAX(M, m)	((M) > (m) ? (M) : (m))
#define MIN(m, M)	((m) < (M) ? (m) : (M))
#endif
