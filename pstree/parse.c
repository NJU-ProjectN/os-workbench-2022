#include <stdio.h>
#include <regex.h>
#include <stdlib.h>


// parse process name
// generate an array names[pid]=name
// generate an array ppid[pid]= parent pid, through function getppid()

// function get_pname()
int get_pname(/* file name*/) {
    char *src = "me:   gdm-session-wor\n";
    char *pattern = "^me:\\s*([A-Za-z0-9].*)\\s*\\n";
    regex_t *compiled = malloc(sizeof(regex_t));
    int status;

    status = regcomp(compiled, pattern, REG_EXTENDED | REG_NEWLINE);
    if (status) {
        printf("compiled failed\n");
        return status;
    }
    
    int nmatch = 2;
    regmatch_t matches[nmatch];
    status = regexec(compiled, src, nmatch, matches, 0);
    if (status) {
        printf("matche failed!\n");
    }
    for (int i = 0; i < nmatch; i++) {
        printf("%.*s", matches[i] 
    }
    return status;
}

void main() {
    get_pname();
}
