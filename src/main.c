#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "parser/parser.h"
#include "codegen/codegen.h"
#include <direct.h>
#include <errno.h>

static void ensure_directory(const char *dir)
{
    if (_mkdir(dir) != 0 && errno != EEXIST)
    {
        fprintf(stderr, "Error creating directory '%s': %s\n", dir, strerror(errno));
        exit(1); // or handle gracefully
    }
}

// WinMain wrapper (if MinGW needs it)
#ifdef _WIN32
#include <windows.h>

int main(int argc, char **argv);

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main(__argc, __argv);
}
#endif

static void print_usage(const char *prog)
{
    fprintf(stderr, "Quasar compiler - v0.0.1\n\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s                    (debug mode: compile & run built‑in test)\n", prog);
    fprintf(stderr, "  %s <source.qs>        (compile to <source>.exe)\n", prog);
    fprintf(stderr, "  %s <source.qs> -o <out.exe> (compile to specified output)\n", prog);
    fprintf(stderr, "  %s --help             (show this message)\n", prog);
}

int main(int argc, char **argv)
{
    // -----------------------------------------------------------------
    // Debug mode: no arguments → compile & run a hardcoded test
    // -----------------------------------------------------------------
    if (argc == 1)
    {
        const char *test_source = "print(42);";
        ASTNode *ast = parse_program(test_source);
        if (!ast)
        {
            fprintf(stderr, "Debug parse failed.\n");
            return 1;
        }

        // Ensure build folder exists
        ensure_directory("build");

        // Write generated C code to build/__temp_qs_output.c
        FILE *cfile = fopen("build/__temp_qs_output.c", "w");
        if (!cfile)
        {
            fprintf(stderr, "Error creating debug C file\n");
            free_ast(ast);
            return 1;
        }
        generate_code(ast, cfile);
        fclose(cfile);

        printf("\nCompiling & running debug test...\n");
        system("gcc -Wall -Wextra -std=c99 build\\__temp_qs_output.c -o build\\__temp_test.exe");
        system("build\\__temp_test.exe");

        free_ast(ast);
        return 0;
    }

    // -----------------------------------------------------------------
    // CLI mode: process arguments
    // -----------------------------------------------------------------
    if (argc == 2 && strcmp(argv[1], "--help") == 0)
    {
        print_usage(argv[0]);
        return 0;
    }

    const char *source_file = NULL;
    char *output_file = NULL;

    // Parse Args
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-o") == 0)
        {
            if (i + 1 < argc)
            {
                output_file = strdup(argv[++i]); // copy to heap
                if (!output_file)
                {
                    fprintf(stderr, "Memory error\n");
                    return 1;
                }
            }
            else
            {
                fprintf(stderr, "Error: -o requires an argument\n");
                return 1;
            }
        }
        else if (argv[i][0] == '-')
        {
            fprintf(stderr, "Unknown flag: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
        else
        {
            if (source_file)
            {
                fprintf(stderr, "Error: multiple source files specified\n");
                return 1;
            }
            source_file = argv[i];
        }
    }

    if (!source_file)
    {
        fprintf(stderr, "Error: no source file specified\n");
        print_usage(argv[0]);
        return 1;
    }

    // Default output name: replace .qs with .exe
    // Default output name: executables/<source_basename>.exe
    if (!output_file)
    {
        const char *basename = source_file;
        // Strip path if present (only use the filename part)
        const char *last_slash = strrchr(source_file, '/');
        if (last_slash)
            basename = last_slash + 1;
        const char *last_backslash = strrchr(source_file, '\\');
        if (last_backslash)
            basename = last_backslash + 1;
        // Remove .qs extension
        char *temp = strdup(basename);
        if (!temp)
        {
            fprintf(stderr, "Memory error\n");
            return 1;
        }
        char *dot = strrchr(temp, '.');
        if (dot && strcmp(dot, ".qs") == 0)
            *dot = '\0';
        // Build final path: executables/<name>.exe
        ensure_directory("executables");
        output_file = malloc(strlen(temp) + 20);
        if (!output_file)
        {
            free(temp);
            fprintf(stderr, "Memory error\n");
            return 1;
        }
        sprintf(output_file, "executables\\%s.exe", temp);
        free(temp);
    }

    // Read the entire .qs file
    FILE *in = fopen(source_file, "r");
    if (!in)
    {
        fprintf(stderr, "Error: cannot open '%s'\n", source_file);
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long len = ftell(in);
    fseek(in, 0, SEEK_SET);
    char *source = malloc(len + 1);
    if (!source)
    {
        fclose(in);
        fprintf(stderr, "Memory error\n");
        return 1;
    }
    fread(source, 1, len, in);
    source[len] = '\0';
    fclose(in);

    // Parse
    ASTNode *ast = parse_program(source);
    free(source);
    if (!ast)
    {
        fprintf(stderr, "Parsing failed.\n");
        return 1;
    }

    // Generate C code to temporary file
    ensure_directory("build");
    FILE *cfile = fopen("build/__temp_qs_output.c", "w");
    if (!cfile)
    {
        fprintf(stderr, "Error creating temp file\n");
        free_ast(ast);
        return 1;
    }
    generate_code(ast, cfile);
    fclose(cfile);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "gcc -Wall -Wextra -std=c99 build\\__temp_qs_output.c -o %s", output_file);
    int rc = system(cmd);
    if (rc != 0)
    {
        fprintf(stderr, "C compilation failed.\n");
        free_ast(ast);
        return 1;
    }

    printf("Quasar: compiled '%s' -> '%s'\n", source_file, output_file);
    free_ast(ast);
    free(output_file);
    return 0;
}