/*
 * xfreq-api.c by CyrIng
 *
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */


#include <errno.h>

#include "xfreq-smbios.h"
#include "xfreq-api.h"

unsigned int ROL32(unsigned int r32, unsigned short int m16)
{
	__asm__	volatile
	(
		"mov	%0, %%eax	\n\t"
		"movw	%1, %%cx	\n\t"
		"rol	%%cl,%%eax	\n\t"
		"mov	%%eax, %0	"
		: "=m" (r32), "=m" (m16)
                : "m" (r32)
	);
	return(r32);
}

unsigned int ROR32(unsigned int r32, unsigned short int m16)
{
	__asm__	volatile
	(
		"mov	%0, %%eax	\n\t"
		"movw	%1, %%cx	\n\t"
		"ror	%%cl, %%eax	\n\t"
		"mov	%%eax, %0	"
		: "=m" (r32), "=m" (m16)
                : "m" (r32)
	);
	return(r32);
}
/*
unsigned long long int BT64(unsigned long long int r64, unsigned long long int i64)
{
	unsigned long long int f64;
	__asm__	volatile
	(
		"mov	%1, %%rax	\n\t"
		"mov	%2, %%rdx	\n\t"
		"bt	%%rdx, %%rax	\n\t"
		"jc	One		\n\t"
		"mov	%%rax, 0x0 	\n\t"
		"jmp	Over		\n\t"
		"One:			\n\t"
		"mov	%%rax, 0x1	\n\t"
		"Over:			\n\t"
		"mov	%%rax, %0"
		: "=m" (f64)
                : "m" (r64), "m" (i64)
	);
	return(f64);
}
*/
void	abstimespec(useconds_t usec, struct timespec *tsec)
{
	tsec->tv_sec=usec / 1000000L;
	tsec->tv_nsec=(usec % 1000000L) * 1000;
}

int	addtimespec(struct timespec *asec, const struct timespec *tsec)
{
	int rc=0;
	if((rc=clock_gettime(CLOCK_REALTIME, asec)) != -1)
	{
		if((asec->tv_nsec += tsec->tv_nsec) >= 1000000000L)
		{
			asec->tv_nsec -= 1000000000L;
			asec->tv_sec += 1;
		}
		asec->tv_sec += tsec->tv_sec;

		return(0);
	}
	else
		return(errno);
}

void	Sync_Init(SYNCHRONIZATION *sync)
{
	XCHG_MAP XChange={.Map={.Addr=0, .Core=0, .Arg=0, .ID=ID_NULL}};
	atomic_init(&sync->IF, 0x0);
	atomic_init(&sync->Rooms, 0x1);
	atomic_init(&sync->Play, XChange.Map64);
	atomic_init(&sync->Data, 0);
}

void	Sync_Destroy(SYNCHRONIZATION *sync)
{
	atomic_store(&sync->IF, 0x0);
}

unsigned int Sync_Open(SYNCHRONIZATION *sync)
{
	unsigned int room;
	for(room=63; room > 0; room--)
	{
		const unsigned long long int roomBit=(unsigned long long int) 1<<room;

		if(!(atomic_load(&sync->Rooms) & roomBit))
		{
			atomic_fetch_or(&sync->Rooms, roomBit);
			break;
		}
	}
	return(room);
}

void	Sync_Close(unsigned int room, SYNCHRONIZATION *sync)
{
	const unsigned long long int roomCmp=(unsigned long long int) ~(1<<room);

	atomic_fetch_and(&sync->Rooms, roomCmp);
}

long int Sync_Wait(unsigned int room, SYNCHRONIZATION *sync, useconds_t idleTime)
{
	const unsigned long int roomBit=(unsigned long long int) 1<<room, roomCmp=~roomBit;
	useconds_t idleRemaining=idleTime;

	while(!(atomic_load(&sync->IF) & roomBit) && idleRemaining)
	{
		usleep(IDLE_BASE_USEC);
		idleRemaining--;
	}
	atomic_fetch_and(&sync->IF, roomCmp);
	return(idleRemaining);
}

void	Sync_Signal(unsigned int room, SYNCHRONIZATION *sync)
{
	atomic_fetch_or(&sync->IF, (!room) ? 0x1 : 0xfffffffffffffffe);
}

char *Smb_Find_String(struct STRUCTINFO *smb, int ID)
{
	struct STRING *pstr=NULL;

	if(smb != NULL)
	{
		pstr=smb->String;
		while((pstr != NULL) && (pstr->ID != ID))
			pstr=pstr->Link;
	}
	return((pstr != NULL) ? pstr->Buffer : "");
}
