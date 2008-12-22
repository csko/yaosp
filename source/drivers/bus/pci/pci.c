#include <console.h>

int init_module( void ) {
    kprintf( "PCI: Init module!\n" );
    return 0;
}

int destroy_module( void ) {
    kprintf( "PCI: Destroy module!\n" );
    return 0;
}
