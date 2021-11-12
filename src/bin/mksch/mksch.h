/* each Keyword-Value prompt pair */
typedef struct {
    char *prompt;	/* prompt text */
    char *tip;		/* tip text */
    Widget w;		/* TF widget */
    char *str;		/* string pulled from w */
} KV;


/* gui.c */
extern KV kv[];
extern int nkv;
extern Widget msg_w;
extern String fallbacks[];
extern void mkGui (void);
extern void initGui (void);

/* mksch.c */
extern char version[];

/* msg.c */
extern void msg (char *fmt, ...);

/* obsreqest.c */
extern Widget toplevel_w;

/* sched.c */
extern void writeSched (void);
extern int testSched (void);

/* tips.c */
extern void wtip (Widget w, char *tip);
