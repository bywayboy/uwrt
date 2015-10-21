/* Include asprintf() if not on your OS. */
#ifndef HAVE_ASPRINTF
int _asprintf(char **str, const char *fmt, ...)
{
        va_list ap;
        int ret;
        
        *str = NULL;
        va_start(ap, fmt);
        ret = _vasprintf(str, fmt, ap);
        va_end(ap);

        return ret;
}

int _snprintf(char* str, size_t size, const char* format, ...);
#endif