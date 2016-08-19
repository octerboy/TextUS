int js_fgets(char *buf, int size, FILE *file)
{
    int n, i, c;
    JSBool crflag;

    n = size - 1;
    if (n < 0)
        return -1;

    crflag = JS_FALSE;
    for (i = 0; i < n && (c = getc(file)) != EOF; i++) {
        buf[i] = c;
        if (c == '\n') {        /* any \n ends a line */
            i++;                /* keep the \n; we know there is room for \0 */
            break;
        }
        if (crflag) {           /* \r not followed by \n ends line at the \r */
            ungetc(c, file);
            break;              /* and overwrite c in buf with \0 */
        }
        crflag = (c == '\r');
    }

    buf[i] = '\0';
    return i;
}

static JSBool compileOnly = JS_FALSE;
static JSBool
Version(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc > 0 && JSVAL_IS_INT(argv[0]))
        *rval = INT_TO_JSVAL(JS_SetVersion(cx, (JSVersion) JSVAL_TO_INT(argv[0])));
    else
        *rval = INT_TO_JSVAL(JS_GetVersion(cx));
    return JS_TRUE;
}


static struct {
    const char  *name;
    uint32      flag;
} js_options[] = {
    {"strict",          JSOPTION_STRICT},
    {"werror",          JSOPTION_WERROR},
    {"atline",          JSOPTION_ATLINE},
    {"xml",             JSOPTION_XML},
    {0,                 0}
};

static JSBool
Options(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uint32 optset, flag;
    uintN i, j;
    JSString *str;
    const char *opt;
    char names[128];

    optset = 0;
    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        opt = JS_GetStringBytes(str);
        for (j = 0; js_options[j].name; j++) {
            if (strcmp(js_options[j].name, opt) == 0) {
                optset |= js_options[j].flag;
                break;
            }
        }
    }
    optset = JS_ToggleOptions(cx, optset);

    names[0]= '\0';
    while (optset != 0) {
        flag = optset;
        optset &= optset - 1;
        flag &= ~optset;
        for (j = 0; js_options[j].name; j++) {
            if (js_options[j].flag == flag) {
		if (strlen(names) > 0 )
			memcpy(&names[strlen(names)], ",", 1);
		memcpy(&names[strlen(names)], js_options[j].name, strlen(js_options[j].name));

                break;
            }
        }
    }

    str = JS_NewString(cx, names, strlen(names));
    if (!str) {
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    uint32 oldopts;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        errno = 0;
        oldopts = JS_GetOptions(cx);
        JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
        script = JS_CompileFile(cx, obj, filename);
        if (!script) {
            ok = JS_FALSE;
        } else {
            ok = !compileOnly
                 ? JS_ExecuteScript(cx, obj, script, &result)
                 : JS_TRUE;
            JS_DestroyScript(cx, script);
        }
        JS_SetOptions(cx, oldopts);
        if (!ok)
            return JS_FALSE;
    }

    return JS_TRUE;
}

/*
 * function readline()
 * Provides a hook for scripts to read a line from stdin.
 */
static JSBool
ReadLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#define BUFSIZE 256
    FILE *from;
    char *buf, *tmp;
    size_t bufsize, buflength, gotlength;
    JSString *str;

    from = stdin;
    buflength = 0;
    bufsize = BUFSIZE;
    buf = (char*) JS_malloc(cx, bufsize);
    if (!buf)
        return JS_FALSE;

    while ((gotlength =
            js_fgets(buf + buflength, bufsize - buflength, from)) > 0) {
        buflength += gotlength;

        /* Are we done? */
        if (buf[buflength - 1] == '\n') {
            buf[buflength - 1] = '\0';
            break;
        }

        /* Else, grow our buffer for another pass. */
        tmp = (char*) JS_realloc(cx, buf, bufsize * 2);
        if (!tmp) {
            JS_free(cx, buf);
            return JS_FALSE;
        }

        bufsize *= 2;
        buf = tmp;
    }

    /* Treat the empty string specially. */
    if (buflength == 0) {
        *rval = JS_GetEmptyStringValue(cx);
        JS_free(cx, buf);
        return JS_TRUE;
    }

    /* Shrink the buffer to the real size. */
    tmp = (char*) JS_realloc(cx, buf, buflength);
    if (!tmp) {
        JS_free(cx, buf);
        return JS_FALSE;
    }

    buf = tmp;

    /*
     * Turn buf into a JSString. Note that buflength includes the trailing null
     * character.
     */
    str = JS_NewString(cx, buf, buflength - 1);
    if (!str) {
        JS_free(cx, buf);
        return JS_FALSE;
    }

    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i, n;
    JSString *str;

    for (i = n = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        //fprintf(gOutFile, "%s%s", i ? " " : "", JS_GetStringBytes(str));
        fprintf(stdout, "%s%s", i ? " " : "", JS_GetStringBytes(str));
    }
    n++;
    //if (n)
        //fputc('\n', gOutFile);
    return JS_TRUE;
}

#ifdef USE_ALL
static JSBool
Help(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSBool
Quit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
#ifdef LIVECONNECT
    JSJ_SimpleShutdown();
#endif

    JS_ConvertArguments(cx, argc, argv,"/ i", &gExitCode);

    gQuitting = JS_TRUE;
    return JS_FALSE;
}

static JSBool
GC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSRuntime *rt;
    uint32 preBytes;

    rt = cx->runtime;
    preBytes = rt->gcBytes;
#ifdef GC_MARK_DEBUG
    if (argc && JSVAL_IS_STRING(argv[0])) {
        char *name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
        FILE *file = fopen(name, "w");
        if (!file) {
            fprintf(gErrFile, "gc: can't open %s: %s\n", strerror(errno));
            return JS_FALSE;
        }
        js_DumpGCHeap = file;
    } else {
        js_DumpGCHeap = stdout;
    }
#endif
    JS_GC(cx);
#ifdef GC_MARK_DEBUG
    if (js_DumpGCHeap != stdout)
        fclose(js_DumpGCHeap);
    js_DumpGCHeap = NULL;
#endif
    fprintf(gOutFile, "before %lu, after %lu, break %08lx\n",
            (unsigned long)preBytes, (unsigned long)rt->gcBytes,
#ifdef XP_UNIX
            (unsigned long)sbrk(0)
#else
            0
#endif
            );
#ifdef JS_GCMETER
    js_DumpGCStats(rt, stdout);
#endif
    return JS_TRUE;
}

static JSScript *
ValueToScript(JSContext *cx, jsval v)
{
    JSScript *script;
    JSFunction *fun;

    if (!JSVAL_IS_PRIMITIVE(v) &&
        JS_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_ScriptClass) {
        script = (JSScript *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(v));
    } else {
        fun = JS_ValueToFunction(cx, v);
        if (!fun)
            return NULL;
        script = FUN_SCRIPT(fun);
    }
    return script;
}

static JSBool
GetTrapArgs(JSContext *cx, uintN argc, jsval *argv, JSScript **scriptp,
            int32 *ip)
{
    jsval v;
    uintN intarg;
    JSScript *script;

    *scriptp = cx->fp->down->script;
    *ip = 0;
    if (argc != 0) {
        v = argv[0];
        intarg = 0;
        if (!JSVAL_IS_PRIMITIVE(v) &&
            (JS_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_FunctionClass ||
             JS_GET_CLASS(cx, JSVAL_TO_OBJECT(v)) == &js_ScriptClass)) {
            script = ValueToScript(cx, v);
            if (!script)
                return JS_FALSE;
            *scriptp = script;
            intarg++;
        }
        if (argc > intarg) {
            if (!JS_ValueToInt32(cx, argv[intarg], ip))
                return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSTrapStatus
TrapHandler(JSContext *cx, JSScript *script, jsbytecode *pc, jsval *rval,
            void *closure)
{
    JSString *str;
    JSStackFrame *caller;

    str = (JSString *) closure;
    caller = JS_GetScriptedCaller(cx, NULL);
    if (!JS_EvaluateScript(cx, caller->scopeChain,
                           JS_GetStringBytes(str), JS_GetStringLength(str),
                           caller->script->filename, caller->script->lineno,
                           rval)) {
        return JSTRAP_ERROR;
    }
    if (*rval != JSVAL_VOID)
        return JSTRAP_RETURN;
    return JSTRAP_CONTINUE;
}

static JSBool
Trap(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    JSScript *script;
    int32 i;

    if (argc == 0) {
        JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL, JSSMSG_TRAP_USAGE);
        return JS_FALSE;
    }
    argc--;
    str = JS_ValueToString(cx, argv[argc]);
    if (!str)
        return JS_FALSE;
    argv[argc] = STRING_TO_JSVAL(str);
    if (!GetTrapArgs(cx, argc, argv, &script, &i))
        return JS_FALSE;
    return JS_SetTrap(cx, script, script->code + i, TrapHandler, str);
}

static JSBool
Untrap(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSScript *script;
    int32 i;

    if (!GetTrapArgs(cx, argc, argv, &script, &i))
        return JS_FALSE;
    JS_ClearTrap(cx, script, script->code + i, NULL, NULL);
    return JS_TRUE;
}

static JSBool
LineToPC(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSScript *script;
    int32 i;
    uintN lineno;
    jsbytecode *pc;

    if (argc == 0) {
        JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL, JSSMSG_LINE2PC_USAGE);
        return JS_FALSE;
    }
    script = cx->fp->down->script;
    if (!GetTrapArgs(cx, argc, argv, &script, &i))
        return JS_FALSE;
    lineno = (i == 0) ? script->lineno : (uintN)i;
    pc = JS_LineNumberToPC(cx, script, lineno);
    if (!pc)
        return JS_FALSE;
    *rval = INT_TO_JSVAL(PTRDIFF(pc, script->code, jsbytecode));
    return JS_TRUE;
}

static JSBool
PCToLine(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSScript *script;
    int32 i;
    uintN lineno;

    if (!GetTrapArgs(cx, argc, argv, &script, &i))
        return JS_FALSE;
    lineno = JS_PCToLineNumber(cx, script, script->code + i);
    if (!lineno)
        return JS_FALSE;
    *rval = INT_TO_JSVAL(lineno);
    return JS_TRUE;
}

#ifdef DEBUG

static void
GetSwitchTableBounds(JSScript *script, uintN offset,
                     uintN *start, uintN *end)
{
    jsbytecode *pc;
    JSOp op;
    ptrdiff_t jmplen;
    jsint low, high, n;

    pc = script->code + offset;
    op = *pc;
    switch (op) {
      case JSOP_TABLESWITCHX:
        jmplen = JUMPX_OFFSET_LEN;
        goto jump_table;
      case JSOP_TABLESWITCH:
        jmplen = JUMP_OFFSET_LEN;
      jump_table:
        pc += jmplen;
        low = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        high = GET_JUMP_OFFSET(pc);
        pc += JUMP_OFFSET_LEN;
        n = high - low + 1;
        break;

      case JSOP_LOOKUPSWITCHX:
        jmplen = JUMPX_OFFSET_LEN;
        goto lookup_table;
      default:
        JS_ASSERT(op == JSOP_LOOKUPSWITCH);
        jmplen = JUMP_OFFSET_LEN;
      lookup_table:
        pc += jmplen;
        n = GET_ATOM_INDEX(pc);
        pc += ATOM_INDEX_LEN;
        jmplen += ATOM_INDEX_LEN;
        break;
    }

    *start = (uintN)(pc - script->code);
    *end = *start + (uintN)(n * jmplen);
}


/*
 * SrcNotes assumes that SRC_METHODBASE should be distinguished from SRC_LABEL
 * using the bytecode the source note points to.
 */
JS_STATIC_ASSERT(SRC_LABEL == SRC_METHODBASE);

static void
SrcNotes(JSContext *cx, JSScript *script)
{
    uintN offset, delta, caseOff, switchTableStart, switchTableEnd;
    jssrcnote *notes, *sn;
    JSSrcNoteType type;
    const char *name;
    JSOp op;
    jsatomid atomIndex;
    JSAtom *atom;

    fprintf(gOutFile, "\nSource notes:\n");
    offset = 0;
    notes = SCRIPT_NOTES(script);
    switchTableEnd = switchTableStart = 0;
    for (sn = notes; !SN_IS_TERMINATOR(sn); sn = SN_NEXT(sn)) {
        delta = SN_DELTA(sn);
        offset += delta;
        type = (JSSrcNoteType) SN_TYPE(sn);
        name = js_SrcNoteSpec[type].name;
        if (type == SRC_LABEL) {
            /* Heavily overloaded case. */
            if (switchTableStart <= offset && offset < switchTableEnd) {
                name = "case";
            } else {
                op = script->code[offset];
                if (op == JSOP_GETMETHOD || op == JSOP_SETMETHOD) {
                    /* This is SRC_METHODBASE which we print as SRC_PCBASE. */
                    type = SRC_PCBASE;
                    name = "methodbase";
                } else {
                    JS_ASSERT(op == JSOP_NOP);
                }
            }
        }
        fprintf(gOutFile, "%3u: %5u [%4u] %-8s",
                PTRDIFF(sn, notes, jssrcnote), offset, delta, name);
        switch (type) {
          case SRC_SETLINE:
            fprintf(gOutFile, " lineno %u", (uintN) js_GetSrcNoteOffset(sn, 0));
            break;
          case SRC_FOR:
            fprintf(gOutFile, " cond %u update %u tail %u",
                   (uintN) js_GetSrcNoteOffset(sn, 0),
                   (uintN) js_GetSrcNoteOffset(sn, 1),
                   (uintN) js_GetSrcNoteOffset(sn, 2));
            break;
          case SRC_IF_ELSE:
            fprintf(gOutFile, " else %u elseif %u",
                   (uintN) js_GetSrcNoteOffset(sn, 0),
                   (uintN) js_GetSrcNoteOffset(sn, 1));
            break;
          case SRC_COND:
          case SRC_WHILE:
          case SRC_PCBASE:
          case SRC_PCDELTA:
          case SRC_DECL:
          case SRC_BRACE:
            fprintf(gOutFile, " offset %u", (uintN) js_GetSrcNoteOffset(sn, 0));
            break;
          case SRC_LABEL:
          case SRC_LABELBRACE:
          case SRC_BREAK2LABEL:
          case SRC_CONT2LABEL:
          case SRC_FUNCDEF: {
            const char *bytes;
            JSFunction *fun;
            JSString *str;

            atomIndex = (jsatomid) js_GetSrcNoteOffset(sn, 0);
            atom = js_GetAtom(cx, &script->atomMap, atomIndex);
            if (type != SRC_FUNCDEF) {
                bytes = js_AtomToPrintableString(cx, atom);
            } else {
                fun = (JSFunction *)
                    JS_GetPrivate(cx, ATOM_TO_OBJECT(atom));
                str = JS_DecompileFunction(cx, fun, JS_DONT_PRETTY_PRINT);
                bytes = str ? JS_GetStringBytes(str) : "N/A";
            }
            fprintf(gOutFile, " atom %u (%s)", (uintN)atomIndex, bytes);
            break;
          }
          case SRC_SWITCH:
            fprintf(gOutFile, " length %u", (uintN) js_GetSrcNoteOffset(sn, 0));
            caseOff = (uintN) js_GetSrcNoteOffset(sn, 1);
            if (caseOff)
                fprintf(gOutFile, " first case offset %u", caseOff);
            GetSwitchTableBounds(script, offset,
                                 &switchTableStart, &switchTableEnd);
            break;
          case SRC_CATCH:
            delta = (uintN) js_GetSrcNoteOffset(sn, 0);
            if (delta) {
                if (script->main[offset] == JSOP_LEAVEBLOCK)
                    fprintf(gOutFile, " stack depth %u", delta);
                else
                    fprintf(gOutFile, " guard delta %u", delta);
            }
            break;
          default:;
        }
        fputc('\n', gOutFile);
    }
}

static JSBool
Notes(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSScript *script;

    for (i = 0; i < argc; i++) {
        script = ValueToScript(cx, argv[i]);
        if (!script)
            continue;

        SrcNotes(cx, script);
    }
    return JS_TRUE;
}

static JSBool
TryNotes(JSContext *cx, JSScript *script)
{
    JSTryNote *tn = script->trynotes;

    if (!tn)
        return JS_TRUE;
    fprintf(gOutFile, "\nException table:\nstart\tend\tcatch\n");
    while (tn->start && tn->catchStart) {
        fprintf(gOutFile, "  %d\t%d\t%d\n",
               tn->start, tn->start + tn->length, tn->catchStart);
        tn++;
    }
    return JS_TRUE;
}

static JSBool
Disassemble(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool lines;
    uintN i;
    JSScript *script;

    if (argc > 0 &&
        JSVAL_IS_STRING(argv[0]) &&
        !strcmp(JS_GetStringBytes(JSVAL_TO_STRING(argv[0])), "-l")) {
        lines = JS_TRUE;
        argv++, argc--;
    } else {
        lines = JS_FALSE;
    }
    for (i = 0; i < argc; i++) {
        script = ValueToScript(cx, argv[i]);
        if (!script)
            return JS_FALSE;

        if (VALUE_IS_FUNCTION(cx, argv[i])) {
            JSFunction *fun = JS_ValueToFunction(cx, argv[i]);
            if (fun && (fun->flags & JSFUN_FLAGS_MASK)) {
                uint16 flags = fun->flags;
                fputs("flags:", stdout);

#define SHOW_FLAG(flag) if (flags & JSFUN_##flag) fputs(" " #flag, stdout);

                SHOW_FLAG(LAMBDA);
                SHOW_FLAG(SETTER);
                SHOW_FLAG(GETTER);
                SHOW_FLAG(BOUND_METHOD);
                SHOW_FLAG(HEAVYWEIGHT);
                SHOW_FLAG(THISP_STRING);
                SHOW_FLAG(THISP_NUMBER);
                SHOW_FLAG(THISP_BOOLEAN);
                SHOW_FLAG(INTERPRETED);

#undef SHOW_FLAG
                putchar('\n');
            }
        }

        if (!js_Disassemble(cx, script, lines, stdout))
            return JS_FALSE;
        SrcNotes(cx, script);
        TryNotes(cx, script);
    }
    return JS_TRUE;
}

static JSBool
DisassWithSrc(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
#define LINE_BUF_LEN 512
    uintN i, len, line1, line2, bupline;
    JSScript *script;
    FILE *file;
    char linebuf[LINE_BUF_LEN];
    jsbytecode *pc, *end;
    static char sep[] = ";-------------------------";

    for (i = 0; i < argc; i++) {
        script = ValueToScript(cx, argv[i]);
        if (!script)
            return JS_FALSE;

        if (!script || !script->filename) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                                            JSSMSG_FILE_SCRIPTS_ONLY);
            return JS_FALSE;
        }

        file = fopen(script->filename, "r");
        if (!file) {
            JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                            JSSMSG_CANT_OPEN,
                            script->filename, strerror(errno));
            return JS_FALSE;
        }

        pc = script->code;
        end = pc + script->length;

        /* burn the leading lines */
        line2 = JS_PCToLineNumber(cx, script, pc);
        for (line1 = 0; line1 < line2 - 1; line1++)
            fgets(linebuf, LINE_BUF_LEN, file);

        bupline = 0;
        while (pc < end) {
            line2 = JS_PCToLineNumber(cx, script, pc);

            if (line2 < line1) {
                if (bupline != line2) {
                    bupline = line2;
                    fprintf(gOutFile, "%s %3u: BACKUP\n", sep, line2);
                }
            } else {
                if (bupline && line1 == line2)
                    fprintf(gOutFile, "%s %3u: RESTORE\n", sep, line2);
                bupline = 0;
                while (line1 < line2) {
                    if (!fgets(linebuf, LINE_BUF_LEN, file)) {
                        JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL,
                                             JSSMSG_UNEXPECTED_EOF,
                                             script->filename);
                        goto bail;
                    }
                    line1++;
                    fprintf(gOutFile, "%s %3u: %s", sep, line1, linebuf);
                }
            }

            len = js_Disassemble1(cx, script, pc,
                                  PTRDIFF(pc, script->code, jsbytecode),
                                  JS_TRUE, stdout);
            if (!len)
                return JS_FALSE;
            pc += len;
        }

      bail:
        fclose(file);
    }
    return JS_TRUE;
#undef LINE_BUF_LEN
}

static JSBool
Tracing(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool bval;
    JSString *str;

    if (argc == 0) {
        *rval = BOOLEAN_TO_JSVAL(cx->tracefp != 0);
        return JS_TRUE;
    }

    switch (JS_TypeOfValue(cx, argv[0])) {
      case JSTYPE_NUMBER:
        bval = JSVAL_IS_INT(argv[0])
               ? JSVAL_TO_INT(argv[0])
               : (jsint) *JSVAL_TO_DOUBLE(argv[0]);
        break;
      case JSTYPE_BOOLEAN:
        bval = JSVAL_TO_BOOLEAN(argv[0]);
        break;
      default:
        str = JS_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        fprintf(gErrFile, "tracing: illegal argument %s\n",
                JS_GetStringBytes(str));
        return JS_TRUE;
    }
    cx->tracefp = bval ? stderr : NULL;
    return JS_TRUE;
}

typedef struct DumpAtomArgs {
    JSContext   *cx;
    FILE        *fp;
} DumpAtomArgs;

static int
DumpAtom(JSHashEntry *he, int i, void *arg)
{
    DumpAtomArgs *args = (DumpAtomArgs *)arg;
    FILE *fp = args->fp;
    JSAtom *atom = (JSAtom *)he;

    fprintf(fp, "%3d %08x %5lu ",
            i, (uintN)he->keyHash, (unsigned long)atom->number);
    if (ATOM_IS_STRING(atom))
        fprintf(fp, "\"%s\"\n", js_AtomToPrintableString(args->cx, atom));
    else if (ATOM_IS_INT(atom))
        fprintf(fp, "%ld\n", (long)ATOM_TO_INT(atom));
    else
        fprintf(fp, "%.16g\n", *ATOM_TO_DOUBLE(atom));
    return HT_ENUMERATE_NEXT;
}

static void
DumpScope(JSContext *cx, JSObject *obj, FILE *fp)
{
    uintN i;
    JSScope *scope;
    JSScopeProperty *sprop;

    i = 0;
    scope = OBJ_SCOPE(obj);
    for (sprop = SCOPE_LAST_PROP(scope); sprop; sprop = sprop->parent) {
        if (SCOPE_HAD_MIDDLE_DELETE(scope) && !SCOPE_HAS_PROPERTY(scope, sprop))
            continue;
        fprintf(fp, "%3u %p", i, (void *)sprop);
        if (JSID_IS_INT(sprop->id)) {
            fprintf(fp, " [%ld]", (long)JSVAL_TO_INT(sprop->id));
        } else if (JSID_IS_ATOM(sprop->id)) {
            JSAtom *atom = JSID_TO_ATOM(sprop->id);
            fprintf(fp, " \"%s\"", js_AtomToPrintableString(cx, atom));
        } else {
            jsval v = OBJECT_TO_JSVAL(JSID_TO_OBJECT(sprop->id));
            fprintf(fp, " \"%s\"", js_ValueToPrintableString(cx, v));
        }

#define DUMP_ATTR(name) if (sprop->attrs & JSPROP_##name) fputs(" " #name, fp)
        DUMP_ATTR(ENUMERATE);
        DUMP_ATTR(READONLY);
        DUMP_ATTR(PERMANENT);
        DUMP_ATTR(EXPORTED);
        DUMP_ATTR(GETTER);
        DUMP_ATTR(SETTER);
#undef  DUMP_ATTR

        fprintf(fp, " slot %lu flags %x shortid %d\n",
                (unsigned long)sprop->slot, sprop->flags, sprop->shortid);
    }
}

static JSBool
DumpStats(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *bytes;
    JSAtom *atom;
    JSObject *obj2;
    JSProperty *prop;
    jsval value;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        bytes = JS_GetStringBytes(str);
        if (strcmp(bytes, "arena") == 0) {
#ifdef JS_ARENAMETER
            JS_DumpArenaStats(stdout);
#endif
        } else if (strcmp(bytes, "atom") == 0) {
            DumpAtomArgs args;

            fprintf(gOutFile, "\natom table contents:\n");
            args.cx = cx;
            args.fp = stdout;
            JS_HashTableEnumerateEntries(cx->runtime->atomState.table,
                                         DumpAtom,
                                         &args);
#ifdef HASHMETER
            JS_HashTableDumpMeter(cx->runtime->atomState.table,
                                  DumpAtom,
                                  stdout);
#endif
        } else if (strcmp(bytes, "global") == 0) {
            DumpScope(cx, cx->globalObject, stdout);
        } else {
            atom = js_Atomize(cx, bytes, JS_GetStringLength(str), 0);
            if (!atom)
                return JS_FALSE;
            if (!js_FindProperty(cx, ATOM_TO_JSID(atom), &obj, &obj2, &prop))
                return JS_FALSE;
            if (prop) {
                OBJ_DROP_PROPERTY(cx, obj2, prop);
                if (!OBJ_GET_PROPERTY(cx, obj, ATOM_TO_JSID(atom), &value))
                    return JS_FALSE;
            }
            if (!prop || !JSVAL_IS_OBJECT(value)) {
                fprintf(gErrFile, "js: invalid stats argument %s\n",
                        bytes);
                continue;
            }
            obj = JSVAL_TO_OBJECT(value);
            if (obj)
                DumpScope(cx, obj, stdout);
        }
    }
    return JS_TRUE;
}

#endif /* DEBUG */

#ifdef TEST_EXPORT
static JSBool
DoExport(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSAtom *atom;
    JSObject *obj2;
    JSProperty *prop;
    JSBool ok;
    uintN attrs;

    if (argc != 2) {
        JS_ReportErrorNumber(cx, my_GetErrorMessage, NULL, JSSMSG_DOEXP_USAGE);
        return JS_FALSE;
    }
    if (!JS_ValueToObject(cx, argv[0], &obj))
        return JS_FALSE;
    argv[0] = OBJECT_TO_JSVAL(obj);
    atom = js_ValueToStringAtom(cx, argv[1]);
    if (!atom)
        return JS_FALSE;
    if (!OBJ_LOOKUP_PROPERTY(cx, obj, ATOM_TO_JSID(atom), &obj2, &prop))
        return JS_FALSE;
    if (!prop) {
        ok = OBJ_DEFINE_PROPERTY(cx, obj, id, JSVAL_VOID, NULL, NULL,
                                 JSPROP_EXPORTED, NULL);
    } else {
        ok = OBJ_GET_ATTRIBUTES(cx, obj, ATOM_TO_JSID(atom), prop, &attrs);
        if (ok) {
            attrs |= JSPROP_EXPORTED;
            ok = OBJ_SET_ATTRIBUTES(cx, obj, ATOM_TO_JSID(atom), prop, &attrs);
        }
        OBJ_DROP_PROPERTY(cx, obj2, prop);
    }
    return ok;
}
#endif

#ifdef TEST_CVTARGS
#include <ctype.h>

static const char *
EscapeWideString(jschar *w)
{
    static char enuf[80];
    static char hex[] = "0123456789abcdef";
    jschar u;
    unsigned char b, c;
    int i, j;

    if (!w)
        return "";
    for (i = j = 0; i < sizeof enuf - 1; i++, j++) {
        u = w[j];
        if (u == 0)
            break;
        b = (unsigned char)(u >> 8);
        c = (unsigned char)(u);
        if (b) {
            if (i >= sizeof enuf - 6)
                break;
            enuf[i++] = '\\';
            enuf[i++] = 'u';
            enuf[i++] = hex[b >> 4];
            enuf[i++] = hex[b & 15];
            enuf[i++] = hex[c >> 4];
            enuf[i] = hex[c & 15];
        } else if (!isprint(c)) {
            if (i >= sizeof enuf - 4)
                break;
            enuf[i++] = '\\';
            enuf[i++] = 'x';
            enuf[i++] = hex[c >> 4];
            enuf[i] = hex[c & 15];
        } else {
            enuf[i] = (char)c;
        }
    }
    enuf[i] = 0;
    return enuf;
}

#include <stdarg.h>

static JSBool
ZZ_formatter(JSContext *cx, const char *format, JSBool fromJS, jsval **vpp,
             va_list *app)
{
    jsval *vp;
    va_list ap;
    jsdouble re, im;

    printf("entering ZZ_formatter");
    vp = *vpp;
    ap = *app;
    if (fromJS) {
        if (!JS_ValueToNumber(cx, vp[0], &re))
            return JS_FALSE;
        if (!JS_ValueToNumber(cx, vp[1], &im))
            return JS_FALSE;
        *va_arg(ap, jsdouble *) = re;
        *va_arg(ap, jsdouble *) = im;
    } else {
        re = va_arg(ap, jsdouble);
        im = va_arg(ap, jsdouble);
        if (!JS_NewNumberValue(cx, re, &vp[0]))
            return JS_FALSE;
        if (!JS_NewNumberValue(cx, im, &vp[1]))
            return JS_FALSE;
    }
    *vpp = vp + 2;
    *app = ap;
    printf("leaving ZZ_formatter");
    return JS_TRUE;
}

static JSBool
ConvertArgs(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSBool b = JS_FALSE;
    jschar c = 0;
    int32 i = 0, j = 0;
    uint32 u = 0;
    jsdouble d = 0, I = 0, re = 0, im = 0;
    char *s = NULL;
    JSString *str = NULL;
    jschar *w = NULL;
    JSObject *obj2 = NULL;
    JSFunction *fun = NULL;
    jsval v = JSVAL_VOID;
    JSBool ok;

    if (!JS_AddArgumentFormatter(cx, "ZZ", ZZ_formatter))
        return JS_FALSE;;
    ok = JS_ConvertArguments(cx, argc, argv, "b/ciujdIsSWofvZZ*",
                             &b, &c, &i, &u, &j, &d, &I, &s, &str, &w, &obj2,
                             &fun, &v, &re, &im);
    JS_RemoveArgumentFormatter(cx, "ZZ");
    if (!ok)
        return JS_FALSE;
    fprintf(gOutFile,
            "b %u, c %x (%c), i %ld, u %lu, j %ld\n",
            b, c, (char)c, i, u, j);
    fprintf(gOutFile,
            "d %g, I %g, s %s, S %s, W %s, obj %s, fun %s\n"
            "v %s, re %g, im %g\n",
            d, I, s, str ? JS_GetStringBytes(str) : "", EscapeWideString(w),
            JS_GetStringBytes(JS_ValueToString(cx, OBJECT_TO_JSVAL(obj2))),
            fun ? JS_GetStringBytes(JS_DecompileFunction(cx, fun, 4)) : "",
            JS_GetStringBytes(JS_ValueToString(cx, v)), re, im);
    return JS_TRUE;
}
#endif

static JSBool
BuildDate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    char version[20] = "\n";
#if JS_VERSION < 150
    sprintf(version, " for version %d\n", JS_VERSION);
#endif
    fprintf(gOutFile, "built on %s at %s%s", __DATE__, __TIME__, version);
    return JS_TRUE;
}

static JSBool
Clear(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    if (argc != 0 && !JS_ValueToObject(cx, argv[0], &obj))
        return JS_FALSE;
    JS_ClearScope(cx, obj);
    return JS_TRUE;
}

static JSBool
Intern(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;

    str = JS_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    if (!JS_InternUCStringN(cx, JS_GetStringChars(str),
                                JS_GetStringLength(str))) {
        return JS_FALSE;
    }
    return JS_TRUE;
}

static JSBool
Clone(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSFunction *fun;
    JSObject *funobj, *parent, *clone;

    fun = JS_ValueToFunction(cx, argv[0]);
    if (!fun)
        return JS_FALSE;
    funobj = JS_GetFunctionObject(fun);
    if (argc > 1) {
        if (!JS_ValueToObject(cx, argv[1], &parent))
            return JS_FALSE;
    } else {
        parent = JS_GetParent(cx, funobj);
    }
    clone = JS_CloneFunctionObject(cx, funobj, parent);
    if (!clone)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(clone);
    return JS_TRUE;
}

static JSBool
Seal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *target;
    JSBool deep = JS_FALSE;

    if (!JS_ConvertArguments(cx, argc, argv, "o/b", &target, &deep))
        return JS_FALSE;
    if (!target)
        return JS_TRUE;
    return JS_SealObject(cx, target, deep);
}

static JSBool
GetPDA(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSObject *vobj, *aobj, *pdobj;
    JSBool ok;
    JSPropertyDescArray pda;
    JSPropertyDesc *pd;
    uint32 i;
    jsval v;

    if (!JS_ValueToObject(cx, argv[0], &vobj))
        return JS_FALSE;
    if (!vobj)
        return JS_TRUE;

    aobj = JS_NewArrayObject(cx, 0, NULL);
    if (!aobj)
        return JS_FALSE;
    *rval = OBJECT_TO_JSVAL(aobj);

    ok = JS_GetPropertyDescArray(cx, vobj, &pda);
    if (!ok)
        return JS_FALSE;
    pd = pda.array;
    for (i = 0; i < pda.length; i++) {
        pdobj = JS_NewObject(cx, NULL, NULL, NULL);
        if (!pdobj) {
            ok = JS_FALSE;
            break;
        }

        ok = JS_SetProperty(cx, pdobj, "id", &pd->id) &&
             JS_SetProperty(cx, pdobj, "value", &pd->value) &&
             (v = INT_TO_JSVAL(pd->flags),
              JS_SetProperty(cx, pdobj, "flags", &v)) &&
             (v = INT_TO_JSVAL(pd->slot),
              JS_SetProperty(cx, pdobj, "slot", &v)) &&
             JS_SetProperty(cx, pdobj, "alias", &pd->alias);
        if (!ok)
            break;

        v = OBJECT_TO_JSVAL(pdobj);
        ok = JS_SetElement(cx, aobj, i, &v);
        if (!ok)
            break;
    }
    JS_PutPropertyDescArray(cx, &pda);
    return ok;
}

static JSBool
GetSLX(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSScript *script;

    script = ValueToScript(cx, argv[0]);
    if (!script)
        return JS_FALSE;
    *rval = INT_TO_JSVAL(js_GetScriptLineExtent(script));
    return JS_TRUE;
}

static JSBool
ToInt32(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    int32 i;

    if (!JS_ValueToInt32(cx, argv[0], &i))
        return JS_FALSE;
    return JS_NewNumberValue(cx, i, rval);
}

static JSBool
StringsAreUtf8(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
               jsval *rval)
{
    *rval = JS_CStringsAreUTF8() ? JSVAL_TRUE : JSVAL_FALSE;
    return JS_TRUE;
}

static const char* badUtf8 = "...\xC0...";
static const char* bigUtf8 = "...\xFB\xBF\xBF\xBF\xBF...";
static const jschar badSurrogate[] = { 'A', 'B', 'C', 0xDEEE, 'D', 'E', 0 };

static JSBool
TestUtf8(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    intN mode = 1;
    jschar chars[20];
    size_t charsLength = 5;
    char bytes[20];
    size_t bytesLength = 20;
    if (argc && !JS_ValueToInt32(cx, *argv, &mode))
        return JS_FALSE;

    /* The following throw errors if compiled with UTF-8. */
    switch (mode) {
      /* mode 1: malformed UTF-8 string. */
      case 1:
        JS_NewStringCopyZ(cx, badUtf8);
        break;
      /* mode 2: big UTF-8 character. */
      case 2:
        JS_NewStringCopyZ(cx, bigUtf8);
        break;
      /* mode 3: bad surrogate character. */
      case 3:
        JS_EncodeCharacters(cx, badSurrogate, 6, bytes, &bytesLength);
        break;
      /* mode 4: use a too small buffer. */
      case 4:
        JS_DecodeBytes(cx, "1234567890", 10, chars, &charsLength);
        break;
      default:
        JS_ReportError(cx, "invalid mode parameter");
        return JS_FALSE;
    }
    return !JS_IsExceptionPending (cx);
}

static JSBool
ThrowError(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JS_ReportError(cx, "This is an error");
    return JS_FALSE;
}

#define LAZY_STANDARD_CLASSES

/* A class for easily testing the inner/outer object callbacks. */
typedef struct ComplexObject {
    JSBool isInner;
    JSObject *inner;
    JSObject *outer;
} ComplexObject;

static JSObject *
split_create_outer(JSContext *cx);

static JSObject *
split_create_inner(JSContext *cx, JSObject *outer);

static ComplexObject *
split_get_private(JSContext *cx, JSObject *obj);

JS_STATIC_DLL_CALLBACK(JSBool)
split_addProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ComplexObject *cpx;
    jsid asId;

    cpx = split_get_private(cx, obj);
    if (!cpx)
        return JS_TRUE;
    if (!cpx->isInner && cpx->inner) {
        /* Make sure to define this property on the inner object. */
        if (!JS_ValueToId(cx, *vp, &asId))
            return JS_FALSE;
        return OBJ_DEFINE_PROPERTY(cx, cpx->inner, asId, *vp, NULL, NULL,
                                   JSPROP_ENUMERATE, NULL);
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
split_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ComplexObject *cpx;

    cpx = split_get_private(cx, obj);
    if (!cpx)
        return JS_TRUE;
    if (!cpx->isInner && cpx->inner) {
        if (JSVAL_IS_STRING(id)) {
            JSString *str;

            str = JSVAL_TO_STRING(id);
            return JS_GetUCProperty(cx, cpx->inner, JS_GetStringChars(str),
                                    JS_GetStringLength(str), vp);
        }
        if (JSVAL_IS_INT(id))
            return JS_GetElement(cx, cpx->inner, JSVAL_TO_INT(id), vp);
        return JS_TRUE;
    }

    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
split_setProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ComplexObject *cpx;

    cpx = split_get_private(cx, obj);
    if (!cpx)
        return JS_TRUE;
    if (!cpx->isInner && cpx->inner) {
        if (JSVAL_IS_STRING(id)) {
            JSString *str;

            str = JSVAL_TO_STRING(id);
            return JS_SetUCProperty(cx, cpx->inner, JS_GetStringChars(str),
                                    JS_GetStringLength(str), vp);
        }
        if (JSVAL_IS_INT(id))
            return JS_SetElement(cx, cpx->inner, JSVAL_TO_INT(id), vp);
        return JS_TRUE;
    }

    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
split_delProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ComplexObject *cpx;
    jsid asId;

    cpx = split_get_private(cx, obj);
    if (!cpx)
        return JS_TRUE;
    if (!cpx->isInner && cpx->inner) {
        /* Make sure to define this property on the inner object. */
        if (!JS_ValueToId(cx, *vp, &asId))
            return JS_FALSE;
        return OBJ_DELETE_PROPERTY(cx, cpx->inner, asId, vp);
    }
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
split_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                  jsval *statep, jsid *idp)
{
    ComplexObject *cpx;
    JSObject *iterator;

    switch (enum_op) {
      case JSENUMERATE_INIT:
        cpx = JS_GetPrivate(cx, obj);

        if (!cpx->isInner && cpx->inner)
            obj = cpx->inner;

        iterator = JS_NewPropertyIterator(cx, obj);
        if (!iterator)
            return JS_FALSE;

        *statep = OBJECT_TO_JSVAL(iterator);
        if (idp)
            *idp = JSVAL_ZERO;
        break;

      case JSENUMERATE_NEXT:
        iterator = (JSObject*)JSVAL_TO_OBJECT(*statep);
        if (!JS_NextProperty(cx, iterator, idp))
            return JS_FALSE;

        if (*idp != JSVAL_VOID)
            break;
        /* Fall through. */

      case JSENUMERATE_DESTROY:
        /* Let GC at our iterator object. */
        *statep = JSVAL_NULL;
        break;
    }

    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(JSBool)
split_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                JSObject **objp)
{
    ComplexObject *cpx;

    cpx = split_get_private(cx, obj);
    if (!cpx)
        return JS_TRUE;
    if (!cpx->isInner && cpx->inner) {
        jsid asId;
        JSProperty *prop;

        if (!JS_ValueToId(cx, id, &asId))
            return JS_FALSE;

        if (!OBJ_LOOKUP_PROPERTY(cx, cpx->inner, asId, objp, &prop))
            return JS_FALSE;
        if (prop)
            OBJ_DROP_PROPERTY(cx, cpx->inner, prop);

        return JS_TRUE;
    }

#ifdef LAZY_STANDARD_CLASSES
    if (!(flags & JSRESOLVE_ASSIGNING)) {
        JSBool resolved;

        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return JS_FALSE;

        if (resolved) {
            *objp = obj;
            return JS_TRUE;
        }
    }
#endif

    /* XXX For additional realism, let's resolve some random property here. */
    return JS_TRUE;
}

JS_STATIC_DLL_CALLBACK(void)
split_finalize(JSContext *cx, JSObject *obj)
{
    JS_free(cx, JS_GetPrivate(cx, obj));
}

JS_STATIC_DLL_CALLBACK(uint32)
split_mark(JSContext *cx, JSObject *obj, void *arg)
{
    ComplexObject *cpx;

    cpx = JS_GetPrivate(cx, obj);

    if (!cpx->isInner && cpx->inner) {
        /* Mark the inner object. */
        JS_MarkGCThing(cx, cpx->inner, "ComplexObject.inner", arg);
    }

    return 0;
}

JS_STATIC_DLL_CALLBACK(JSObject *)
split_outerObject(JSContext *cx, JSObject *obj)
{
    ComplexObject *cpx;

    cpx = JS_GetPrivate(cx, obj);
    return cpx->isInner ? cpx->outer : obj;
}

JS_STATIC_DLL_CALLBACK(JSObject *)
split_innerObject(JSContext *cx, JSObject *obj)
{
    ComplexObject *cpx;

    cpx = JS_GetPrivate(cx, obj);
    return !cpx->isInner ? cpx->inner : obj;
}

static JSExtendedClass split_global_class = {
    {"split_global",
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE | JSCLASS_IS_EXTENDED,
    split_addProperty, split_delProperty,
    split_getProperty, split_setProperty,
    (JSEnumerateOp)split_enumerate,
    (JSResolveOp)split_resolve,
    JS_ConvertStub, split_finalize,
    NULL, NULL, NULL, NULL, NULL, NULL,
    split_mark, NULL},
    NULL, split_outerObject, split_innerObject,
    NULL, NULL, NULL, NULL, NULL
};

JSObject *
split_create_outer(JSContext *cx)
{
    ComplexObject *cpx;
    JSObject *obj;

    cpx = JS_malloc(cx, sizeof *obj);
    if (!cpx)
        return NULL;
    cpx->outer = NULL;
    cpx->inner = NULL;
    cpx->isInner = JS_FALSE;

    obj = JS_NewObject(cx, &split_global_class.base, NULL, NULL);
    if (!obj) {
        JS_free(cx, cpx);
        return NULL;
    }

    JS_ASSERT(!JS_GetParent(cx, obj));
    if (!JS_SetPrivate(cx, obj, cpx)) {
        JS_free(cx, cpx);
        return NULL;
    }

    return obj;
}

static JSObject *
split_create_inner(JSContext *cx, JSObject *outer)
{
    ComplexObject *cpx, *outercpx;
    JSObject *obj;

    JS_ASSERT(JS_GET_CLASS(cx, outer) == &split_global_class.base);

    cpx = JS_malloc(cx, sizeof *cpx);
    if (!cpx)
        return NULL;
    cpx->outer = outer;
    cpx->inner = NULL;
    cpx->isInner = JS_TRUE;

    obj = JS_NewObject(cx, &split_global_class.base, NULL, NULL);
    if (!obj || !JS_SetParent(cx, obj, NULL) || !JS_SetPrivate(cx, obj, cpx)) {
        JS_free(cx, cpx);
        return NULL;
    }

    outercpx = JS_GetPrivate(cx, outer);
    outercpx->inner = obj;

    return obj;
}

static ComplexObject *
split_get_private(JSContext *cx, JSObject *obj)
{
    do {
        if (JS_GET_CLASS(cx, obj) == &split_global_class.base)
            return JS_GetPrivate(cx, obj);
        obj = JS_GetParent(cx, obj);
    } while (obj);

    return NULL;
}

static JSBool
sandbox_enumerate(JSContext *cx, JSObject *obj)
{
    jsval v;
    JSBool b;

    if (!JS_GetProperty(cx, obj, "lazy", &v) || !JS_ValueToBoolean(cx, v, &b))
        return JS_FALSE;
    return !b || JS_EnumerateStandardClasses(cx, obj);
}

static JSBool
sandbox_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
                JSObject **objp)
{
    jsval v;
    JSBool b, resolved;

    if (!JS_GetProperty(cx, obj, "lazy", &v) || !JS_ValueToBoolean(cx, v, &b))
        return JS_FALSE;
    if (b && (flags & JSRESOLVE_ASSIGNING) == 0) {
        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return JS_FALSE;
        if (resolved) {
            *objp = obj;
            return JS_TRUE;
        }
    }
    *objp = NULL;
    return JS_TRUE;
}

static JSClass sandbox_class = {
    "sandbox",
    JSCLASS_NEW_RESOLVE,
    JS_PropertyStub,   JS_PropertyStub,
    JS_PropertyStub,   JS_PropertyStub,
    sandbox_enumerate, (JSResolveOp)sandbox_resolve,
    JS_ConvertStub,    JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool
EvalInContext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
              jsval *rval)
{
    JSString *str;
    JSObject *sobj;
    JSContext *scx;
    const jschar *src;
    size_t srclen;
    JSBool lazy, ok;
    jsval v;
    JSStackFrame *fp;

    sobj = NULL;
    if (!JS_ConvertArguments(cx, argc, argv, "S / o", &str, &sobj))
        return JS_FALSE;

    scx = JS_NewContext(JS_GetRuntime(cx), gStackChunkSize);
    if (!scx) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    src = JS_GetStringChars(str);
    srclen = JS_GetStringLength(str);
    lazy = JS_FALSE;
    if (srclen == 4 &&
        src[0] == 'l' && src[1] == 'a' && src[2] == 'z' && src[3] == 'y') {
        lazy = JS_TRUE;
        srclen = 0;
    }

    if (!sobj) {
        sobj = JS_NewObject(scx, &sandbox_class, NULL, NULL);
        if (!sobj || (!lazy && !JS_InitStandardClasses(scx, sobj))) {
            ok = JS_FALSE;
            goto out;
        }
        v = BOOLEAN_TO_JSVAL(v);
        ok = JS_SetProperty(cx, sobj, "lazy", &v);
        if (!ok)
            goto out;
    }

    if (srclen == 0) {
        *rval = OBJECT_TO_JSVAL(sobj);
        ok = JS_TRUE;
    } else {
        fp = JS_GetScriptedCaller(cx, NULL);
        ok = JS_EvaluateUCScript(scx, sobj, src, srclen,
                                 fp->script->filename,
                                 JS_PCToLineNumber(cx, fp->script, fp->pc),
                                 rval);
    }

out:
    JS_DestroyContext(scx);
    return ok;
}
#endif /* ifdef USE_ALL */

static JSFunctionSpec shell_functions[] = {
    {"version",         Version,        0,0,0},
    {"options",         Options,        0,0,0},
    {"load",            Load,           1,0,0},
    {"readline",        ReadLine,       0,0,0},
    {"print",           Print,          0,0,0},
#ifdef USE_ALL
    {"help",            Help,           0,0,0},
    {"quit",            Quit,           0,0,0},
    {"gc",              GC,             0,0,0},
    {"trap",            Trap,           3,0,0},
    {"untrap",          Untrap,         2,0,0},
    {"line2pc",         LineToPC,       0,0,0},
    {"pc2line",         PCToLine,       0,0,0},
    {"stringsAreUtf8",  StringsAreUtf8, 0,0,0},
    {"testUtf8",        TestUtf8,       1,0,0},
    {"throwError",      ThrowError,     0,0,0},
#ifdef DEBUG
    {"dis",             Disassemble,    1,0,0},
    {"dissrc",          DisassWithSrc,  1,0,0},
    {"notes",           Notes,          1,0,0},
    {"tracing",         Tracing,        0,0,0},
    {"stats",           DumpStats,      1,0,0},
#endif
#ifdef TEST_EXPORT
    {"xport",           DoExport,       2,0,0},
#endif
#ifdef TEST_CVTARGS
    {"cvtargs",         ConvertArgs,    0,0,12},
#endif
    {"build",           BuildDate,      0,0,0},
    {"clear",           Clear,          0,0,0},
    {"intern",          Intern,         1,0,0},
    {"clone",           Clone,          1,0,0},
    {"seal",            Seal,           1,0,1},
    {"getpda",          GetPDA,         1,0,0},
    {"getslx",          GetSLX,         1,0,0},
    {"toint32",         ToInt32,        1,0,0},
    {"evalcx",          EvalInContext,  1,0,0},
#endif
    {NULL,NULL,0,0,0}
};

