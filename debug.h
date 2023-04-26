#ifndef __DEBUG_H
#define __DEBUG_H

#ifdef DEBUG
extern FILE *debugfp;

#define Debug(m)	do {\
			 if (debugfp) {\
				 fputs(m,debugfp);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug1(m,a1)	do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug2(m,a1,a2)	do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug3(m,a1,a2,a3)	do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2,a3);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug4(m,a1,a2,a3,a4)	do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2,a3,a4);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug5(m,a1,a2,a3,a4,a5) do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2,a3,a4,a5);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug6(m,a1,a2,a3,a4,a5,a6) do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2,a3,a4,a5,a6);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define Debug7(m,a1,a2,a3,a4,a5,a6,a7) do {\
			 if (debugfp) {\
				 fprintf(debugfp,m,a1,a2,a3,a4,a5,a6,a7);\
				 fputc('\n', debugfp);\
				 fflush(debugfp);\
			 }\
			} while (0)

#define ConstructTrace(m)   if (debugfp) fprintf(debugfp,"%X %s Constructed\n", this, m)
#define DestructTrace(m)    if (debugfp) fprintf(debugfp,"%X %s Destructed\n", this, m)

#else

#define Debug(m)			do { ; } while(0)
#define Debug1(m,a1)			do { ; } while(0)
#define Debug2(m,a1,a2)			do { ; } while(0)
#define Debug3(m,a1,a2,a3)		do { ; } while(0)
#define Debug4(m,a1,a2,a3,a4)		do { ; } while(0)
#define Debug5(m,a1,a2,a3,a4,a5)	do { ; } while(0)
#define Debug6(m,a1,a2,a3,a4,a5,a6)	do { ; } while(0)
#define Debug7(m,a1,a2,a3,a4,a5,a6,a7)	do { ; } while(0)
#define ConstructTrace(m)
#define DestructTrace(m)

#endif
#endif

