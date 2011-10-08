#ifndef IPC_H
#define IPC_H

#include "filesystem/fsmanager.h"


typedef enum
{
    IPC_SUCCESSFUL = 0,
    IPC_NOTENOUGHMEMORY, IPC_NOTFOUND, IPC_ACCESSDENIED, IPC_WRONGTYPE
} IPC_ERROR;

typedef enum
{
    IPC_NONE = 0,
    IPC_READ = 1, IPC_WRITE = 2, IPC_DELEGATERIGHTS = 4
} IPC_RIGHTS;

typedef enum
{
    IPC_INTEGER, IPC_FLOAT, IPC_STRING, IPC_FILE, IPC_FOLDER
} IPC_TYPE;


typedef struct
{
    uint32_t   owner;
    IPC_RIGHTS privileges;
} ipc_certificate_t;

typedef struct ipc_node
{
    char* name;
    IPC_TYPE    type;
    union
    {
        int64_t integer;
        double  floatnum;
        char*   string;
        file_t* file;
        list_t* folder;
    } data;
    uint32_t         owner;       // Owner has full access
    IPC_RIGHTS       general;     // General access rights
    list_t*          accessTable; // list of ipc_certificate_t*. Content overrides general ipc certificate
    struct ipc_node* parent;      // Points to the parent item in the tree
} ipc_node_t;


// Public interface (syscalls)
file_t*   ipc_fopen(const char* path, const char* mode); // Opens IPC file. Further access via fsmanager.
IPC_ERROR ipc_getFolder(const char* path, char*       destination, size_t length); // Writes names of all subnodes to destination, separated by the path separator '|'
IPC_ERROR ipc_getString(const char* path, char*       destination, size_t length);
IPC_ERROR ipc_setString(const char* path, const char* source);
IPC_ERROR ipc_getInt   (const char* path, int64_t*    destination);
IPC_ERROR ipc_setInt   (const char* path, int64_t*    source);
IPC_ERROR ipc_getDouble(const char* path, double*     destination);
IPC_ERROR ipc_setDouble(const char* path, double*     source);
IPC_ERROR ipc_deleteKey(const char* path);
IPC_ERROR ipc_setAccess(const char* path, IPC_RIGHTS permissions, uint32_t task); // Task: Given as PID. PID 0: Set general access rights

// Public interface (kernel)
ipc_node_t* ipc_getNode(const char* path);
IPC_ERROR   ipc_createNode(const char* path, ipc_node_t** node, IPC_TYPE type);
void        ipc_print();


#endif
