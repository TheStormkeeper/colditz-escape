/**
 **  Colditz Maps Explorer
 **
 **  Utility functions
 **
 **  Aligned malloc code from Satya Kiran Popuri (http://www.cs.uic.edu/~spopuri/amalloc.html)
 **
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(WIN32)
#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#elif defined(PSP)
#include <stdarg.h>
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspgu.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif

#include "colditz.h"
#include "utilities.h"

// Some global variable specific to utilities
int  underflow_flag = 0;
u32	compressed_size, checksum;
u8  obs_to_sprite[NB_OBS_TO_SPRITE];
#if defined(PSP)
//void* framebuffer = 0;
//static unsigned int __attribute__((aligned(16))) list[262144];
#endif


/* The handy ones, in big endian mode */
u32 readlong(u8* buffer, u32 addr)
{
	return ((((u32)buffer[addr+0])<<24) + (((u32)buffer[addr+1])<<16) +
		(((u32)buffer[addr+2])<<8) + ((u32)buffer[addr+3]));
}

void writelong(u8* buffer, u32 addr, u32 value)
{
	buffer[addr]   = (u8)(value>>24);
	buffer[addr+1] = (u8)(value>>16);
	buffer[addr+2] = (u8)(value>>8);
	buffer[addr+3] = (u8)value;
}

u16 readword(u8* buffer, u32 addr)
{
	return ((((u16)buffer[addr+0])<<8) + ((u16)buffer[addr+1]));
}

void writeword(u8* buffer, u32 addr, u16 value)
{
	buffer[addr]   = (u8)(value>>8);
	buffer[addr+1] = (u8)value;
}


u8 readbyte(u8* buffer, u32 addr)
{
	return buffer[addr];
}

void writebyte(u8* buffer, u32 addr, u8 value)
{
	buffer[addr] = value;
}


/* Returns a piece of memory aligned to the given
 * alignment parameter. Alignment must be a power of
 * 2.
 * This function returns memory of length 'bytes' or more
 */
void *aligned_malloc(size_t bytes, size_t alignment)
{
	size_t size;
	size_t delta;
	void *malloc_ptr;
	void *new_ptr;
	void *aligned_ptr;

        /* Check if alignment is a power of 2
         * as promised by the caller.
         */
        if ( alignment & (alignment-1)) /* If not a power of 2 */
                return NULL;
        
        /* Determine how much more to allocate
         * to make room for the alignment:
         * 
         * We need (alignment - 1) extra locations 
         * in the worst case - i.e., malloc returns an
         * address off by 1 byte from an aligned
         * address.
         */
        size = bytes + alignment - 1; 

        /* Additional storage space for storing a delta. */
        size += sizeof(size_t);

        /* Allocate memory using malloc() */
        malloc_ptr = calloc(size, 1);

        if (NULL == malloc_ptr)
                return NULL;

        /* Move pointer to account for storage of delta */
        new_ptr = (void *) ((char *)malloc_ptr + sizeof(size_t));

        /* Make ptr a multiple of alignment,
         * using the standard trick. This is
         * used everywhere in the Linux kernel
         * for example.
         */
        aligned_ptr = (void *) (((size_t)new_ptr + alignment - 1) & ~(alignment -1));

        delta = (size_t)aligned_ptr - (size_t)malloc_ptr;

        /* write the delta just before the place we return to user */
        *((size_t *)aligned_ptr - 1) = delta;

        return aligned_ptr;
}


/* Frees a chunk of memory returned by aligned_malloc() */
void aligned_free(void *ptr)
{
	size_t delta;
	void *malloc_ptr;

        if (NULL == ptr)
                return;

        /* Retrieve delta */
        delta = *( (size_t *)ptr - 1);

        /* Calculate the original ptr returned by malloc() */
        malloc_ptr = (void *) ( (size_t)ptr - delta);

        free(malloc_ptr);
}



#if !defined(PSP)
// Prints a line of text on the top right corner
void glutPrintf(const char *fmt, ...) 
{
	char		text[256];			// Holds Our String
	va_list		ap;					// Pointer To List Of Arguments
	int			w, h;
	char*		t;
	float		length;				// text length, in pixels
	const float coef = 0.09f;

	// Parses The String For Variables
	if (fmt==0) return;
	va_start(ap, fmt);	
	vsprintf(text, fmt, ap);
	va_end(ap);

    glMatrixMode(GL_PROJECTION);
	glPushMatrix();
    glLoadIdentity();
    w = glutGet(GLUT_WINDOW_WIDTH);
    h = glutGet(GLUT_WINDOW_HEIGHT);

	// Set up a 2d ortho projection that matches the window
//    gluOrtho2D(-w/2,w/2,-h/2,h/2);
    glOrtho(-w/2,w/2,-h/2,h/2,-1,1);
	// Get the pixel length
	length = glutStrokeLength(GLUT_STROKE_MONO_ROMAN, text)*coef;
	// Move text to top right corner
	glTranslatef(w/2-length-2, h/2-16.0f, 0.0f);  
    glScalef(coef, coef, 1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);	// White colour
	glPushMatrix();	
	// Render each character
    t = text;
	while (*t) 
		glutStrokeCharacter(GLUT_STROKE_MONO_ROMAN, *t++); 
	// Apply the various transformations
    glPopMatrix();		
    glPopMatrix(); 
}
#endif

// Get one bit and read ahead if needed
u32 getbit(u32 *address, u32 *data)
{
	// Read one bit and rotate data
	u32 bit = (*data) & 1;
	(*data) >>= 1;
	if ((*data) == 0)
	{	// End of current bitstream? => read another longword
		(*data) = readlong(mbuffer, *address);
		checksum ^= (*data);
		if (opt_debug)
			print("(-%X).l = %08X\n",(uint)(compressed_size-*address+LOADER_DATA_START+8), (uint)*data);
		(*address)-=4;
		// Lose the 1 bit marker on read ahead
		bit = (*data) & 1; 
		// Rotate data and introduce a 1 bit marker as MSb
		// This to ensure that zeros in high order bits are processed too
		(*data) = ((*data)>>1) | 0x80000000;
	}
	return bit;
}

// Get sequence of streamsize bits (in reverse order)
u32 getbitstream(u32 *address, u32 *data, u32 streamsize)
{
	u32 bitstream = 0;
	u32 i;
	for (i=0; i<streamsize; i++)
		bitstream = (bitstream<<1) | getbit (address, data);
	return bitstream;
}

// Decrement address by one byte and check for buffer underflow
void decrement(u32 *address)
{
	if (underflow_flag)
		print("uncompress(): Buffer underflow error.\n");
	if ((*address)!=0)
		(*address)--;
	else
		underflow_flag = 1;
}

// Duplicate nb_bytes from address+offset to address
void duplicate(u32 *address, u32 offset, u32 nb_bytes)
{
	u32 i;
	if (offset == 0)
		print("uncompress(): WARNING - zero offset value found for duplication\n");
	for (i=0; i<nb_bytes; i++)
	{
		writebyte(fbuffer[LOADER], (*address), readbyte(fbuffer[LOADER],(*address)+offset));
		decrement(address);
	}
}



// Uncompress loader data
int uncompress(u32 expected_size)
{
	u32 source = LOADER_DATA_START;
	u32 uncompressed_size, current;
	u32 dest, offset;
	u32 bit, nb_bits_to_process, nb_bytes_to_process;
	u32 j;
	compressed_size = readlong(mbuffer, source); 
	source +=4;
	uncompressed_size = readlong(mbuffer, source); 
	source +=4;
	if (uncompressed_size != expected_size)
	{
		print("uncompress(): uncompressed data size does not match expected size\n");
		return -1;
	}
	checksum = readlong(mbuffer, source);	// There's a compression checksum
	source +=4;	// Keeping this last +/- 4 on source for clarity

	if (opt_verbose)
	{
		print("  Compressed size=%X, uncompressed size=%X\n", 
			(uint)compressed_size, (uint)uncompressed_size);
	}

	source += (compressed_size-4);	// We read compressed data (long) starting from the end
	dest = uncompressed_size-1;		// We fill the uncompressed data (byte) from the end too

	current = readlong(mbuffer, source); 
	source -= 4;
	// Note that the longword above (last one) will not have the one bit marker
	// => Unlike other longwords, we might read ahead BEFORE all 32 bits
	// have been rotated out of the last longword of compressed data
	// (i.e., as soon as rotated long is zero)

	checksum ^= current;
	if (opt_debug)
		print("(-%X).l = %08X\n", (uint)(compressed_size-source+LOADER_DATA_START+8), (uint)current);

	while (dest != 0)
	{
		// Read bit 0 of multiplier
		bit = getbit (&source, &current);
		if (bit)
		{	// bit0 = 1 => 3 bit multiplier
			// Read bits 1 and 2
			bit = getbitstream(&source, &current, 2);
			// OK, this is no longer a bit, but who cares?
			switch (bit)
			{
			case 2:	// mult: 011 (01 reversed  = 10)
				// Read # of bytes to duplicate (8 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 8)+1;
				// Read offset (12 bit value)
				offset = getbitstream(&source, &current, 12);
				duplicate(&dest, offset, nb_bytes_to_process);
				if (opt_debug)
					print("  o mult=011: duplicated %d bytes at (start) offset %X to address %X\n", 
						(uint)nb_bytes_to_process, (int)offset, (uint)dest+1);
				break;
			case 3:	// mult: 111
				// Read # of bytes to read and copy (8 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 8) + 9;
				// We add 8 above, because a [1-9] value 
				// would be taken care of by a 3 bit bitstream
				for (j=0; j<nb_bytes_to_process; j++)
				{	// Read and copy nb_bytes+1
					writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
					decrement(&dest);
				}
				if (opt_debug)
					print("  o mult=111: copied %d bytes to address %X\n", (int)nb_bytes_to_process, (uint)dest+1);
				break;
			default: // mult: x01
				// Read offset (9 or 10 bit value)
		        nb_bits_to_process = bit+9;
				offset = getbitstream(&source, &current, nb_bits_to_process);
				// Duplicate 2 or 3 bytes 
				nb_bytes_to_process = bit+3;
				duplicate(&dest, offset, nb_bytes_to_process);
				if (opt_debug)
					print("  o mult=%d01: duplicated %d bytes at (start) offset %X to address %X\n", 
						(int)bit&1, (int)nb_bytes_to_process, (uint)offset, (uint)dest+1);
				break;
			}
		}
		else
		{	// bit0=0 => 2 bit multiplier
			bit = getbit (&source, &current);
			if (bit)
			{	// mult: 10
				// Read 8 bit offset
				offset = getbitstream(&source, &current, 8);
				// Duplicate 1 byte
				duplicate(&dest, offset, 2);
				if (opt_debug)
					print("  o mult=10: duplicated 2 bytes at (start) offset %X to address %X\n", 
						(uint)offset, (uint)dest+1);
			}
			else
			{	// mult: 00
				// Read # of bytes to read and copy (3 bit value)
				nb_bytes_to_process = getbitstream(&source, &current, 3) + 1;
				for (j=0; j<nb_bytes_to_process; j++)
				{	// Read and copy nb_bytes+1
					writebyte(fbuffer[LOADER], dest, (u8)getbitstream(&source, &current, 8));
					decrement(&dest);
				}
				if (opt_debug)
					print("  o mult=00: copied 2 bytes to address %X\n", (uint)dest+1);

			}
		} 
	}

	if (checksum != 0)
	{
		print("uncompress(): checksum error\n");
		return -1;
	}
	return 0;
}


void load_all_files()
{
	size_t read;
	u32 i;
	int compressed_loader = 0;

	for (i=0; i<NB_FILES; i++)
	{
		if ( (fbuffer[i] = (u8*) aligned_malloc(fsize[i], 16)) == NULL)
		{
			perr("Could not allocate buffers\n");
			ERR_EXIT;
		}

		if ((fd = fopen (fname[i], "rb")) == NULL)
		{
			if (opt_verbose)
				perror ("fopen()");
			perr("Can't find file '%s'\n", fname[i]);

			/* Take care of the compressed loader if present */
			if (i == LOADER)
			{
				// Uncompressed loader was not found
				// Maybe there's a compressed one?
				perr("  Trying to use compressed loader '%s' instead\n",ALT_LOADER);
				if ((fd = fopen (ALT_LOADER, "rb")) == NULL)
				{
					print("  '%s' not found.\n", ALT_LOADER);
					ERR_EXIT;
				}
				// OK, file was found - let's allocated the compressed data buffer
				if ((mbuffer = (u8*) aligned_malloc(ALT_LOADER_SIZE, 16)) == NULL)
				{
					perr("Could not allocate source buffer for uncompress\n");
					ERR_EXIT;
				}
				if (opt_verbose)
					print("Reading file '%s'...\n", ALT_LOADER);
				read = fread (mbuffer, 1, ALT_LOADER_SIZE, fd);
				if (read != ALT_LOADER_SIZE)
				{
					if (opt_verbose)
						perror ("fread()");
					perr("'%s': Unexpected file size or read error\n", ALT_LOADER);
					ERR_EXIT;
				}
				compressed_loader = 1;

				perr("  Uncompressing...\n");
				if (uncompress(fsize[LOADER]))
				{
					perr("Decompression error\n");
					ERR_EXIT;
				}
				perr("  OK. Now saving file as '%s'\n",fname[LOADER]);
				if ((fd = fopen (fname[LOADER], "wb")) == NULL)
				{
					if (opt_verbose)
						perror ("fopen()");
					perr("Can't create file '%s'\n", fname[LOADER]);
					ERR_EXIT;
				}
				
				// Write file
				if (opt_verbose)
						print("Writing file '%s'...\n", fname[LOADER]);
				read = fwrite (fbuffer[LOADER], 1, fsize[LOADER], fd);
				if (read != fsize[LOADER])
				{
					if (opt_verbose)
						perror ("fwrite()");
					perr("'%s': Unexpected file size or write error\n", fname[LOADER]);
					ERR_EXIT;
				}				
			}
			else 
				ERR_EXIT;
		}
	
		// Read file (except in the case of a compressed loader)
		if (!((i == LOADER) && (compressed_loader)))
		{
			if (opt_verbose)
				print("Reading file '%s'...\n", fname[i]);
			read = fread (fbuffer[i], 1, fsize[i], fd);
			if (read != fsize[i])
			{
				if (opt_verbose)
					perror ("fread()");
				perr("'%s': Unexpected file size or read error\n", fname[i]);
				ERR_EXIT;
			}
		}

		fclose (fd);
		fd = NULL;
	}
}


// Get some properties (max/min/...) according to file data
void getProperties()
{
	u16 room_index;
	u32 ignore = 0;
	u32 offset;
	u8  i;

	// Get the number of rooms
	for (room_index=0; ;room_index++)
	{	
		// Read the offset
		offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*room_index);
		if (offset == 0xFFFFFFFF)
		{	// For some reason there is a break in the middle
			ignore++;
			if (ignore > FFs_TO_IGNORE)
				break;
		}
	}
	nb_rooms = room_index;
	print("nb_rooms = %X\n", nb_rooms);

	// A backdrop cell is exactly 256 bytes
	nb_cells = fsize[CELLS] / 0x100;
	cell_texid = malloc(sizeof(GLuint) * nb_cells);
	GLCHK(glGenTextures(nb_cells, cell_texid));
	print("nb_cells = %X\n", nb_cells);

	nb_sprites = readword(fbuffer[SPRITES],0) + 1;
	sprite_texid = malloc(sizeof(GLuint) * nb_sprites);
	GLCHK(glGenTextures(nb_sprites, sprite_texid));
	print("nb_sprites = %X\n", nb_sprites);

	nb_objects = readword(fbuffer[OBJECTS],0) + 1;
	print("nb_objects = %X\n", nb_objects);
	for (i=0; i<NB_OBS_TO_SPRITE; i++)
		obs_to_sprite[i] = readbyte(fbuffer[LOADER],OBS_TO_SPRITE_START+i);
}


// Reorganize cell bitplanes from interleaved lines (4*32 bits)
// to interleaved pixels (4 bits per pixel)
void cells_to_interleaved(u8* buffer, u32 size)
{
	u32 line[4];
	u8 byte = 0;
	u8 tmp;
	u32 index, i, j;
	int k;

	for (i=0; i<size; i+=16)	
	// 4 colours *4 bytes per line
	{
		for (j=0; j<4; j++)
			line[j] = readlong(buffer, i+j*4);
		index = 15;
		for (j=0; j<32; j++)	// 32 nibbles
		{
			if (!(j&0x01))	// don't lose the former nibble if j is odd (2 nibbles per byte)
				byte = 0;
			// if k is not a signed integer, we'll have issues with the following line
			for (k=3; k>=0; k--)
			// last bitplane is MSb dammit!!!
			{
				byte = (byte << 1) + (u8)(line[k] & 0x1);
				line[k] = line[k] >> 1;
			}
			if (j&0x01)		// if j is odd, we read 2 nibbles => time to write a byte
			{
				tmp = byte;
				byte = (byte << 4) | (tmp >> 4);
				writebyte(buffer, i+index, byte);
				index--;
			}
		}
	}
}


// Reorganize sprites from separate bitplanes (multiples of 16 bits)
// to interleaved pixels (4 bits per pixel)
void sprites_to_interleaved(u8* buffer, u32 bitplane_size)
{
	u8* sbuffer;
	u8  bitplane_byte[4];
	u32 interleaved;
	u32 i, j;

	sbuffer = (u8*) aligned_malloc(4*bitplane_size,16);

	// Yeah, I know, we could be smarter than allocate a buffer every time
	if (sbuffer == NULL)
	{
		perr("remap_sprite: could not allocate sprite buffer\n");
		ERR_EXIT;
	}
	// First, let's copy the buffer
	for (i=0; i<4*bitplane_size; i++)
		sbuffer[i] = buffer[i];

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<4; j++)
			// bitplanes are in reverse order
			bitplane_byte[3-j] = readbyte(sbuffer, i+(j*bitplane_size));

		// ...and create the interleaved longword out of it
		interleaved = 0;
		for (j=0; j<32; j++)	
		{
			// You sure want to rotate BEFORE you add the last bit!
			interleaved = interleaved << 1;
			interleaved |= (bitplane_byte[j%4] >> ((31-j)/4)) & 1;
		}
		writelong(buffer,4*i,interleaved);
	}
	aligned_free(sbuffer);
}

// Convert an Amiga 12 bit RGB colour palette to 16 bit GRAB
void to_16bit_Palette(u8 palette_index)
{
	u32 i;
	u16 rgb, grab;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		print("Using Amiga Palette index: %d\n", palette_index);


	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
		if (opt_verbose)
		{
			print(" %03X", rgb); 
			if (i==7)
				print("\n");
		}
		// OK, we need to convert our rgb to grab
		// 1) Leave the R&B values as they are
		grab = rgb & 0x0F0F;
		// 2) Set Alpha to no transparency
		grab |= 0x00F0;
		// 3) Set Green
		grab |= (rgb << 8) & 0xF000;
		// 4) Write in the palette
		aPalette[i] = grab;
	}
	if (opt_verbose)
		print("\n\n");
}



// Convert an Amiga 12 bit colour palette to 24 bit
void to_24bit_Palette(u8 palette_index)
{
	u32 i;
	int colour;		// 0 = Red, 1 = Green, 2 = Blue
	u16 rgb;

	int palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		print("Using Amiga Palette index: %d\n", palette_index);
	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
		if (opt_verbose)
		{
			print(" %03X", rgb); 
			if (i==7)
				print("\n");
		}
		for (colour=2; colour>=0; colour--)
		{
			bPalette[colour][i] = (rgb&0x000F) * 0x11;
			rgb = rgb>>4;
		}
	}
	if (opt_verbose)
		print("\n\n");
}


// Convert an Amiga 12 bit colour palette to a 48 bit (TIFF) one
void to_48bit_Palette(u16 wPalette[3][16], u8 palette_index)
{
	u32 i;
	int colour;		// 0 = Red, 1 = Green, 2 = Blue
	u16 rgb;

	u16 palette_start = palette_index * 0x20;

	// Read the palette
	if (opt_verbose)
		print("Using Amiga Palette index: %d\n", palette_index);
	for (i=0; i<16; i++)		// 16 colours
	{
		rgb = readword(fbuffer[PALETTES], palette_start + 2*i);
		if (opt_verbose)
		{
			print(" %03X", rgb); 
			if (i==7)
				print("\n");
		}
		for (colour=2; colour>=0; colour--)
		{
			wPalette[colour][i] = (rgb&0x000F) * 0x1111;
			rgb = rgb>>4;
		}
	}
	if (opt_verbose)
		print("\n\n");
}


// Convert a 4 bit line-interleaved source to 24 bit RGB destination
void line_interleaved_to_RGB(u8* source, u8* dest, u16 w, u16 h)
{
	u8 colour_index;
	u32 i,j,l,pos;
	int k;
	u32 wb;
	u8 line_byte[4];

	// the width of interest to us is the one in bytes.
	wb = w/8;

	// We'll write sequentially to the destination
	pos = 0;
	for (i=0; i<h; i++)
	{	// h lines to process
		for (j=0; j<wb; j++)
		{	// wb bytes per line
			for (k=0; k<4; k++)
				// Read one byte from each of the 4 lines (starting from max y for openGL)
				line_byte[3-k] = readbyte(source, 4*(wb*(h-1-i)) + k*wb + j);
			// Write 8 RGB values
			for (k=0; k<8; k++)
			{
				colour_index = 0;
				// Get the palette colour index and rotate the line bytes
				for (l=0; l<4; l++)
				{
					colour_index <<= 1;
					colour_index |= (line_byte[l]&0x80)?1:0;
					line_byte[l] <<= 1;
				}
				// writebyte only uses pos once, so we can afford pos++
				writebyte(dest, pos++, bPalette[RED][colour_index]);
				writebyte(dest, pos++, bPalette[GREEN][colour_index]);
				writebyte(dest, pos++, bPalette[BLUE][colour_index]);
			}
		}
	}
}


// Convert a 4 bit line-interleaved source to 16 bit RGBA (GRAB) destination
void line_interleaved_to_wGRAB(u8* source, u8* dest, u16 w, u16 h)
{
	u8 colour_index;
	u32 i,j,l,pos;
	int k;
	u32 wb;
	u8 line_byte[4];

	// the width of interest to us is the one in bytes.
	wb = w/8;

	// We'll write sequentially to the destination
	pos = 0;
	for (i=0; i<h; i++)
	{	// h lines to process
		for (j=0; j<wb; j++)
		{	// wb bytes per line
			for (k=0; k<4; k++)
				// Read one byte from each of the 4 lines (starting from max y for openGL)
				line_byte[3-k] = readbyte(source, 4*(wb*i) + k*wb + j);
			// Write 8 RGBA values
			for (k=0; k<8; k++)
			{
				colour_index = 0;
				// Get the palette colour index and rotate the line bytes
				for (l=0; l<4; l++)
				{
					colour_index <<= 1;
					colour_index |= (line_byte[l]&0x80)?1:0;
					line_byte[l] <<= 1;
				}
				// Alpha is always set to 0
				writeword(dest, pos, aPalette[colour_index]);
				pos += 2;
			}
		}
	}
}


// Convert a 1+4 bits (mask+colour) bitplane source
// to 24 bit RGBA destination
void bitplane_to_RGBA(u8* source, u8* dest, u16 w, u16 h)
{
	u16 bitplane_size;
	u8  colour_index;
	u16 i,j,k,wb;
	u8  bitplane_byte[5], mask_byte;
	u32 pos = 0;
//	int no_mask = 0;

	wb = w/8;	// width in bytes
	bitplane_size = h*wb; 

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<5; j++)
			// bitplanes are in reverse order for colour
			// and so is openGL's coordinate system for y
			bitplane_byte[4-j] = readbyte(source, 
				wb*(h-1-i/wb) + i%wb + (j*bitplane_size) );

		// For clarity
		mask_byte = bitplane_byte[4];

		// Write 8 RGBA quadruplets 
		for (k=0; k<8; k++)
		{
			colour_index = 0;
			// Get the palette colour index and rotate the bitplane bytes
			for (j=0; j<4; j++)
			{
				colour_index <<= 1;
				colour_index |= (bitplane_byte[j]&0x80)?1:0;
				bitplane_byte[j] <<= 1;
			}
			writebyte(dest, pos++, bPalette[RED][colour_index]);
			writebyte(dest, pos++, bPalette[GREEN][colour_index]);
			writebyte(dest, pos++, bPalette[BLUE][colour_index]);
			// Alpha
			writebyte(dest, pos++, (mask_byte&0x80)?0xFF:0);
			mask_byte <<=1;
		}
	}
}


// Convert a 1+4 bits (mask+colour) bitplane source
// to 16 bit RGBA (GRAB) destination
void bitplane_to_wGRAB(u8* source, u8* dest, u16 w, u16 ext_w, u16 h)
{
	u16 bitplane_size;
	u8  colour_index;
	u16 i,j,k,wb,ext_wb;
	u8  bitplane_byte[5], mask_byte;
	u32 pos = 0;

	wb = w/8;	// width in bytes
	ext_wb = ext_w/8;
	bitplane_size = h*wb; 

	for (i=0; i<bitplane_size; i++)	
	{
		// Read one byte from each bitplane...
		for (j=0; j<5; j++)
			// bitplanes are in reverse order for colour
			// and so is openGL's coordinate system for y
			bitplane_byte[4-j] = readbyte(source, i + (j*bitplane_size) );
//			bitplane_byte[4-j] = readbyte(source, 
//				wb*(h-1-i/wb) + i%wb + (j*bitplane_size) );

		// For clarity
		mask_byte = bitplane_byte[4];

		// Write 8 RGBA words 
		for (k=0; k<8; k++)
		{

			colour_index = 0;
			// Get the palette colour index and rotate the bitplane bytes
			for (j=0; j<4; j++)
			{
				colour_index <<= 1;
				colour_index |= (bitplane_byte[j]&0x80)?1:0;
				bitplane_byte[j] <<= 1;
			}
			// Alpha is in 3rd position, and needs to be cleared on empty mask
			writeword(dest, pos, aPalette[colour_index] & ((mask_byte&0x80)?0xFFFF:0xFF0F));
			pos += 2;
			// Takes care of padding in width
			while ((u16)(pos%(2*ext_w))>=(2*w))
				pos +=2;	// calloced to zero, so just skim
			mask_byte <<=1;
		}
	}
}


// Converts the room cells to RGB data we can handle
void cells_to_RGB(u8* source, u8* dest, u32 size)
{
	u32 i;

	// Convert each 32x16x4bit (=256 bytes) cell to RGB
	for (i=0; i<(size/256); i++)
		line_interleaved_to_RGB(source + (256*i), dest+(6*256*i), 32, 16);
}

// Converts the room cells to RGB data we can handle
void cells_to_wGRAB(u8* source, u8* dest)
{
	u32 i;

	// Convert each 32x16x4bit (=256 bytes) cell to RGB
	for (i=0; i<nb_cells; i++)
	{
		line_interleaved_to_wGRAB(source + (256*i), dest+(2*RGBA_SIZE*256*i), 32, 16);
		GLCHK(glBindTexture(GL_TEXTURE_2D, cell_texid[i]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32, 16, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, 
			((u8*)rgbCells) + i*2*RGBA_SIZE*0x100));
	}

}

// Power-of-two-err...ize
// We need this to change a dimension to the closest greater power of two
// as pspgl can only deal with power of two dimensionned textures
u16 powerize(u16 n)
{
	u16 retval;
	int i, first_one, last_one;

	retval = n;	// left unchanged if already power of two
				// also works if n == 0
	first_one = -1;
	last_one = -1;

	for (i=0; i<16; i++)
	{
		if (n & 0x0001)
		{
			if (first_one == -1)
				first_one = i;					
			last_one = i;
		}
		n >>= 1;
	}
	if (first_one != last_one)
		retval = 1<<(last_one+1);

	return retval;
}


// Initialize the sprite array
void init_sprites()
{
	u32 index = 2;	// We need to ignore the first word (nb of sprites)
	u16 sprite_index = 0;
	u16 sprite_w;	// width, in words
	u32 sprite_address;

	// Allocate the sprites and overlay arrays
	sprite = aligned_malloc(nb_sprites * sizeof(s_sprite), 16);
	overlay = aligned_malloc(MAX_OVERLAY * sizeof(s_overlay), 16);

	// First thing we do is populate the sprite offsets at the beginning of the table
	sprite_address = index + 4* (readword(fbuffer[SPRITES],0) + 1);
	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		sprite_address += readlong(fbuffer[SPRITES],index);
		writelong(fbuffer[SPRITES],index,sprite_address);
		index+=4;
	}
	// Each sprite is prefixed by 2 words (x size in words, y size in pixels)
	// and one longword (size of one bitplane, in bytes)
	// NB: MSb on x size will be set if sprite is animated
	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);
//		print("sprite[%X] address = %08X\n", sprite_index, sprite_address);
		// x size is given in words
		sprite_w = readword(fbuffer[SPRITES],sprite_address);
		// w is fine as it's either 2^4 or 2^5
		sprite[sprite_index].w = 16*(sprite_w & 0x7FFF);
		sprite[sprite_index].corrected_w = powerize(sprite[sprite_index].w);
		// h will be problematic as pspgl wants a power of 2
		sprite[sprite_index].h = readword(fbuffer[SPRITES],sprite_address+2);
		sprite[sprite_index].corrected_h = powerize(sprite[sprite_index].h);
		print("(%X,%X) => (%X,%X)\n", sprite[sprite_index].w, sprite[sprite_index].h, sprite[sprite_index].corrected_w, sprite[sprite_index].corrected_h);
		
		// According to MSb of sprite_w (=no_mask), we'll need to use RGBA or RGB
//		sprite[sprite_index].type = (sprite_w & 0x8000)?GL_RGB:GL_RGBA;
		// There's an offset to position the sprite depending on the mask's presence
		sprite[sprite_index].x_offset = (sprite_w & 0x8000)?16:1;
		sprite[sprite_index].data = aligned_malloc( RGBA_SIZE * 
			sprite[sprite_index].corrected_w * sprite[sprite_index].corrected_h, 16);
//		print("  w,h = %0X, %0X\n", sprite[sprite_index].w , sprite[sprite_index].h);
	}
}


// Converts the sprites to 16 bit GRAB data we can handle
void sprites_to_wGRAB()
{
	u16 sprite_index;
	u16 bitplane_size;
	u32 sprite_address;
	u8* sbuffer;
	u16 w,h;
	int no_mask = 0;

	for (sprite_index=0; sprite_index<nb_sprites; sprite_index++)
	{
		// Get the base in the original Colditz sprite file
		sprite_address = readlong(fbuffer[SPRITES],2+4*sprite_index);

		// if MSb is set, we have 4 bitplanes instead of 5
		w = readword(fbuffer[SPRITES],sprite_address);
		no_mask = w & 0x8000;
		w *= 2;		// width in bytes
		h = sprite[sprite_index].h;

		bitplane_size = readword(fbuffer[SPRITES],sprite_address+6);
		if (bitplane_size != w*h)
			print("sprites_to_wGRAB: Integrity check failure on bitplane_size\n");

		// Source address
		sbuffer = fbuffer[SPRITES] + sprite_address + 8; 

		if (no_mask)
			// Bitplanes that have no mask are line-interleaved, like cells
			line_interleaved_to_wGRAB(sbuffer, sprite[sprite_index].data, sprite[sprite_index].w, h);
		else
			bitplane_to_wGRAB(sbuffer, sprite[sprite_index].data, sprite[sprite_index].w,
				sprite[sprite_index].corrected_w, h);

		// Now that we have data in a GL readable format, let's texturize it!
		GLCHK(glBindTexture(GL_TEXTURE_2D, sprite_texid[sprite_index]));
		GLCHK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sprite[sprite_index].corrected_w, 
			sprite[sprite_index].corrected_h, 0, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV,
			sprite[sprite_index].data));

	}
}


// Populates the overlay table
void set_overlays(int x, int y, u32 current_tile, u16 room_x)
{
	u16 tile1_data, tile2_data;
	u16 i;
	short sx, sy;
	u16 sid;	// sprite index

	// read current tile
	tile1_data = readword(fbuffer[ROOMS], current_tile) & 0xFF80;
	for (i=0; i<(12*NB_SPECIAL_TILES); i+=12)
	{
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i) != tile1_data)
			continue;
		sx = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+8);
		if (opt_debug)
			print("  match: %04X, direction: %04X\n", tile1_data, sx);
		if (i >= (12*(NB_SPECIAL_TILES-4)))
		// The four last special tiles are exits. We need to check is they are open
		{
			// Get the exit data (same tile if tunnel, 2 rows down in door)
			tile2_data = readword(fbuffer[ROOMS], current_tile + 
				(i==(12*(NB_SPECIAL_TILES-1)))?0:(4*room_x));
//			print("got exit: %04X\n", tile2_data);
			// Validity check
			if (!(tile2_data & 0x000F))
				print("set_overlays: Integrity check failure on exit tile\n");
			// if the tile is an exit and the exit is open
			if (tile2_data & 0x0010)
			{
				if (opt_debug)
					print("    exit open: ignoring overlay\n");
				// The second check on exits is always an FA00, thus we can safely
				break;
			}
		}
			 
		if (sx < 0)
			tile2_data = readword(fbuffer[ROOMS], current_tile-2) & 0xFF80;
		else
			tile2_data = readword(fbuffer[ROOMS], current_tile+2) & 0xFF80;
		// ignore if special tile that follows is matched
		if (readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2) == tile2_data)
		{
			if (opt_debug)
				print("    ignored as %04X matches\n", tile2_data);
			continue;
		}
		sid = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+4);
		overlay[overlay_index].sid = sid;
		if (opt_debug)
			print("    overlay as %04X != %04X => %X\n", tile2_data, readword(fbuffer[LOADER], SPECIAL_TILES_START+i+2), sid);
		sy = readword(fbuffer[LOADER], SPECIAL_TILES_START+i+6);
		if (opt_debug)
			print("    sx: %04X, sy: %04X\n", sx, sy);
		overlay[overlay_index].x = x + (int)sx - (int)sprite[sid].w + (int)(sprite[sid].x_offset);
		overlay[overlay_index].y = y + (int)sy - (int)sprite[sid].h + 1;
		overlay_index++;
		// No point in looking for overlays any further if we met our match 
		// UNLESS this is a double bed overlay, in which case the same tile
		// need to be checked for a double match (in both in +x and -x)
		if (tile1_data != 0xEF00)
			break;
	}
}


// Read the pickable objects from obs.bin
void set_objects()
{
	u16 i;

	for (i=0; i<(8*nb_objects); i+=8)
	{
		if (readword(fbuffer[OBJECTS],i+2) != current_room_index)
			continue;
		overlay[overlay_index].sid = obs_to_sprite[readword(fbuffer[OBJECTS],i+2+6)];
		overlay[overlay_index].x = gl_off_x + readword(fbuffer[OBJECTS],i+2+4) - 15;
		overlay[overlay_index].y = gl_off_y + readword(fbuffer[OBJECTS],i+2+2) - 3;
		if (opt_debug)
			print("  pickup object match: sid=%X\n", overlay[overlay_index].sid);
		overlay_index++;
	}
	
}


#define GLCHK1(x) {x;}
void displaySprite(u16 x, u16 y, u16 w, u16 h, GLuint texid) 
{
	GLCHK1(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCHK1(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	//GL_CLAMP_TO_EDGE

	GLCHK1(glTranslatef(origin_x + x, origin_y + y, 0));
	
	GLCHK1(glBindTexture(GL_TEXTURE_2D, texid));
	GLCHK1(glBegin(GL_TRIANGLE_FAN));

	GLCHK(glColor3f(1.0f, 0.0f, 0.0f));
	GLCHK(glTexCoord2f(0.0f, 0.0f));
	GLCHK(glVertex3f(0.0f, 0.0f, 0.0f));

	GLCHK(glColor3f(0.0f, 1.0f, 0.0f));
	GLCHK(glTexCoord2f(0.0f, 1.0f));
	GLCHK(glVertex3f(0.0f, h*1.0f, 0.0f));

	GLCHK(glColor3f(0.0f, 0.0f, 1.0f));
	GLCHK(glTexCoord2f(1.0f, 1.0f));
	GLCHK(glVertex3f(w*1.0f, h*1.0f, 0.0f));

	GLCHK(glColor3f(1.0f, 1.0f, 1.0f));
	GLCHK(glTexCoord2f(1.0f, 0.0f));
	GLCHK(glVertex3f(w*1.0f, 0.0f, 0.0f));

	GLCHK1(glEnd());

	GLCHK1(glTranslatef(-(origin_x + x), -(origin_y + y), 0));
}

// Display all our overlays
void display_overlays()
{
	u8 i;

	for (i=0; i<overlay_index; i++)
	{
		displaySprite(overlay[i].x, overlay[i].y, sprite[overlay[i].sid].corrected_w, 
			sprite[overlay[i].sid].corrected_h, sprite_texid[overlay[i].sid]);

//		glRasterPos2i(overlay[i].x,overlay[i].y);
//		glDrawPixels(sprite[overlay[i].sid].w, sprite[overlay[i].sid].h,
//			GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4_REV, sprite[overlay[i].sid].data);

	}

}

// Display room
void displayRoom(u16 room_index)
{
	u32 offset;					// Offsets to each rooms are given at
								// the beginning of the Rooms Map file
	u16 room_x, room_y, tile_data;
	int tile_x, tile_y;
	int pixel_x, pixel_y;

	// Read the offset
	offset = readlong((u8*)fbuffer[ROOMS], OFFSETS_START+4*room_index);
	if (opt_verbose)
		print("\noffset[%03X] = %08X ", room_index, (uint)offset);
	if (offset == 0xFFFFFFFF)
	{
		// For some reason there is a break in the middle
		if (opt_verbose)
			print("\n  IGNORED");
		return;
	}

	// Now that we have the offset, let's look at the room

	// The 2 first words are the room Y and X dimension (in tiles),
	// in that order
	room_y = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
	offset +=2;
	room_x = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);
	offset +=2;
	if (opt_verbose)
		print("(room_x=%X,room_y=%X)\n", room_x, room_y);
	gl_off_x = (gl_width - (room_x*32))/2;
	gl_off_y = (gl_height - (room_y*16))/2;

	// reset room overlays
	overlay_index = 0;

	// Before we do anything, let's set the pickable objects in
	// our overlay table (so that room overlays go on top of 'em)
	set_objects();

	// Read the tiles data
	for (tile_y=0; tile_y<room_y; tile_y++)
	{
		if (opt_verbose)
			print("    ");	// Start of a line
		for(tile_x=0; tile_x<room_x; tile_x++)
		{
			pixel_x = gl_off_x+tile_x*32;
			pixel_y = gl_off_y+tile_y*16;


			// A tile is 32(x)*16(y)*4(bits) = 256 bytes
			// A specific room tile is identified by a word
			tile_data = readword((u8*)fbuffer[ROOMS], ROOMS_START+offset);

			displaySprite(pixel_x,pixel_y,32,16, 
				cell_texid[(tile_data>>7) + ((room_index>0x202)?0x1E0:0)]);

			// Display sprite overlay
			set_overlays(pixel_x, pixel_y, ROOMS_START+offset, room_x);

			offset +=2;		// Read next tile

			if (opt_verbose)
				print("%04X ", tile_data);
		}
		if (opt_verbose)
			print("\n");
	}

	if (opt_debug)
		print("\n");


	// Let add our guy
	overlay[overlay_index].sid = 0x3;
	overlay[overlay_index].x = gl_off_x + room_x*16;
	overlay[overlay_index++].y = gl_off_y + room_y*8;

	// Now that the background is done, and that we have the overlays, display the overlay sprites
	display_overlays();

//	displaySprite(100,100,32,16,cell_texid[4]);
//	displaySprite(32,0,32,16,cell_texid[5]);


}

