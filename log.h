#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define INFO 1
#define DEVEL 2

FILE *log_file;
bool initialized = false;
bool log_init(){
    //open file for writing append
    log_file = fopen("log.txt", "a");
    if(log_file == NULL){
        return false;
    }
    return true;
}

void log_it(int level, const char *format, ...) {
    if( !initialized){
        initialized = log_init();
    }
    char *prefix;
    switch (level)
    {
    case INFO:
        prefix = "INFO: ";
        break;
    case DEVEL:
        prefix = "DEVEL: ";
        break;
    default:
        break;
    }
    va_list args;
    va_start(args, format);

    fprintf(log_file, "%s", prefix);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    
    fflush(log_file);
    va_end(args);

}