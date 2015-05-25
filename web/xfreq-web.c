/*
 * xfreq-web.c by CyrIng
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

#define	_APPNAME "XFreq-Web"

#include "xfreq-types.h"
#include "xfreq-smbios.h"
#include "xfreq-api.h"
#include "xfreq-web.h"

static  char    Version[] = AutoDate;


uARG	A={
		.FD={.Shm=0, .SmBIOS=0}, .SHM=MAP_FAILED, .SmBIOS=MAP_FAILED,
		.LOOP=TRUE,
		.fSuspended=FALSE,
		.Options=
		{
			{"-B", "%s", NULL,	"(n/a) Transmit the SmBIOS tree (Bool) [0/1]"	},
			{"-l", "%s", NULL,	"(n/a) Log to a file (String)\n"		\
						"\t\t  argument is the log file path"		}
		}
	};

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

size_t jsonStringify(unsigned int what, char *jsonStr)
{
	if(A.fSuspended)
	{
		strcpy(jsonStr,
						"{"					\
						"\"Transmission\":"			\
							"{"				\
							"\"Suspended\":true"		\
							"}"				\
						"}");
	}
	else
	{
		sprintf(jsonStr,
						"{"					\
						"\"Transmission\":"			\
							"{"				\
							"\"What\":%u,"			\
							"\"Suspended\":false"		\
							"},"				\
						"\"P\":"				\
							"{", what);
		if(what & 0x00000001)
		{
			char localStr[64], Array[32]={0}, Boost[4];
			unsigned int i=0;
			for(i=0; i < 10; i++)  {
				sprintf(Boost, "%2u%c", A.SHM->P.Boost[i], (i < 9) ? ',':'\0');
				strcat(Array, Boost);
			}
			sprintf(localStr,		"\"ClockSpeed\":%f,"		\
							"\"CPU\":%u,"			\
							"\"OnLine\":%u,"		\
							"\"Boost\":[%s]",
							A.SHM->P.ClockSpeed,
							A.SHM->P.CPU,
							A.SHM->P.OnLine,
							Array);
			strcat(jsonStr, localStr);
		}
		if(what & 0x80000000)
		{
			char localStr[256];
			sprintf(localStr,		"\"Avg\":"			\
								"{"			\
								"\"Turbo\":%f,"		\
								"\"C0\":%f,"		\
								"\"C1\":%f,"		\
								"\"C3\":%f,"		\
								"\"C6\":%f,"		\
								"\"C7\":%f"		\
								"},"			\
							"\"Top\":%u,"			\
							"\"Hot\":%u,"			\
							"\"Cold\":%u,"			\
							"\"PerCore\":%u,"		\
							"\"ClockSrc\":%u,"		\
							"\"IdleTime\":%u",
								100.f * A.SHM->P.Avg.Turbo,
								100.f * A.SHM->P.Avg.C0,
								100.f * A.SHM->P.Avg.C1,
								100.f * A.SHM->P.Avg.C3,
								100.f * A.SHM->P.Avg.C6,
								100.f * A.SHM->P.Avg.C7,
							A.SHM->P.Top,
							A.SHM->P.Hot,
							A.SHM->P.Cold,
							A.SHM->P.PerCore,
							A.SHM->P.ClockSrc,
							A.SHM->P.IdleTime);
			strcat(jsonStr, localStr);
		}
		strcat(jsonStr,				"}");
		if(what & 0x80000000)
		{
			char *Core=malloc(1024), *Array=calloc(A.SHM->P.CPU, 1024);
			unsigned int i=0;
			for(i=0; i < A.SHM->P.CPU; i++)
			{
				sprintf(Core,	"{"					\
						"\"Cycles\":"				\
							"{"				\
							"\"INST\":[%llu,%llu],"		\
							"\"C0\":["			\
								"{"			\
								"\"UCC\":%llu,"		\
								"\"URC\":%llu"		\
								"},"			\
								"{"			\
								"\"UCC\":%llu,"		\
								"\"URC\":%llu"		\
								"}],"			\
							"\"C1\":[%llu,%llu],"		\
							"\"C3\":[%llu,%llu],"		\
							"\"C6\":[%llu,%llu],"		\
							"\"C7\":[%llu,%llu],"		\
							"\"TSC\":[%llu,%llu]"		\
							"},"				\
						"\"Delta\":"				\
							"{"				\
							"\"INST\":%llu,"		\
							"\"C0\":"			\
								"{"			\
								"\"UCC\":%llu,"		\
								"\"URC\":%llu"		\
								"},"			\
							"\"C1\":%llu,"			\
							"\"C3\":%llu,"			\
							"\"C6\":%llu,"			\
							"\"C7\":%llu,"			\
							"\"TSC\":%llu"			\
							"},"				\
						"\"IPS\":%f,"				\
						"\"IPC\":%f,"				\
						"\"CPI\":%f,"				\
						"\"State\":"				\
							"{"				\
							"\"Turbo\":%f,"			\
							"\"C0\":%f,"			\
							"\"C1\":%f,"			\
							"\"C3\":%f,"			\
							"\"C6\":%f,"			\
							"\"C7\":%f"			\
							"},"				\
						"\"RelativeRatio\":%f,"			\
						"\"RelativeFreq\":%f,"			\
						"\"TjMax\":"				\
							"{"				\
							"\"Target\":%u"			\
							"},"				\
						"\"ThermStat\":"			\
							"{"				\
							"\"DTS\":%u"			\
							"}"				\
						"%s",
						A.SHM->C[i].Cycles.INST[0], A.SHM->C[i].Cycles.INST[1],
						A.SHM->C[i].Cycles.C0[0].UCC, A.SHM->C[i].Cycles.C0[0].URC,
						A.SHM->C[i].Cycles.C0[1].UCC, A.SHM->C[i].Cycles.C0[1].URC,
						A.SHM->C[i].Cycles.C1[0], A.SHM->C[i].Cycles.C1[1],
						A.SHM->C[i].Cycles.C3[0], A.SHM->C[i].Cycles.C3[1],
						A.SHM->C[i].Cycles.C6[0], A.SHM->C[i].Cycles.C6[1],
						A.SHM->C[i].Cycles.C7[0], A.SHM->C[i].Cycles.C7[1],
						A.SHM->C[i].Cycles.TSC[0], A.SHM->C[i].Cycles.TSC[1],
						A.SHM->C[i].Delta.INST,
						A.SHM->C[i].Delta.C0.UCC, A.SHM->C[i].Delta.C0.URC,
						A.SHM->C[i].Delta.C1,
						A.SHM->C[i].Delta.C3,
						A.SHM->C[i].Delta.C6,
						A.SHM->C[i].Delta.C7,
						A.SHM->C[i].Delta.TSC,
						A.SHM->C[i].IPS,
						A.SHM->C[i].IPC,
						A.SHM->C[i].CPI,
						A.SHM->C[i].State.Turbo,
						A.SHM->C[i].State.C0,
						A.SHM->C[i].State.C1,
						A.SHM->C[i].State.C3,
						A.SHM->C[i].State.C6,
						A.SHM->C[i].State.C7,
						A.SHM->C[i].RelativeRatio,
						A.SHM->C[i].RelativeFreq,
						A.SHM->C[i].TjMax.Target,
						A.SHM->C[i].ThermStat.DTS,
						(i < (A.SHM->P.CPU - 1)) ? "},":"}");
				strcat(Array, Core);
			}
			strcat(jsonStr,		","					\
						"\"C\":"				\
							"[");
			strcat(jsonStr, Array);
			strcat(jsonStr,			"]");
			free(Array);
			free(Core);
		}
		strcat(jsonStr,			"}");
	}
	size_t fullLen=strlen(jsonStr);
	return(fullLen);
}


#include <libwebsockets.h>

#define	QUOTES '"'
#define	ROOTDIR "http"

typedef struct
{
	char *filePath;
	struct stat fileStat;
} SESSION_HTTP;

const char *MIME_type(const char *filePath)
{
	struct
	{
		const char *extension;
		const char *fullType;
	} MIME_db[]=
	{
		{".html", "text/html"},
		{".css",  "text/css"},
		{".ico",  "image/x-icon"},
		{".js",   "text/javascript"},
		{".png",  "image/png"},
		{NULL,    "text/plain"}
	}, *walker=NULL;
	for(walker=MIME_db; walker->extension != NULL; walker++)
		if(!strcmp(&filePath[strlen(filePath)-strlen(walker->extension)], walker->extension))
			break;
	return(walker->fullType);
}

static int callback_http(struct libwebsocket_context *ctx,
			 struct libwebsocket *wsi,
			 enum libwebsocket_callback_reasons reason,
			 void *user, void *in, size_t len)
{
	SESSION_HTTP *session=(SESSION_HTTP *) user;
	char *pIn=(char *)in;

	int rc=0;
	switch(reason)
	{
		case LWS_CALLBACK_HTTP:
			if(len > 0)
			{
				session->filePath=calloc(2048, 1);
				strcat(session->filePath, ROOTDIR);

				if((len == 1) && (pIn[0] == '/'))
					strcat(session->filePath, "/index.html");
				else
					strncat(session->filePath, pIn, len);

				if(!stat(session->filePath, &session->fileStat)
				&& (session->fileStat.st_size > 0)
				&& (S_ISREG(session->fileStat.st_mode)))
				{
					if(libwebsockets_serve_http_file(ctx, wsi,
									 session->filePath,
									 MIME_type(session->filePath),
									 NULL, 0) < 0)
						rc=-1;
					else
						rc=0;
				}
				else
				{
					libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_NOT_FOUND, NULL);
					rc=-1;
				}
				free(session->filePath);
			}
			else
			{
				libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_BAD_REQUEST, NULL);
				if(lws_http_transaction_completed(wsi))
					rc=-1;
			}
		break;
		case LWS_CALLBACK_HTTP_BODY:
		break;
		case LWS_CALLBACK_HTTP_BODY_COMPLETION:
			libwebsockets_return_http_status(ctx, wsi, HTTP_STATUS_OK, NULL);
		break;
		case LWS_CALLBACK_HTTP_WRITEABLE:
		break;
		case LWS_CALLBACK_CLOSED_HTTP:
		break;
		default:
		break;
	}
	return(rc);
}

typedef struct
{
	int prefixSum;
	int remainder;
	unsigned char *buffer;
} SESSION_JSON;

static int callback_json(struct libwebsocket_context *ctx,
				struct libwebsocket *wsi,
				enum libwebsocket_callback_reasons reason,
				void *user, void *in, size_t len)
{
	SESSION_JSON *session=(SESSION_JSON *) user;

	int rc=0;
	switch(reason)
	{
		case LWS_CALLBACK_ESTABLISHED:
		{
			char *jsonString=malloc(16384);
			session->remainder=jsonStringify(0x00000001, jsonString);
			session->prefixSum=0;
			session->buffer=malloc(	LWS_SEND_BUFFER_PRE_PADDING
						+ session->remainder
						+ LWS_SEND_BUFFER_POST_PADDING);
			strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
					jsonString,
					session->remainder);
			free(jsonString);
		}
		break;
		case LWS_CALLBACK_RECEIVE:
			if(len > 0)
			{
				char *jsonStr=calloc(len+1, 1);
				strncpy(jsonStr, in, len);
				const char jsonCmp[2][12+1]=
				{
					{QUOTES,'R','e','s','u','m','e','B','t','n',QUOTES,'\0'},
					{QUOTES,'S','u','s','p','e','n','d','B','t','n',QUOTES,'\0'}
				};
				int i=0;
				for(i=0; i < 2; i++)
					if(!strcmp(jsonStr, jsonCmp[i]))
					{
						A.fSuspended=i;
						break;
					}
				free(jsonStr);
			}
		break;
		case LWS_CALLBACK_SERVER_WRITEABLE:
		{
			int written;
			if(session->remainder > 0)
			{
				written=libwebsocket_write(wsi,
						&session->buffer[session->prefixSum + LWS_SEND_BUFFER_PRE_PADDING],
							session->remainder, LWS_WRITE_TEXT);
				if(written < 0) {
					rc=1;
					break;
				}
				else {
					session->prefixSum+=written;
					session->remainder-=written;
					if(session->remainder)
						libwebsocket_callback_on_writable(ctx, wsi);
				}
			}
			else
			{
				char *jsonString=malloc(16384);
				session->remainder=jsonStringify(0x80000000, jsonString);
				session->prefixSum=0;
				session->buffer=(unsigned char*) realloc(session->buffer,
									LWS_SEND_BUFFER_PRE_PADDING
									+ session->remainder
									+ LWS_SEND_BUFFER_POST_PADDING);
				strncpy((char *) &session->buffer[LWS_SEND_BUFFER_PRE_PADDING],
						jsonString,
						session->remainder);
				free(jsonString);
			}
		}
		break;
		case LWS_CALLBACK_CLOSED:
		{
			free(session->buffer);
			lwsl_notice("JSON DISC\n");
		}
		break;
		default:
		break;
	}
	return(rc);
}

static struct libwebsocket_protocols protocols[]=
{
	{"http-only", callback_http, sizeof(SESSION_HTTP)},
	{"json-lite", callback_json, sizeof(SESSION_JSON)},
	{NULL, NULL, 0}
};

// Verify the prerequisites & start the threads.
int main(int argc, char *argv[])
{
	uid_t	UID=geteuid();
	Bool32	ROOT=(UID == 0);	// Check root access.
	int	rc=0;

	if(ScanOptions(&A, argc, argv))
	{
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

			struct libwebsocket_context *context;
			struct lws_context_creation_info info;
			memset(&info, 0, sizeof(info));
			info.port=(argc == 2) ? atoi(argv[1]) : 8080;
			info.gid=-1;
			info.uid=-1;
			info.protocols=protocols;
			if((context=libwebsocket_create_context(&info)) != NULL)
			{
				while(A.LOOP)
				{
					const unsigned long int roomBit=(unsigned long long int) 1<<A.Room, roomCmp=~roomBit;
					if(atomic_load(&A.SHM->Sync.IF) & roomBit)
					{
						libwebsocket_callback_on_writable_all_protocol(&protocols[1]);
						atomic_fetch_and(&A.SHM->Sync.IF, roomCmp);
					}
					libwebsocket_service(context, 50);
				}
				libwebsocket_context_destroy(context);
			}

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
	}
	else
		rc=1;

	return(rc);
}
