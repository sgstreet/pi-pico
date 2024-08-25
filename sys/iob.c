#include <sys/iob.h>

extern int picolibc_putc(char c, FILE *file);
extern int picolibc_getc(FILE *file);
extern int picolibc_flush(FILE *file);
extern int picolibc_close(FILE *file);

struct iob _stdio = IOB_DEV_SETUP(picolibc_putc, picolibc_getc, picolibc_flush, picolibc_close, __SRD | __SWR, 0);
struct iob _stderr = IOB_DEV_SETUP(picolibc_putc, picolibc_getc, picolibc_flush, picolibc_close, __SRD | __SWR, 0);
struct iob _stddiag = IOB_DEV_SETUP(picolibc_putc, picolibc_getc, picolibc_flush, picolibc_close, __SRD | __SWR, 0);

FILE *const stdin = (FILE *)&_stdio;
FILE *const stdout =(FILE *)&_stdio;
FILE *const stderr = (FILE *)&_stderr;
FILE *const stddiag = (FILE *)&_stddiag;

__weak int picolibc_putc(char c, FILE *file)
{
	return EOF;
}

__weak int picolibc_getc(FILE *file)
{
	return EOF;
}

__weak int picolibc_flush(FILE *file)
{
	return 0;
}

__weak int picolibc_close(FILE *file)
{
	return file->flush(file);
}

