#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define opt(short,long,x) (!strcmp(short, argv[x]) || \
                           !strcmp(long, argv[x]))
#define MAX_POSTS 1010
#define MAX_POSTNUM 15

typedef struct
{
    char pn[MAX_POSTNUM];
    int so, eo;
    short np;
} postoff_t; 

void
help(int err)
{
    puts("Usage: 4archive [options...] <archive file>");
    puts(" -f, --file <file> \t Add posts from a file with a newline-separated list");
    puts(" -h, --help \t\t Show this help message");
    puts(" -l, --list <list> \t Add posts with a comma-separated list");
    exit(err);
}

postoff_t *
get_posts(FILE *archive)
{
    char c;
    int nelc;
    char buf[1024];
    regex_t reg_post;
    const char REG_POST[] = "<div class=\"postContainer (reply|op)Container\" id=\"pc([0-9]+)\">";
    int offset;
    int nposts;
    regmatch_t postnum[3];

    postoff_t *posts = malloc(sizeof(postoff_t) * MAX_POSTS);
    if (!posts)
        return NULL;

    regcomp(&reg_post, REG_POST, REG_EXTENDED);

    char *p_buf = buf;

    nposts = 0, nelc = 0;
    while ((c = fgetc(archive)) != EOF) {
        if (c == '<')
            nelc = 1;
        else if (!nelc)
            continue;

        if (c != '>') {
            *p_buf++ = c;
            nelc++;
            continue;
        }
        *p_buf++ = '>';
        *p_buf = '\0';

        if (!regexec(&reg_post, buf, 3, postnum, 0)) {
            for (int i = 0; postnum[2].rm_so < postnum[2].rm_eo; i++) 
                posts[nposts].pn[i] = buf[postnum[2].rm_so++];
            offset = ftell(archive) - nelc;
            posts[nposts].so = offset;
        } else if (!strcmp("</blockquote>", buf)) {
            offset = ftell(archive) + sizeof("</div></div>") - 1;
            posts[nposts].eo = offset;
            nposts++;
        }

        p_buf = buf;
        nelc = 0;
    }

    regfree(&reg_post);
    rewind(archive);

    if (!nposts) {
        free(posts);
        return NULL;
    }

    posts = realloc(posts, sizeof(postoff_t) * nposts);
    posts[0].np = nposts;

    return posts;
}

void
parse_file(FILE *flist, char *buf, int **posts_index, postoff_t *posts)
{
    char c;

    char *p_buf = buf;
    int *p_postIn = *posts_index;

    for (int nc = 0; (c = fgetc(flist)) != EOF; nc++) {
        if (c != '\n' && nc < MAX_POSTNUM) {
            *p_buf++ = c;
            continue;
        } else if (c != '\n') 
            continue;
        else if (!nc) {
            nc = -1;
            continue;
        }
        
        *p_buf = '\0';

        for (int i = 0; i < posts[0].np; i++) 
            if (!strcmp(buf, posts[i].pn)) {
                *p_postIn++ = i + 1;
                break;
            }

        p_buf = buf;
        nc = -1;
    }
    *p_postIn = 0;

    fclose(flist);
}

void
parse_list(char *list, char *buf, int **posts_index, postoff_t *posts)
{
    char *p_buf = buf;
    char *p_list = list;
    int *p_postIn = *posts_index;

    for (int nc = 0; *p_list; nc++, p_list++) {
        if (*p_list != ',' && nc < MAX_POSTNUM && *(p_list+1)) {
            *p_buf++ = *p_list;
            continue;
        } else if (*p_list != ',' && *(p_list+1))
            continue;
        else if (!*(p_list+1))
            *p_buf++ = *p_list;

        *p_buf = '\0';

        for (int i = 0; i < posts[0].np; i++) 
            if (!strcmp(buf, posts[i].pn)) {
                *p_postIn++ = i + 1;
                break;
            }

        p_buf = buf;
        nc = -1;
    }
    *p_postIn = 0;
}

typedef enum
{
    PARSE_FILE,
    PARSE_LIST
} parse_t;

int *
parse_posts(parse_t option, char *list, postoff_t *posts)
{
    FILE *flist;
    char buf[MAX_POSTNUM];
    int *posts_index;

    posts_index = malloc(sizeof(int) * posts[0].np + 1);
    if (!posts_index)
        return NULL;

    if (option == PARSE_FILE && (flist = fopen(list, "r"))) 
        parse_file(flist, buf, &posts_index, posts);
    else if (option == PARSE_LIST)
        parse_list(list, buf, &posts_index, posts);
    else
        return NULL;

    if (!posts_index[0])
        return NULL;
    
    return posts_index;
}

FILE *
top_page(FILE *archive)
{
    char c;
    int nelc;
    regex_t we;
    char buf[1024];
    const char IGNORE[] = "<hr class=\"abovePostForm\">";
    //const char W_START[] = "div id=\"danbo-s-t\" class=\"danbo-slot\"";
    const char W_START[] = "<hr class=\"desktop\" id=\"op\">";
    const char W_END[] = "<div class=\"thread\" id=\"t([0-9]+)\">" ;
    int igo, wso, weo;
    regmatch_t reg_threadid[2];
    char *threadid;
    FILE *new_archive;

    threadid = malloc(sizeof(char) * MAX_POSTNUM + 1);
    if (!threadid)
        return NULL;

    regcomp(&we, W_END, REG_EXTENDED);

    char *p_buf = buf;
    char *p_id = threadid;

    nelc = 0, igo = 0, wso = 0, weo = 0;
    while ((c = fgetc(archive)) != EOF) {
        if (c == '<')
            nelc = 1;
        else if (!nelc)
            continue;

        if (c != '>') {
            *p_buf++ = c;
            nelc++;
            continue;
        }
        *p_buf++ = '>';
        *p_buf = '\0';

        if (!strcmp(IGNORE, buf)) 
            igo = ftell(archive) - nelc;
        else if (!strcmp(W_START, buf)) 
            wso = ftell(archive) - nelc;
        else if (!regexec(&we, buf, 2, reg_threadid, 0)) {
            weo = ftell(archive);
            p_buf = buf + reg_threadid[1].rm_so;
            int i;
            for (i = 0; reg_threadid[1].rm_so++ < reg_threadid[1].rm_eo; i++)
                *p_id++ = *p_buf++;
            threadid = realloc(threadid, sizeof(char) * (i + sizeof(".new")));
            for (char *new = ".new"; *new; new++)
                *p_id++ = *new;
            *p_id = '\0';
            break;
        }

        p_buf = buf;
        nelc = 0;
    }

    regfree(&we);

    if (!(new_archive = fopen(threadid, "w"))) {
        free(threadid);
        return NULL;
    }

    free(threadid);

    rewind(archive);
    for (int nc = 0; nc < igo && (c = fgetc(archive)) != EOF; nc++)
        fputc(c, new_archive);
    fseek(archive, wso, SEEK_SET);
    for (int nc = wso; nc < weo && (c = fgetc(archive)) != EOF; nc++)
        fputc(c, new_archive);

    return new_archive;
}

int
add_posts(postoff_t *posts, int *posts_index, FILE *archive, FILE *new_archive)
{
    char c;

    for (int *p_postIn = posts_index; *p_postIn; p_postIn++) {
        *p_postIn = *p_postIn - 1;
        fseek(archive, posts[*p_postIn].so, SEEK_SET);
        for (int i = posts[*p_postIn].so; i < posts[*p_postIn].eo && (c = fgetc(archive)) != EOF; i++) 
            fputc(c, new_archive);
    }

    rewind(archive);

    return 0;
}

int
bottom_page(FILE *archive, FILE *new_archive)
{
    char c;
    int nelc;
    char buf[1024];
    const char W_START[] = "<script type=\"text/javascript\" src=\"https://s.4cdn.org/js/prettify/prettify.1057.js\">";
    _Bool write;

    char *p_buf = buf;

    nelc = 0, write = 0;
    while ((c = fgetc(archive)) != EOF) {
        if (write) {
            fputc(c, new_archive);
            continue;
        }

        if (c == '<')
            nelc = 1;
        else if (!nelc)
            continue;

        if (c != '>') {
            *p_buf++ = c;
            nelc++;
            continue;
        }
        *p_buf++ = '>';
        *p_buf = '\0';

        if (!strcmp(W_START, buf)) {
            write = 1;
            fseek(archive, -(nelc), SEEK_CUR);
        }

        p_buf = buf;
        nelc = 0;
    }

    if (!write)
        return -1;

    rewind(archive);
    rewind(new_archive);

    return 0;
}

void
modify_archive(parse_t option, char *list, char *pathname)
{
    FILE *archive;
    int res = 0;
    char c;
    postoff_t *posts = NULL;
    int *posts_index = NULL;
    FILE *new_archive;

    if (!(archive = fopen(pathname, "r"))) {
        fprintf(stderr, "modify_archive(): fopen() archive failed: %s\n", strerror(errno));
        exit(1);
    }

    if (!(posts = get_posts(archive))) {
        fputs("get_posts() failed: No posts were found\n", stderr);
        res = 1;
        goto clean;
    }

    if (!(posts_index = parse_posts(option, list, posts))) {
        fputs("parse_posts() failed: Post not found in archive\n", stderr);
        res = 1;
        goto clean;
    }

    if (!(new_archive = top_page(archive))) {
        fprintf(stderr, "top_page() failed: %s\n", strerror(errno));
        res = 1;
        goto cleanall;
    }

    add_posts(posts, posts_index, archive, new_archive);

    if (bottom_page(archive, new_archive) == -1) { 
        fputs("bottom_page() failed: Missing HTML element", stderr);
        res = 1;
    }

 cleanall:
    fclose(new_archive);
 clean:
    fclose(archive);
    free(posts);
    free(posts_index);

    exit(res);
}


int
main(int argc, char **argv)
{
    if (argc < 2)
        help(1);

    if (opt("-f", "--file", 1)) 
        (argc == 4) ? (modify_archive(PARSE_FILE, argv[2], argv[3])) : help(1);
    else if (opt("-h", "--help", 1))
        help(0);
    else if (opt("-l", "--list", 1))
        (argc == 4) ? (modify_archive(PARSE_LIST, argv[2], argv[3])) : help(1);
    else 
        help(1);
    
    return 0;
}
