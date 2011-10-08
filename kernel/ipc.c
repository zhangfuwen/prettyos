/*
 *  license and disclaimer for the use of this source code as per statement below
 *  Lizenz und Haftungsausschluss für die Verwendung dieses Sourcecodes siehe unten
 */

#include "ipc.h"
#include "task.h"
#include "util.h"
#include "kheap.h"


static ipc_node_t root = {.name        = 0, 
                          .type        = IPC_FOLDER, 
                          .data.folder = 0, 
                          .owner       = 0, 
                          .general     = IPC_READ, 
                          .accessTable = 0, 
                          .parent      = 0};

// private interface
static ipc_node_t* getNode(const char* remainingPath, ipc_node_t* node)
{
    const char* end = strpbrk(remainingPath, "/|\\");

    if (end == 0) // Final element
    {
        for (dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
        {
            ipc_node_t* newnode = e->data;
            if (strcmp(remainingPath, newnode->name) == 0)
            {
                return (newnode);
            }
        }
    }
    else
    {
        size_t sublength = end-remainingPath;
        for(dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
        {
            ipc_node_t* newnode = e->data;
            
            if (strncmp(remainingPath, newnode->name, sublength) == 0 && strlen(newnode->name) == sublength)
            {
                if (newnode->type == IPC_FOLDER)
                {
                    return (getNode(end+1, newnode)); // One layer deeper
                }
                else
                {
                    return (0); // Node with name found, but we cannot go one step deeper, because its no folder. -> Not found.
                }
            }
        }
    }

    return (0); // Not found
}

static bool accessAllowed(ipc_node_t* node, IPC_RIGHTS needed)
{
    uint32_t pid = getpid();

    if (pid == 0 || pid == node->owner) // Kernel and owner have full access
    {
        return (true);
    }

    if (node->accessTable)
    {
        for (dlelement_t* e = node->accessTable->head; e != 0; e = e->next)
        {
            ipc_certificate_t* certificate = e->data;
            if (certificate->owner == pid) // Task has a specific certificate to access this node
            {
                return ((certificate->privileges & needed) == needed);
            }
        }
    }

    return ((node->general & needed) == needed); // Use general access rights as fallback
}

static IPC_ERROR prepareNodeToWrite(ipc_node_t** node, const char* path, IPC_TYPE type)
{
    if (*node == 0)
    {
        IPC_ERROR err = createNode(path, node, type);
        if(err != IPC_SUCCESSFUL)
        {
            return (err);
        }
    }
    else if ((*node)->type != type)
    {
        return (IPC_WRONGTYPE);
    }
    else if (!accessAllowed(*node, IPC_WRITE))
    {
        return (IPC_ACCESSDENIED);
    }

    return (IPC_SUCCESSFUL);
}

static IPC_ERROR deleteNode(ipc_node_t* node)
{
    if (!accessAllowed(node, IPC_WRITE))
    {
        return (IPC_ACCESSDENIED);
    }
    if(node->type == IPC_FOLDER)
    {
        IPC_ERROR err = IPC_SUCCESSFUL;
     
        for (dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
        {
            if (deleteNode(e->data) != IPC_SUCCESSFUL)
            {
                err = IPC_ACCESSDENIED;
            }
        }
        if (err == IPC_ACCESSDENIED)
        {
            return (IPC_ACCESSDENIED);
        }
    }

    // TODO: Delete node.

    return (IPC_SUCCESSFUL);
}


// Public interface (kernel)

ipc_node_t* ipc_getNode(const char* path)
{
    return (getNode(path, &root));
}

IPC_ERROR createNode(const char* path, ipc_node_t** node, IPC_TYPE type)
{
    // TODO: Create folders up to this element (recursivly)
    // TODO: Get Parent

    // Find out nodes name. (TODO: Can this be solved by first TODO as well?)
    const char* lastbreak;
    const char* temp = path;
    
    do
    {
        lastbreak = temp;
        temp = strpbrk(temp+1, "/|\\");
    } 
    while (temp);

    *node = malloc(sizeof(ipc_node_t), 0, "ipc_node_t");
    (*node)->name = malloc(strlen(lastbreak)+1, 0, "ipc_node_t::name");
    strcpy((*node)->name, lastbreak);
    (*node)->type = type;
    (*node)->owner = getpid();
    (*node)->general = IPC_READ; // TODO: Use parents rights?
    (*node)->accessTable = 0;
    
    return (IPC_SUCCESSFUL); // TODO
}

void ipc_print(ipc_node_t* node, int layer)
{
    putch('\n');
    
    if (node->name)
    {
        printf("%s: ", node->name);
    }

    switch(node->type)
    {
        case IPC_INTEGER:
            printf("%u", node->data.integer);
            break;
        case IPC_FLOAT:
            printf("%f", node->data.floatnum);
            break;
        case IPC_STRING:
            puts(node->data.string);
            break;
        case IPC_FILE:
            printf("%s (file)", node->data.file->name);
            break;
        case IPC_FOLDER:
            for(dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
            {
                ipc_print(e->data, layer+1);
            }
            break;
    }
}


// Public interface (syscalls)

file_t* ipc_fopen(const char* path, const char* mode)
{
    return (0); // TODO
}

IPC_ERROR ipc_getFolder(const char* path, char* destination, size_t length) // TODO: Its possible to solve this more efficient. For example we currently require one byte too much as string length
{
    ipc_node_t* node = ipc_getNode(path);
    
    if (node == 0)                       return (IPC_NOTFOUND);
    if (node->type != IPC_FOLDER)        return (IPC_WRONGTYPE);
    if (!accessAllowed(node, IPC_READ))  return (IPC_ACCESSDENIED);

    // Collect length to check if destination is large enough
    size_t neededLength = 2;
    
    for (dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
    {
        ipc_node_t* child = e->data;
        neededLength += strlen(child->data.string) + 1;
    }
    
    if (neededLength > length)
    {
        if (length > sizeof(size_t))
        {
            *(size_t*)destination = neededLength;
        }

        return (IPC_NOTENOUGHMEMORY);
    }
    
    destination[0] = 0; // Clear destination
    
    for (dlelement_t* e = node->data.folder->head; e != 0; e = e->next)
    {
        ipc_node_t* child = e->data;
        strcat(destination, child->data.string);
        strcat(destination, "|");
    }

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_getString(const char* path, char* destination, size_t length)
{
    ipc_node_t* node = ipc_getNode(path);
    
    if (node == 0)                       return (IPC_NOTFOUND);
    if (node->type != IPC_STRING)        return (IPC_WRONGTYPE);
    if (!accessAllowed(node, IPC_READ))  return (IPC_ACCESSDENIED);
    
    if(strlen(node->data.string)+1 > length)
    {
        if(length > sizeof(size_t))
        {
            *(size_t*)destination = strlen(node->data.string)+1;
        }
        
        return (IPC_NOTENOUGHMEMORY);
    }

    strcpy(destination, node->data.string);

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_getInt(const char* path, int64_t* destination)
{
    ipc_node_t* node = ipc_getNode(path);
    if (node == 0)                       return (IPC_NOTFOUND);
    if (node->type != IPC_INTEGER)       return (IPC_WRONGTYPE);
    if (!accessAllowed(node, IPC_READ))  return (IPC_ACCESSDENIED);

    *destination = node->data.integer;

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_getDouble(const char* path, double* destination)
{
    ipc_node_t* node = ipc_getNode(path);
    if (node == 0)                       return (IPC_NOTFOUND);
    if (node->type != IPC_FLOAT)         return (IPC_WRONGTYPE);
    if (!accessAllowed(node, IPC_READ))  return (IPC_ACCESSDENIED);

    *destination = node->data.floatnum;

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_setString(const char* path, const char* val)
{
    ipc_node_t* node = ipc_getNode(path);

    IPC_ERROR err = prepareNodeToWrite(&node, path, IPC_STRING);
    
    if (err != IPC_SUCCESSFUL) 
    {
        return(err);
    }

    //node->data.string = realloc(node->data.string, strlen(val)+1); // TODO
    strcpy(node->data.string, val);

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_setInt(const char* path, int64_t val)
{
    ipc_node_t* node = ipc_getNode(path);

    IPC_ERROR err = prepareNodeToWrite(&node, path, IPC_INTEGER);
    
    if (err != IPC_SUCCESSFUL) return(err);

    node->data.integer = val;

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_setDouble(const char* path, double val)
{
    ipc_node_t* node = ipc_getNode(path);

    IPC_ERROR err = prepareNodeToWrite(&node, path, IPC_FLOAT);
    if (err != IPC_SUCCESSFUL) return(err);

    node->data.floatnum = val;

    return (IPC_SUCCESSFUL);
}

IPC_ERROR ipc_deleteKey(const char* path)
{
    ipc_node_t* node = ipc_getNode(path);
    
    if (node == 0)
    {
        return (IPC_NOTFOUND);
    }
    
    if (!node->parent || !accessAllowed(node->parent, IPC_WRITE)) // Deleting requires write access to nodes parent
    {
        return (IPC_ACCESSDENIED);
    }

    return (deleteNode(node));
}

IPC_ERROR ipc_setAccess(const char* path, IPC_RIGHTS permissions, uint32_t task)
{
    ipc_node_t* node = ipc_getNode(path);

    if (node == 0)
    {
        return(IPC_NOTFOUND);
    }
    
    if (!accessAllowed(node, IPC_DELEGATERIGHTS))
    {
        return(IPC_ACCESSDENIED);
    }

    if (task == 0)
    {
        node->general = permissions;
    }
    else
    {
        for (dlelement_t* e = node->accessTable->head; e != 0; e = e->next)
        {
            ipc_certificate_t* certificate = e->data;
            if (certificate->owner == task) // Task has already specific certificate to access this node
            {
                certificate->privileges = permissions;
                return (IPC_SUCCESSFUL);
            }
        }
        ipc_certificate_t* certificate = malloc(sizeof(ipc_certificate_t), 0, "ipc_certificate_t");
        certificate->owner = task;
        certificate->privileges = permissions;
        
        if (node->accessTable == 0)
        {
            node->accessTable = list_create();
        }
        
        list_append(node->accessTable, certificate);
    }

    return (IPC_SUCCESSFUL);
}


/*
* Copyright (c) 2011 The PrettyOS Project. All rights reserved.
*
* http://www.c-plusplus.de/forum/viewforum-var-f-is-62.html
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
* TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
* PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
* CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
* PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
* WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
* ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
