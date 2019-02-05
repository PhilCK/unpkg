

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _WIN32
#include <windows.h>
#endif


/* ------------------------------------------------------------- 3rd party -- */
/*
 *  STB's stretchy buffer.
 *  https://github.com/nothings/stb/blob/master/stretchy_buffer.h
 */

#define sb_free   stb_sb_free
#define sb_push   stb_sb_push
#define sb_count  stb_sb_count
#define sb_add    stb_sb_add
#define sb_last   stb_sb_last

#define stb_sb_free(a)         ((a) ? free(stb__sbraw(a)),0 : 0)
#define stb_sb_push(a,v)       (stb__sbmaybegrow(a,1), (a)[stb__sbn(a)++] = (v))
#define stb_sb_count(a)        ((a) ? stb__sbn(a) : 0)
#define stb_sb_add(a,n)        (stb__sbmaybegrow(a,n), stb__sbn(a)+=(n), &(a)[stb__sbn(a)-(n)])
#define stb_sb_last(a)         ((a)[stb__sbn(a)-1])

#define stb__sbraw(a) ((int *) (a) - 2)
#define stb__sbm(a)   stb__sbraw(a)[0]
#define stb__sbn(a)   stb__sbraw(a)[1]

#define stb__sbneedgrow(a,n)  ((a)==0 || stb__sbn(a)+(n) >= stb__sbm(a))
#define stb__sbmaybegrow(a,n) (stb__sbneedgrow(a,(n)) ? stb__sbgrow(a,n) : 0)
#define stb__sbgrow(a,n)      (*((void **)&(a)) = stb__sbgrowf((a), (n), sizeof(*(a))))

static void * stb__sbgrowf(void *arr, int increment, int itemsize)
{
   int dbl_cur = arr ? 2*stb__sbm(arr) : 0;
   int min_needed = stb_sb_count(arr) + increment;
   int m = dbl_cur > min_needed ? dbl_cur : min_needed;
   int *p = (int *) realloc(arr ? stb__sbraw(arr) : 0, itemsize * m + sizeof(int)*2);
   if (p) {
      if (!arr)
         p[1] = 0;
      p[0] = m;
      return p+2;
   } else {
      return (void *) (2*sizeof(int)); // try to force a NULL pointer exception later
   }
}

/* ---------------------------------------------------------------- Config -- */
/* 
 * Platform and configuration settings etc.
 */


/* DEBUG_PRINT spews out alot of text - off by default */
#ifndef DEBUG_PRINT
#define DEBUG_PRINT 0
#endif


/* wrap MS's interface under POSIX's */
#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif


/* Platform idents */
#if defined(__APPLE__)
const char *platform = "macOS";
#elif defined(__linux__)
const char *platform = "Linux";
#elif defined(_WIN32)
const char *platform = "Windows";
#endif


/* Features - on launch check these */
int has_git = 0;
int has_curl = 0;
int has_tar = 0;
int has_unzip = 0;

const char *target_tab = 0;


/* ----------------------------------------------------------------- Parse -- */
/*
 * Unpkg needs to parse a TOML file, these are the helpers todo that.
 */

int
is_whitespace(char c) {
        if(c == '\n') { return 1; }
        if(c == '\t') { return 1; }
        if(c == ' ')  { return 1; }
        
        return 0;
}


int
is_eof(char c) {
        return c == '\0';
}


int
is_eol(char c) {
        return c == '\n';
}


int
is_alpha(char c) {
        if(c >= 97 && c <= 122) { return 1; }
        if(c >= 65 && c <= 90) { return 1; }

        return 0;
}


int
contains_key_value(const char *str) {
        int offset = 0;

        /* chop all whitespace */
        while(is_whitespace(str[offset])) { ++offset; }

        /* starts with alpha */
        if(!is_alpha(str[offset])) { return 0; }
        offset += 1;

        while(str[offset] == 0) {
                offset += 1;
        }

        return 1;
}


int
starts_with_char(const char *str, char c) {
        int offset = 0;

        while(str[offset] != c && str[offset] != '\n') {
                offset += 1;
        }

        return str[offset] == c ? 1 : 0;  
}


int
contains_char(const char *str, char c) {
        int offset = 0;

        while(str[offset] != c && str[offset] != '\n') {
                offset += 1;
        }

        return str[offset] == c ? 1 : 0;
}


int
starts_with_alpha(const char *str) {
        int offset = 0;

        while(str[offset] != '\n') {
                if(is_whitespace(str[offset])) {
                        continue;
                }

                if(is_alpha(str[offset])) {
                        return 1;
                }
        }

        return 0;
};


struct ast_table {
        const char *str;
        int len;
};


struct ast_key_value {
        const char *key;
        int key_len;

        const char *value;
        int value_len;
};


typedef enum ast_node_type {
        AST_NONE,
        AST_TABLE,
        AST_KEY_VALUE,
} ast_node_type;


struct ast_node {
        ast_node_type type;

        union {
                struct ast_table tab;
                struct ast_key_value kv;
        };
};


struct setpkg_data {
        const char *name;
        int name_len;

        const char *url;
        int url_len;

        const char *type;
        int type_len;

        const char *target;
        int target_len;

        const char *select;
        int select_len;

        const char *platform;
        int platform_len;
};


int
parse_file(
        struct ast_node *n,
        char **b)
{
        const char *curr = *b;
        int i = 0;

        struct ast_node node = {0};
        node.type = AST_NONE;

        /* comment */
        if(is_eof(curr[i])) {
                if(DEBUG_PRINT) {
                        printf("EOF\n");
                }

                return 0;
        }
        if(starts_with_char(curr, '#')) {
                if(DEBUG_PRINT) {
                        printf("Comment\n");
                }

                /* swallow line */
                while(!is_eol(curr[i])) { ++i; }
        }
        /* table */
        else if(starts_with_char(curr, '[')) {
                if(DEBUG_PRINT) {
                        printf("Table - ");
                }

                node.type = AST_TABLE;

                while(curr[i] != '[') { ++i; }
                ++i;

                node.tab.str = &curr[i];
                int len = 0;

                while(curr[i] != ']') { ++i; ++len; }
                node.tab.len = len;

                /* swallow line */
                while(!is_eol(curr[i])) { ++i; }

                if(DEBUG_PRINT) {
                        int str_len = node.tab.len;
                        const char *str = node.tab.str;

                        printf("%.*s\n", str_len, str);
                }
        }
        /* key value */
        else if(starts_with_alpha(curr) && contains_char(curr, '=')) {
                if(DEBUG_PRINT) {
                        printf("KV Pair - ");
                }

                node.type = AST_KEY_VALUE;

                /* key */
                /* chomp whitespace */
                while(is_whitespace(curr[i])) { ++i; }

                node.kv.key = &curr[i];

                int len = 0;
                while(!is_whitespace(curr[i])) { ++i; ++len; }

                node.kv.key_len = len;

                if(DEBUG_PRINT) {
                        printf("Key:%.*s, ", len, node.kv.key);
                }

                /* value */
                while(curr[i] != '"') { ++i; }
                ++i;

                len = 0;
                node.kv.value = &curr[i];

                while(curr[i] != '"') { ++i; ++len; }

                node.kv.value_len = len;

                if(DEBUG_PRINT) {
                        printf("Value:%.*s\n", len, node.kv.value);
                }

                /* swallow line */
                while(!is_eol(curr[i])) { ++i; }
        }
        else {
                if(DEBUG_PRINT) {
                        printf("Chomp!\n");
                }
        }
        
        *n = node;

        /* swallow line */
        while(!is_eol(curr[i])) { ++i; }

        *b += (i + 1);

        /* find name */

        return 1;

        // return is_eof(curr[i]) ? 0 : 1;
}


/* -------------------------------------------------------------- Commands -- */
/*
 * The command line interactions.
 */

int
cmd(const char * cmd) {
        FILE *pipe = popen(cmd, "r");
        int success = 0;

        char buf[1024] = {0};

        if(pipe) {
                while(fgets(buf, sizeof(buf), pipe) != NULL) {
                        success = 1;
                }

                pclose(pipe);
        }

        return success;
};


int
cmd_mkdir_tmp() {
        char buf[4096] = {0};
        snprintf(
                buf,
                sizeof(buf),
                #ifndef _WIN32
                "mkdir -v -p /tmp/unpkg"
                #else
                "mkdir C:\\temp\\unpkg"
                #endif
                );

        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };

        return ret;
}


int
cmd_curl(const char *url, int url_len) {
        char buf[4096] = {0};

        snprintf(
                buf,
                sizeof(buf),
                "curl -L %.*s",
                url_len,
                url);

        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };

        return ret;
}


int
cmd_curl_tmp(const char *url, int url_len) {
        char buf[4096] = {0};

        snprintf(
                buf,
                sizeof(buf),
                #ifndef _WIN32
                "curl -L %.*s -o /tmp/unpkg_f && echo 1",
                #else
                "curl -L %.*s -o C:\\temp\\unpkg_f && echo 1",
                #endif
                url_len,
                url);

        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };

        return ret;
}


int
cmd_tar_tmp() {
        char buf[4096] = {0};

        snprintf(
                buf,
                sizeof(buf),
                #ifndef _WIN32
                "tar -v -xf /tmp/unpkg_f -C /tmp/unpkg"
                #else
                "tar -xf C:\\temp\\unpkg_f -C C:\\temp\\unpkg\\"
                #endif
                );

        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };
        return ret;
}




int
cmd_cp(const char *src, int src_len, const char *dst_dir, int dst_dir_len) {
        char buf[4096] = {0};

        snprintf(
                buf,
                sizeof(buf),
                #ifndef _WIN32
                "cp -v /tmp/unpkg/%.*s %.*s/%.*s",
                #else
                "copy C:\\temp\\unpkg\\%.*s %.*s\\%.*s",
                #endif
                src_len,
                src,
                dst_dir_len,
                dst_dir,
                src_len,
                src);

        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };
        return ret;
}


int
cmd_rm_tmp_dir() {
        char buf[4096] = {0};
        snprintf(buf, sizeof(buf), "rm -rf -v /tmp/unpkg");
        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };
        return ret; 
}


int
cmd_rm_tmp_file() {
        char buf[4096] = {0};
        snprintf(buf, sizeof(buf), "rm -v /tmp/unpkg_f");
        int ret = cmd(buf);
        if(DEBUG_PRINT) {
                printf("CMD %d: %s\n", ret, buf);
        };
        return ret; 
}


/* --------------------------------------------------------------- Actions -- */
/*
 * Features of Unpkg, these are the actions that need to be performed from a
 * unpkg.toml file.
 */ 

int
download(
        struct setpkg_data *d)
{
        printf("\n--[Unpkg:Download]--\n\n");

        printf("Downloading %.*s\n", d->name_len, d->name);

        /* download */
        if(!d->select) {
                return cmd_curl(d->url, d->url_len);  
        }
        else {
                if(!d->target) {
                        #ifndef _WIN32
                        d->target = "./";
                        #else
                        d->target = ".\\";
                        #endif

                        d->target_len = strlen(d->target);
                }

                cmd_rm_tmp_dir();
                cmd_mkdir_tmp();
                cmd_curl_tmp(d->url, d->url_len);
                cmd_tar_tmp();
                cmd_cp(d->select, d->select_len, d->target, d->target_len);
                cmd_rm_tmp_dir();
                cmd_rm_tmp_file();
        }

        return 1;
}


int
git_clone(
        struct setpkg_data *d)
{
        printf("\n--[Unpkg:Git]--\n\n");

        printf("Cloning %.*s\n", d->name_len, d->name);

        if(!has_git) {
                printf("Git not installed\n");
                return 0;
        }

        char cmd[4096] = {0};
        int l;

        if(!d->select) {
                const char *fmt = "git clone %.*s";
                l = snprintf(cmd, sizeof(cmd), fmt, d->url_len, d->url);

                if(DEBUG_PRINT) {
                        printf("%s\n", cmd);
                }
        }
        else {
                if(!d->target) {
                        #ifndef _WIN32
                        d->target = "./";
                        #else
                        d->target = ".\\";
                        #endif

                        d->target_len = strlen(d->target);
                }

                #ifdef _WIN32
                const char *fmt =
                        "rmdir /Q /S C:\\temp\\pkg_c"
                        " & "
                        "mkdir C:\\temp\\pkg_c"
                        " && "
                        "git clone %.*s C:\\temp\\pkg_c"
                        " && "
                        "copy C:\\temp\\pkg_c\\%.*s %.*s"
                        " && "
                        "rmdir /Q /S C:\\temp\\pkg_c";

                l = snprintf(
                        cmd,
                        sizeof(cmd),
                        fmt,
                        d->url_len,
                        d->url,
                        d->select_len,
                        d->select,
                        d->target_len,
                        d->target);

                #else
                const char *fmt = "mkdir -p /tmp/pkg_c"
                        " && "
                        "git clone %.*s /tmp/pkg_c"
                        " && "
                        "cp /tmp/pkg_c/%.*s %.*s/%.*s"
                        " && "
                        "rm -rf /tmp/pkg_c";

                l = snprintf(
                        cmd,
                        sizeof(cmd),
                        fmt,
                        d->url_len,
                        d->url,
                        d->select_len,
                        d->select,
                        d->target_len,
                        d->target,
                        d->select_len,
                        d->select);
                #endif
        }

        if(l < sizeof(cmd)) {
                if(DEBUG_PRINT) {
                        printf("CMD: %s\n", cmd);
                }

                system(cmd);
        }

        return 1;
}


/* ----------------------------------------------------------- Application -- */


struct setpkg_data *pkgs = {0};


int
main(
        int argc, 
        const char **argv)
{
        printf("\n--[Unpkg:Env]--\n\n");

        if(DEBUG_PRINT) {
                int i;
                for(i = 0; i < argc; ++i) {
                        printf("ARG: %d. %s\n", i, argv[i]);
                }
        }

        if(argc > 1) {
                target_tab = argv[1];
        }

        /* setup */
        fflush(stdout);
        freopen("/dev/null", "w", stderr);
        setvbuf(stdout, NULL, _IONBF, 0);

        /* call help to check if programs exist */
        has_git = cmd("git --help");
        has_curl = cmd("curl --help");
        has_tar = cmd("tar --help");
        has_unzip = cmd("unzip -h");

        printf("Has Git %s\n", has_git ? "YES" : "NO");
        printf("Has cURL %s\n", has_curl ? "YES" : "NO");
        printf("Has Tar %s\n", has_tar ? "YES" : "NO");
        printf("Has UnZip %s\n", has_unzip ? "YES" : "NO");

        /* open */
        FILE *f = fopen("unpkg.toml", "rb");

        if(!f) {
                printf("No setpkg file\n");
                return EXIT_FAILURE;
        }

        /* read */
        fseek(f, 0 , SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET); 

        if(!size) {
                printf("File empty\n");
                return EXIT_FAILURE;
        }

        char *buf = (char*)malloc(sizeof(char) * size + 1);

        if(!buf) {
                printf("Failed to read file\n");
        }

        fread(buf,1,size,f);
        buf[size] = 0;
        fclose(f);

        /* parse */
        char *curr = buf;
        struct ast_node n;
        int curr_pkg = -1;

        /* parse the TOML file */
        printf("\n--[Unpkg:Parse]--\n\n");

        while(1) {
                int parsed = parse_file(&n, &curr);

                /* new table or about to quit */
                /* if quitting we might have a pending table to process */
                if(n.type == AST_TABLE) {
                        struct setpkg_data pkg = {0};
                        sb_push(pkgs, pkg);

                        curr_pkg += 1;

                        pkgs[curr_pkg].name = n.tab.str;
                        pkgs[curr_pkg].name_len = n.tab.len;
                        
                }
                else if(n.type == AST_KEY_VALUE) {
                        const char *key = n.kv.key;
                        int key_len = n.kv.key_len;

                        if(strncmp(key, "url", key_len) == 0) {
                                pkgs[curr_pkg].url = n.kv.value;
                                pkgs[curr_pkg].url_len = n.kv.value_len;
                        }
                        else if(strncmp(key, "type", key_len) == 0) {
                                pkgs[curr_pkg].type = n.kv.value;
                                pkgs[curr_pkg].type_len = n.kv.value_len;
                        }
                        else if(strncmp(key, "target", key_len) == 0) {
                                pkgs[curr_pkg].target = n.kv.value;
                                pkgs[curr_pkg].target_len = n.kv.value_len;
                        }
                        else if(strncmp(key, "select", key_len) == 0) {
                                pkgs[curr_pkg].select = n.kv.value;
                                pkgs[curr_pkg].select_len = n.kv.value_len;
                        }
                        else if(strncmp(key, "platform", key_len) == 0) {
                                pkgs[curr_pkg].platform = n.kv.value;
                                pkgs[curr_pkg].platform_len = n.kv.value_len;
                        }
                }

                if(!parsed) {
                        break;
                }
                
        }

        printf("Done\n");

        /* get the packages */
        int i;
        int count = sb_count(pkgs);

        for(i = 0; i < count; ++i) {
                
                /* if target table has an os preference */
                if(pkgs[i].platform) {
                        const char *pk_plat = pkgs[i].platform;
                        const char *th_plat = platform;

                        if(strncmp(pk_plat, th_plat, strlen(th_plat) != 0)) {
                                continue;
                        }
                }

                /* if we have a target table */
                if(target_tab) {
                        const char *th_tab = target_tab;
                        const char *pk_tab = pkgs[i].name;
                        int pk_len = pkgs[i].name_len;

                        if(strncmp(th_tab, pk_tab, pk_len) != 0) {
                                continue;
                        }
                }

                const char *type = pkgs[i].type;
                int len = pkgs[i].type_len;
                                
                if(strncmp(type, "git", len) == 0) {
                        git_clone(&pkgs[i]);
                }
                else if(strncmp(type, "archive", len) == 0) {
                        download(&pkgs[i]);
                }
        };

        printf("\n--[Unpkg:Done]--\n\n");

        /* clean up */
        if(DEBUG_PRINT) {
                printf("Cleanup\n");
        }

        free(buf);

        return EXIT_SUCCESS;
}
