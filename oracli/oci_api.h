/* Copyright (c) 2005-2007 by Ju Haibo (octerboy@21cn.com)
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

sword   OCIInitialize   (ub4 mode, dvoid *ctxp, 
                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr) );

sword   OCITerminate( ub4 mode);

sword   OCIEnvCreate (OCIEnv **envp, ub4 mode, dvoid *ctxp,
                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
                 size_t xtramem_sz, dvoid **usrmempp);

sword   OCIEnvNlsCreate (OCIEnv **envp, ub4 mode, dvoid *ctxp,
                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
                 size_t xtramem_sz, dvoid **usrmempp,
                 ub2 charset, ub2 ncharset);

sword OCINlsEnvironmentVariableGet(dvoid  *valp, size_t size, ub2 item,
                                   ub2 charset, size_t *rsize);

sword   OCIFEnvCreate (OCIEnv **envp, ub4 mode, dvoid *ctxp,
                 dvoid *(*malocfp)(dvoid *ctxp, size_t size),
                 dvoid *(*ralocfp)(dvoid *ctxp, dvoid *memptr, size_t newsize),
                 void   (*mfreefp)(dvoid *ctxp, dvoid *memptr),
                 size_t xtramem_sz, dvoid **usrmempp, dvoid *fupg);

sword   OCIHandleAlloc(CONST dvoid *parenth, dvoid **hndlpp, CONST ub4 type, 
                       CONST size_t xtramem_sz, dvoid **usrmempp);

sword   OCIHandleFree(dvoid *hndlp, CONST ub4 type);


sword   OCIDescriptorAlloc(CONST dvoid *parenth, dvoid **descpp, 
                           CONST ub4 type, CONST size_t xtramem_sz, 
                           dvoid **usrmempp);

sword   OCIDescriptorFree(dvoid *descp, CONST ub4 type);

sword   OCIEnvInit (OCIEnv **envp, ub4 mode, 
                    size_t xtramem_sz, dvoid **usrmempp);

sword   OCIServerAttach  (OCIServer *srvhp, OCIError *errhp,
                          CONST OraText *dblink, sb4 dblink_len, ub4 mode);

sword   OCIServerDetach  (OCIServer *srvhp, OCIError *errhp, ub4 mode);

sword   OCISessionBegin  (OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp,
                          ub4 credt, ub4 mode);

sword   OCISessionEnd   (OCISvcCtx *svchp, OCIError *errhp, OCISession *usrhp, 
                         ub4 mode);

sword   OCILogon (OCIEnv *envhp, OCIError *errhp, OCISvcCtx **svchp, 
                  CONST OraText *username, ub4 uname_len, 
                  CONST OraText *password, ub4 passwd_len, 
                  CONST OraText *dbname, ub4 dbname_len);

sword   OCILogon2 (OCIEnv *envhp, OCIError *errhp, OCISvcCtx **svchp,
                  CONST OraText *username, ub4 uname_len,
                  CONST OraText *password, ub4 passwd_len,
                  CONST OraText *dbname, ub4 dbname_len,
                  ub4 mode);

sword   OCILogoff (OCISvcCtx *svchp, OCIError *errhp);


sword   OCIPasswordChange   (OCISvcCtx *svchp, OCIError *errhp, 
                             CONST OraText *user_name, ub4 usernm_len, 
                             CONST OraText *opasswd, ub4 opasswd_len, 
                             CONST OraText *npasswd, ub4 npasswd_len, 
                             ub4 mode);

sword   OCIStmtPrepare   (OCIStmt *stmtp, OCIError *errhp, CONST OraText *stmt,
                          ub4 stmt_len, ub4 language, ub4 mode);

sword OCIStmtPrepare2 ( OCISvcCtx *svchp, OCIStmt **stmtp, OCIError *errhp,
                     CONST OraText *stmt, ub4 stmt_len, CONST OraText *key,
                     ub4 key_len, ub4 language, ub4 mode);

sword OCIStmtRelease ( OCIStmt *stmtp, OCIError *errhp, CONST OraText *key,
                       ub4 key_len, ub4 mode);

sword   OCIBindByPos  (OCIStmt *stmtp, OCIBind **bindp, OCIError *errhp,
                       ub4 position, dvoid *valuep, sb4 value_sz,
                       ub2 dty, dvoid *indp, ub2 *alenp, ub2 *rcodep,
                       ub4 maxarr_len, ub4 *curelep, ub4 mode);

sword   OCIBindByName   (OCIStmt *stmtp, OCIBind **bindp, OCIError *errhp,
                         CONST OraText *placeholder, sb4 placeh_len, 
                         dvoid *valuep, sb4 value_sz, ub2 dty, 
                         dvoid *indp, ub2 *alenp, ub2 *rcodep, 
                         ub4 maxarr_len, ub4 *curelep, ub4 mode);

sword   OCIBindObject  (OCIBind *bindp, OCIError *errhp, CONST OCIType *type, 
                        dvoid **pgvpp, ub4 *pvszsp, dvoid **indpp, 
                        ub4 *indszp);

sword   OCIBindDynamic   (OCIBind *bindp, OCIError *errhp, dvoid *ictxp,
                          OCICallbackInBind icbfp, dvoid *octxp,
                          OCICallbackOutBind ocbfp);

sword   OCIBindArrayOfStruct   (OCIBind *bindp, OCIError *errhp, 
                                ub4 pvskip, ub4 indskip,
                                ub4 alskip, ub4 rcskip);

sword   OCIStmtGetPieceInfo   (OCIStmt *stmtp, OCIError *errhp, 
                               dvoid **hndlpp, ub4 *typep,
                               ub1 *in_outp, ub4 *iterp, ub4 *idxp, 
                               ub1 *piecep);

sword   OCIStmtSetPieceInfo   (dvoid *hndlp, ub4 type, OCIError *errhp, 
                               CONST dvoid *bufp, ub4 *alenp, ub1 piece, 
                               CONST dvoid *indp, ub2 *rcodep);

sword   OCIStmtExecute  (OCISvcCtx *svchp, OCIStmt *stmtp, OCIError *errhp, 
                         ub4 iters, ub4 rowoff, CONST OCISnapshot *snap_in, 
                         OCISnapshot *snap_out, ub4 mode);

sword   OCIDefineByPos  (OCIStmt *stmtp, OCIDefine **defnp, OCIError *errhp,
                         ub4 position, dvoid *valuep, sb4 value_sz, ub2 dty,
                         dvoid *indp, ub2 *rlenp, ub2 *rcodep, ub4 mode);

sword   OCIDefineObject  (OCIDefine *defnp, OCIError *errhp, 
                          CONST OCIType *type, dvoid **pgvpp, 
                          ub4 *pvszsp, dvoid **indpp, ub4 *indszp);

sword   OCIDefineDynamic   (OCIDefine *defnp, OCIError *errhp, dvoid *octxp,
                            OCICallbackDefine ocbfp);

sword   OCIRowidToChar  (OCIRowid *rowidDesc, OraText *outbfp, ub2 *outbflp,
                         OCIError *errhp);

sword   OCIDefineArrayOfStruct  (OCIDefine *defnp, OCIError *errhp, ub4 pvskip,
                                 ub4 indskip, ub4 rlskip, ub4 rcskip);

sword   OCIStmtFetch   (OCIStmt *stmtp, OCIError *errhp, ub4 nrows, 
                        ub2 orientation, ub4 mode);

sword   OCIStmtFetch2   (OCIStmt *stmtp, OCIError *errhp, ub4 nrows, 
                        ub2 orientation, sb4 scrollOffset, ub4 mode);

sword   OCIStmtGetBindInfo   (OCIStmt *stmtp, OCIError *errhp, ub4 size, 
                              ub4 startloc,
                              sb4 *found, OraText *bvnp[], ub1 bvnl[],
                              OraText *invp[], ub1 inpl[], ub1 dupl[],
                              OCIBind **hndl);

sword   OCIDescribeAny  (OCISvcCtx *svchp, OCIError *errhp, 
                         dvoid *objptr, 
                         ub4 objnm_len, ub1 objptr_typ, ub1 info_level,
                         ub1 objtyp, OCIDescribe *dschp);

sword   OCIParamGet (CONST dvoid *hndlp, ub4 htype, OCIError *errhp, 
                     dvoid **parmdpp, ub4 pos);

sword   OCIParamSet(dvoid *hdlp, ub4 htyp, OCIError *errhp, CONST dvoid *dscp,
                    ub4 dtyp, ub4 pos);

sword   OCITransStart  (OCISvcCtx *svchp, OCIError *errhp, 
                        uword timeout, ub4 flags );

sword   OCITransDetach  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags );

sword   OCITransCommit  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);

sword   OCITransRollback  (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);

sword   OCITransPrepare (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);

sword   OCITransMultiPrepare (OCISvcCtx *svchp, ub4 numBranches, 
                              OCITrans **txns, OCIError **errhp);

sword   OCITransForget (OCISvcCtx *svchp, OCIError *errhp, ub4 flags);

sword   OCIErrorGet   (dvoid *hndlp, ub4 recordno, OraText *sqlstate,
                       sb4 *errcodep, OraText *bufp, ub4 bufsiz, ub4 type);

sword   OCILobAppend  (OCISvcCtx *svchp, OCIError *errhp, 
                       OCILobLocator *dst_locp,
                       OCILobLocator *src_locp);

sword   OCILobAssign (OCIEnv *envhp, OCIError *errhp, 
                      CONST OCILobLocator *src_locp, 
                      OCILobLocator **dst_locpp);

sword   OCILobCharSetForm (OCIEnv *envhp, OCIError *errhp, 
                           CONST OCILobLocator *locp, ub1 *csfrm);

sword   OCILobCharSetId (OCIEnv *envhp, OCIError *errhp, 
                         CONST OCILobLocator *locp, ub2 *csid);

sword   OCILobCopy (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *dst_locp,
                    OCILobLocator *src_locp, ub4 amount, ub4 dst_offset, 
                    ub4 src_offset);

sword OCILobCreateTemporary(OCISvcCtx          *svchp,
                            OCIError           *errhp,
                            OCILobLocator      *locp,
                            ub2                 csid,
                            ub1                 csfrm,
                            ub1                 lobtype,
                            dboolean             cache,
                            OCIDuration         duration);


sword OCILobClose( OCISvcCtx        *svchp,
                   OCIError         *errhp,
                   OCILobLocator    *locp );


sword   OCILobDisableBuffering (OCISvcCtx      *svchp,
                                OCIError       *errhp,
                                OCILobLocator  *locp);

sword   OCILobEnableBuffering (OCISvcCtx      *svchp,
                               OCIError       *errhp,
                               OCILobLocator  *locp);

sword   OCILobErase (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                      ub4 *amount, ub4 offset);

sword   OCILobFileClose (OCISvcCtx *svchp, OCIError *errhp, 
                         OCILobLocator *filep);

sword   OCILobFileCloseAll (OCISvcCtx *svchp, OCIError *errhp);

sword   OCILobFileExists (OCISvcCtx *svchp, OCIError *errhp, 
                          OCILobLocator *filep,
                          dboolean *flag);

sword   OCILobFileGetName (OCIEnv *envhp, OCIError *errhp, 
                           CONST OCILobLocator *filep, 
                           OraText *dir_alias, ub2 *d_length, 
                           OraText *filename, ub2 *f_length);

sword   OCILobFileIsOpen (OCISvcCtx *svchp, OCIError *errhp, 
                          OCILobLocator *filep,
                          dboolean *flag);

sword   OCILobFileOpen (OCISvcCtx *svchp, OCIError *errhp, 
                        OCILobLocator *filep,
                        ub1 mode);

sword   OCILobFileSetName (OCIEnv *envhp, OCIError *errhp, 
                           OCILobLocator **filepp, 
                           CONST OraText *dir_alias, ub2 d_length, 
                           CONST OraText *filename, ub2 f_length);

sword   OCILobFlushBuffer (OCISvcCtx       *svchp,
                           OCIError        *errhp,
                           OCILobLocator   *locp,
                           ub4              flag);

sword OCILobFreeTemporary(OCISvcCtx          *svchp,
                          OCIError           *errhp,
                          OCILobLocator      *locp);

sword OCILobGetChunkSize(OCISvcCtx         *svchp,
                         OCIError          *errhp,
                         OCILobLocator     *locp,
                         ub4               *chunksizep);

sword   OCILobGetLength  (OCISvcCtx *svchp, OCIError *errhp, 
                          OCILobLocator *locp,
                          ub4 *lenp);

sword   OCILobIsEqual  (OCIEnv *envhp, CONST OCILobLocator *x, 
                        CONST OCILobLocator *y, 
                        dboolean *is_equal);

sword OCILobIsOpen( OCISvcCtx     *svchp,
                    OCIError      *errhp,
                    OCILobLocator *locp,
                    dboolean       *flag);

sword OCILobIsTemporary(OCIEnv            *envp,
                        OCIError          *errhp,
                        OCILobLocator     *locp,
                        dboolean           *is_temporary);

sword   OCILobLoadFromFile (OCISvcCtx *svchp, OCIError *errhp, 
                            OCILobLocator *dst_locp,
                            OCILobLocator *src_filep, 
                            ub4 amount, ub4 dst_offset, 
                            ub4 src_offset);

sword   OCILobLocatorAssign  (OCISvcCtx *svchp, OCIError *errhp, 
                            CONST OCILobLocator *src_locp, 
                            OCILobLocator **dst_locpp);


sword   OCILobLocatorIsInit (OCIEnv *envhp, OCIError *errhp, 
                             CONST OCILobLocator *locp, 
                             dboolean *is_initialized);

sword   OCILobOpen( OCISvcCtx        *svchp,
                   OCIError         *errhp,
                   OCILobLocator    *locp,
                   ub1               mode );
 
sword   OCILobRead  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                     ub4 *amtp, ub4 offset, dvoid *bufp, ub4 bufl, dvoid *ctxp,
                     OCICallbackLobRead cbfp, ub2 csid, ub1 csfrm);

sword   OCILobTrim  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                     ub4 newlen);

sword   OCILobWrite  (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                      ub4 *amtp, ub4 offset, dvoid *bufp, ub4 buflen, ub1 piece,
                      dvoid *ctxp, OCICallbackLobWrite cbfp, ub2 csid,
                      ub1 csfrm);

sword OCILobWriteAppend(OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *lobp,
                        ub4 *amtp, dvoid *bufp, ub4 bufl, ub1 piece, dvoid *ctxp,
                        OCICallbackLobWrite cbfp, ub2 csid, ub1 csfrm);

sword   OCIBreak (dvoid *hndlp, OCIError *errhp);

sword   OCIReset (dvoid *hndlp, OCIError *errhp);

sword   OCIServerVersion  (dvoid *hndlp, OCIError *errhp, OraText *bufp, 
                           ub4 bufsz,
                           ub1 hndltype);

sword   OCIServerRelease  (dvoid *hndlp, OCIError *errhp, OraText *bufp,
                           ub4 bufsz,
                           ub1 hndltype, ub4 *version);

sword   OCIAttrGet (CONST dvoid *trgthndlp, ub4 trghndltyp, 
                    dvoid *attributep, ub4 *sizep, ub4 attrtype, 
                    OCIError *errhp);

sword   OCIAttrSet (dvoid *trgthndlp, ub4 trghndltyp, dvoid *attributep,
                    ub4 size, ub4 attrtype, OCIError *errhp);

//sword   OCISvcCtxToLda (OCISvcCtx *svchp, OCIError *errhp, Lda_Def *ldap);

//sword   OCILdaToSvcCtx (OCISvcCtx **svchpp, OCIError *errhp, Lda_Def *ldap);

sword   OCIResultSetToStmt (OCIResult *rsetdp, OCIError *errhp);

//sword OCIFileClose ( dvoid  *hndl, OCIError *err, OCIFileObject *filep );

sword   OCIUserCallbackRegister(dvoid *hndlp, ub4 type, dvoid *ehndlp,
                                    OCIUserCallback callback, dvoid *ctxp,
                                    ub4 fcode, ub4 when, OCIUcb *ucbDesc);

sword   OCIUserCallbackGet(dvoid *hndlp, ub4 type, dvoid *ehndlp,
                               ub4 fcode, ub4 when, OCIUserCallback *callbackp,
                               dvoid **ctxpp, OCIUcb *ucbDesc);

sword   OCISharedLibInit(dvoid *metaCtx, dvoid *libCtx, ub4 argfmt, sword argc,
                         dvoid **argv, OCIEnvCallbackType envCallback);

sword OCIFileExists ( dvoid  *hndl, OCIError *err, OraText *filename,
                     OraText *path, ub1 *flag  );

//sword OCIFileFlush( dvoid *hndl, OCIError *err, OCIFileObject *filep  );


sword OCIFileGetLength( dvoid *hndl, OCIError *err, OraText *filename,
                        OraText *path, ubig_ora *lenp  );

sword OCIFileInit ( dvoid *hndl, OCIError *err );

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

sword OCIFileTerm ( dvoid *hndl, OCIError *err );


/* sword OCIFileWrite ( dvoid *hndl, OCIError *err, OCIFileObject   *filep,
                     dvoid *bufp, ub4 buflen, ub4 *byteswritten );
*/

#ifdef ORAXB8_DEFINED

sword   OCILobCopy2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *dst_locp,
                     OCILobLocator *src_locp, oraub8 amount, oraub8 dst_offset, 
                     oraub8 src_offset);

sword   OCILobErase2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                      oraub8 *amount, oraub8 offset);

sword   OCILobGetLength2 (OCISvcCtx *svchp, OCIError *errhp, 
                          OCILobLocator *locp, oraub8 *lenp);

sword   OCILobLoadFromFile2 (OCISvcCtx *svchp, OCIError *errhp, 
                             OCILobLocator *dst_locp,
                             OCILobLocator *src_filep, 
                             oraub8 amount, oraub8 dst_offset, 
                             oraub8 src_offset);

sword   OCILobRead2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                     oraub8 *byte_amtp, oraub8 *char_amtp, oraub8 offset,
                     dvoid *bufp, oraub8 bufl, ub1 piece, dvoid *ctxp,
                     OCICallbackLobRead2 cbfp, ub2 csid, ub1 csfrm);

sword   OCILobArrayRead (OCISvcCtx *svchp, OCIError *errhp, ub4 *array_iter,
                         OCILobLocator **lobp_arr, oraub8 *byte_amt_arr,
                         oraub8 *char_amt_arr, oraub8 *offset_arr,
                         dvoid **bufp_arr, oraub8 *bufl_arr, ub1 piece,
                         dvoid *ctxp, OCICallbackLobArrayRead cbfp, ub2 csid,
                         ub1 csfrm);

sword   OCILobTrim2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                     oraub8 newlen);

sword   OCILobWrite2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *locp,
                      oraub8 *byte_amtp, oraub8 *char_amtp, oraub8 offset,
                      dvoid *bufp, oraub8 buflen, ub1 piece, dvoid *ctxp, 
                      OCICallbackLobWrite2 cbfp, ub2 csid, ub1 csfrm);

sword   OCILobArrayWrite (OCISvcCtx *svchp, OCIError *errhp, ub4 *array_iter,
                          OCILobLocator **lobp_arr, oraub8 *byte_amt_arr,
                          oraub8 *char_amt_arr, oraub8 *offset_arr,
                          dvoid **bufp_arr, oraub8 *bufl_arr, ub1 piece,
                          dvoid *ctxp, OCICallbackLobArrayWrite cbfp, ub2 csid,
                          ub1 csfrm);

sword OCILobWriteAppend2 (OCISvcCtx *svchp, OCIError *errhp, OCILobLocator *lobp,
                          oraub8 *byte_amtp, oraub8 *char_amtp, dvoid *bufp,
                          oraub8 bufl, ub1 piece, dvoid *ctxp,
                          OCICallbackLobWrite2 cbfp, ub2 csid, ub1 csfrm);

sword OCILobGetStorageLimit (OCISvcCtx *svchp, OCIError *errhp,
                             OCILobLocator *lobp, oraub8 *limitp);

#endif

