bool init_temporary_storage(TemporaryStorage* temporary_storage, u64 size) {
    temporary_storage->data = calloc(size, 1);
    temporary_storage->size = size;
    return (temporary_storage->data != 0);
}

void destroy_temporary_storage(TemporaryStorage* temporary_storage, bool verbose) {
    if (verbose) {
        printf("Destroying temporary storage\n");
    }
    free(temporary_storage->data);
}

void* allocate(TemporaryStorage* temporary_storage, u64 size) {
    void* data = {};
    if (temporary_storage->usage + size < temporary_storage->size) {
        data = (u8*)temporary_storage->data + temporary_storage->usage;
        temporary_storage->usage += size;
    } else {
        data = malloc(size);
        printf("Temporary Storage overflow\n");
        temporary_storage->outside_usage += size;
    }
    
    if (temporary_storage->outside_usage + temporary_storage->usage > temporary_storage->max_usage) {
        temporary_storage->max_usage = temporary_storage->outside_usage + temporary_storage->usage;
    }
    
    return data;
}

void* zero_allocate(TemporaryStorage* temporary_storage, u64 size) {
    void* data = allocate(temporary_storage, size);
    for (u8* d = (u8*)data;d < (u8*)data + size;++d) {
        *d = 0;
    }
    
    return data;
}

void reset(TemporaryStorage* temporary_storage) {
    temporary_storage->usage = 0;
    temporary_storage->outside_usage = 0;
}

char* to_string(TemporaryStorage to_print, TemporaryStorage* temporary_storage, u64 indentation_level){
    char* indent_space = (char*)allocate(temporary_storage, indentation_level + 1);
    for (u32 i = 0;i < indentation_level;i++) {
        indent_space[i] = ' ';
    }
    indent_space[indentation_level] = 0;
    char* str = (char*)allocate(temporary_storage, 10000);
    
    sprintf(str,
            "TemporaryStorage {\n"
            "%s    data: %p\n"
            "%s    size: %ld\n"
            "%s    usage: %ld\n"
            "%s    outside_usage: %ld\n"
            "%s    max_usage: %ld\n"
            "%s}",
            indent_space, to_print.data,
            indent_space, to_print.size,
            indent_space, to_print.usage,
            indent_space, to_print.outside_usage,
            indent_space, to_print.max_usage,
            indent_space);
    
    return str;
}