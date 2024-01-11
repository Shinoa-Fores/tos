#help_index "Graphics/Color"
public U8 gr_rainbow_10[10]={
  BLACK,BROWN,RED,LTRED,YELLOW,GREEN,BLUE,PURPLE,LTGRAY,WHITE};

DefineLstLoad("ST_RAINBOW_10",
  "BLACK\0BROWN\0RED\0LTRED\0YELLOW\0GREEN\0BLUE\0PURPLE\0LTGRAY\0WHITE\0");

U0 GrPaletteIndicesSet()
{//There is a level of indirection that we identity map.
// Set 16-colors to use first 16 DAC BGR entries, so we
// never need to worry about indexes, just DAC palette settings.
  I64 i;
}

//Eye saver as default palette.

public CBGR48 gr_palette_std[COLORS_NUM]={
  0xebebdbdbb2b2, // BLACK
  0x454585858888, // BLUE
  0x989897971a1a, // GREEN
  0x68689d9d6a6a, // CYAN
  0xcccc24241d1d, // RED
  0xb1b162628686, // PURPLE
  0xfefe80801919, // BROWN
  0x3c3c38383636, // LTGRAY
  0x505049494545, // DKGRAY
  0x8383a5a59898, // LTBLUE
  0xb8b8bbbb2626, // LTGREEN
  0x8e8ec0c07c7c, // LTCYAN
  0xfbfb49493434, // LTRED
  0xd3d386869b9b, // LTPURPLE
  0xfafabdbd2f2f, // YELLOW
  0x282828282828  // WHITE
};

public CBGR48 gr_palette_gray[COLORS_NUM]={
0x000000000000,0x111111111111,0x222222222222,0x333333333333,
0x444444444444,0x555555555555,0x666666666666,0x777777777777,
0x888888888888,0x999999999999,0xAAAAAAAAAAAA,0xBBBBBBBBBBBB,
0xCCCCCCCCCCCC,0xDDDDDDDDDDDD,0xEEEEEEEEEEEE,0xFFFFFFFFFFFF};

CBGR48 pal[16];

CBGR48 GrPaletteColorGet(I64 i)  {
	return pal[i];
}

public U0 GrPaletteGet(CBGR48 *bgr48)
{//16 colors
  I64 i;
  for (i=0;i<COLORS_NUM;i++)
    bgr48[i]=GrPaletteColorGet(i);
}

public U0 GrPaletteSet(CBGR48 *bgr48)
{//16 colors
  I64 i;
  for (i=0;i<COLORS_NUM;i++) {
    pal[i]=bgr48[i];
    _GrPaletteColorSet(i,bgr48[i]);
  }
}

public U0 GrPaletteColorSet(I64 color,CBGR48 bgr48)
{//16 colors
  pal[color]=bgr48;
  _GrPaletteColorSet(color,bgr48);
}


public U0 PaletteSetStd()
{//Activate std palette.
  GrPaletteSet(gr_palette_std);
}
