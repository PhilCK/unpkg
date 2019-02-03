
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

/* ---------------------------------------------------------------- Config -- */


#ifndef DEBUG_PRINT
#define DEBUG_PRINT 1
#endif


#if defined(__APPLE__)
const char *platform = "macOS";
#elif defined(__linux__)
const char *platform = "Linux";
#elif defined(_WIN32)
const char *platform = "Windows";
#endif


/* on launch check these */
int has_git = 0;
int has_curl = 0;
int has_tar = 0;
int has_unzip = 0;


/* ----------------------------------------------------------------- Parse -- */


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


/* --------------------------------------------------------------- Actions -- */


int
download(
        struct setpkg_data *d)
{
        printf("\n-------------------------------------[Unpkg:Download]--\n\n");

        char cmd[4096] = {0};
        int l = 0;

        /* download */
        if(!d->select) {
                const char *fmt = "curl -L %.*s\n";
                l = snprintf(cmd, sizeof(cmd), fmt, d->url_len, d->url);

                if(l < sizeof(cmd)) {
                        system(cmd);
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

                #ifndef _WIN32
                const char *fmt = "mkdir -p /tmp/pkg_c"
                        " && "
                        "curl -L %.*s -o /tmp/pkg"
                        " && "
                        "tar -xf /tmp/pkg -C /tmp/pkg_c"
                        " && "
                        "cp /tmp/pkg_c/%.*s %.*s/%.*s"
                        " && "
                        "rm -rf /tmp/pkg_c"
                        " && "
                        "rm /tmp/pkg";

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

                #else
                const char *fmt = "rmdir /Q /S C:\\temp\\pkg_c"
                        " & "
                        "mkdir C:\\temp\\pkg_c"
                        " && "
                        "curl -L %.*s -o C:\\temp\\pkg"
                        " && "
                        "tar -xf C:\\temp\\pkg -C C:\\temp\\pkg_c\\"
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

                #endif
        }

        if(DEBUG_PRINT) {
                printf("Archive %d CMD: %s\n", l, cmd);
        }

        if(l < sizeof(cmd)) {
                system(cmd);
        }

        return 1;
}


int
git_clone(
        struct setpkg_data *d)
{
        printf("\n------------------------------------------[Unpkg:Git]--\n\n");

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


int
main(
        int argc, 
        const char **argv)
{
        printf("Unpkg\n");
        fflush(stdout);
        //setvbuf(NULL, NULL, _IONBF, 0);

        /* config */
        has_git = system("git --version");
        has_curl = system("curl --version");
        has_tar = system("tar --version");
        has_unzip = system("unzifdadp --version");

        setvbuf(stdout, NULL, _IONBF, 0);
        fflush(stdout);

        printf("Has Git %d\n", has_git);
        printf("Has cURL %d\n", has_curl);
        printf("Has Tar %d\n", has_tar);
        printf("Has UnZip %d\n", has_tar);

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
        struct setpkg_data pkg = {0};

        while(1) {
                int parsed = parse_file(&n, &curr);

                /* new table or about to quit */
                /* if quitting we might have a pending table to process */
                if(n.type == AST_TABLE || (!parsed && pkg.name_len)) {
                        /* is pkg ready */
                        if(pkg.name_len && strncmp(pkg.platform, platform, pkg.platform_len) == 0) {
                                if(DEBUG_PRINT) {
                                        const char *fmt = "process tab %.*s\n";
                                        printf(fmt, pkg.name_len, pkg.name);
                                }

                                const char *type = pkg.type;
                                int len = pkg.type_len;

                                if(strncmp(type, "git", len) == 0) {
                                        git_clone(&pkg);
                                }
                                else if(strncmp(type, "archive", len) == 0) {
                                        download(&pkg);
                                }
                        }

                        memset(&pkg, 0, sizeof(pkg));

                        pkg.name = n.tab.str;
                        pkg.name_len = n.tab.len;
                }
                else if(n.type == AST_KEY_VALUE) {
                        if(strncmp(n.kv.key, "url", n.kv.key_len) == 0) {
                                pkg.url = n.kv.value;
                                pkg.url_len = n.kv.value_len;
                        }
                        else if(strncmp(n.kv.key, "type", n.kv.key_len) == 0) {
                                pkg.type = n.kv.value;
                                pkg.type_len = n.kv.value_len;
                        }
                        else if(strncmp(n.kv.key, "target", n.kv.key_len) == 0) {
                                pkg.target = n.kv.value;
                                pkg.target_len = n.kv.value_len;
                        }
                        else if(strncmp(n.kv.key, "select", n.kv.key_len) == 0) {
                                pkg.select = n.kv.value;
                                pkg.select_len = n.kv.value_len;
                        }
                        else if(strncmp(n.kv.key, "platform", n.kv.key_len) == 0) {
                                pkg.platform = n.kv.value;
                                pkg.platform_len = n.kv.value_len;
                        }
                }

                if(!parsed) {
                        break;
                }
        }

        printf("\n-----------------------------------------[Unpkg:Done]--\n\n");

        /* clean up */
        if(DEBUG_PRINT) {
                printf("Cleanup\n");
        }

        free(buf);

        return EXIT_SUCCESS;
}
