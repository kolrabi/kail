//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2001-2009 by Denton Woods
// Last modified: 03/07/2009
//
// Filename: src-IL/src/il_icon.c
//
// Description: Reads from a Windows icon (.ico) file.
//
//-----------------------------------------------------------------------------


#include "il_internal.h"
#ifndef IL_NO_ICO
#include "il_icon.h"
#ifndef IL_NO_PNG
  #include <png.h>
#endif

#ifndef _WIN32
  #ifdef HAVE_STDINT_H
    #include <stdint.h>
    typedef uint32_t DWORD, *LPDWORD;
    typedef uint16_t WORD, *LPWORD;
    typedef uint8_t  BYTE, *LPBYTE;
  #else 
    // let's just hope for the best
    #warning "TODO: add correct definitions for your platform"
    typedef ILuint   DWORD, *LPDWORD;
    typedef ILushort WORD, *LPWORD;
    typedef ILuchar  BYTE, *LPBYTE;
  #endif
#endif

#ifndef IL_NO_PNG
static ILboolean iLoadIconPNG(SIO *io, ICOIMAGE *Icon);
#endif

// Internal function to get the header and check it.
static ILboolean iIsValidIcon(SIO *io)
{
  ICODIR dir;
  ICODIRENTRY entry;

  // Icons are difficult to detect, but it may suffice
  // We need at least one correct image header in the icon
  // Not sure whether big endian architectures are a concern in 2014
  ILuint Start = SIOtell(io);
  ILuint read = SIOread(io, &dir, 1, sizeof(dir));
  read += SIOread(io, &entry, 1, sizeof(entry));
  SIOseek(io, Start, IL_SEEK_SET);

  if (read != sizeof(dir) + sizeof(entry))
    return IL_FALSE;

  if (dir.Reserved != 0)
    return IL_FALSE;

  if (dir.Type != 1 && dir.Type != 2)
    return IL_FALSE;

  if (dir.Count == 0)
    return IL_FALSE;

  if (entry.Reserved != 0 && entry.Reserved != 0xFF)
    return IL_FALSE;

  if (entry.SizeOfData == 0 && entry.Offset < read)
    return IL_FALSE;

  return IL_TRUE;
}

// Internal function used to load the icon.
static ILboolean iLoadIconInternal(ILimage* image) {
  ICODIR    IconDir;
  ICODIRENTRY *DirEntries = NULL;
  ICOIMAGE  *IconImages = NULL;
  ILimage   *Image = NULL;
  ILint     i;
  ILuint    Size, PadSize, ANDPadSize, j, k, l, m, x, w, CurAndByte; //, AndBytes;
  ILuint    Bps;
  ILboolean BaseCreated = IL_FALSE;
  ILubyte   PNGTest[3];
  SIO *io;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  if (SIOread(io, &IconDir, 1, sizeof(IconDir)) != sizeof(IconDir))
    return IL_FALSE;

  Short(&IconDir.Reserved);
  Short(&IconDir.Type);
  Short(&IconDir.Count);

  if (IconDir.Reserved != 0) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  if (IconDir.Type != 1 && IconDir.Type != 2) {
    iSetError(IL_INVALID_FILE_HEADER);
    return IL_FALSE;
  }

  if (IconDir.Count == 0) {
    return IL_FALSE;
  }

  DirEntries = (ICODIRENTRY*)icalloc(sizeof(ICODIRENTRY), IconDir.Count);
  IconImages = (ICOIMAGE*)   icalloc(sizeof(ICOIMAGE),    IconDir.Count);

  if (DirEntries == NULL || IconImages == NULL) {
    ifree(DirEntries);
    ifree(IconImages);
    return IL_FALSE;
  }

  for (i = 0; i < IconDir.Count; ++i) {
    if (SIOread(io, DirEntries + i, 1, sizeof(ICODIRENTRY)) != sizeof(ICODIRENTRY)) {
      if (i == 0) 
        goto file_read_error;
      break;
      // return IL_FALSE;
    }

    Short(&DirEntries[i].Planes);
    Short(&DirEntries[i].Bpp);
    UInt (&DirEntries[i].SizeOfData);
    UInt (&DirEntries[i].Offset);
  }

  for (i = 0; i < IconDir.Count; i++) {
    SIOseek(io, DirEntries[i].Offset, IL_SEEK_SET);

    // 08-22-2008: Adding test for compressed PNG data
    SIOgetc(io); // Skip the first character...seems to vary.
    if (SIOread(io, PNGTest, 3, 1) && memcmp(PNGTest, "PNG", 3) == 0)  // Characters 'P', 'N', 'G' for PNG header
    {
#ifdef IL_NO_PNG
      iTrace("**** PNG icons not supported, IL was built with IL_NO_PNG");
      iSetError(IL_FORMAT_NOT_SUPPORTED);  // Cannot handle these without libpng.
      goto file_read_error;
#else
      SIOseek(io, DirEntries[i].Offset, IL_SEEK_SET);
      if (!iLoadIconPNG(io, &IconImages[i]))
        goto file_read_error;
#endif
    }
    else
    {
      // Need to go back the 4 bytes that were just read.
      SIOseek(io, DirEntries[i].Offset, IL_SEEK_SET);

      if (SIOread(io, &IconImages[i].Head, 1, sizeof(INFOHEAD)) != sizeof(INFOHEAD)) {
        goto file_read_error;
      }

      Int  (&IconImages[i].Head.Size);
      Int  (&IconImages[i].Head.Width);
      Int  (&IconImages[i].Head.Height);
      Short(&IconImages[i].Head.Planes);
      Short(&IconImages[i].Head.BitCount);
      Int  (&IconImages[i].Head.Compression);
      Int  (&IconImages[i].Head.SizeImage);
      Int  (&IconImages[i].Head.XPixPerMeter);
      Int  (&IconImages[i].Head.YPixPerMeter);
      Int  (&IconImages[i].Head.ColourUsed);
      Int  (&IconImages[i].Head.ColourImportant);

      if (IconImages[i].Head.BitCount < 8) {
        if (IconImages[i].Head.ColourUsed == 0) {
          switch (IconImages[i].Head.BitCount)
          {
            case 1:
              IconImages[i].Head.ColourUsed = 2;
              break;
            case 4:
              IconImages[i].Head.ColourUsed = 16;
              break;
          }
        }
        IconImages[i].Pal = (ILubyte*)ialloc(IconImages[i].Head.ColourUsed * 4);
        if (IconImages[i].Pal == NULL)
          goto file_read_error;  // @TODO: Rename the label.
        if (SIOread(io, IconImages[i].Pal, IconImages[i].Head.ColourUsed * 4, 1) != 1)
          goto file_read_error;
      }
      else if (IconImages[i].Head.BitCount == 8) {
        IconImages[i].Pal = (ILubyte*)ialloc(256 * 4);
        if (IconImages[i].Pal == NULL)
          goto file_read_error;
        if (SIOread(io, IconImages[i].Pal, 1, 256 * 4) != 256*4)
          goto file_read_error;
      }
      else {
        IconImages[i].Pal = NULL;
      }
      
      Bps        = (ILuint)((IconImages[i].Head.Width*IconImages[i].Head.BitCount + 7) / 8);
      PadSize    = (4 - Bps % 4) % 4;  // Has to be DWORD-aligned.
      ANDPadSize = (4 - ((IconImages[i].Head.Width                             + 7) / 8) % 4) % 4;  // AND is 1 bit/pixel
      Size =       (    Bps + PadSize) * (ILuint)(IconImages[i].Head.Height / 2);

      if (IconImages[i].Head.SizeImage > Size)
        Size = IconImages[i].Head.SizeImage;

      IconImages[i].Data = (ILubyte*)ialloc(Size);
      if (IconImages[i].Data == NULL)
        goto file_read_error;

      if (SIOread(io, IconImages[i].Data, 1, Size) != Size)
        goto file_read_error;

      Size = (Bps + ANDPadSize) * (ILuint)(IconImages[i].Head.Height / 2);

      if (IconImages[i].Head.BitCount < 32) {
        IconImages[i].AND = (ILubyte*)ialloc(Size);
        if (IconImages[i].AND == NULL)
          goto file_read_error;

        if (SIOread(io, IconImages[i].AND, 1, Size) != Size) {
          ifree(IconImages[i].AND);
          //goto file_read_error;
        }
      }
    }
  }

  for (i = 0; i < IconDir.Count; i++) {
    if (IconImages[i].Head.BitCount != 1 && IconImages[i].Head.BitCount != 4 &&
        IconImages[i].Head.BitCount != 8 && IconImages[i].Head.BitCount != 24 &&
        IconImages[i].Head.BitCount != 32) {
      iTrace("**** Skipping icon %d. Unsupported bit count %d", i, IconImages[i].Head.BitCount);
      continue;
    }

    if (!BaseCreated) {
      // first icon
      if (IconImages[i].Head.Size == 0) { // PNG compressed icon
        iTexImage(image, IconImages[i].Head.Width, IconImages[i].Head.Height, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
      } else {
        iTexImage(image, IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, IL_BGRA, IL_UNSIGNED_BYTE, NULL);
      }
      Image         = image;
      BaseCreated   = IL_TRUE;
    } else {
      // next icon
      if (IconImages[i].Head.Size == 0) { // PNG compressed icon
        Image->Next = iNewImage(IconImages[i].Head.Width, IconImages[i].Head.Height, 1, 4, 1);
      } else {
        Image->Next = iNewImage(IconImages[i].Head.Width, IconImages[i].Head.Height / 2, 1, 4, 1);
      }
      Image         = Image->Next;
      Image->Format = IL_BGRA;
    }
    Image->Origin = IL_ORIGIN_LOWER_LEFT;
    Image->Type   = IL_UNSIGNED_BYTE;

    j = 0;  k = 0;  l = 128;  CurAndByte = 0; x = 0;

    w = IconImages[i].Head.Width;
    PadSize = (4 - ((w*IconImages[i].Head.BitCount + 7) / 8) % 4) % 4;  // Has to be DWORD-aligned.

    ANDPadSize = (4 - ((w + 7) / 8) % 4) % 4;  // AND is 1 bit/pixel
    // AndBytes = (w + 7) / 8;

    if (IconImages[i].Head.BitCount == 1) {
      // monochrome icon
      for (; j < Image->SizeOfData; k++) {
        for (m = 128; m && x < w; m >>= 1) {
          Image->Data[j  ] =  IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4    ];
          Image->Data[j+1] =  IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 1];
          Image->Data[j+2] =  IconImages[i].Pal[!!(IconImages[i].Data[k] & m) * 4 + 2];
          Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
          j += 4;
          l >>= 1;

          ++x;
        }

        if (l == 0 || x == w) {
          l = 128;
          CurAndByte++;
          if (x == w) {
            CurAndByte += ANDPadSize;
            k += PadSize;
            x = 0;
          }
        }
      }
    } else if (IconImages[i].Head.BitCount == 4) {
      // 16 color icon
      for (; j < Image->SizeOfData; j += 8, k++) {
        Image->Data[j  ] =  IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4    ];
        Image->Data[j+1] =  IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 1];
        Image->Data[j+2] =  IconImages[i].Pal[((IconImages[i].Data[k] & 0xF0) >> 4) * 4 + 2];
        Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
        l >>= 1;

        ++x;

        if(x < w) {
          Image->Data[j+4] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4];
          Image->Data[j+5] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 1];
          Image->Data[j+6] = IconImages[i].Pal[(IconImages[i].Data[k] & 0x0F) * 4 + 2];
          Image->Data[j+7] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
          l >>= 1;

          ++x;
        } else {
          j -= 4;
        }

        if (l == 0 || x == w) {
          l = 128;
          CurAndByte++;
          if (x == w) {
            CurAndByte += ANDPadSize;

            k += PadSize;
            x = 0;
          }
        }
      }
    } else if (IconImages[i].Head.BitCount == 8) {
      // 256 color palette
      for (; j < Image->SizeOfData; j += 4, k++) {
        Image->Data[j  ] = IconImages[i].Pal[IconImages[i].Data[k] * 4    ];
        Image->Data[j+1] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 1];
        Image->Data[j+2] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 2];
        if (IconImages[i].AND == NULL) { // PNG Palette
          Image->Data[j+3] = IconImages[i].Pal[IconImages[i].Data[k] * 4 + 3];
        } else {
          Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
        }
        l >>= 1;

        ++x;
        if (l == 0 || x == w) {
          l = 128;
          CurAndByte++;
          if (x == w) {
            CurAndByte += ANDPadSize;

            k += PadSize;
            x = 0;
          }
        }
      }
    } else if (IconImages[i].Head.BitCount == 24) {
      // 24 bit true color, transparency from AND
      for (; j < Image->SizeOfData; j += 4, k += 3) {
        Image->Data[j  ] = IconImages[i].Data[k  ];
        Image->Data[j+1] = IconImages[i].Data[k+1];
        Image->Data[j+2] = IconImages[i].Data[k+2];
        if (IconImages[i].AND != NULL)
          Image->Data[j+3] = (IconImages[i].AND[CurAndByte] & l) != 0 ? 0 : 255;
        l >>= 1;

        ++x;
        if (l == 0 || x == w) {
          l = 128;
          CurAndByte++;
          if (x == w) {
            CurAndByte += ANDPadSize;

            k += PadSize;
            x = 0;
          }
        }
      }
    } else if (IconImages[i].Head.BitCount == 32) {
      // 32 bit trucolor + alpha
      for (; j < Image->SizeOfData; j += 4, k += 4) {
        Image->Data[j  ] = IconImages[i].Data[k];
        Image->Data[j+1] = IconImages[i].Data[k+1];
        Image->Data[j+2] = IconImages[i].Data[k+2];

        //If the icon has 4 channels, use 4th channel for alpha...
        //(for Windows XP style icons with true alpha channel
        Image->Data[j+3] = IconImages[i].Data[k+3];
      }
    }
  }

  // cleanup
  for (i = 0; i < IconDir.Count; i++) {
    ifree(IconImages[i].Pal);
    ifree(IconImages[i].Data);
    ifree(IconImages[i].AND);
  }
  ifree(IconImages);
  ifree(DirEntries);

  return IL_TRUE;

file_read_error:
  if (IconImages) {
    for (i = 0; i < IconDir.Count; i++) {
      if (IconImages[i].Pal)
        ifree(IconImages[i].Pal);
      if (IconImages[i].Data)
        ifree(IconImages[i].Data);
      if (IconImages[i].AND)
        ifree(IconImages[i].AND);
    }
    ifree(IconImages);
  }
  if (DirEntries)
    ifree(DirEntries);
  return IL_FALSE;
}

// Internal function used to load the icon.
static ILboolean iSaveIconInternal(ILimage* image) {
  ILimage   *Image = NULL;
  SIO *io;
  ILuint    Start, ImageNum;

  if (image == NULL) {
    iSetError(IL_ILLEGAL_OPERATION);
    return IL_FALSE;
  }

  io = &image->io;

  Start = SIOtell(io);

  SaveLittleUShort(io, 0); // Reserved (must be 0)
  SaveLittleUShort(io, 1); // Type (1 for icons, 2 for cursors)
  SaveLittleUShort(io, iGetInteger(image, IL_NUM_IMAGES)+1);  // How many different images?

  for (Image = image; Image; Image = Image->Next) {
    if (Image->Width > 256 || Image->Height > 256) {
      iTrace("----- Image dimension larger than 256. Icon file won't be compatible with older system.")
    }

    SIOputc(io, Image->Width  > 256 ? 0 : (ILubyte)Image->Width);  // Width, in pixels
    SIOputc(io, Image->Height > 256 ? 0 : (ILubyte)Image->Height); // Height, in pixels
    SIOputc(io, 0); // Number of colors in image (0 if >=8bpp)
    SIOputc(io, 0); // Reserved (must be 0)
    SaveLittleUShort(io, 0); // Colour planes
    SaveLittleUShort(io, 0); // Bits per pixel
    SaveLittleUInt(io, 0);   // How many bytes in this resource? (Update later)
    SaveLittleUInt(io, 0);  // Offset from beginning of the file (Update later)
  }

  ImageNum = 0;
  for (Image = image; Image; Image = Image->Next) {
    ILuint Pos = SIOtell(io);
    ILuint Pos2;
    ILubyte *Buffer = iConvertBuffer(Image->SizeOfData, Image->Format, IL_BGRA, Image->Type, IL_UNSIGNED_BYTE, &Image->Pal, Image->Data);

    if (!Buffer) {
      iSetError(IL_INTERNAL_ERROR);
      return IL_FALSE;
    }

    SIOseek(io, Start + 6 + sizeof(ICODIRENTRY) * ImageNum + 12, IL_SEEK_SET);
    SaveLittleUInt(io, Pos);
    SIOseek(io, Pos, IL_SEEK_SET);

    // TODO: add support for PNG writing

    SaveLittleUInt(io, sizeof(INFOHEAD));
    SaveLittleUInt(io, Image->Width);
    SaveLittleUInt(io, Image->Height * 2);
    SaveLittleUShort(io, 1);
    SaveLittleUShort(io, 32);
    SaveLittleUInt(io, 0);
    SaveLittleUInt(io, Image->Width * Image->Height * 4);
    SaveLittleUInt(io, 0);
    SaveLittleUInt(io, 0);
    SaveLittleUInt(io, 0);
    SaveLittleUInt(io, 0);

    if (!SIOwrite(io, Buffer, Image->Width * Image->Height * 4, 1)) {
      ifree(Buffer);
      return IL_FALSE;
    }
    Pos2 = SIOtell(io);

    SIOseek(io, Start + 6 + sizeof(ICODIRENTRY) * ImageNum +  8, IL_SEEK_SET);
    SaveLittleUInt(io, Pos2-Pos);
    SIOseek(io, Pos2, IL_SEEK_SET);

    ifree(Buffer);

    ImageNum++;
  }

  return IL_TRUE;
}

#ifndef IL_NO_PNG
// 08-22-2008: Copying a lot of this over from il_png.c for the moment.
// @TODO: Make .ico and .png use the same functions.
struct IconData {
  png_structp ico_png_ptr;
  png_infop   ico_info_ptr;
  ILint   ico_color_type;
};

static void ico_png_read(png_structp png_ptr, png_bytep data, png_size_t length) {
  SIO *io = (SIO*)png_get_io_ptr(png_ptr);
  SIOread(io, data, 1, (ILuint)length);
}

static void ico_png_error_func(png_structp png_ptr, png_const_charp message) NORETURN;
static void ico_png_error_func(png_structp png_ptr, png_const_charp message) {
  iTrace("**** PNG Error : %s", message);
  iSetError(IL_LIB_PNG_ERROR);
  longjmp(png_jmpbuf(png_ptr), 1);
}

static void ico_png_warn_func(png_structp png_ptr, png_const_charp message) {
  (void)png_ptr;
  iTrace("**** PNG Warning : %s", message);
}

static ILint ico_readpng_init(SIO *io, struct IconData* data) {
  if (data == NULL)
    return 1;

  data->ico_png_ptr = NULL;

  data->ico_png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, ico_png_error_func, ico_png_warn_func);
  if (!data->ico_png_ptr)
    return 4; /* out of memory */

  data->ico_info_ptr = png_create_info_struct(data->ico_png_ptr);
  if (!data->ico_info_ptr) {
    png_destroy_read_struct(&data->ico_png_ptr, NULL, NULL);
    return 4; /* out of memory */
  }


  /* we could create a second info struct here (end_info), but it's only
   * useful if we want to keep pre- and post-IDAT chunk info separated
   * (mainly for PNG-aware image editors and converters) */


  /* setjmp() must be called in every function that calls a PNG-reading
   * libpng function */

  if (setjmp(png_jmpbuf(data->ico_png_ptr))) {
    png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
    return 2;
  }

  png_set_read_fn(data->ico_png_ptr, io, ico_png_read);
  png_set_error_fn(data->ico_png_ptr, NULL, ico_png_error_func, ico_png_warn_func);

//  png_set_sig_bytes(ico_png_ptr, 8);  /* we already read the 8 signature bytes */

  png_read_info(data->ico_png_ptr, data->ico_info_ptr);  /* read all PNG info up to image data */

  /* alternatively, could make separate calls to png_get_image_width(),
   * etc., but want bit_depth and ico_color_type for later [don't care about
   * compression_type and filter_type => NULLs] */

  /* OK, that's all we need for now; return happy */

  return 0;
}

static void ico_readpng_cleanup(struct IconData* data) {
  if (data->ico_png_ptr && data->ico_info_ptr) {
    png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
    data->ico_png_ptr = NULL;
    data->ico_info_ptr = NULL;
  }
}

static ILboolean ico_readpng_get_image(struct IconData* data, ICOIMAGE *Icon) {
  png_bytepp  row_pointers = NULL;
  png_uint_32 width, height; // Changed the type to fix AMD64 bit problems, thanks to Eric Werness
  ILuint    i;
  ILenum    format;
  png_colorp  palette;
  ILint   num_palette, j, bit_depth;

  /* setjmp() must be called in every function that calls a PNG-reading
   * libpng function */

  if (setjmp(png_jmpbuf(data->ico_png_ptr))) {
    png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
    return IL_FALSE;
  }

  png_get_IHDR(data->ico_png_ptr, data->ico_info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
               &bit_depth, &data->ico_color_type, NULL, NULL, NULL);

  // Expand low-bit-depth grayscale images to 8 bits
  if (data->ico_color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
    png_set_expand_gray_1_2_4_to_8(data->ico_png_ptr);
  }

  // Expand RGB images with transparency to full alpha channels
  //  so the data will be available as RGBA quartets.
  // But don't expand paletted images, since we want alpha palettes!
  if (png_get_valid(data->ico_png_ptr, data->ico_info_ptr, PNG_INFO_tRNS)
  && !(png_get_valid(data->ico_png_ptr, data->ico_info_ptr, PNG_INFO_PLTE)))
    png_set_tRNS_to_alpha(data->ico_png_ptr);

  //refresh information (added 20040224)
  png_get_IHDR(data->ico_png_ptr, data->ico_info_ptr, (png_uint_32*)&width, (png_uint_32*)&height,
               &bit_depth, &data->ico_color_type, NULL, NULL, NULL);

  if (bit_depth < 8) {  // Expanded earlier for grayscale, now take care of palette and rgb
    //bit_depth = 8;
    png_set_packing(data->ico_png_ptr);
  }

  // Perform gamma correction.
  // @TODO:  Determine if we should call png_set_gamma if image_gamma is 1.0.

  //fix endianess
#ifdef WORDS_LITTLEENDIAN
  if (bit_depth == 16)
    png_set_swap(data->ico_png_ptr);
#endif

  png_read_update_info(data->ico_png_ptr, data->ico_info_ptr);
  // channels = (ILint)png_get_channels(data->ico_png_ptr, data->ico_info_ptr);
  // added 20040224: update ico_color_type so that it has the correct value
  // in iLoadPngInternal
  data->ico_color_type = png_get_color_type(data->ico_png_ptr, data->ico_info_ptr);

  // Determine internal format
  switch (data->ico_color_type)
  {
    case PNG_COLOR_TYPE_PALETTE:
      Icon->Head.BitCount = 8;
      format = IL_COLOUR_INDEX;
      break;
    case PNG_COLOR_TYPE_RGB:
      Icon->Head.BitCount = 24;
      format = IL_RGB;
      break;
    case PNG_COLOR_TYPE_RGB_ALPHA:
      Icon->Head.BitCount = 32;
      format = IL_RGBA;
      break;
    default:
      iSetError(IL_ILLEGAL_FILE_VALUE);
      png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
      return IL_FALSE;
  }

  if (data->ico_color_type & PNG_COLOR_MASK_COLOR)
    png_set_bgr(data->ico_png_ptr);

  Icon->Head.Width = width;
  Icon->Head.Height = height;
  Icon->Data = (ILubyte*) ialloc(width * height * Icon->Head.BitCount / 8);
  if (Icon->Data == NULL)
  {
    png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
    return IL_FALSE;
  }

  // Copy Palette
  if (format == IL_COLOUR_INDEX)
  {
    ILubyte chans;
    png_bytep trans = NULL;
    ILint num_trans = -1;
    if (!png_get_PLTE(data->ico_png_ptr, data->ico_info_ptr, &palette, &num_palette)) {
      iSetError(IL_ILLEGAL_FILE_VALUE);
      png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
      return IL_FALSE;
    }

    chans = 4;

    if (png_get_valid(data->ico_png_ptr, data->ico_info_ptr, PNG_INFO_tRNS)) {
      png_get_tRNS(data->ico_png_ptr, data->ico_info_ptr, &trans, &num_trans, NULL);
    }

// @TODO: We may need to keep track of the size of the palette.
    Icon->Pal = (ILubyte*)ialloc((ILuint)num_palette * chans);

    for (j = 0; j < num_palette; ++j) {
      Icon->Pal[chans*j + 0] = palette[j].blue;
      Icon->Pal[chans*j + 1] = palette[j].green;
      Icon->Pal[chans*j + 2] = palette[j].red;
      if (trans != NULL) {
        if (j < num_trans)
          Icon->Pal[chans*j + 3] = trans[j];
        else
          Icon->Pal[chans*j + 3] = 255;
      }
    }

    Icon->AND = NULL;  // Transparency information is obtained from libpng.
  }

  //allocate row pointers
  if ((row_pointers = (png_bytepp)ialloc(height * sizeof(png_bytep))) == NULL) {
    png_destroy_read_struct(&data->ico_png_ptr, &data->ico_info_ptr, NULL);
    return IL_FALSE;
  }


  // Set the individual row_pointers to point at the correct offsets
  // Needs to be flipped
  for (i = 0; i < height; i++)
    row_pointers[height - i - 1] = Icon->Data + i * width * Icon->Head.BitCount / 8;
    //row_pointers[i] = Icon->Data + i * width * Icon->Head.BitCount / 8;


  // Now we can go ahead and just read the whole image
  png_read_image(data->ico_png_ptr, row_pointers);

  /* and we're done!  (png_read_end() can be omitted if no processing of
   * post-IDAT text/time/etc. is desired) */
  //png_read_end(ico_png_ptr, NULL);
  ifree(row_pointers);

  return IL_TRUE;
}

static ILboolean iLoadIconPNG(SIO *io, struct ICOIMAGE *Icon) {
  struct IconData data;
  ILint init = ico_readpng_init(io, &data);
  if (init)
    return IL_FALSE;
  if (!ico_readpng_get_image(&data, Icon))
    return IL_FALSE;

  ico_readpng_cleanup(&data);
  Icon->Head.Size = 0;  // Easiest way to tell that this is a PNG.
              // In the normal routine, it will not be 0.

  return IL_TRUE;
}
#else
/*
static ILboolean iLoadIconPNG(SIO *io, struct ICOIMAGE *Icon) {
  (void)io;
  (void)Icon;
  return IL_FALSE;
}
*/
#endif

static ILconst_string iFormatExtsICO[] = { 
  IL_TEXT("ico"), 
  IL_TEXT("cur"), 
  NULL 
};

ILformat iFormatICO = { 
  /* .Validate = */ iIsValidIcon, 
  /* .Load     = */ iLoadIconInternal, 
  /* .Save     = */ iSaveIconInternal, 
  /* .Exts     = */ iFormatExtsICO
};

#endif//IL_NO_ICO
