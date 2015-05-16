/*
 * xfreq-web.h by CyrIng
 *
 * XFreq
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define	OPTIONS_COUNT	2
typedef struct
{
	char		*argument;
	char		*format;
	void		*pointer;
	char		*manual;
} OPTIONS;

typedef struct
{
	struct
	{
		int	Shm,
			SmBIOS;
	} FD;

	SHM_STRUCT	*SHM;
	SMBIOS_TREE	*SmBIOS;

	unsigned int	Room;

	Bool32		LOOP,
			fSuspended;
	OPTIONS		Options[OPTIONS_COUNT];
} uARG;
