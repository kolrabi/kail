#if defined(_MSC_VER) || defined(MACOSX) || defined(__GNUC__)
  #pragma pack(push)
  #pragma pack(1)
#else
  #warning "Compiler does not seem support struct packing. Good luck!"
#endif
