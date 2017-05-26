/* Copyright (c) 2005-2017 by Ju Haibo (octerboy@gmail.com)
 * All rights reserved.
 *
 * This file is part of the TextUS.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
/*
 * ora_api.h
 *
 * Oracle Functions
 */
#include "oradefs.h"
/******************************************************************************/
/*-----------------------Dynamic Callback Function Pointers------------------*/
 
 
typedef sb4 (*OCICallbackInBind)(dvoid *ictxp, OCIBind *bindp, ub4 iter,
                                  ub4 index, dvoid **bufpp, ub4 *alenp,
                                  ub1 *piecep, dvoid **indp);
 
typedef sb4 (*OCICallbackOutBind)(dvoid *octxp, OCIBind *bindp, ub4 iter,
                                 ub4 index, dvoid **bufpp, ub4 **alenp,
                                 ub1 *piecep, dvoid **indp,
                                 ub2 **rcodep);
 
typedef sb4 (*OCICallbackDefine)(dvoid *octxp, OCIDefine *defnp, ub4 iter,
                                 dvoid **bufpp, ub4 **alenp, ub1 *piecep,
                                 dvoid **indp, ub2 **rcodep);

typedef sword (*OCIUserCallback)(dvoid *ctxp, dvoid *hndlp, ub4 type,
                                 ub4 fcode, ub4 when, sword returnCode,
                                 sb4 *errnop, va_list arglist);

typedef sword (*OCIEnvCallbackType)(OCIEnv *env, ub4 mode,
                                   size_t xtramem_sz, dvoid *usrmemp,
                                   OCIUcb *ucbDesc); 

typedef sb4 (*OCICallbackLobRead)(dvoid *ctxp, CONST dvoid *bufp,
                                             ub4 len, ub1 piece);

typedef sb4 (*OCICallbackLobWrite)(dvoid *ctxp, dvoid *bufp, 
                                          ub4 *lenp, ub1 *piece);

#ifdef ORAXB8_DEFINED

typedef sb4 (*OCICallbackLobRead2)(dvoid *ctxp, CONST dvoid *bufp, oraub8 len,
                                   ub1 piece, dvoid **changed_bufpp,
                                   oraub8 *changed_lenp);

typedef sb4 (*OCICallbackLobWrite2)(dvoid *ctxp, dvoid *bufp, oraub8 *lenp,
                                    ub1 *piece, dvoid **changed_bufpp,
                                    oraub8 *changed_lenp);

typedef sb4 (*OCICallbackLobArrayRead)(dvoid *ctxp, ub4 array_iter,
                                       CONST dvoid *bufp, oraub8 len,
                                       ub1 piece, dvoid **changed_bufpp,
                                       oraub8 *changed_lenp);

typedef sb4 (*OCICallbackLobArrayWrite)(dvoid *ctxp, ub4 array_iter,
                                        dvoid *bufp, oraub8 *lenp,
                                        ub1 *piece, dvoid **changed_bufpp,
                                        oraub8 *changed_lenp);

#endif

typedef sb4 (*OCICallbackAQEnq)(dvoid *ctxp, dvoid **payload, 
                                dvoid **payload_ind);

typedef sb4 (*OCICallbackAQDeq)(dvoid *ctxp, dvoid **payload, 
                                dvoid **payload_ind);

/*--------------------------Failover Callback Structure ---------------------*/
typedef sb4 (*OCICallbackFailover)(dvoid *svcctx, dvoid *envctx,
                                   dvoid *fo_ctx, ub4 fo_type,
                                   ub4 fo_event);

typedef struct
{
  OCICallbackFailover callback_function;
  dvoid *fo_ctx;
} 
OCIFocbkStruct;

/*--------------------------HA Callback Structure ---------------------*/
typedef void (*OCIEventCallback)(dvoid *evtctx, OCIEvent *eventhp);


/*****************************************************************************
                         ACTUAL PROTOTYPE DECLARATIONS
******************************************************************************/

typedef sword (*OCIINITIALIZE)   (ub4 mode, dvoid *ctxp, 
	                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
	                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
	                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr) );
OCIINITIALIZE	OCIInitialize;


typedef sword (*OCITERMINATE)( ub4 mode);
OCITERMINATE	OCITerminate;

typedef sword (*OCIENVCREATE) (OCIEnv **envp, ub4 mode, dvoid *ctxp,
	                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
	                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
	                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
	                 size_t xtramem_sz, dvoid **usrmempp);
OCIENVCREATE	OCIEnvCreate;


typedef sword (*OCIENVNLSCREATE) (OCIEnv **envp, ub4 mode, dvoid *ctxp,
	                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
	                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
	                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
	                 size_t xtramem_sz, dvoid **usrmempp,
	                 ub2 charset, ub2 ncharset);
OCIENVNLSCREATE	OCIEnvNlsCreate;


typedef sword (*OCINLSENVIRONMENTVARIABLEGET)(dvoid  *valp, size_t size, ub2 item,
	                                   ub2 charset, size_t *rsize);
OCINLSENVIRONMENTVARIABLEGET	OCINlsEnvironmentVariableGet;


typedef sword (*OCIFENVCREATE) (OCIEnv **envp, ub4 mode, dvoid *ctxp,
	                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
	                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
	                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
	                 size_t xtramem_sz, dvoid **usrmempp, dvoid *fupg);
OCIFENVCREATE	OCIFEnvCreate;


typedef sword (*OCIHANDLEALLOC)(CONST dvoid *parenth, dvoid **hndlpp, CONST ub4 type, 
	                       CONST size_t xtramem_sz, dvoid **usrmempp);
OCIHANDLEALLOC	OCIHandleAlloc;


typedef sword (*OCIHANDLEFREE)(dvoid *hndlp, CONST ub4 type);
OCIHANDLEFREE	OCIHandleFree;


typedef sword (*OCIDESCRIPTORALLOC)(CONST dvoid *parenth, dvoid **descpp, 
	                           CONST ub4 type, CONST size_t xtramem_sz, 
	                           dvoid **usrmempp);
OCIDESCRIPTORALLOC	OCIDescriptorAlloc;


typedef sword (*OCIDESCRIPTORFREE)(dvoid *descp, CONST ub4 type);
OCIDESCRIPTORFREE	OCIDescriptorFree;

typedef sword (*OCIENVINIT) (OCIEnv **envp, ub4 mode, 
	                    size_t xtramem_sz, dvoid **usrmempp);
OCIENVINIT	OCIEnvInit;


typedef sword (*OCISERVERATTACH)  (OCIServer *srvhp, OCIError *errhp,
	                          CONST OraText *dblink, sb4 dblink_len, ub4 mode);
OCISERVERATTACH	OCIServerAttach;


typedef sword (*OCISERVERDETACH)  (OCIServer *srvhp, OCIError *errhp, ub4 mode);
OCISERVERDETACH	OCIServerDetach;

typedef sword (*OCISESSIONBEGIN)  (OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp,
	                          ub4 credt, ub4 mode);
OCISESSIONBEGIN	OCISessionBegin;


typedef sword (*OCISESSIONEND)   (OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp, 
	                         ub4 mode);
OCISESSIONEND	OCISessionEnd;


typedef sword (*OCILOGON) (OCIEnv *envhp, OCIError *errhp, OCISvcCtx **svchp, 
	                  CONST OraText *username, ub4 uname_len, 
	                  CONST OraText *password, ub4 passwd_len, 
	                  CONST OraText *dbname, ub4 dbname_len);
OCILOGON	OCILogon;


typedef sword (*OCILOGON2) (OCIEnv *envhp, OCIError *errhp, OCISvcCtx **svchp,
	                  CONST OraText *username, ub4 uname_len,
	                  CONST OraText *password, ub4 passwd_len,
	                  CONST OraText *dbname, ub4 dbname_len,
	                  ub4 mode);
OCILOGON2	OCILogon2;


typedef sword (*OCILOGOFF) (OCISvcCtx *svchp, OCIError *errhp);
OCILOGOFF	OCILogoff;


typedef sword (*OCIPASSWORDCHANGE)   (OCISvcCtx *svchp, OCIError *errhp, 
	                             CONST OraText *user_name, ub4 usernm_len, 
	                             CONST OraText *opasswd, ub4 opasswd_len, 
	                             CONST OraText *npasswd, ub4 npasswd_len, 
	                             ub4 mode);
OCIPASSWORDCHANGE	OCIPasswordChange;


typedef sword (*OCISTMTPREPARE)   (OCIStmt *stmtp, OCIError *errhp, CONST OraText *stmt,
	                          ub4 stmt_len, ub4 language, ub4 mode);
OCISTMTPREPARE	OCIStmtPrepare;


typedef sword (*OCISTMTPREPARE2) ( OCISvcCtx *svchp, OCIStmt **stmtp, OCIError *errhp,
	                     CONST OraText *stmt, ub4 stmt_len, CONST OraText *key,
	                     ub4 key_len, ub4 language, ub4 mode);
OCISTMTPREPARE2	OCIStmtPrepare2;


typedef sword (*OCISTMTRELEASE) ( OCIStmt *stmtp, OCIError *errhp, CONST OraText *key,
	                       ub4 key_len, ub4 mode);
OCISTMTRELEASE	OCIStmtRelease;


typedef sword (*OCIBINDBYPOS)  (OCIStmt *stmtp, OCIBind **bindp, OCIError *errhp,
	                       ub4 position, dvoid *valuep, sb4 value_sz,
	                       ub2 dty, dvoid *indp, ub2 *alenp, ub2 *rcodep,
	                       ub4 maxarr_len, ub4 *curelep, ub4 mode);
OCIBINDBYPOS	OCIBindByPos;


typedef sword (*OCIBINDBYNAME)   (OCIStmt *stmtp, OCIBind **bindp, OCIError *errhp,
	                         CONST OraText *placeholder, sb4 placeh_len, 
	                         dvoid *valuep, sb4 value_sz, ub2 dty, 
	                         dvoid *indp, ub2 *alenp, ub2 *rcodep, 
	                         ub4 maxarr_len, ub4 *curelep, ub4 mode);
OCIBINDBYNAME	OCIBindByName;


typedef sword (*OCIBINDOBJECT)  (OCIBind *bindp, OCIError *errhp, CONST OCIType *type, 
	                        dvoid **pgvpp, ub4 *pvszsp, dvoid **indpp, 
	                        ub4 *indszp);
OCIBINDOBJECT	OCIBindObject;


typedef sword (*OCIBINDDYNAMIC)   (OCIBind *bindp, OCIError *errhp, dvoid *ictxp,
	                          OCICallbackInBind icbfp, dvoid *octxp,
	                          OCICallbackOutBind ocbfp);
OCIBINDDYNAMIC	OCIBindDynamic;


typedef sword (*OCIBINDARRAYOFSTRUCT)   (OCIBind *bindp, OCIError *errhp, 
	                                ub4 pvskip, ub4 indskip,
	                                ub4 alskip, ub4 rcskip);
OCIBINDARRAYOFSTRUCT	OCIBindArrayOfStruct;


typedef sword (*OCISTMTGETPIECEINFO)   (OCIStmt *stmtp, OCIError *errhp, 
	                               dvoid **hndlpp, ub4 *typep,
	                               ub1 *in_outp, ub4 *iterp, ub4 *idxp, 
	                               ub1 *piecep);
OCISTMTGETPIECEINFO	OCIStmtGetPieceInfo;


typedef sword (*OCISTMTSETPIECEINFO)   (dvoid *hndlp, ub4 type, OCIError *errhp, 
	                               CONST dvoid *bufp, ub4 *alenp, ub1 piece, 
	                               CONST dvoid *indp, ub2 *rcodep);
OCISTMTSETPIECEINFO	OCIStmtSetPieceInfo;


typedef sword (*OCISTMTEXECUTE)  (OCISvcCtx *svchp, OCIStmt *stmtp, OCIError *errhp, 
	                         ub4 iters, ub4 rowoff, CONST OCISnapshot *snap_in, 
	                         OCISnapshot *snap_out, ub4 mode);
OCISTMTEXECUTE	OCIStmtExecute;


typedef sword (*OCIDEFINEBYPOS)  (OCIStmt *stmtp, OCIDefine **defnp, OCIError *errhp,
	                         ub4 position, dvoid *valuep, sb4 value_sz, ub2 dty,
	                         dvoid *indp, ub2 *rlenp, ub2 *rcodep, ub4 mode);
OCIDEFINEBYPOS	OCIDefineByPos;


typedef sword (*OCIDEFINEOBJECT)  (OCIDefine *defnp, OCIError *errhp, 
	                          CONST OCIType *type, dvoid **pgvpp, 
	                          ub4 *pvszsp, dvoid **indpp, ub4 *indszp);
OCIDEFINEOBJECT	OCIDefineObject;


typedef sword (*OCIDEFINEDYNAMIC)   (OCIDefine *defnp, OCIError *errhp, dvoid *octxp,
	                            OCICallbackDefine ocbfp);
OCIDEFINEDYNAMIC	OCIDefineDynamic;


typedef sword (*OCIROWIDTOCHAR)  (OCIRowid *rowidDesc, OraText *outbfp, ub2 *outbflp,
	                         OCIError *errhp);
OCIROWIDTOCHAR	OCIRowidToChar;


typedef sword (*OCIDEFINEARRAYOFSTRUCT)  (OCIDefine *defnp, OCIError *errhp, ub4 pvskip,
	                                 ub4 indskip, ub4 rlskip, ub4 rcskip);
OCIDEFINEARRAYOFSTRUCT	OCIDefineArrayOfStruct;


typedef sword (*OCISTMTFETCH)   (OCIStmt *stmtp, OCIError *errhp, ub4 nrows, 
	                        ub2 orientation, ub4 mode);
OCISTMTFETCH	OCIStmtFetch;


typedef sword (*OCISTMTFETCH2)   (OCIStmt *stmtp, OCIError *errhp, ub4 nrows, 
	                        ub2 orientation, sb4 scrollOffset, ub4 mode);
OCISTMTFETCH2	OCIStmtFetch2;


typedef sword (*OCISTMTGETBINDINFO)   (OCIStmt *stmtp, OCIError *errhp, ub4 size, 
	                              ub4 startloc,
	                              sb4 *found, OraText *bvnp[], ub1 bvnl[],
	                              OraText *invp[], ub1 inpl[], ub1 dupl[],
	                              OCIBind **hndl);
OCISTMTGETBINDINFO	OCIStmtGetBindInfo;


typedef sword (*OCIDESCRIBEANY)  (OCISvcCtx *svchp, OCIError *errhp, 
	                         dvoid *objptr, 
	                         ub4 objnm_len, ub1 objptr_typ, ub1 info_level,
	                         ub1 objtyp, OCIDescribe *dschp);
OCIDESCRIBEANY	OCIDescribeAny;


typedef sword (*OCIPARAMGET) (CONST dvoid *hndlp, ub4 htype, OCIError *errhp, 
	                     dvoid **parmdpp, ub4 pos);
OCIPARAMGET	OCIParamGet;


typedef sword (*OCIPARAMSET)(dvoid *hdlp, ub4 htyp, OCIError *errhp, CONST dvoid *dscp,
	                    ub4 dtyp, ub4 pos);
OCIPARAMSET	OCIParamSet;


typedef sword (*OCITRANSSTART)  (OCISvcCtx *svchp, OCIError *errhp, 
	                        uword timeout, ub4 flags );
OCITRANSSTART	OCITransStart;


typedef sword (*OCITRANSDETACH)  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags );
OCITRANSDETACH	OCITransDetach;

typedef sword (*OCITRANSCOMMIT)  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
OCITRANSCOMMIT	OCITransCommit;

typedef sword (*OCITRANSROLLBACK)  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
OCITRANSROLLBACK	OCITransRollback;

typedef sword (*OCITRANSPREPARE) (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
OCITRANSPREPARE	OCITransPrepare;

typedef sword (*OCITRANSMULTIPREPARE) (OCISvcCtx *svchp, ub4 numBranches, 
	                              OCITrans **txns, OCIError **errhp);
OCITRANSMULTIPREPARE	OCITransMultiPrepare;


typedef sword (*OCITRANSFORGET) (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);
OCITRANSFORGET	OCITransForget;

typedef sword (*OCIERRORGET)   (dvoid *hndlp, ub4 recordno, OraText *sqlstate,
	                       sb4 *errcodep, OraText *bufp, ub4 bufsiz, ub4 type);
OCIERRORGET	OCIErrorGet;


typedef sword (*OCILOBAPPEND)  (OCISvcCtx *svchp, OCIError *errhp, 
	                       OCILobLocator *dst_locp,
	                       OCILobLocator *src_locp);
OCILOBAPPEND	OCILobAppend;


typedef sword (*OCILOBASSIGN) (OCIEnv *envhp, OCIError *errhp, 
	                      CONST OCILobLocator *src_locp, 
	                      OCILobLocator **dst_locpp);
OCILOBASSIGN	OCILobAssign;


typedef sword (*OCILOBCHARSETFORM) (OCIEnv *envhp, OCIError *errhp, 
	                           CONST OCILobLocator *locp, ub1 *csfrm);
OCILOBCHARSETFORM	OCILobCharSetForm;


typedef sword (*OCILOBCHARSETID) (OCIEnv *envhp, OCIError *errhp, 
	                         CONST OCILobLocator *locp, ub2 *csid);
OCILOBCHARSETID	OCILobCharSetId;


typedef sword (*OCILOBCOPY) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *dst_locp,
	                    OCILobLocator *src_locp, ub4 amount, ub4 dst_offset, 
	                    ub4 src_offset);
OCILOBCOPY	OCILobCopy;


typedef sword (*OCILOBCREATETEMPORARY)(OCISvcCtx          *svchp,
	                            OCIError           *errhp,
	                            OCILobLocator      *locp,
	                            ub2                 csid,
	                            ub1                 csfrm,
	                            ub1                 lobtype,
	                            dboolean             cache,
	                            OCIDuration         duration);
OCILOBCREATETEMPORARY	OCILobCreateTemporary;



typedef sword (*OCILOBCLOSE)( OCISvcCtx        *svchp,
	                   OCIError         *errhp,
	                   OCILobLocator    *locp );
OCILOBCLOSE	OCILobClose;



typedef sword (*OCILOBDISABLEBUFFERING) (OCISvcCtx      *svchp,
	                                OCIError       *errhp,
	                                OCILobLocator  *locp);
OCILOBDISABLEBUFFERING	OCILobDisableBuffering;


typedef sword (*OCILOBENABLEBUFFERING) (OCISvcCtx      *svchp,
	                               OCIError       *errhp,
	                               OCILobLocator  *locp);
OCILOBENABLEBUFFERING	OCILobEnableBuffering;


typedef sword (*OCILOBERASE) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                      ub4 *amount, ub4 offset);
OCILOBERASE	OCILobErase;


typedef sword (*OCILOBFILECLOSE) (OCISvcCtx *svchp, OCIError *errhp, 
	                         OCILobLocator *filep);
OCILOBFILECLOSE	OCILobFileClose;


typedef sword (*OCILOBFILECLOSEALL) (OCISvcCtx *svchp, OCIError *errhp);
OCILOBFILECLOSEALL	OCILobFileCloseAll;

typedef sword (*OCILOBFILEEXISTS) (OCISvcCtx *svchp, OCIError *errhp, 
	                          OCILobLocator *filep,
	                          dboolean *flag);
OCILOBFILEEXISTS	OCILobFileExists;


typedef sword (*OCILOBFILEGETNAME) (OCIEnv *envhp, OCIError *errhp, 
	                           CONST OCILobLocator *filep, 
	                           OraText *dir_alias, ub2 *d_length, 
	                           OraText *filename, ub2 *f_length);
OCILOBFILEGETNAME	OCILobFileGetName;


typedef sword (*OCILOBFILEISOPEN) (OCISvcCtx *svchp, OCIError *errhp, 
	                          OCILobLocator *filep,
	                          dboolean *flag);
OCILOBFILEISOPEN	OCILobFileIsOpen;


typedef sword (*OCILOBFILEOPEN) (OCISvcCtx *svchp, OCIError *errhp, 
	                        OCILobLocator *filep,
	                        ub1 mode);
OCILOBFILEOPEN	OCILobFileOpen;


typedef sword (*OCILOBFILESETNAME) (OCIEnv *envhp, OCIError *errhp, 
	                           OCILobLocator **filepp, 
	                           CONST OraText *dir_alias, ub2 d_length, 
	                           CONST OraText *filename, ub2 f_length);
OCILOBFILESETNAME	OCILobFileSetName;


typedef sword (*OCILOBFLUSHBUFFER) (OCISvcCtx       *svchp,
	                           OCIError        *errhp,
	                           OCILobLocator   *locp,
	                           ub4              flag);
OCILOBFLUSHBUFFER	OCILobFlushBuffer;


typedef sword (*OCILOBFREETEMPORARY)(OCISvcCtx          *svchp,
	                          OCIError           *errhp,
	                          OCILobLocator      *locp);
OCILOBFREETEMPORARY	OCILobFreeTemporary;


typedef sword (*OCILOBGETCHUNKSIZE)(OCISvcCtx         *svchp,
	                         OCIError          *errhp,
	                         OCILobLocator     *locp,
	                         ub4               *chunksizep);
OCILOBGETCHUNKSIZE	OCILobGetChunkSize;


typedef sword (*OCILOBGETLENGTH)  (OCISvcCtx *svchp, OCIError *errhp, 
	                          OCILobLocator *locp,
	                          ub4 *lenp);
OCILOBGETLENGTH	OCILobGetLength;


typedef sword (*OCILOBISEQUAL)  (OCIEnv *envhp, CONST OCILobLocator *x, 
	                        CONST OCILobLocator *y, 
	                        dboolean *is_equal);
OCILOBISEQUAL	OCILobIsEqual;


typedef sword (*OCILOBISOPEN)( OCISvcCtx     *svchp,
	                    OCIError      *errhp,
	                    OCILobLocator *locp,
	                    dboolean       *flag);
OCILOBISOPEN	OCILobIsOpen;


typedef sword (*OCILOBISTEMPORARY)(OCIEnv            *envp,
	                        OCIError          *errhp,
	                        OCILobLocator     *locp,
	                        dboolean           *is_temporary);
OCILOBISTEMPORARY	OCILobIsTemporary;


typedef sword (*OCILOBLOADFROMFILE) (OCISvcCtx *svchp, OCIError *errhp, 
	                            OCILobLocator *dst_locp,
	                            OCILobLocator *src_filep, 
	                            ub4 amount, ub4 dst_offset, 
	                            ub4 src_offset);
OCILOBLOADFROMFILE	OCILobLoadFromFile;


typedef sword (*OCILOBLOCATORASSIGN)  (OCISvcCtx *svchp, OCIError *errhp, 
	                            CONST OCILobLocator *src_locp, 
	                            OCILobLocator **dst_locpp);
OCILOBLOCATORASSIGN	OCILobLocatorAssign;



typedef sword (*OCILOBLOCATORISINIT) (OCIEnv *envhp, OCIError *errhp, 
	                             CONST OCILobLocator *locp, 
	                             dboolean *is_initialized);
OCILOBLOCATORISINIT	OCILobLocatorIsInit;


typedef sword (*OCILOBOPEN)( OCISvcCtx        *svchp,
	                   OCIError         *errhp,
	                   OCILobLocator    *locp,
	                   ub1               mode );
OCILOBOPEN	OCILobOpen;

 
typedef sword (*OCILOBREAD)  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                     ub4 *amtp, ub4 offset, dvoid *bufp, ub4 bufl, dvoid *ctxp,
	                     OCICallbackLobRead cbfp, ub2 csid, ub1 csfrm);
OCILOBREAD	OCILobRead;


typedef sword (*OCILOBTRIM)  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                     ub4 newlen);
OCILOBTRIM	OCILobTrim;


typedef sword (*OCILOBWRITE)  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                      ub4 *amtp, ub4 offset, dvoid *bufp, ub4 buflen, ub1 piece,
	                      dvoid *ctxp, OCICallbackLobWrite cbfp, ub2 csid,
	                      ub1 csfrm);
OCILOBWRITE	OCILobWrite;


typedef sword (*OCILOBWRITEAPPEND)(OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *lobp,
	                        ub4 *amtp, dvoid *bufp, ub4 bufl, ub1 piece, dvoid *ctxp,
	                        OCICallbackLobWrite cbfp, ub2 csid, ub1 csfrm);
OCILOBWRITEAPPEND	OCILobWriteAppend;


typedef sword (*OCIBREAK) (dvoid *hndlp, OCIError *errhp);
OCIBREAK	OCIBreak;

typedef sword (*OCIRESET) (dvoid *hndlp, OCIError *errhp);
OCIRESET	OCIReset;

typedef sword (*OCISERVERVERSION)  (dvoid *hndlp, OCIError *errhp, OraText *bufp, 
	                           ub4 bufsz,
	                           ub1 hndltype);
OCISERVERVERSION	OCIServerVersion;


typedef sword (*OCISERVERRELEASE)  (dvoid *hndlp, OCIError *errhp, OraText *bufp,
	                           ub4 bufsz,
	                           ub1 hndltype, ub4 *version);
OCISERVERRELEASE	OCIServerRelease;


typedef sword (*OCIATTRGET) (CONST dvoid *trgthndlp, ub4 trghndltyp, 
	                    dvoid *attributep, ub4 *sizep, ub4 attrtype, 
	                    OCIError *errhp);
OCIATTRGET	OCIAttrGet;


typedef sword (*OCIATTRSET) (dvoid *trgthndlp, ub4 trghndltyp, dvoid *attributep,
	                    ub4 size, ub4 attrtype, OCIError *errhp);
OCIATTRSET	OCIAttrSet;


//sword   OCISvcCtxToLda (OCISvcCtx *svchp, OCIError *errhp, Lda_Def *ldap);

//sword   OCILdaToSvcCtx (OCISvcCtx **svchpp, OCIError *errhp, Lda_Def *ldap);

typedef sword (*OCIRESULTSETTOSTMT) (OCIResult *rsetdp, OCIError *errhp);
OCIRESULTSETTOSTMT	OCIResultSetToStmt;

//sword OCIFileClose ( dvoid  *hndl, OCIError *err, OCIFileObject *filep );

typedef sword (*OCIUSERCALLBACKREGISTER)(dvoid *hndlp, ub4 type, dvoid *ehndlp,
	                                    OCIUserCallback callback, dvoid *ctxp,
	                                    ub4 fcode, ub4 when, OCIUcb *ucbDesc);
OCIUSERCALLBACKREGISTER	OCIUserCallbackRegister;


typedef sword (*OCIUSERCALLBACKGET)(dvoid *hndlp, ub4 type, dvoid *ehndlp,
	                               ub4 fcode, ub4 when, OCIUserCallback *callbackp,
	                               dvoid **ctxpp, OCIUcb *ucbDesc);
OCIUSERCALLBACKGET	OCIUserCallbackGet;


typedef sword (*OCISHAREDLIBINIT)(dvoid *metaCtx, dvoid *libCtx, ub4 argfmt, sword argc,
	                         dvoid **argv, OCIEnvCallbackType envCallback);
OCISHAREDLIBINIT	OCISharedLibInit;


typedef sword (*OCIFILEEXISTS) ( dvoid  *hndl, OCIError *err, OraText *filename,
	                     OraText *path, ub1 *flag  );
OCIFILEEXISTS	OCIFileExists;


//sword OCIFileFlush( dvoid *hndl, OCIError *err, OCIFileObject *filep  );


typedef sword (*OCIFILEGETLENGTH)( dvoid *hndl, OCIError *err, OraText *filename,
	                        OraText *path, ubig_ora *lenp  );
OCIFILEGETLENGTH	OCIFileGetLength;


typedef sword (*OCIFILEINIT) ( dvoid *hndl, OCIError *err );
OCIFILEINIT	OCIFileInit;

/*sword OCIFileOpen ( dvoid *hndl, OCIError *err, OCIFileObject **filep,
                    OraText *filename, OraText *path, ub4 mode, ub4 create, 
                    ub4 type );
*/

/*sword OCIFileRead ( dvoid *hndl, OCIError *err, OCIFileObject *filep,
                    dvoid *bufp, ub4 bufl, ub4 *bytesread );
*/

/*sword OCIFileSeek ( dvoid *hndl, OCIError *err, OCIFileObject *filep,
                     uword origin, ubig_ora offset, sb1 dir );
*/

typedef sword (*OCIFILETERM) ( dvoid *hndl, OCIError *err );
OCIFILETERM	OCIFileTerm;


/* sword OCIFileWrite ( dvoid *hndl, OCIError *err, OCIFileObject   *filep,
                     dvoid *bufp, ub4 buflen, ub4 *byteswritten );
*/

#ifdef ORAXB8_DEFINED

typedef sword (*OCILOBCOPY2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *dst_locp,
	                     OCILobLocator *src_locp, oraub8 amount, oraub8 dst_offset, 
	                     oraub8 src_offset);
OCILOBCOPY2	OCILobCopy2;


typedef sword (*OCILOBERASE2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                      oraub8 *amount, oraub8 offset);
OCILOBERASE2	OCILobErase2;


typedef sword (*OCILOBGETLENGTH2) (OCISvcCtx *svchp, OCIError *errhp, 
	                          OCILobLocator *locp, oraub8 *lenp);
OCILOBGETLENGTH2	OCILobGetLength2;


typedef sword (*OCILOBLOADFROMFILE2) (OCISvcCtx *svchp, OCIError *errhp, 
	                             OCILobLocator *dst_locp,
	                             OCILobLocator *src_filep, 
	                             oraub8 amount, oraub8 dst_offset, 
	                             oraub8 src_offset);
OCILOBLOADFROMFILE2	OCILobLoadFromFile2;


typedef sword (*OCILOBREAD2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                     oraub8 *byte_amtp, oraub8 *char_amtp, oraub8 offset,
	                     dvoid *bufp, oraub8 bufl, ub1 piece, dvoid *ctxp,
	                     OCICallbackLobRead2 cbfp, ub2 csid, ub1 csfrm);
OCILOBREAD2	OCILobRead2;


typedef sword (*OCILOBARRAYREAD) (OCISvcCtx *svchp, OCIError *errhp, ub4 *array_iter,
	                         OCILobLocator **lobp_arr, oraub8 *byte_amt_arr,
	                         oraub8 *char_amt_arr, oraub8 *offset_arr,
	                         dvoid **bufp_arr, oraub8 *bufl_arr, ub1 piece,
	                         dvoid *ctxp, OCICallbackLobArrayRead cbfp, ub2 csid,
	                         ub1 csfrm);
OCILOBARRAYREAD	OCILobArrayRead;


typedef sword (*OCILOBTRIM2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                     oraub8 newlen);
OCILOBTRIM2	OCILobTrim2;


typedef sword (*OCILOBWRITE2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
	                      oraub8 *byte_amtp, oraub8 *char_amtp, oraub8 offset,
	                      dvoid *bufp, oraub8 buflen, ub1 piece, dvoid *ctxp, 
	                      OCICallbackLobWrite2 cbfp, ub2 csid, ub1 csfrm);
OCILOBWRITE2	OCILobWrite2;


typedef sword (*OCILOBARRAYWRITE) (OCISvcCtx *svchp, OCIError *errhp, ub4 *array_iter,
	                          OCILobLocator **lobp_arr, oraub8 *byte_amt_arr,
	                          oraub8 *char_amt_arr, oraub8 *offset_arr,
	                          dvoid **bufp_arr, oraub8 *bufl_arr, ub1 piece,
	                          dvoid *ctxp, OCICallbackLobArrayWrite cbfp, ub2 csid,
	                          ub1 csfrm);
OCILOBARRAYWRITE	OCILobArrayWrite;


typedef sword (*OCILOBWRITEAPPEND2) (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *lobp,
	                          oraub8 *byte_amtp, oraub8 *char_amtp, dvoid *bufp,
	                          oraub8 bufl, ub1 piece, dvoid *ctxp,
	                          OCICallbackLobWrite2 cbfp, ub2 csid, ub1 csfrm);
OCILOBWRITEAPPEND2	OCILobWriteAppend2;


typedef sword (*OCILOBGETSTORAGELIMIT) (OCISvcCtx *svchp, OCIError *errhp,
	                             OCILobLocator *lobp, oraub8 *limitp);
OCILOBGETSTORAGELIMIT	OCILobGetStorageLimit;


#endif

