#include "os.h"
int32_t INT_MAX = 2147483647;

void sti() {	__asm__ volatile ( "sti" ); }	// Enable interrupts
void cli() { __asm__ volatile ( "cli" ); }	// Disable interrupts
void nop() { __asm__ volatile ( "nop" ); }	// Do nothing
oda_t* pODA = &ODA;

void initODA()
{
    int32_t i;
    for(i=0;i<KQSIZE;++i)
       pODA->KEYQUEUE[i]=0;          // circular queue buffer
    pODA->pHeadKQ = pODA->KEYQUEUE;  // pointer to the head of valid data
    pODA->pTailKQ = pODA->KEYQUEUE;  // pointer to the tail of valid data
    pODA->KQ_count_read  = 0;        // number of data read from queue buffer
    pODA->KQ_count_write = 0;        // number of data put into queue buffer
}

uint32_t fetchESP()
{
    uint32_t esp;
    __asm__ volatile("mov %%esp, %0" : "=r"(esp));
    return esp;
}

uint32_t fetchEBP()
{
    uint32_t ebp;
    __asm__ volatile("mov %%ebp, %0" : "=r"(ebp));
    return ebp;
}

uint32_t fetchSS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ( "movl %ss,%eax" );
    return eax;
}

uint32_t fetchCS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ( "movl %cs,%eax" );
    return eax;
}

uint32_t fetchDS()
{
    register uint32_t eax __asm__("%eax");
    __asm__ volatile ( "movl %ds,%eax" );
    return eax;
}

uint32_t inportb(uint16_t port)
{
	uint32_t ret_val;
	__asm__ volatile ("inb %w1,%b0"	: "=a"(ret_val)	: "d"(port));
	return ret_val;
}

uint32_t inportl(uint16_t port)
{
	uint32_t ret_val;
	__asm__ volatile ("inl %1,%0" : "=a" (ret_val) : "Nd" (port));
	return ret_val;
}

void outportb(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outb %b0,%w1" : : "a"(val), "d"(port));
}

void outportl(uint16_t port, uint32_t val)
{
    __asm__ volatile ("outl %0,%1" : : "a"(val), "Nd"(port));
}

void panic_assert(char* file, uint32_t line, char* desc) // why char ?
{
    cli();
    printformat("ASSERTION FAILED(");
    printformat("%s",desc);
    printformat(") at ");
    printformat("%s",file);
    printformat(":");
    printformat("%i",line);
    printformat("OPERATING SYSTEM HALTED\n");
    // Halt by going into an infinite loop.
    for(;;);
}

void k_memshow(void* start, size_t count)
{
    const uint8_t* end = (const uint8_t*)(start+count);
    for(; count != 0; count--) printformat("%x ",*(end-count));
}

void* k_memcpy(void* dest, const void* src, size_t count)
{
    const uint8_t* sp = (const uint8_t*)src;
    uint8_t* dp = (uint8_t*)dest;
    for(; count != 0; count--) *dp++ = *sp++;
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count)
{
	return k_memcpy( dest, src, count );
}

void* k_memset(void* dest, int8_t val, size_t count)
{
    int8_t* temp = (int8_t*)dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

uint16_t* k_memsetw(uint16_t* dest, uint16_t val, size_t count)
{
    uint16_t* temp = (uint16_t*) dest;
    for( ; count != 0; count--) *temp++ = val;
    return dest;
}

size_t k_strlen(const char* str)
{
    size_t retval;
    for(retval = 0; *str != '\0'; ++str)
        ++retval;
    return retval;
}

// Compare two strings. Returns -1 if str1 < str2, 0 if they are equal or 1 otherwise.
int32_t k_strcmp( const char* s1, const char* s2 )
{
    while ( ( *s1 ) && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
    }
    return ( *s1 - *s2 );
}

// Copy the NUL-terminated string src into dest, and return dest.
char* k_strcpy(char* dest, const char* src)
{
    do { *dest++ = *src++;} while(*src);
    return dest;
}

char* k_strncpy(char* dest, const char* src, size_t n)
{
    if(n != 0)
    {
        char* d       = dest;
        const char* s = src;
        do
        {
            if ((*d++ = *s++) == 0)
            {
                /* NUL pad the remaining n-1 bytes */
                while(--n != 0)
                   *d++ = 0;
                break;
            }
        }
        while(--n != 0);
     }
     return (dest);
}

char* k_strcat(char* dest, const char* src)
{
    while ( *dest) { dest++;     }
    do    { *dest++ = *src++;    } while(*src);
    return dest;
}


void reboot()
{
	int32_t temp; // A temporary int for storing keyboard info. The keyboard is used to reboot
    do //flush the keyboard controller
    {
       temp = inportb( 0x64 );
       if( temp & 1 )
         inportb( 0x60 );
    }
	while ( temp & 2 );

    // Reboot
    outportb(0x64, 0xFE);
}

void k_itoa(int32_t value, char* valuestring)
{
  int32_t min_flag;
  char  swap;
  char* p;
  min_flag = 0;

  if (0 > value)
  {
    *valuestring++ = '-';
    value = -INT_MAX > value ? min_flag = INT_MAX : -value;
  }

  p = valuestring;

  do
  {
    *p++ = (int8_t)(value % 10) + '0';
    value /= 10;
  } while (value);

  if (min_flag != 0)
  {
    ++*valuestring;
  }
  *p-- = '\0';

  while (p > valuestring)
  {
    swap = *valuestring;
    *valuestring++ = *p;
    *p-- = swap;
  }
}

void k_i2hex(uint32_t val, char* dest, int32_t len)
{
	char* cp;
	char  x;
	uint32_t n;
	n = val;
	cp = &dest[len];
	while (cp > dest)
	{
		x = n & 0xF;
		n >>= 4;
		*--cp = x + ((x > 9) ? 'A' - 10 : '0');
	}
    dest[len]  ='h';
    dest[len+1]='\0';
}

void float2string(float value, int32_t decimal, char* valuestring) // float --> string
{
   int32_t neg = 0;
   char tempstr[20];
   int32_t i = 0;
   int32_t j = 0;
   int32_t c;
   int32_t val1, val2;
   char* tempstring;

   tempstring = valuestring;
   if (value < 0)
   {
     {neg = 1; value = -value;}
   }
   for (j=0; j < decimal; ++j)
   {
     {value = value * 10;}
   }
   val1 = (value * 2);
   val2 = (val1 / 2) + (val1 % 2);
   while (val2 !=0)
   {
     if ((decimal > 0) && (i == decimal))
     {
       tempstr[i] = (char)(0x2E);
       ++i;
     }
     else
     {
       c = (val2 % 10);
       tempstr[i] = (char) (c + 0x30);
       val2 = val2 / 10;
       ++i;
     }
   }
   if (neg)
   {
     *tempstring = '-';
      ++tempstring;
   }
   i--;
   for (;i > -1;i--)
   {
     *tempstring = tempstr[i];
     ++tempstring;
   }
   *tempstring = '\0';
}

uint32_t alignUp( uint32_t val, uint32_t alignment )
{
	if ( ! alignment )
		return val;
	--alignment;
	return (val+alignment) & ~alignment;
}

uint32_t alignDown( uint32_t val, uint32_t alignment )
{
	if ( ! alignment )
		return val;
	return val & ~(alignment-1);
}

uint32_t max( uint32_t a, uint32_t b )
{
	return a>=b? a : b;
}

uint32_t min( uint32_t a, uint32_t b )
{
	return a<=b? a : b;
}

 uint8_t PackedBCD2Decimal(uint8_t PackedBCDVal)
 {
     return ((PackedBCDVal >> 4) * 10 + (PackedBCDVal & 0xF));
 }
