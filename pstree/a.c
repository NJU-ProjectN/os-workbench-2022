// test file readin 
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void main(){
    FILE *f;
    ssize_t pos = 0;
    size_t buf = 200;
    char *line;
    int i = 0;
    
    line =(char *)malloc(sizeof(char) * buf);
    f = fopen("t.txt", "r");
    while((pos = getline(&line, &buf, f)) != -1 && i < 20) {
        printf("%s", line);
        i++;
    }
    return;
}
