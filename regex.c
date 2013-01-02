#include <stdlib.h>
#include <stdio.h>
#include <regex.h>

int count_lines(FILE *fp) {
    int ch, number_of_lines = 0;
    
    do  {
        ch = fgetc(fp);
        if(ch == '\n') {
            number_of_lines++;
        }
    } while (ch != EOF);
    
    /* last line doesn't end with a new line! */
    /* but there has to be a line at least before the last line */
    if(ch != '\n' && number_of_lines != 0) 
        number_of_lines++;
    
    /* REWIND ! */
    
    return number_of_lines
}

void prepare_regex(char *regex_str) {
    regex_t regex;
    int reti;
    
    /* Compile regular expression */
    reti = regcomp(&regex, regex_str, 0);
    if (reti) { 
        fprintf(stderr, "Could not compile regex\n"); 
        exit(1); 
    }
    
    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(&regex);
}

void match_regex(regex_t *regex, char *str) {
    int reti;
    char msgbuf[100];
    
    /* Execute regular expression */
    reti = regexec(regex, str, 0, NULL, 0);
    if (!reti) {
        puts("Match");
    } else if( reti == REG_NOMATCH ) {
        puts("No match");
    } else {
        regerror(reti, regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        exit(1);
    }
}

void free_regex(regex_t *regex) {
    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(regex);
}

void prepare_matching(void) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("categories.rgxp", "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }
    printf("Read the rgxp file\n");
    while ((read = getline(&line, &len, fp)) != -1) {
        /*
         first char:
           k --> keyword in text
           r --> regex matching the text
           e --> exact matching
         second char: 
           delimiter 
         following chars ... : 
           expression to match
         ... until delimiter.
         following chars ... :
           category to apply
         ... until delimiter.
         trailing chars:
           (empty) --> all the text fields
           l --> only the label
           d --> only the description
         */
    }
    
    if (line) {
        free(line);
    }
    fclose(fp);
}

char *apply_matching(char *str) {
    return str;
}

void free_matching(void) {
    
}

void main() {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    printf("------ INIT\n");
    prepare_matching();
    printf("------ RUN\n");
    fp = fopen("categories.data", "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }
    
    while ((read = getline(&line, &len, fp)) != -1) {
        line[read-1] = '\0';
        printf("%s", line);
        printf(" --> %s\n", apply_matching(line));
    }
    printf("------ FINISH\n");
    if (line) {
        free(line);
    }
    fclose(fp);
    free_matching();
    exit(EXIT_SUCCESS);
}
