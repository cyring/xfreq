/*
 * xfreq-cli.c by CyrIng
 *
 * XFreq
 * Copyright (C) 2013-2015 CYRIL INGENIERIE
 * Licenses: GPL2
 */

#define _GNU_SOURCE
#include <sched.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdatomic.h>

#if defined(FreeBSD)
#include <pthread_np.h>
#endif

#define	_APPNAME "XFreq-Client"

#include "xfreq-types.h"
#include "xfreq-smbios.h"
#include "xfreq-api.h"
#include "xfreq-cli.h"

static  char    Version[] = AutoDate;


void	Play(uARG *A, char ID)
{
	switch(ID)
	{
		case ID_QUIT:
			{
			A->LOOP=FALSE;
			}
			break;
	}
}

static void *uRead_Freq(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Read, "xfreq-cli-read");

	long int idleRemaining;
	while(A->LOOP)
		if((idleRemaining=Sync_Wait(A->Room, &A->SHM->Sync, IDLE_COEF_MAX + IDLE_COEF_DEF + IDLE_COEF_MIN)))
		{
			printf("\nCPU#  F=%6.2f x R  Temp   IPS   IPC   CPI\tTask scheduling\n", A->SHM->P.ClockSpeed);
			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
					printf(	"%3d  %7.2f %5.2f  %3d  %5.2f %5.2f %5.2f %15s(%5ld)\n",
						cpu,
						A->SHM->C[cpu].RelativeFreq,
						A->SHM->C[cpu].RelativeRatio,
						A->SHM->C[cpu].TjMax.Target - A->SHM->C[cpu].ThermStat.DTS,
						A->SHM->C[cpu].IPS,
						A->SHM->C[cpu].IPC,
						A->SHM->C[cpu].CPI,
						A->SHM->C[cpu].Task[0].comm, A->SHM->C[cpu].Task[0].pid);

			printf(	"\nAverage C-states\n" \
				"Turbo\t  C0\t  C1\t  C3\t  C6\t  C7\n" \
				"%6.2f%%\t%6.2f%%\t%6.2f%%\t%6.2f\t%6.2f%%\t%6.2f%%\n",
				100.f * A->SHM->P.Avg.Turbo,
				100.f * A->SHM->P.Avg.C0,
				100.f * A->SHM->P.Avg.C1,
				100.f * A->SHM->P.Avg.C3,
				100.f * A->SHM->P.Avg.C6,
				100.f * A->SHM->P.Avg.C7);
		}
		else
			Play(A, ID_QUIT);
	return(NULL);
}

static void *uRead_States(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Read, "xfreq-cli-read");

	long int idleRemaining;
	while(A->LOOP)
		if((idleRemaining=Sync_Wait(A->Room, &A->SHM->Sync, IDLE_COEF_MAX + IDLE_COEF_DEF + IDLE_COEF_MIN)))
		{
			printf("\nCPU#                 UCC                  URC                   C1                   C3                   C6                   C7                  TSC\n");

			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
					printf(	"%-3d %20lld %20lld %20lld %20lld %20lld %20lld %20lld\n",
						cpu,
						A->SHM->C[cpu].Delta.C0.UCC,
						A->SHM->C[cpu].Delta.C0.URC,
						A->SHM->C[cpu].Delta.C1,
						A->SHM->C[cpu].Delta.C3,
						A->SHM->C[cpu].Delta.C6,
						A->SHM->C[cpu].Delta.C7,
						A->SHM->C[cpu].Delta.TSC);
		}
		else
			Play(A, ID_QUIT);
	return(NULL);
}

static void *uRead_Inst(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_Read, "xfreq-cli-read");

	long int idleRemaining;
	while(A->LOOP)
		if((idleRemaining=Sync_Wait(A->Room, &A->SHM->Sync, IDLE_COEF_MAX + IDLE_COEF_DEF + IDLE_COEF_MIN)))
		{
			printf("\nCPU#                INST                  UCC                  URC                  TSC\n");

			int cpu=0;
			for(cpu=0; cpu < A->SHM->P.CPU; cpu++)
				if(A->SHM->C[cpu].T.Offline != TRUE)
					printf(	"%-3d %20lld %20lld %20lld %20lld\n",
						cpu,
						A->SHM->C[cpu].Delta.INST,
						A->SHM->C[cpu].Delta.C0.UCC,
						A->SHM->C[cpu].Delta.C0.URC,
						A->SHM->C[cpu].Delta.TSC);
		}
		else
			Play(A, ID_QUIT);
	return(NULL);
}

// Load settings
char	*FQN_Settings(const char *fName)
{
	char *Home=getenv("HOME");
	if(Home != NULL)
	{
		char	*lHome=strdup(Home),
			*rHome=strdup(Home),
			*dName=dirname(lHome),
			*bName=basename(rHome),
			*FQN=malloc(4096);

		if(!strcmp(dName, "/"))
			sprintf(FQN, "%s%s/%s", dName, bName, fName);
		else
			sprintf(FQN, "%s/%s/%s", dName, bName, fName);

		free(lHome);
		free(rHome);

		return(FQN);
	}
	return(NULL);
}

int	ScanOptions(uARG *A, int argc, char *argv[])
{
	int i=0, j=0;
	Bool32 noerr=TRUE;

	//  Parse the command line options which may override settings.
	if( (argc - ((argc >> 1) << 1)) )
	{
		for(j=1; j < argc; j+=2)
		{
			for(i=0; i < OPTIONS_COUNT; i++)
				if(A->Options[i].pointer && !strcmp(argv[j], A->Options[i].argument))
				{
					sscanf(argv[j+1], A->Options[i].format, A->Options[i].pointer);
					break;
				}
			if(i == OPTIONS_COUNT)
			{
				noerr=FALSE;
				break;
			}
		}
	}
	else
		noerr=FALSE;

	if(noerr == FALSE)
	{
		char	*program=strdup(argv[0]),
			*progName=basename(program);

		if(!strcmp(argv[1], "-h"))
		{
			printf(	_APPNAME" ""usage:\n\t%s [-option argument] .. [-option argument]\n\nwhere options include:\n", progName);

			for(i=0; i < OPTIONS_COUNT; i++)
				printf("\t%s\t%s\n", A->Options[i].argument, A->Options[i].manual);

			printf(	"\t-v\tPrint version information\n" \
				"\t-h\tPrint out this message\n" \
				"\nExit status:\n0\tif OK,\n1\tif problems,\n2\tif serious trouble.\n" \
				"\nReport bugs to xfreq[at]cyring.fr\n");
		}
		else if(!strcmp(argv[1], "-v"))
			printf(Version);
		else
		{
			if(j > 0)
				printf("%s: unrecognized option '%s'\nTry '%s -h' for more information.\n", progName, argv[j], progName);
			else
				printf("%s: malformed options.\nTry '%s -h' for more information.\n", progName, progName);
		}
		free(program);
	}
	return(noerr);
}

static void *uEmergency(void *uArg)
{
	uARG *A=(uARG *) uArg;
	pthread_setname_np(A->TID_SigHandler, "xfreq-cli-kill");

	int caught=0;
	while(A->LOOP && !sigwait(&A->Signal, &caught))
		switch(caught)
		{
			case SIGINT:
			case SIGQUIT:
			case SIGUSR1:
			case SIGTERM:
			{
				char str[sizeof(SIG_EMERGENCY_FMT)];
				sprintf(str, SIG_EMERGENCY_FMT, caught);
				tracerr(str);
				Play(A, ID_QUIT);
			}
				break;
			case SIGCONT:
			{
			}
				break;
			case SIGUSR2:
//			case SIGTSTP:
			{
			}
		}
	return(NULL);
}

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[])
{
	pthread_setname_np(pthread_self(), "xfreq-cli-main");

	uARG	A={
			.FD={.Shm=0, .SmBIOS=0}, .SHM=MAP_FAILED, .SmBIOS=MAP_FAILED,
			.TID_SigHandler=0,
			.TID_Read=0,
			.iFunc=0,
			.uFunc=uRead_Freq,
			.LOOP=TRUE,
			.Options=
			{
				{"-f", "%u", &A.iFunc,	"Function thread (Int)\n"						\
							"\t\t  argument is one of the [0]Freq [1]States [2]Inst"		},
				{"-B", "%s", NULL,	"(n/a) Print the SmBIOS tree (Bool) [0/1]"					},
				{"-l", "%s", NULL,	"(n/a) Log to a file (String)\n"						\
							"\t\t  argument is the log file path"					},
			}
		};
	uid_t	UID=geteuid();
	Bool32	ROOT=(UID == 0),	// Check root access.
		fEmergencyThread=FALSE;
	int	rc=0;

	if(ScanOptions(&A, argc, argv))
	{
		switch(A.iFunc)
		{
			case 2:
				A.uFunc=uRead_Inst;
			break;
			case 1:
				A.uFunc=uRead_States;
			break;
			case 0:
			default:
				A.uFunc=uRead_Freq;
			break;
		}
		sigemptyset(&A.Signal);
		sigaddset(&A.Signal, SIGINT);	// [CTRL] + [C]
		sigaddset(&A.Signal, SIGQUIT);
		sigaddset(&A.Signal, SIGUSR1);
		sigaddset(&A.Signal, SIGUSR2);
		sigaddset(&A.Signal, SIGTERM);
		sigaddset(&A.Signal, SIGCONT);
//		sigaddset(&A.Signal, SIGTSTP);	// [CTRL] + [Z]

		if(!pthread_sigmask(SIG_BLOCK, &A.Signal, NULL)
		&& !pthread_create(&A.TID_SigHandler, NULL, uEmergency, &A))
			fEmergencyThread=TRUE;
		else
			tracerr("Remark: cannot start the signal handler");

		if(ROOT)
			tracerr("Warning: running as root");

		struct stat shmStat={0}, smbStat={0};
		if(((A.FD.Shm=shm_open(SHM_FILENAME, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) != -1)
		&& ((fstat(A.FD.Shm, &shmStat) != -1)
		&& ((A.SHM=mmap(0, shmStat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, A.FD.Shm, 0)) != MAP_FAILED))
		&& ((A.Room=Sync_Open(&A.SHM->Sync)) != 0))
		{
			if(((A.FD.SmBIOS=shm_open(SMB_FILENAME, O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH)) == -1)
			|| ((fstat(A.FD.Shm, &smbStat) == -1))
			|| ((A.SmBIOS=mmap(A.SHM->B, smbStat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_FIXED, A.FD.SmBIOS, 0)) == MAP_FAILED))
				tracerr("Error: opening the SmBIOS shared memory");
#if defined(DEBUG)
					printf(	"\n--- SHM Map ---\n"
						"A.SHM[%p]\n" \
						"A.SHM->P[%p]\n" \
						"A.SHM->H[%p]\n" \
						"A.SHM->D[%p]\n" \
						"A.SHM->M[%p]\n" \
						"A.SHM->S[%p]\n" \
						"A.SHM->B[%p]\n" \
						"A.SHM->C[%p]\n", \
						A.SHM, \
						&A.SHM->P, \
						&A.SHM->H, \
						&A.SHM->D, \
						&A.SHM->M, \
						&A.SHM->S, \
						A.SHM->B, \
						&A.SHM->C);
#endif
			if(!pthread_create(&A.TID_Read, NULL, A.uFunc, &A))
			{
				printf("\n%s Ready with [%s]\n\n", _APPNAME, A.SHM->AppName);

				while(A.LOOP)
				{
					usleep(IDLE_BASE_USEC * A.SHM->P.IdleTime);
				}

				// shutting down
				struct timespec absoluteTime={.tv_sec=0, .tv_nsec=0}, gracePeriod={.tv_sec=0, .tv_nsec=0};
				abstimespec(IDLE_BASE_USEC * IDLE_COEF_MAX, &absoluteTime);
				if(!addtimespec(&gracePeriod, &absoluteTime)
				&& pthread_timedjoin_np(A.TID_Read, NULL, &gracePeriod))
				{
					rc=pthread_kill(A.TID_Read, SIGKILL);
					rc=pthread_join(A.TID_Read, NULL);
				}
			}
			else
			{
				tracerr("Error: cannot start the thread uRead()");
				rc=2;
			}
			if((A.SmBIOS != MAP_FAILED) && (munmap(A.SmBIOS, smbStat.st_size) == -1))
				tracerr("Error: unmapping the SmBIOS shared memory");
		}
		else
		{
			tracerr("Error: opening the shared memory");
			rc=2;
		}
		// Release the ressources.
		if(A.Room)
			Sync_Close(A.Room, &A.SHM->Sync);
		if((A.SHM != MAP_FAILED) && (munmap(A.SHM, shmStat.st_size) == -1))
			tracerr("Error: unmapping the shared memory");
		if((A.FD.Shm != -1) && (close(A.FD.Shm) == -1))
			tracerr("Error: closing the shared memory");

		if(fEmergencyThread)
		{
			pthread_kill(A.TID_SigHandler, SIGUSR1);
			pthread_join(A.TID_SigHandler, NULL);
		}
	}
	else
		rc=1;

	return(rc);
}
