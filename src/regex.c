#include <stdlib.h>
#include <stdio.h>
#include <regex.h>
#include <string.h>
#include <glib-2.0/glib.h>

// gcc regex.c `pkg-config --cflags --libs glib-2.0`

GSList *regexp_lst = NULL;

typedef struct {
    regex_t *reg;
    char *str;
} reg_str_s;

regex_t * 
prepare_regex(char *regex_str) {
    regex_t *regex;
    int reti;
    
    regex = (regex_t *) malloc(sizeof(regex_t));
    if (regex == NULL) {
        fprintf(stderr, "Could not allocate memory for regex\n"); 
        return NULL;
    }
    
    /* Compile regular expression */
    reti = regcomp(regex, regex_str, 0);
    if (reti) { 
        fprintf(stderr, "Could not compile regex\n"); 
        return NULL;
    }
    
    return regex;
}

int
match_regex(regex_t *regex, char *str) {
    int reti;
    char msgbuf[100];
    
    /* Execute regular expression */
    reti = regexec(regex, str, 0, NULL, 0);
    if (!reti) {
        // puts("Match");
    } else if( reti == REG_NOMATCH ) {
        // puts("No match");
    } else {
        regerror(reti, regex, msgbuf, sizeof(msgbuf));
        fprintf(stderr, "Regex match failed: %s\n", msgbuf);
        return -1;
    }
    
    return !reti;
}

void 
free_str_reg(reg_str_s *str_reg, void *not_used) {
    free(str_reg->str);
    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(str_reg->reg);
}

void 
prepare_matching(void) {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("categories.rgxp", "r");
    if (fp == NULL) {
        perror("Could not open 'categories.rgxp'");
        exit(EXIT_FAILURE);
    }
    
    printf("Read the rgxp file\n\n");
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
        char *my_line = line;
        int sep_idx;
        char sep, type, what;
        reg_str_s *reg_str = (reg_str_s *) malloc(sizeof(reg_str_s));
        
        if (reg_str == NULL) {
            perror("Couldn't allocate memory space ...");
            break;
        }
        type = my_line[0];
        sep = my_line[1];
        what = my_line[read-1];
        my_line[read-1] = '\0'; //remove last \n
        printf("Prepare '%s'\n", my_line);
        
        my_line += 2; //remove TYPE and SET
        my_line[read-4] = '\0'; //remove last SEP
        read -= 4;
        //remove 'WHAT' ?
        
        
        sep_idx = strcspn(my_line, &sep);
        my_line[sep_idx] = '\0'; //hide second part
        
        printf("--> regex:  '%s'\n", my_line);
        reg_str->reg = prepare_regex(my_line);
        
        if (reg_str->reg == NULL) {
            perror("Couldn't prepare regex ...");
            continue;
        }
        
        my_line += sep_idx + 1; //jump to second part 
        
        reg_str->str = strdup(my_line);
        printf("--> string: '%s'\n", reg_str->str);
        regexp_lst = g_slist_prepend (regexp_lst, reg_str);
        printf("\n");
    }
    
    if (line) {
        free(line);
    }
    fclose(fp);
}

char *apply_matching(char *str) {
    GSList *current = regexp_lst ;
    int found = 0;
    reg_str_s *reg_str = NULL ;
    
    while (!found && current != NULL) {
        reg_str = (reg_str_s *) g_slist_nth_data(current, 0);
        int match = match_regex(reg_str->reg, str);
        // printf("'%s' is '%s' ?\n", str, reg_str->str);
        if (match) {
            found = 1;
            break;
        } else if (match == -1) {
            printf("FAILED :-(\n");
        }
        
        current = g_slist_next(current);
    }
    if (found) {
        return reg_str->str;
    } else {
        return "NOP";
    }
}

void free_matching(void) {
    g_slist_foreach (regexp_lst, (GFunc) free_str_reg, NULL);
}

void main() {
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    prepare_matching();
    printf("------ RUN\n");
    fp = fopen("categories.data", "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }
    
    while ((read = getline(&line, &len, fp)) != -1) {
        line[read - 1] = '\0'; //remove last \n
        printf("Q: %s\n", line);
        printf("A: %s\n", apply_matching(line));
    }
    printf("------ FINISH\n");
    if (line) {
        free(line);
    }
    fclose(fp);
    free_matching();
    exit(EXIT_SUCCESS);
}
