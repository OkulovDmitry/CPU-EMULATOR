#include "bytecode.h"

//============ bytecode_read - производит полное чтение файла с байт кодом и производит его запись в Elem_t* ============
code_t* bytecode_read(const char* path, int* size)
{
    FILE* in = fopen(path, "rb");
    if (in == NULL) { perror(path); return NULL; }

    BytecodeHeader header;
    if (fread(&header, sizeof(header), 1, in) != 1)
    {
        fprintf(stderr, "%s: file is too short for a header\n", path);
        fclose(in);
        return NULL;
    }

    if (header.magic != BC_MAGIC)
    {
        fprintf(stderr, "%s: not a bytecode file (bad magic)\n", path);
        fclose(in);
        return NULL;
    }

    if (header.version != BC_VERSION)
    {
        fprintf(stderr, "%s: unsupported bytecode version %u\n", path, header.version);
        fclose(in);
        return NULL;
    }

    // проверка на размер файла, который должен совпадать с заголовком
    if (fseek(in, 0, SEEK_END) != 0) { perror(path); fclose(in); return NULL; }
    long file_size = ftell(in);
    if (file_size < 0)               { perror(path); fclose(in); return NULL; }

    uint64_t payload = (uint64_t)file_size - sizeof(header);
    if (payload % sizeof(code_t) != 0 || payload / sizeof(code_t) != header.count || header.count > INT_MAX)
    {
        fprintf(stderr, "%s: file size does not match header (count = %" PRIu32 ")\n", path, header.count);
        fclose(in);
        return NULL;
    }

    if (fseek(in, (long)sizeof(header), SEEK_SET) != 0) { perror(path); fclose(in); return NULL; }

    size_t count = (size_t)header.count;                                                      // количество считанных команд и их аргументов - размер bytecode
    code_t* bytecode = malloc((count ? count : 1) * sizeof(code_t));
    if (bytecode == NULL)
    {
        fclose(in);
        fprintf(stderr, "bytecode out of memory in file %s\n", path);
        return NULL;
    }

    if (fread(bytecode, sizeof(code_t), count, in) != count)
    {
        fprintf(stderr, "%s: error reading bytecode\n", path);
        free(bytecode);
        fclose(in);
        return NULL;
    }
    fclose(in);

    // проходимся по командам: проверка допустимости каждой команды и 
    size_t i = 0;
    while (i < count)
    {
        code_t cell = bytecode[i];
        if (!(isa_cell_ok(cell)))                                      // проверка совместимости флагов и команды
        {
            fprintf(stderr, "%s: bad opcode %" PRIu64 "\n", path, cell);
            free(bytecode);
            return NULL;
        }

        i += 1 + (size_t)operand_cells(CELL_FLAGS(cell));
        if (i > count)
        {
            fprintf(stderr, "%s: missing argument at end of file\n", path);
            free(bytecode);
            return NULL;
        }
    }

    *size = (int)count;
    return bytecode;
}
//=======================================================================================================================

//============ bytecode_write - производит запись байт кода в файл ============
int bytecode_write(const char* path, const code_t* code, const int size)
{
    FILE* out = fopen(path, "wb");
    if (out == NULL) { perror(path); return -1; }

    BytecodeHeader header = { .magic = BC_MAGIC, .version = BC_VERSION, .count = (uint64_t)size};

    if (fwrite(&header, sizeof(header), 1,            out) != 1 ||
        fwrite(code,   sizeof(code_t), (size_t)size, out) != (size_t)size)
    {
        fprintf(stderr, "error writing %s\n", path);
        fclose(out);
        return -1;
    }

    if (fclose(out) != 0) { perror(path); return -1; };
    return 0;
}
//=============================================================================

/*int bytecode_decoder(const char* path, const char* path_dec)
{
    FILE* in = fopen(path, "rb");
    if (in == NULL) { perror(path); return -1; }

    int size = 0;
    code_t bytecode = bytecode_read(in, &size);

    FILE* out = fopen(path_dec, "wb");
    if (out == NULL) { perror(path_dec); return -1; }

    if (fseek(in, 0, SEEK_END) != 0) { perror(path); fclose(in); return NULL; }
    long file_size = ftell(in);
    if (fseek(in, (long)sizeof(BytecodeHeader), SEEK_SET) != 0) { perror(path); fclose(in); return NULL; }

    fprintf(out, "%b", );
}*/