#include "initrd.h"
#include "kheap.h"

initrd_header_t*       initrd_header; // The header.
initrd_file_header_t*  file_headers;  // The list of file headers.
fs_node_t*             initrd_root;   // Our root directory node.
fs_node_t*             initrd_dev;    // We also add a directory node for /dev, so we can mount devfs later on.
fs_node_t*             root_nodes;    // List of file nodes.
int                    nroot_nodes;   // Number of file nodes.

struct dirent dirent;

static uint32_t initrd_read(fs_node_t* node, uint32_t offset, uint32_t size, char* buffer)
{
    initrd_file_header_t header = file_headers[node->inode];
    size = header.length;
    if (offset > header.length)
    {
        return 0;
    }
    if (offset+size > header.length)
    {
        size = header.length-offset;
    }
    memcpy(buffer, (void*)(header.off + offset), size);
    return size;
}

static struct dirent* initrd_readdir(fs_node_t* node, uint32_t index)
{
    if( (node == initrd_root) && (index == 0) )
    {
      strcpy(dirent.name, (const char*)"dev");
      dirent.name[3] = 0; // NUL-terminate the string
      dirent.ino     = 0;
      return &dirent;
    }

    if( index-1 >= nroot_nodes )
    {
        return 0;
    }
    strcpy(dirent.name, root_nodes[index-1].name);
    dirent.name[strlen(root_nodes[index-1].name)] = 0; // NUL-terminate the string
    dirent.ino = root_nodes[index-1].inode;
    return &dirent;
}

static fs_node_t* initrd_finddir(fs_node_t* node, const char* name)
{
    if( (node == initrd_root) && (!strcmp(name,(const char*)"dev")) )
    {
        return initrd_dev;
    }
    for(int32_t i=0; i<nroot_nodes; ++i)
    {
        if( !strcmp(name, root_nodes[i].name) )
        {
            return &root_nodes[i];
        }
    }
    return 0;
}

fs_node_t* install_initrd(uint32_t location)
{
    // Initialise the main and file header pointers and populate the root directory.
    initrd_header = (initrd_header_t*) location;
    file_headers  = (initrd_file_header_t*) (location+sizeof(initrd_header_t));

    // Initialise the root directory.
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("rd_root: ");
    settextcolor(15,0);
    #endif
    ///

    initrd_root = (fs_node_t*) malloc( sizeof(fs_node_t),PAGESIZE );
    strcpy(initrd_root->name, (const char*)"dev");
    initrd_root->mask    = initrd_root->uid = initrd_root->gid = initrd_root->inode = initrd_root->length = 0;
    initrd_root->flags   = FS_DIRECTORY;
    initrd_root->read    = 0;
    initrd_root->write   = 0;
    initrd_root->open    = 0;
    initrd_root->close   = 0;
    initrd_root->readdir = &initrd_readdir;
    initrd_root->finddir = &initrd_finddir;
    initrd_root->ptr     = 0;
    initrd_root->impl    = 0;

    // Initialise the /dev directory (required!)
    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("rd_dev: ");
    settextcolor(15,0);
    #endif
    ///

    initrd_dev = (fs_node_t*)malloc(sizeof(fs_node_t),PAGESIZE);
    strcpy(initrd_dev->name, (const char*)"ramdisk");
    initrd_dev->mask     = initrd_dev->uid = initrd_dev->gid = initrd_dev->inode = initrd_dev->length = 0;
    initrd_dev->flags    = FS_DIRECTORY;
    initrd_dev->read     = 0;
    initrd_dev->write    = 0;
    initrd_dev->open     = 0;
    initrd_dev->close    = 0;
    initrd_dev->readdir  = &initrd_readdir;
    initrd_dev->finddir  = &initrd_finddir;
    initrd_dev->ptr      = 0;
    initrd_dev->impl     = 0;

    ///
    #ifdef _DIAGNOSIS_
    settextcolor(2,0);
    printf("root_nodes: ");
    settextcolor(15,0);
    #endif
    ///

    root_nodes = (fs_node_t*) malloc( sizeof(fs_node_t)*initrd_header->nfiles,PAGESIZE);
    nroot_nodes = initrd_header->nfiles;

    // For every file...
    for(uint32_t i=0; i<initrd_header->nfiles; ++i)
    {
        // Edit the file's header - currently it holds the file offset relative to the start of the ramdisk.
        file_headers[i].off += (location); /// We want it relative to the start of memory. ///

        // Create a new file node.
        strncpy(root_nodes[i].name, file_headers[i].name, 64); /// critical !!!
        root_nodes[i].name[64] = 0;

        root_nodes[i].mask    = root_nodes[i].uid = root_nodes[i].gid = 0;
        root_nodes[i].length  = file_headers[i].length;
        root_nodes[i].inode   = i;
        root_nodes[i].flags   = FS_FILE;
        root_nodes[i].read    = (read_type_t) &initrd_read;
        root_nodes[i].write   = 0;
        root_nodes[i].readdir = 0;
        root_nodes[i].finddir = 0;
        root_nodes[i].open    = 0;
        root_nodes[i].close   = 0;
        root_nodes[i].impl    = 0;
    }
    return initrd_root;
}
