CORFIL *corfile_create_w(void);
CORFIL *corfile_create_r(char *text);
void corfile_putc(int c, CORFIL *f);
void corfile_puts(char *s, CORFIL *f);
void corfile_flush(CORFIL *f);
void corfile_rm(CORFIL *f);
int corfile_getc(CORFIL *f);
void corfile_ungetc(CORFIL *f);
#define corfile_ungetc(f)  (--f->p)
MYFLT corfile_get_flt(CORFIL *f);
void corfile_rewind(CORFIL *f);
#define corfile_rewind(f) (f->p=0)
int corfile_tell(CORFIL *f);
#define corfile_tell(f) (f->p)
char *cirfile_body(CORFIL *f);
#define corfile_body(f) (f->body)
CORFIL *copy_to_corefile(char *);
//CORFIL *copy_to_corefile_path(CSOUND *, char *);
int corfile_length(CORFIL *f);
#define corfile_length(f) (strlen(f->body))
