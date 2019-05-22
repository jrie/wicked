//------------------------------------------------------------------------------
// Author: Jan Riechers <jan@dwrox.net>
//------------------------------------------------------------------------------
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>

//------------------------------------------------------------------------------

// Switches
#define BEVERBOSE false
#define DEBUG false
#define DOWRITEOUT true
#define LINESTOPROCESS 0
#define STRAIGHTWRITEOUT false
//#define LINESTOPROCESS 1085
//#define LINESTOPROCESS 110
//#define LINESTOPROCESS 128
//#define LINESTOPROCESS 861531
//#define LINESTOPROCESS 0

//#define SOURCEFILE "data/enwik8_small"
//#define SOURCEFILE "data/enwik8"
#define SOURCEFILE "data/enwiki-20160720-pages-meta-current1.xml-p000000010p000030303"

#if STRAIGHTWRITEOUT
#define DICTIONARYFILE "words.txt"
#define WIKITAGSFILE "wikitags.txt"
#define XMLTAGFILE "xmltags.txt"
#define XMLDATAFILE "xmldata.txt"
#define ENTITIESFILE "entities.txt"
#else
#define DICTIONARYFILE "words_sort.txt"
#define WIKITAGSFILE "wikitags_sort.txt"
#define XMLTAGFILE "xmltags_sort.txt"
#define XMLDATAFILE "xmldata_sort.txt"
#define ENTITIESFILE "entities_sort.txt"
#endif
/*
#define DICTIONARYFILE "data/words.txt"
#define WIKITAGSFILE "data/wikitags.txt"
#define XMLTAGFILE "data/xmltags.txt"
#define ENTITIESFILE "data/entities.txt"
*/

// Buffers
#define LINEBUFFERBASE 5120

// Counts of predefined const datatypes
#define FORMATS 8
#define ENTITIES 211
#define INDENTS 3
#define TEMPLATES 11
#define TAGTYPES 11
#define TAGCLOSINGS 3
#define MATHTAG 0

//------------------------------------------------------------------------------

// Custom datatypes
#pragma pack()
typedef struct keyValuePair {
  char *key;
  char *value;
} keyValuePair;

#pragma pack()
typedef struct word {
  unsigned int lineNum;
  unsigned int position;
  short dataFormatType;
  short ownFormatType;
  bool formatStart;
  bool formatEnd;
  bool hasPipe;
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  char *data;
} word;

#pragma pack()
typedef struct entity {
  unsigned int lineNum;
  unsigned int position;
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  short dataFormatType;
  short ownFormatType;
  bool formatStart;
  bool formatEnd;
  bool hasPipe;
  char data[8];
} entity;

#pragma pack()
typedef struct wikiTag {
  unsigned int lineNum;
  unsigned int position;
  unsigned int wordCount;
  unsigned int wTagCount;
  unsigned int entityCount;
  unsigned int tagLength;
  unsigned int wikiTagFileIndex;
  unsigned char tagType;
  unsigned char preSpacesCount;
  unsigned char spacesCount;
  short dataFormatType;
  short ownFormatType;
  bool formatStart;
  bool formatEnd;
  bool hasPipe;
  char *target;
  struct word *pipedWords;
  struct wikiTag *pipedTags;
  struct entity *pipedEntities;
} wikiTag;

#pragma pack()
typedef struct xmlNode {
  unsigned short indent;
  unsigned int keyValuePairs;
  unsigned int wordCount;
  unsigned int entityCount;
  unsigned int wTagCount;
  unsigned int start;
  unsigned int end;
  short firstAddedType; // 0 WORD, 1 WIKITAG, 2 ENTITY
  short lastAddedType; // 0 WORD, 1 WIKITAG, 2 ENTITY
  bool isDataNode;
  bool isClosed;
  char *name;
  struct keyValuePair *keyValues;
  struct word *words;
  struct entity *entities;
  struct wikiTag *wikiTags;
} xmlNode;

typedef struct xmlDataCollection {
  unsigned int count;
  unsigned int openNodeCount;
  unsigned int *openNodes;
  struct xmlNode *nodes;
} xmlDataCollection;

typedef struct collectionStatistics {
  unsigned int wordCount;
  unsigned int entityCount;
  unsigned int wikiTagCount;
  unsigned int keyCount;
  unsigned int valueCount;
  unsigned int byteWords;
  unsigned int byteEntites;
  unsigned int byteWikiTags;
  unsigned int byteWhitespace;
  unsigned int byteKeys;
  unsigned int byteValues;
  unsigned int bytePreWhiteSpace;
  unsigned int byteXMLsaved;
  unsigned int byteNewLine;
  unsigned int byteFormatting;
  unsigned int failedElements;
} collectionStatistics;

//------------------------------------------------------------------------------

typedef struct parserBaseStore {
  FILE* dictFile;
  FILE* wtagFile;
  FILE* xmltagFile;
  FILE* xmldataFile;
  FILE* entitiesFile;
  unsigned int currentPosition;
  unsigned int currentLine;
  bool isMathSection;
  struct xmlDataCollection* xmlCollection;
  struct collectionStatistics* cData;
} parserBaseStore;

//------------------------------------------------------------------------------

/*
  Wikipedia data types and such

  See for wikitags
  https://en.wikipedia.org/wiki/Help:Wiki_markup
  https://en.wikipedia.org/wiki/Help:Cheatsheet

  Reference for templates:
  https://en.wikipedia.org/wiki/Help:Wiki_markup
*/

const char formatNames[FORMATS][15] = {
  "Bold + Italic",
  "Bold",
  "Italic",
  "Heading 6",
  "Heading 5",
  "Heading 4",
  "Heading 3",
  "Heading 2"
};

const char formats[FORMATS][7] = {
  "'''''", // bold + italic
  "'''", // bold
  "''", // italic
  "======", // Heading 6 - heading start
  "=====",
  "====",
  "===",
  "==" // Heading 2 - heading end
};

/*
  NOTE: Indents are only used at the beginning of the line!
*/
const char indents[INDENTS][2] = {
  "*", // List element, multiple levels, "**" Element of element
  "#", // Numbered list element, multiple levels = "##" "Element of element"
  ":", // "indent 1", multiple levels using ":::" = 3
};

const char templates[TEMPLATES][18] = {
  // NOTE: Should we cover template tags as well for shortening?
  "{{-",
  "{{align",
  "{{break",
  "{{clear",
  "{{float",
  "{{stack",
  "{{outdent",
  "{{plainlist",
  "{{fake heading",
  "{{endplainlist",
  "{{unbulleted list"
};

const char wikiTagNames[TAGTYPES][19] = {
  "Math type",
  "Definition/Anchor",
  "Table",
  "Category",
  "Media type",
  "File type",
  "Image type",
  "Sound type",
  "Wiktionary",
  "Wikipedia",
  "Link"
};

const char tagTypes[TAGTYPES][18] = {
  // NOTE: Handle definitions after, in case we threat, templates too
  "{{math", // https://en.wikipedia.org/wiki/Wikipedia:Rendering_math
  "{{", // Definition => {{Main|autistic savant}}
  "{|", // Table start => ! (headline), |- (seperation, row/border), | "entry/ column data"
  "[[category:",
  "[[media:", // Media types start
  "[[file:",
  "[[image:", // [[Image:LeoTolstoy.jpg|thumb|150px|[[Leo Tolstoy|Leo Tolstoy]] 1828-1910]]
  "[[sound:", // Media types end
  //"#REDIRECT", // Redirect #REDIRECT [[United States]] (article) --- #REDIRECT [[United States#History]] (section)
  "[[wiktionary:", // [[wiktionary:terrace|terrace]]s
  "[[wikipedia:", // [[Wikipedia:Nupedia and Wikipedia]]
  "[[" // Link => [[Autistic community#Declaration from the autism community|sent a letter to the United Nations]]
};

const char tagClosingsTypes[TAGCLOSINGS][3] = {
  "}}",
  "]]",
  "|}"
};

const char entities[ENTITIES][2][32] = {
  //Commercial symbols
  {"trade", "\xe2\x84\xa2"},
  {"copy", "\xc2\xa9"},
  {"reg", "\xc2\xae"},
  {"cent", "\xc2\xa2"},
  {"euro", "\xe2\x82\xac"},
  {"yen", "\xc2\xa5"},
  {"pound", "\xc2\xa3"},
  {"curren", "\xc2\xa4"},
  //Diacritical marks
  {"Agrave", "\xc3\x80"},
  {"Aacute", "\xc3\x81"},
  {"Acirc", "\xc3\x82"},
  {"Atilde", "\xc3\x83"},
  {"Auml", "\xc3\x84"},
  {"Aring", "\xc3\x85"},
  {"AElig", "\xc3\x86"},
  {"Ccedil", "\xc3\x87"},
  {"Egrave", "\xc3\x88"},
  {"Eacute", "\xc3\x89"},
  {"Ecirc", "\xc3\x8a"},
  {"Euml", "\xc3\x8b"},
  {"Igrave", "\xc3\x8c"},
  {"Iacute", "\xc3\x8d"},
  {"Icirc", "\xc3\x8e"},
  {"Iuml", "\xc3\x8f"},
  {"Ntilde", "\xc3\x91"},
  {"Ograve", "\xc3\x92"},
  {"Oacute", "\xc3\x93"},
  {"Ocirc", "\xc3\x94"},
  {"Otilde", "\xc3\x95"},
  {"Ouml", "\xc3\x96"},
  {"Oslash", "\xc3\x98"},
  {"OElig", "\xc5\x92"},
  {"Ugrave", "\xc3\x99"},
  {"Uacute", "\xc3\x9a"},
  {"Ucirc", "\xc3\x9b"},
  {"Uuml", "\xc3\x9c"},
  {"Yuml", "\xc5\xb8"},
  {"szlig", "\xc3\x9f"},
  {"agrave", "\xc3\xa0"},
  {"aacute", "\xc3\xa1"},
  {"acirc", "\xc3\xa2"},
  {"atilde", "\xc3\xa3"},
  {"auml", "\xc3\xa4"},
  {"aring", "\xc3\xa5"},
  {"aelig", "\xc3\xa6"},
  {"ccedil", "\xc3\xa7"},
  {"egrave", "\xc3\xa8"},
  {"eacute", "\xc3\xa9"},
  {"ecirc", "\xc3\xaa"},
  {"euml", "\xc3\xab"},
  {"igrave", "\xc3\xac"},
  {"iacute", "\xc3\xad"},
  {"icirc", "\xc3\xae"},
  {"iuml", "\xc3\xaf"},
  {"ntilde", "\xc3\xb1"},
  {"ograve", "\xc3\xb2"},
  {"oacute", "\xc3\xb3"},
  {"ocirc", "\xc3\xb4"},
  {"otilde", "\xc3\xb5"},
  {"ouml", "\xc3\xb6"},
  {"oslash", "\xc3\xb8"},
  {"oelig", "\xc5\x93"},
  {"ugrave", "\xc3\xb9"},
  {"uacute", "\xc3\xba"},
  {"ucirc", "\xc3\xbb"},
  {"uuml", "\xc3\xbc"},
  {"yuml", "\xc3\xbf"},
  //Greek characters
  {"alpha", "\xce\xb1"},
  {"beta", "\xce\xb2"},
  {"gamma", "\xce\xb3"},
  {"delta", "\xce\xb4"},
  {"epsilon", "\xce\xb5"},
  {"zeta", "\xce\xb6"},
  {"Alpha", "\xce\x91"},
  {"Beta", "\xce\x92"},
  {"Gamma", "\xce\x93"},
  {"Delta", "\xce\x94"},
  {"Epsilon", "\xce\x95"},
  {"Zeta", "\xce\x96"},
  {"eta", "\xce\xb7"},
  {"theta", "\xce\xb8"},
  {"iota", "\xce\xb9"},
  {"kappa", "\xce\xba"},
  {"lambda", "\xce\xbb"},
  {"mu", "\xce\xbc"},
  {"nu", "\xce\xbd"},
  {"Eta", "\xce\x97"},
  {"Theta", "\xce\x98"},
  {"Iota", "\xce\x99"},
  {"Kappa", "\xce\x9a"},
  {"Lambda", "\xce\x9b"},
  {"Mu", "\xce\x9c"},
  {"Nu", "\xce\x9d"},
  {"xi", "\xce\xbe"},
  {"omicron", "\xce\xbf"},
  {"pi", "\xcf\x80"},
  {"rho", "\xcf\x81"},
  {"sigma", "\xcf\x83"},
  {"sigmaf", "\xcf\x82"},
  {"Xi", "\xce\x9e"},
  {"Omicron", "\xce\x9f"},
  {"Pi", "\xce\xa0"},
  {"Rho", "\xce\xa1"},
  {"Sigma", "\xce\xa3"},
  {"tau", "\xcf\x84"},
  {"upsilon", "\xcf\x85"},
  {"phi", "\xcf\x86"},
  {"chi", "\xcf\x87"},
  {"psi", "\xcf\x88"},
  {"omega", "\xcf\x89"},
  {"Tau", "\xce\xa4"},
  {"Upsilon", "\xce\xa5"},
  {"Phi", "\xce\xa6"},
  {"Chi", "\xce\xa7"},
  {"Psi", "\xce\xa8"},
  {"Omega", "\xce\xa9"},
  //Mathematical characters and formulae
  {"int", "\xe2\x88\xab"},
  {"sum", "\xe2\x88\x91"},
  {"prod", "\xe2\x88\x8f"},
  {"radic", "\xe2\x88\x9a"},
  {"minus", "\xe2\x88\x92"},
  {"plusmn", "\xc2\xb1"},
  {"infin", "\xe2\x88\x9e"},
  {"asymp", "\xe2\x89\x88"},
  {"prop", "\xe2\x88\x9d"},
  {"equiv", "\xe2\x89\xa1"},
  {"ne", "\xe2\x89\xa0"},
  {"le", "\xe2\x89\xa4"},
  {"ge", "\xe2\x89\xa5"},
  {"times", "\xc3\x97"},
  {"middot", "\xc2\xb7"},
  {"divide", "\xc3\xb7"},
  {"part", "\xe2\x88\x82"},
  {"prime", "\xe2\x80\xb2"},
  {"Prime", "\xe2\x80\xb3"},
  {"nabla", "\xe2\x88\x87"},
  {"permil", "\xe2\x80\xb0"},
  {"deg", "\xc2\xb0"},
  {"there4", "\xe2\x88\xb4"},
  {"alefsym", "\xe2\x84\xb5"},
  {"oslash", "\xc3\xb8"},
  {"isin", "\xe2\x88\x88"},
  {"notin", "\xe2\x88\x89"},
  {"cap", "\xe2\x88\xa9"},
  {"cup", "\xe2\x88\xaa"},
  {"sub", "\xe2\x8a\x82"},
  {"sup", "\xe2\x8a\x83"},
  {"sube", "\xe2\x8a\x86"},
  {"supe", "\xe2\x8a\x87"},
  {"not", "\xc2\xac"},
  {"and", "\xe2\x88\xa7"},
  {"or", "\xe2\x88\xa8"},
  {"exist", "\xe2\x88\x83"},
  {"forall", "\xe2\x88\x80"},
  {"rArr", "\xe2\x87\x92"},
  {"lArr", "\xe2\x87\x90"},
  {"dArr", "\xe2\x87\x93"},
  {"uArr", "\xe2\x87\x91"},
  {"hArr", "\xe2\x87\x94"},
  {"rarr", "\xe2\x86\x92"},
  {"darr", "\xe2\x86\x93"},
  {"uarr", "\xe2\x86\x91"},
  {"larr", "\xe2\x86\x90"},
  {"harr", "\xe2\x86\x94"},
  //Predefined entities in XML
  {"quot", "\""},
  {"amp", "&"},
  {"apos", "'"},
  {"lt", "<"},
  {"gt", ">"},
  //Punctuation special characters
  {"iquest", "\xc2\xbf"},
  {"iexcl", "\xc2\xa1"},
  {"sect", "\xc2\xa7"},
  {"para", "\xc2\xb6"},
  {"dagger", "\xe2\x80\xa0"},
  {"Dagger", "\xe2\x80\xa1"},
  {"bull", "\xe2\x80\xa2"},
  {"ndash", "\xe2\x80\x93"},
  {"mdash", "\xe2\x80\x94"},
  {"lsaquo", "\xe2\x80\xb9"},
  {"rsaquo", "\xe2\x80\xba"},
  {"laquo", "\xc2\xab"},
  {"raquo", "\xc2\xbb"},
  {"lsquo", "\xe2\x80\x98"},
  {"rsquo", "\xe2\x80\x99"},
  {"ldquo", "\xe2\x80\x9c"},
  {"rdquo", "\xe2\x80\x9d"},
  {"apos", "'"},
  //Subscripts and superscripts
  {"#8320", "\xe2\x82\x80"},
  {"#8321", "\xe2\x82\x81"},
  {"#8322", "\xe2\x82\x82"},
  {"#8323", "\xe2\x82\x83"},
  {"#8324", "\xe2\x82\x84"},
  {"#8325", "\xe2\x82\x85"},
  {"#8326", "\xe2\x82\x86"},
  {"#8327", "\xe2\x82\x87"},
  {"#8328", "\xe2\x82\x88"},
  {"#8329", "\xe2\x82\x89"},
  {"#8304", "\xe2\x81\xb0"},
  {"sup1", "\xc2\xb9"},
  {"sup2", "\xc2\xb2"},
  {"sup3", "\xc2\xb3"},
  {"#8308", "\xe2\x81\xb4"},
  {"#8309", "\xe2\x81\xb5"},
  {"#8310", "\xe2\x81\xb6"},
  {"#8311", "\xe2\x81\xb7"},
  {"#8312", "\xe2\x81\xb8"},
  {"#8313", "\xe2\x81\xb9"},
  {"epsilon", "\xce\xb5"},
  {"times", "\xc3\x97"},
  {"minus", "\xe2\x88\x92"},
};

//------------------------------------------------------------------------------
// Function declarations
int parseXMLNode(const unsigned int, const unsigned int, const char*, struct parserBaseStore*, const bool isSubCall);
int parseXMLData(const unsigned int, const unsigned int, const char*, struct xmlNode*, struct parserBaseStore*);

/*
  NOTE: First parameters of all three functions: elementType.
        elementType = 0 => xmlNode
        elementType = 1 => wikiTag
        void *element = xmlNode/wikiTag
*/
bool addWikiTag(const short, void*, const short, const short, const bool, const bool, const short, const unsigned char, const unsigned char, const bool, const char*, struct parserBaseStore*);
bool addEntity(const short, void*, const short, const short, const bool, const bool, const unsigned char, unsigned const char, const bool, const char*, struct parserBaseStore*);
bool addWord(const short, void*, const short, const short, const bool, const bool, const unsigned char, const unsigned char, const bool, const char*, struct parserBaseStore*);

// Writeout
bool writeOutDataFiles(const struct parserBaseStore*, struct xmlDataCollection*);
bool writeOutTagData(const struct parserBaseStore*, struct wikiTag*);
bool writeOutTagDataByLine(const struct parserBaseStore*, struct wikiTag*, const unsigned int);

// Clean up functions
void freeXMLCollection(struct xmlDataCollection*);
void freeXMLCollectionTag(wikiTag*);

//------------------------------------------------------------------------------
// Main routine
int main(int argc, char *argv[]) {
  printf("[ INFO ] Starting parsing process on file \"%s\".\n", SOURCEFILE);

  FILE *inputFile = fopen(SOURCEFILE, "r");

  FILE *dictFile = NULL;
  FILE *wtagFile = NULL;
  FILE *xmltagFile = NULL;
  FILE *xmldataFile = NULL;
  FILE *entitiesFile = NULL;

  if (DOWRITEOUT) {
    remove(DICTIONARYFILE);
    remove(WIKITAGSFILE);
    remove(XMLTAGFILE);
    remove(XMLDATAFILE);
    remove(ENTITIESFILE);
    dictFile = fopen(DICTIONARYFILE, "w");
    wtagFile = fopen(WIKITAGSFILE, "w");
    xmltagFile = fopen(XMLTAGFILE, "w");
    xmldataFile = fopen(XMLDATAFILE, "w");
    entitiesFile = fopen(ENTITIESFILE, "w");
  }

  collectionStatistics cData = {0};

  if (inputFile == NULL) {
    printf("Cannot open file '%s' for reading.\n", SOURCEFILE);
    return 1;
  }

  time_t startTime = time(NULL);

  // Parser variables
  unsigned int lineBuffer = LINEBUFFERBASE;
  char *line = malloc(sizeof(char) * LINEBUFFERBASE);

  unsigned int lineLength = 0;
  char tmpChar = '\0';
  unsigned int readerPos = 0;

  xmlDataCollection xmlCollection = {0, 0, NULL, NULL};

  parserBaseStore parserRunTimeData;
  parserRunTimeData.dictFile = dictFile;
  parserRunTimeData.wtagFile = wtagFile;
  parserRunTimeData.xmltagFile = xmltagFile;
  parserRunTimeData.xmldataFile = xmldataFile;
  parserRunTimeData.entitiesFile = entitiesFile;
  parserRunTimeData.xmlCollection = &xmlCollection;
  parserRunTimeData.cData = &cData;
  parserRunTimeData.currentPosition = 0;
  parserRunTimeData.currentLine = 1;
  parserRunTimeData.isMathSection = false;

  //----------------------------------------------------------------------------
  // Parser start
  while (tmpChar != EOF) {
    lineLength = 0;

    if (LINESTOPROCESS != 0 && parserRunTimeData.currentLine > LINESTOPROCESS) break;

    // Read a line from file
    do {
      tmpChar = fgetc(inputFile);
      if (tmpChar == ' ' || tmpChar == '\t') {
        ++cData.byteWhitespace;
        if (lineLength == 0) continue;
      }

      line[lineLength] = tmpChar;
      ++lineLength;

      if (lineLength == lineBuffer) {
        lineBuffer += LINEBUFFERBASE;
        line = (char*) realloc(line, sizeof(char) * lineBuffer);
      }

    } while (tmpChar != '\r' && tmpChar != '\n' && tmpChar != EOF);

    if (line[0] == '\n' || line[0] == '\r') {
      ++cData.byteNewLine;
      ++parserRunTimeData.currentLine;
      parserRunTimeData.currentPosition = 0;
      continue;
    }

    line[lineLength] = '\0';
    ++cData.byteNewLine;
    parserRunTimeData.currentPosition = 0;
    parserRunTimeData.isMathSection = false;

    // Find XML Tags on line
    if (line[0] != '<') {
      readerPos = parseXMLData(0, lineLength, line, &xmlCollection.nodes[xmlCollection.count-1], &parserRunTimeData);
      if (readerPos < lineLength - 1) parseXMLNode(readerPos, lineLength, &line[readerPos], &parserRunTimeData, true);
    } else parseXMLNode(0, lineLength, line, &parserRunTimeData, false);

    ++parserRunTimeData.currentLine;
  }

  --parserRunTimeData.currentLine;

  long int duration = difftime(time(NULL), startTime);
  long int durHours = floor(duration / 3600);
  long int durMinutes = floor((duration % 3600) / 60);
  long int durSeconds = (duration % 3600) % 60;
  printf("\n\n[STATUS] RUN TIME FOR PARSING PROCESS: %ldh %ldm %lds\n", durHours, durMinutes, durSeconds);
  printf("[REPORT] PARSED LINES : %d | FAILED ELEMENTS: %d\n", parserRunTimeData.currentLine, cData.failedElements);
  printf("[REPORT] FILE STATISTICS\nXML TAG    : %16d [~ %.3lf MB]\nKEYS       : %16d [~ %.3lf MB]\nVALUES     : %16d [~ %.3lf MB]\nWORDS      : %16d [~ %.3lf MB]\nENTITIES   : %16d [~ %.3lf MB]\nWIKITAGS   : %16d [~ %.3lf MB]\nWHITESPACE : %16d [~ %.3lf MB]\nNEWLINE    : %16d [~ %.3lf MB]\nFORMATTING : [~ %.3lf MB]\n\nTOTAL COLLECTED DATA : ~%.3lf MB\n", xmlCollection.count, cData.byteXMLsaved / 1000000.0, cData.keyCount, cData.byteKeys / 1000000.0, cData.valueCount, cData.byteValues / 1000000.0, cData.wordCount, cData.byteWords / 1000000.0, cData.entityCount, cData.byteEntites / 1000000.0, cData.wikiTagCount, cData.byteWikiTags / 1000000.0, cData.byteWhitespace, cData.byteWhitespace / 1000000.0, cData.byteNewLine, cData.byteNewLine / 1000000.0, cData.byteFormatting / 1000000.0, (cData.byteKeys + cData.byteValues + cData.byteWords + cData.byteEntites + cData.byteWikiTags + cData.byteWhitespace + cData.byteFormatting + cData.bytePreWhiteSpace + cData.byteNewLine + cData.byteXMLsaved) / 1000000.0);
  printf("TOTAL FILE SIZE: ~%.3lf MB\n\n", ftell(inputFile) / 1000000.0);
  fclose(inputFile);

  if (DOWRITEOUT) {
    startTime = time(NULL);
    writeOutDataFiles(&parserRunTimeData, &xmlCollection);
    fclose(parserRunTimeData.dictFile);
    fclose(parserRunTimeData.wtagFile);
    fclose(parserRunTimeData.xmltagFile);
    fclose(parserRunTimeData.xmldataFile);
    fclose(parserRunTimeData.entitiesFile);

    long int duration = difftime(time(NULL), startTime);
    long int durHours = floor(duration / 3600);
    long int durMinutes = floor((duration % 3600) / 60);
    long int durSeconds = (duration % 3600) % 60;
    printf("\n\n[STATUS] SAVING DATA PROCESS: %ldh %ldm %lds\n", durHours, durMinutes, durSeconds);
  }

  //----------------------------------------------------------------------------
  // Cleanup

  free(line);
  freeXMLCollection(&xmlCollection);
  return 0;
}

//------------------------------------------------------------------------------

int parseXMLNode(const unsigned int xmlTagStart, const unsigned int lineLength, const char *line, struct parserBaseStore* parserRunTimeData, const bool isSubCall) {

  xmlDataCollection* xmlCollection = parserRunTimeData->xmlCollection;
  collectionStatistics* cData = parserRunTimeData->cData;

  xmlNode *xmlTag = NULL;
  if (!isSubCall) {
    xmlCollection->nodes = (xmlNode*) realloc(xmlCollection->nodes, sizeof(xmlNode) * (xmlCollection->count + 1));
    xmlTag = &xmlCollection->nodes[xmlCollection->count];

    xmlTag->isClosed = false;
    xmlTag->isDataNode = false;
    xmlTag->indent = xmlTagStart;
    xmlTag->firstAddedType = -1; // 0 WORD, 1 WIKITAG, 2 ENTITY
    xmlTag->lastAddedType = -1; // 0 WORD, 1 WIKITAG, 2 ENTITY
    xmlTag->keyValuePairs = 0;
    xmlTag->wordCount = 0;
    xmlTag->entityCount = 0;
    xmlTag->wTagCount = 0;
    xmlTag->start = parserRunTimeData->currentLine;
    xmlTag->end = parserRunTimeData->currentLine;
    xmlTag->name = NULL;
    xmlTag->keyValues = NULL;
    xmlTag->words = NULL;
    xmlTag->entities = NULL;
    xmlTag->wikiTags = NULL;
  } else xmlTag = &xmlCollection->nodes[xmlCollection->count - 1];

  // Routine variables
  unsigned int readerPos = 1;

  char readIn = '\0';
  char readData[lineLength];
  int writerPos = 0;

  bool xmlHasName = false;
  if (xmlTag->name) xmlHasName = true;

  bool isKey = false;
  bool isValue = false;

  bool nodeClosed = false;
  keyValuePair *xmlKeyValue = NULL;

  // Identify the xmlTag name and key and value pairs
  while (line[readerPos] != '>') {
    while (line[readerPos] != '>') {
      readIn = line[readerPos];

      if (!isValue) {
        if (readIn == ' ') {
          ++readerPos;
          break;
        } else if (readIn == '=') {
          readerPos += 2;
          isKey = true;
          break;
        } else if (readIn == '/') {
          xmlTag->isClosed = true;
          xmlTag->end = parserRunTimeData->currentLine;

          ++readerPos;
          continue;
        }
      } else if (readIn == '"') {
        ++cData->byteXMLsaved;
        ++readerPos;
        break;
      }

      readData[writerPos] = readIn;
      ++writerPos;
      ++readerPos;
      continue;
    }

    if (writerPos != 0 && (readIn == '"' || readIn == '/' || readIn == '=' || ((readIn != ' ' && !isValue) || (readIn == ' ' && !isValue)))) {
      readData[writerPos] = '\0';
      if (!xmlHasName) {
        xmlTag->name = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlTag->name, readData);
        xmlHasName = true;
      } else if (isKey) {
        xmlTag->keyValues = (keyValuePair*) realloc(xmlTag->keyValues, sizeof(keyValuePair) * (xmlTag->keyValuePairs + 1) );

        xmlKeyValue = &xmlTag->keyValues[xmlTag->keyValuePairs];
        xmlKeyValue->value = NULL;

        xmlKeyValue->key = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->key, readData);

        cData->byteKeys += writerPos;
        ++cData->keyCount;

        ++xmlTag->keyValuePairs;

        isKey = false;
        isValue = true;
      } else if (isValue) {

        xmlKeyValue->value = malloc(sizeof(char) * (writerPos + 1));
        strcpy(xmlKeyValue->value, readData);

        cData->byteValues += writerPos;
        ++cData->valueCount;

        isValue = false;
      }

      writerPos = 0;
    }
  }

  //--------------------------------------------------------------------------
  // NOTE: Does adjust so we know if data follows the xmlTag
  ++readerPos;

  //--------------------------------------------------------------------------

  if (xmlHasName) {

    if (!isSubCall) ++xmlCollection->count;

    nodeClosed = false;
    for (unsigned int i = 0; i < xmlCollection->openNodeCount; ++i) {
      xmlNode *openXMLNode = &xmlCollection->nodes[xmlCollection->openNodes[i]];

      if (strcmp(xmlTag->name, openXMLNode->name) == 0) {
        openXMLNode->isClosed = true;
        openXMLNode->end = parserRunTimeData->currentLine;
        cData->byteXMLsaved += (strlen(openXMLNode->name) << 1) + 3;

        if (i < xmlCollection->openNodeCount) {
          memmove(&xmlCollection->openNodes[i], &xmlCollection->openNodes[i+1], (xmlCollection->openNodeCount-(i+1)) * sizeof(unsigned int));
        }

        --xmlCollection->openNodeCount;
        xmlCollection->openNodes = (unsigned int*) realloc(xmlCollection->openNodes, sizeof(unsigned int) * xmlCollection->openNodeCount);

        #if DEBUG || BEVERBOSE
        printf("[STATUS]  | LINE: %8d | %s%sNODE ADDED: '%s'\n", parserRunTimeData->currentLine, xmlTag->isClosed ? "CLOSED " : "", readerPos < lineLength ? "DATA " : "", xmlTag->name);
        for (unsigned int i = 0; i < xmlTag->keyValuePairs; ++i) {
          printf("[STATUS]  | LINE: %8d | KEYVALUE: '%s' > '%s'\n", parserRunTimeData->currentLine, xmlTag->keyValues[i].key, xmlTag->keyValues[i].value);
        }
        fputc('\n', stdout);
        #endif

        nodeClosed = true;

        if (!isSubCall && xmlTag->end == openXMLNode->end) {
          free(xmlTag->name);
          xmlTag = NULL;
          --xmlCollection->count;
          xmlCollection->nodes = (xmlNode*) realloc(xmlCollection->nodes, sizeof(xmlNode) * xmlCollection->count);
        }

        break;
      }
    }

    if (!isSubCall && !nodeClosed && !xmlTag->isClosed) {
      xmlCollection->openNodes = (unsigned int*) realloc(xmlCollection->openNodes, sizeof(unsigned int) * (xmlCollection->openNodeCount + 1));
      xmlCollection->openNodes[xmlCollection->openNodeCount] = xmlCollection->count - 1;
      ++xmlCollection->openNodeCount;
    }
  }

  // Start process the data of the XML tag
  if (!nodeClosed && readerPos < lineLength - 1) {
    //printf("\n## FUNC A ######################################### ---- CL %d\n\n", parserRunTimeData->currentLine);
    xmlTag->isDataNode = true;
    readerPos += parseXMLData(0, lineLength-readerPos, &line[readerPos], xmlTag, parserRunTimeData);
    if (readerPos < lineLength) parseXMLNode(readerPos, lineLength-readerPos, &line[readerPos], parserRunTimeData, true);
  }

  return readerPos;
}

//------------------------------------------------------------------------------

int parseXMLData(unsigned int readerPos, const unsigned int lineLength, const char* line, xmlNode *xmlTag, struct parserBaseStore* parserRunTimeData) {

  collectionStatistics* cData = parserRunTimeData->cData;

  #if DEBUG || BEVERBOSE
  printf("====================================================================\n");
  printf("[DEBUG] START PARSING DATA => TAG: '%s' | READERPOS: %d | LINE: %d\n", xmlTag->name, readerPos, parserRunTimeData->currentLine);
  #elif BEVERBOSE
  printf("====================================================================\n");
  #endif

  char readIn = '\0';
  char readData[lineLength];

  bool isAdded = false;
  bool createWord = false;
  unsigned char spacesCount = 0;
  unsigned char preSpacesCount = 0;

  char tmpChar = '\0';
  unsigned int writerPos = 0;

  char formatData[lineLength];
  unsigned int formatReaderPos = 0;
  unsigned int formatDataPos = 0;
  short dataFormatType = -1;
  short ownFormatType = -1;
  bool isFormatStart = false;
  bool isFormatEnd = false;

  // WIKITAG variables
  short wikiTagType = -1;
  bool isWikiTag = false;
  unsigned short wikiTagDepth = 0;
  bool doCloseWikiTag = false;

  // FORMATs variables
  bool doCloseFormat = false;
  bool doCloseOwnFormat = false;

  bool isEntity = false;
  bool hasPipe = false;
  char entityBuffer[11] = "\0";
  unsigned int entityReadPos = 0;
  unsigned int entityWritePos = 0;


  while (readerPos < lineLength) {
    readIn = line[readerPos];

    // Escape before xml tag closings and such
    switch (readIn) {
      case '\n':
      case '\r':
        preSpacesCount = 0;
        spacesCount = 0;
        if (writerPos != 0) createWord = true;
        break;
      case '\t':
        if (writerPos == 0) {
          ++preSpacesCount;
        } else {
          ++spacesCount;
          createWord = true;
          break;
        }

        ++readerPos;
        continue;
      case ' ':
        if (isWikiTag) {
          if (writerPos != 0) ++spacesCount;
          else ++preSpacesCount;

          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        formatDataPos = readerPos;
        while (line[formatDataPos] == ' ') {
          ++formatDataPos;
          if (writerPos != 0) ++spacesCount;
          else ++preSpacesCount;
        }

        readerPos = formatDataPos;
        if (writerPos == 0) {
          continue;
        } else {
          if (line[formatDataPos] == '\'' && line[formatDataPos + 1] == '\'' && (dataFormatType == -1 || ownFormatType == -1)) continue;
          readerPos = formatDataPos - 1;
          createWord = true;
        }

        break;
      case '\\':
        // NOTE: Are the" \begin" and "\end" tags are only used for math inside text elements?
        if (readerPos + 6 <= lineLength) {
          tmpChar = tolower(line[readerPos + 1]);
          if (tmpChar == 'm' || tmpChar == 'b' || tmpChar == 'e') {
            formatData[0] = tmpChar;
            formatDataPos = 1;
            while (formatDataPos < 6) {
              if (readerPos + formatDataPos + 1 > lineLength) break;
              formatData[formatDataPos] = tolower(line[readerPos + formatDataPos + 1]);
              ++formatDataPos;
            }

            formatData[formatDataPos - 1] = '\0';

            // TODO: Think about, if "\begin" and "\end" are found - should this be filtered out and be stored as information to the "words" file? Or should this only be used to avoid creating wikitags?
            if (!parserRunTimeData->isMathSection && (strncmp(formatData, "math", 4) == 0 || strcmp(formatData, "begin") == 0)) {
              parserRunTimeData->isMathSection = true;
              #if DEBUG || BEVERBOSE
              printf("[INFO ] LINE: %d | READER: %d => MATH SECTION START\n", parserRunTimeData->currentLine, readerPos);
              #endif
            } else if (tmpChar == 'e' && parserRunTimeData->isMathSection && strncmp(formatData, "end", 3) == 0) {
              parserRunTimeData->isMathSection = false;
              #if DEBUG || BEVERBOSE
              printf("[INFO ] LINE: %d | READER: %d => MATH SECTION END\n", parserRunTimeData->currentLine, readerPos);
              #endif
            }
          }
        }

        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case '/':
        // NOTE: Are the" \begin" and "\end" tags are only used for math inside text elements?
        if (parserRunTimeData->isMathSection && readerPos + 3 <= lineLength) {
          tmpChar = tolower(line[readerPos + 1]);
          if (tmpChar == 'm' || tmpChar == 'e') {
            formatData[0] = tmpChar;
            formatDataPos = 1;
            while (formatDataPos < 5) {
              if (readerPos + formatDataPos + 1 > lineLength) break;
              formatData[formatDataPos] = tolower(line[readerPos + formatDataPos + 1]);
              ++formatDataPos;
            }

            formatData[formatDataPos - 1] = '\0';

            // TODO: Think about, if "\begin" and "\end" are found - should this be filtered out and be stored as information to the "words" file? Or should this only be used to avoid creating wikitags?
            if (strcmp(formatData, "math") == 0 || strncmp(formatData, "end", 3) == 0) {
              parserRunTimeData->isMathSection = false;
              #if DEBUG || BEVERBOSE
              printf("[INFO ] LINE: %d | READER: %d => MATH SECTION END\n", parserRunTimeData->currentLine, readerPos);
              #endif
            }
          }
        }

        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case '<':
        createWord = true;
        break;
      case '[':
      case '{':
        if (!parserRunTimeData->isMathSection && readerPos + 1 <= lineLength) {
          tmpChar = line[readerPos + 1];

          if (tmpChar == readIn) {
            if (writerPos != 0) {
              createWord = true;
              --readerPos;
              break;
            }

            formatReaderPos = readerPos;
            formatDataPos = 0;

            tmpChar = line[formatReaderPos];
            do {
              tmpChar = line[formatReaderPos];
              formatData[formatDataPos] = tolower(tmpChar);

              ++formatDataPos;
              ++formatReaderPos;
              if (formatDataPos == 20) break;
            } while (tmpChar != ' ' && tmpChar != ':');

            formatData[formatDataPos] = '\0';

            short tagLength = 0;
            for (short i = 0; i < TAGTYPES; ++i) {
              tagLength = strlen(tagTypes[i]);
              if (strncmp(formatData, tagTypes[i], tagLength) == 0) {
                if (wikiTagType == MATHTAG) parserRunTimeData->isMathSection = true;

                /*if (wikiTagDepth != 0) {
                  cData->byteWikiTags += tagLength;
                  ++wikiTagDepth;
                  readerPos += tagLength - 1;
                  break;
                }*/
                ++wikiTagDepth;
                wikiTagType = i;

                isWikiTag = true;
                cData->byteWikiTags += tagLength;
                readerPos += tagLength - 1;
                break;
              }
            }
            /*
            if (wikiTagDepth > 1) {
              readData[writerPos] = readIn;
              ++writerPos;
              continue;
            }
            */

            if (isWikiTag) break;
          }
        }

        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case ']':
      case '}':
        if (!parserRunTimeData->isMathSection && readerPos + 1 <= lineLength) {
          tmpChar = line[readerPos + 1];

          if (tmpChar == readIn) {
            if (writerPos != 0) {
              if (!isWikiTag) {
                createWord = true;
                --readerPos;
                break;
              }
            }

            if (wikiTagDepth != 0) {
              hasPipe = false;
              --wikiTagDepth;
              ++cData->byteWikiTags;

              if (wikiTagDepth != 0) {
                readerPos += 2;
                continue;
              }

              doCloseWikiTag = true;

              formatReaderPos = readerPos + 2;
              formatDataPos = 0;

              while (line[formatReaderPos] == '\'') {
                formatData[formatDataPos] = line[formatReaderPos];
                ++formatDataPos;
                ++formatReaderPos;
              }

              if (formatDataPos > 1) {
                readerPos += formatDataPos;
                continue;
              }

              ++readerPos;
              break;
            }
          }
        }

        if (entityBuffer[0] == '&') {
          isEntity = true;
          --readerPos;
          break;
        }

        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case '=':
      case '\'':
        if (isWikiTag && !doCloseWikiTag) {
          /*
          if (writerPos != 0) {
            createWord = true;
            break;
          }
          */
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        formatReaderPos = readerPos;
        formatDataPos = 0;

        while (line[formatReaderPos] == readIn || (readIn == '=' && line[formatReaderPos] == ' ')) {
          if (line[formatReaderPos] != ' ') {
            formatData[formatDataPos++] = line[formatReaderPos];
          }
          ++formatReaderPos;
        }

        formatData[formatDataPos] = '\0';

        if (formatDataPos == 1) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        } else if (readIn == '\'' && writerPos != 0 && dataFormatType == -1) {
          --readerPos;
          createWord = true;
          break;
        }

        for (short i = 0; i < FORMATS; ++i) {
          if (strcmp(formats[i], formatData) == 0) {
            //printf("BEFORE: %d | formatType %s | dataformat: %d | ownFormat: %d | isStart: %d | isEnd: %d\n", parserRunTimeData->currentLine, formatNames[i], dataFormatType, ownFormatType, isFormatStart, isFormatEnd);

            if (dataFormatType == -1) {
              dataFormatType = i;
              ownFormatType = i;
              isFormatStart = true;
            } else if (dataFormatType == i) {
              isFormatEnd = true;
              doCloseFormat = true;
            } else if (ownFormatType == -1) {
              isFormatStart = true;
            } else if (ownFormatType == i) {
              isFormatEnd = true;
              doCloseOwnFormat = true;
            } else {
              #if DEBUG
              printf("[ ERROR ] LINE: %d - Formatting does not match. Using %s (%d) instead of %s (%d) and adding it to regular data.\n\n", parserRunTimeData->currentLine, formatNames[ (int) dataFormatType], (int) dataFormatType, formatNames[i], i);
              #endif

              for (int i = 0; i < formatDataPos; ++i) {
                readData[writerPos] = formatData[i];
                ++writerPos;
              }

              readerPos += formatDataPos - 1;
              break;
            }

            //printf("AFTER: %d | formatType %s | dataformat: %d | ownFormat: %d | isStart: %d | isEnd: %d\n", parserRunTimeData->currentLine, formatNames[i], dataFormatType, ownFormatType, isFormatStart, isFormatEnd);
            readerPos += formatDataPos - 1;
            cData->byteFormatting += formatDataPos;
            break;
          }
        }

        if (readIn == '\'' && (isFormatStart || isFormatEnd)) break;

        if (xmlTag->firstAddedType != -1 && xmlTag->lastAddedType != -1) {
          if (writerPos != 0) isFormatEnd = true;
          else {
            short tempDataFormatType = -1;
            short tempOwnFormatType = -1;
            bool isFound = false;

            // 0 WORD, 1 WIKITAG, 2 ENTITY
            for (unsigned int i = 0; i < xmlTag->wTagCount; ++i) {
              if (xmlTag->wikiTags[i].position == 1 && xmlTag->wikiTags[i].lineNum == parserRunTimeData->currentLine) {
                tempDataFormatType = xmlTag->wikiTags[i].dataFormatType;
                tempOwnFormatType = xmlTag->wikiTags[i].ownFormatType;
                isFound = true;
                break;
              }
            }

            if (!isFound) {
              for (unsigned int i = 0; i < xmlTag->wordCount; ++i) {
                if (xmlTag->words[i].position == 1 && xmlTag->words[i].lineNum == parserRunTimeData->currentLine) {
                  tempDataFormatType = xmlTag->words[i].dataFormatType;
                  tempOwnFormatType = xmlTag->words[i].ownFormatType;
                  isFound = true;
                  break;
                }
              }
            }

            if (!isFound) {
              for (unsigned int i = 0; i < xmlTag->entityCount; ++i) {
                if (xmlTag->entities[i].position == 1 && xmlTag->entities[i].lineNum == parserRunTimeData->currentLine) {
                  tempDataFormatType = xmlTag->entities[i].dataFormatType;
                  tempOwnFormatType = xmlTag->entities[i].ownFormatType;
                  isFound = true;
                  break;
                }
              }
            }

            if (isFound && (tempOwnFormatType == ownFormatType || tempDataFormatType == dataFormatType)) {
              if (tempOwnFormatType == tempDataFormatType) tempOwnFormatType = -1;
              if (xmlTag->lastAddedType == 0) {
                struct word* last = &xmlTag->words[xmlTag->wordCount - 1];
                last->dataFormatType = tempDataFormatType;
                last->ownFormatType = tempOwnFormatType;
                last->formatEnd = true;
              } else if (xmlTag->lastAddedType == 1) {
                struct wikiTag* last = &xmlTag->wikiTags[xmlTag->wTagCount - 1];
                last->dataFormatType = tempDataFormatType;
                last->ownFormatType = tempOwnFormatType;
                last->formatEnd = true;
              } else if (xmlTag->lastAddedType == 2) {
                struct entity* last = &xmlTag->entities[xmlTag->entityCount - 1];
                last->dataFormatType = tempDataFormatType;
                last->ownFormatType = tempOwnFormatType;
                last->formatEnd = true;
              }

              doCloseFormat = false;
              ++readerPos;
              continue;
            }
          }
        }
        break;
      case '&':
        if (isWikiTag) {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        entityBuffer[0] = '\0';
        entityReadPos = readerPos;
        entityWritePos = 0;

        isEntity = false;

        while(entityWritePos < 10 && line[entityReadPos] != ' ') {
          entityBuffer[entityWritePos++] = line[entityReadPos++];

          if (line[entityReadPos] == ';') {
            entityBuffer[entityWritePos++] = ';';
            ++entityReadPos;
            isEntity = true;
            break;
          }
        }

        if (isEntity) {
          if (writerPos != 0) {
            createWord = true;
            isEntity = false;
            --readerPos;
            break;
          }

          entityBuffer[entityWritePos] = '\0';

          if (!parserRunTimeData->isMathSection && line[entityReadPos] == 'm' && entityReadPos + 4 <= lineLength && strcmp(entityBuffer, "&lt;") == 0) {
            formatDataPos = 0;
            while (formatDataPos < 5) {
              formatData[formatDataPos] = tolower(line[entityReadPos + formatDataPos]);
              ++formatDataPos;
            }

            formatData[formatDataPos - 1] = '\0';

            // TODO: Think about, if "\begin" and "\end" are found - should this be filtered out and be stored as information to the "words" file? Or should this only be used to avoid creating wikitags?
            if (strcmp(formatData, "math") == 0) {
              parserRunTimeData->isMathSection = true;
              #if DEBUG || BEVERBOSE
              printf("[INFO ] LINE: %d | READER: %d => MATH SECTION START\n", parserRunTimeData->currentLine, readerPos);
              #endif
            }
          }

          if (entityReadPos + 1 <= lineLength) {
            if (line[entityReadPos] != '\'' && line[entityReadPos + 1] != '\'') {
              readerPos = entityReadPos - 1;
              break;
            } else if (lineLength && line[entityReadPos] == '\'' && line[entityReadPos + 1] == '\'') {
              if (dataFormatType == -1) {
                readerPos = entityReadPos - 1;
                break;
              }

              readerPos = entityReadPos;
              continue;
            } else if (entityReadPos <= lineLength && line[entityReadPos] != '\'') {
              readerPos = entityReadPos;

              if (line[entityReadPos] == ' ') {
                formatDataPos = readerPos;
                while (line[formatDataPos] == ' ') {
                  ++formatDataPos;
                  ++spacesCount;
                }

                readerPos = formatDataPos - 1;
                break;
              }

              readerPos = entityReadPos - 1;
              break;
            }
          } else if (entityReadPos <= lineLength && line[entityReadPos] != '\'') {
            readerPos = entityReadPos;

            if (line[entityReadPos] == ' ') {
              formatDataPos = readerPos;
              while (line[formatDataPos] == ' ') {
                ++formatDataPos;
                ++spacesCount;
              }

              readerPos = formatDataPos - 1;
              break;
            }

            readerPos = entityReadPos - 1;
            break;
          }
        } else {
          readData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        break;
      default:
        readData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
    }

    if (createWord || isEntity || doCloseWikiTag || doCloseFormat || isWikiTag) {
      //printf("dcf %d | dcof %d | cw %d | iwt %d | dcwt %d | isent %d | wp %d\n", doCloseFormat, doCloseOwnFormat, createWord, isWikiTag, doCloseWikiTag, isEntity, writerPos);
      createWord = false;
      readData[writerPos] = '\0';
      isAdded = false;
      if (writerPos != 0 || isEntity) {
        #if DEBUG
        printf("[DEBUG] LINE: %d | SPACING: %d/%d | WTD: %d | POS: %8d | READER: %8d | DATA: %5d", parserRunTimeData->currentLine, preSpacesCount, spacesCount, doCloseWikiTag ? wikiTagDepth + 1 : wikiTagDepth, parserRunTimeData->currentPosition, readerPos, isEntity ? entityWritePos : writerPos);
        #endif

        if (isWikiTag) {
          #if DEBUG
          printf(" | WIKITAG | DATA: \"%s\" '%s'", readData, wikiTagNames[wikiTagType]);
          #elif BEVERBOSE
          printf("WIKITAG   | LINE: %8d | %8d | DATA: \"%s\" '%s'", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, readData, wikiTagNames[wikiTagType]);
          #endif

          #if DEBUG
          if (ownFormatType != -1) printf(" << %s", formatNames[ownFormatType]);
          if (dataFormatType != -1) printf(" << *%s", formatNames[dataFormatType]);
          if (isFormatStart) printf(" < isStart");
          if (isFormatEnd) printf(" < isEnd");
          #endif

          if (addWikiTag(0, xmlTag, dataFormatType, ownFormatType, isFormatStart, isFormatEnd, wikiTagType, preSpacesCount, spacesCount, hasPipe, readData, parserRunTimeData)) {
            isAdded = true;
            ++cData->wikiTagCount;
          }

          hasPipe = false;
          preSpacesCount = 0;
          spacesCount = 0;
        } else if (isEntity) {
          #if DEBUG
          printf(" |  ENTITY | DATA: \"%s\" ", entityBuffer);
          #elif BEVERBOSE
          printf("ENTITY    | LINE: %8d | %8d | DATA: \"%s\"", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, entityBuffer);
          #endif

          #if DEBUG
          if (ownFormatType != -1) printf(" << %s", formatNames[ownFormatType]);
          if (dataFormatType != -1) printf(" << *%s", formatNames[dataFormatType]);
          if (isFormatStart) printf(" < isStart");
          if (isFormatEnd) printf(" < isEnd");
          #endif

          // TODO: Add entity handling routine
          if (addEntity(0, xmlTag, dataFormatType, ownFormatType, isFormatStart, isFormatEnd, preSpacesCount, spacesCount, hasPipe, entityBuffer, parserRunTimeData)) {
            isAdded = true;
            ++cData->entityCount;
            cData->byteEntites += entityWritePos;
          }

          entityBuffer[0] = '\0';
          isEntity = false;
          hasPipe = false;
          preSpacesCount = 0;
          spacesCount = 0;
        } else {
          // Is regular word with read in data
          #if DEBUG
          printf(" |   WORD  | DATA: \"%s\" ", readData);
          #elif BEVERBOSE
          printf("WORD      | LINE: %8d | %8d | DATA: \"%s\"", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, readData);
          #endif

          #if DEBUG
          if (ownFormatType != -1) printf(" << %s", formatNames[ownFormatType]);
          if (dataFormatType != -1) printf(" << *%s", formatNames[dataFormatType]);
          if (isFormatStart) printf(" < isStart");
          if (isFormatEnd) printf(" < isEnd");
          #endif

          // TODO: Think of "pre" and "/pre" as switch for nonformatting parts
          if (addWord(0, xmlTag,  dataFormatType, ownFormatType, isFormatStart, isFormatEnd, preSpacesCount, spacesCount, hasPipe, readData, parserRunTimeData)) {
            isAdded = true;
            ++cData->wordCount;
            cData->byteWords += writerPos;
          }

          hasPipe = false;
          preSpacesCount = 0;
          spacesCount = 0;
        }

        //------------------------------------------------------------------------

        if (!isAdded) ++cData->failedElements;
        // else ++parserRunTimeData->currentPosition;

        #if DEBUG || BEVERBOSE
        if (!isAdded) printf("     [FAILED]\n");
        else printf("\n");
        #endif


        isFormatStart = false;
        writerPos = 0;
        spacesCount = 0;
        preSpacesCount = 0;
      }
      //------------------------------------------------------------------------


      if (doCloseWikiTag) {
        wikiTagType = -1;
        doCloseWikiTag = false;
        isWikiTag = false;
      }

      if (doCloseFormat) {
        if (ownFormatType == dataFormatType) ownFormatType = -1;

        if (xmlTag->lastAddedType == 0) {
          struct word* last = &xmlTag->words[xmlTag->wordCount - 1];
          last->dataFormatType = dataFormatType;
          last->ownFormatType = ownFormatType;
          last->formatEnd = true;
        } else if (xmlTag->lastAddedType == 1) {
          struct wikiTag* last = &xmlTag->wikiTags[xmlTag->wTagCount - 1];
          last->dataFormatType = dataFormatType;
          last->ownFormatType = ownFormatType;
          last->formatEnd = true;
        } else if (xmlTag->lastAddedType == 2) {
          struct entity* last = &xmlTag->entities[xmlTag->entityCount - 1];
          last->dataFormatType = dataFormatType;
          last->ownFormatType = ownFormatType;
          last->formatEnd = true;
        }

        dataFormatType = -1;
        doCloseFormat = false;
        isFormatStart = false;
        isFormatEnd = false;
      }

      if (doCloseOwnFormat) {
        ownFormatType = -1;
        doCloseOwnFormat = false;
      }
    }

  if (readIn == '<') return readerPos;

  ++readerPos;
  }

  #if DEBUG || BEVERBOSE
  printf("-----------------------------------------------------------------------------------------------------------------\n");
  #endif

  return readerPos;
}

//------------------------------------------------------------------------------


bool addWikiTag(const short elementType, void *element, const short dataFormatType, const short ownFormatType, const bool isFormatStart, const bool isFormatEnd, const short wikiTagType, const unsigned char preSpacesCount, const unsigned char spacesCount, const bool wikiTaghasPipe, const char *readData, struct parserBaseStore *parserRunTimeData) {

  collectionStatistics* cData = parserRunTimeData->cData;

  wikiTag *tag = NULL;
  xmlNode *xmlTag = NULL;
  wikiTag *parentTag = NULL;
  unsigned int dataLength = strlen(readData);

  if (elementType == 0) {
    xmlTag = element;
    xmlTag->wikiTags = (wikiTag*) realloc(xmlTag->wikiTags, sizeof(wikiTag) * (xmlTag->wTagCount + 1));
    tag = &xmlTag->wikiTags[xmlTag->wTagCount];
    ++xmlTag->wTagCount;
  } else {
    parentTag = element;
    parentTag->pipedTags = (wikiTag*) realloc(parentTag->pipedTags, sizeof(wikiTag) * (parentTag->wTagCount + 1));
    tag = &parentTag->pipedTags[parentTag->wTagCount];
    ++parentTag->wTagCount;
  }

  tag->lineNum = parserRunTimeData->currentLine;
  tag->dataFormatType = dataFormatType;
  tag->tagType = wikiTagType;
  tag->ownFormatType = ownFormatType;
  tag->formatStart = isFormatStart;
  tag->formatEnd = isFormatEnd;
  tag->preSpacesCount = preSpacesCount;
  tag->spacesCount = spacesCount;
  tag->position = ++parserRunTimeData->currentPosition;
  tag->wordCount = 0;
  tag->wTagCount = 0;
  tag->entityCount = 0;
  tag->tagLength = dataLength;
  tag->target = NULL;
  tag->wikiTagFileIndex = cData->wikiTagCount;
  tag->pipedWords = NULL;
  tag->pipedTags = NULL;
  tag->pipedEntities = NULL;
  tag->hasPipe = wikiTaghasPipe;

  if (elementType == 0) {
    if (xmlTag->firstAddedType == -1) xmlTag->firstAddedType = 1;
    xmlTag->lastAddedType = 1;
  }


  #if DEBUG
  printf("\n[DEBUG] PARSING WIKITAG @ LINE: %d  => \"%s\" (%s) \n", parserRunTimeData->currentLine, readData, wikiTagNames[wikiTagType]);
  #endif

  unsigned int readerPos = 0;
  unsigned int writerPos = 0;

  char readIn = '\0';
  char parserData[dataLength];

  unsigned int targetWritePos = 0;
  char targetData[dataLength];
  targetData[0] = '\0';
  bool hasTargetData = false;

  char tmpChar = '\0';

  char formatData[32];
  formatData[0] = '\0';
  unsigned int formatDataPos = 0;
  unsigned int formatReaderPos = 0;
  short dataFormatTypeInternal = -1;
  short ownFormatTypeInternal = -1;
  bool isFormatStartInternal = false;
  bool isFormatEndInternal = false;

  unsigned char spacesCountInternal = 0;
  unsigned char preSpacesCountInternal = 0;

  short tagType = -1;
  bool isWikiTag = false;
  unsigned short wikiTagDepth = 0;
  bool doCloseWikiTag = false;

  bool doCloseFormat = false;
  bool doCloseOwnFormat = false;
  bool createWord = false;
  bool isAdded = false;

  bool isEntity = false;
  bool hasPipe = false;

  char entityBuffer[11] = "\0";
  unsigned short entityReadPos = 0;
  unsigned short entityWritePos = 0;

  while (readerPos <= dataLength) {
    hasPipe = false;
    readIn = readData[readerPos];

    switch (readIn) {
      case '\n':
      case '\r':
        preSpacesCountInternal = 0;
        spacesCountInternal = 0;
        if (writerPos != 0) createWord = true;
        break;
      case '\t':
        if (writerPos == 0) {
          preSpacesCountInternal += 1;
        } else {
          spacesCountInternal += 1;
          createWord = true;
          break;
        }

        ++readerPos;
        continue;
      case ' ':
        if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readData[readerPos];
        else if (wikiTagDepth != 0) {
          while (readData[readerPos] == ' ') {
            if (writerPos == 0) ++preSpacesCountInternal;
            else ++spacesCountInternal;
            ++readerPos;
          }

          if (readData[readerPos] == '|') hasPipe = true;

          if (writerPos != 0)  {
            createWord = true;
            --readerPos;
            break;
          }

          parserData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        } else if (writerPos != 0) {
          while (readData[readerPos] == ' ') {
            ++spacesCountInternal;
            ++readerPos;
          }

          if (readData[readerPos] == '|') hasPipe = true;

          --readerPos;
          createWord = true;
          break;
        }

        ++readerPos;
        continue;
      case '\0':
        if (hasTargetData && writerPos != 0) {
          createWord = true;
        }
        break;
      case '\'':
        formatDataPos = 0;
        formatReaderPos = readerPos;

        while (readData[formatReaderPos] == readIn) {
          formatData[formatDataPos] = readIn;
          ++formatDataPos;
          ++formatReaderPos;
        }

        if (formatDataPos == 1) {
          if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readIn;
          parserData[writerPos] = readData[readerPos];
          ++writerPos;
          break;
        }

        formatData[formatDataPos] = '\0';

        for (short i = 0; i < FORMATS; ++i) {
          if (strcmp(formats[i], formatData) == 0) {
            if (dataFormatTypeInternal == -1) {
              dataFormatTypeInternal = i;
              isFormatStartInternal = true;
            } else if (dataFormatTypeInternal == i) {
              isFormatEndInternal = true;
              doCloseFormat = true;
            } else if (ownFormatTypeInternal == -1) {
              ownFormatTypeInternal = i;
              isFormatStartInternal = true;
            } else if (ownFormatTypeInternal == i) {
              doCloseOwnFormat = true;
            } else {
              #if DEBUG
              printf("[ ERROR ] @ LINE: %d - Formatting does not match. Using %s (%d) instead of %s (%d) and adding it to regular data.\n\n", parserRunTimeData->currentLine, formatNames[ (int) dataFormatType], (int) dataFormatType, formatNames[i], i);
              #endif

              for (int j = 0; j < formatDataPos; ++j) {
                parserData[writerPos] = formatData[j];
                ++writerPos;
              }

              readerPos += formatDataPos - 1;

              continue;
            }

            readerPos += formatDataPos - 1;
            cData->byteFormatting += formatDataPos;
            break;
          }
        }

        if (writerPos != 0) createWord = true;

        formatDataPos = 0;
        formatData[0] = '\0';
        break;
      case '|':
        if (tag->target == NULL && !hasTargetData) {
          targetData[targetWritePos] = '\0';
          tag->target = malloc(sizeof(char) * (targetWritePos + 1));
          strcpy(tag->target, targetData);
          hasTargetData = true;
          tag->hasPipe = true;

          cData->byteWikiTags += targetWritePos;
          #if DEBUG
          printf("[DEBUG] LINE: %d - TARGET            => \"%s\"\n", parserRunTimeData->currentLine, targetData);
          #endif

          writerPos = 0;
          parserData[0] = '\0';
          break;
        }

        hasPipe = true;
        if (wikiTagDepth == 0 && writerPos != 0) {
          if (ownFormatType != -1) doCloseOwnFormat = true;
          if (dataFormatType != -1) doCloseFormat = true;
          createWord = true;
        } else if (wikiTagDepth != 0) {
          parserData[writerPos] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }
        break;
      case '[':
      case '{':
        if (readerPos + 1 <= dataLength) {
          tmpChar = readData[readerPos + 1];

          if (tmpChar == readIn) {

            if (tmpChar == '{' && parserRunTimeData->isMathSection) {
              ++wikiTagDepth;

              parserData[writerPos] = readIn;
              ++writerPos;
              ++readerPos;
              continue;
            }

            if (writerPos != 0) {
              createWord = true;
              --readerPos;
              break;
            }


            formatReaderPos = readerPos;
            formatDataPos = 0;
            formatData[0] = '\0';

            while (tmpChar != '\0' && tmpChar != '\n' && tmpChar != ' ' && tmpChar != ':') {
              tmpChar = readData[formatReaderPos];
              formatData[formatDataPos] = tolower(tmpChar);

              ++formatDataPos;
              ++formatReaderPos;
              if (formatDataPos == 22) break;
            }

            formatData[formatDataPos] = '\0';

            short tagLength = 0;
            for (short i = 0; i < TAGTYPES; ++i) {
              tagLength = strlen(tagTypes[i]);
              if (strncmp(formatData, tagTypes[i], tagLength) == 0) {
                tagType = i;
                isWikiTag = true;
                cData->byteWikiTags += tagLength;
                readerPos += tagLength - 1;
                ++wikiTagDepth;
                break;
              }
            }

            if (isWikiTag) break;
          }
        }

        parserData[writerPos] = readIn;
        if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readData[readerPos];
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case ']':
      case '}':
        if (readerPos + 1 <= dataLength) {
          tmpChar = readData[readerPos + 1];

          if (tmpChar == readIn) {
            hasPipe = false;

            preSpacesCountInternal = 0;
            spacesCountInternal = 0;

            if (wikiTagDepth != 0) {
              --wikiTagDepth;

              if (tmpChar == '}' && parserRunTimeData->isMathSection) {
                parserData[writerPos] = readIn;
                ++writerPos;
                ++readerPos;
                continue;
              }

              ++readerPos;
              doCloseWikiTag = true;

              formatReaderPos = readerPos + 1;
              formatDataPos = 0;

              while (readData[formatReaderPos] == '\'') {
                formatData[formatDataPos] = readData[formatReaderPos];
                ++formatDataPos;
                ++formatReaderPos;
              }

              if (formatDataPos > 1) {
                readerPos = formatReaderPos;
                continue;
                break;
              }

              if (dataFormatType != -1) doCloseFormat = true;
              if (ownFormatType != -1) doCloseOwnFormat = true;
              break;
            } else if (writerPos != 0) {
              spacesCountInternal = 0;

              createWord = true;
              --readerPos;
              break;
            }

            ++readerPos;
            continue;
          }
        }

        if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readIn;
        parserData[writerPos] = readIn;
        ++writerPos;
        ++readerPos;
        continue;
        break;
      case '&':
        entityReadPos = readerPos;
        entityWritePos = 0;

        isEntity = false;

        while(entityWritePos < 10 && readData[entityReadPos] != ' ') {
          entityBuffer[entityWritePos] = readData[entityReadPos];

          ++entityReadPos;
          ++entityWritePos;
          if (readData[entityReadPos] == ';') {
            entityBuffer[entityWritePos] = ';';
            ++entityReadPos;
            ++entityWritePos;
            isEntity = true;
            break;
          }
        }

        entityBuffer[entityWritePos] = '\0';

        if (isEntity && hasTargetData) {
          if (writerPos != 0) {
            createWord = true;
            isEntity = false;
            --readerPos;
            break;
          }

          if (dataLength >= entityReadPos + 1 && readData[entityReadPos] != '\'' && readData[entityReadPos+1] != '\'') {
            writerPos = entityWritePos - 1;
            readerPos += entityWritePos - 1;
          } else {
            ++readerPos;
              continue;
          }
        } else {
          parserData[writerPos] = readIn;
          if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readIn;
          ++writerPos;
          ++readerPos;
          continue;
        }

        break;
      default:
        parserData[writerPos] = readData[readerPos];
        if (tag->target == NULL && !hasTargetData) targetData[targetWritePos++] = readData[readerPos];
        ++writerPos;
        ++readerPos;
        continue;
        break;
    }

    if (doCloseFormat || doCloseOwnFormat || createWord || isWikiTag || doCloseWikiTag || isEntity) {
      parserData[writerPos] = '\0';
      //printf("dcf %d | dcof %d | cw %d | iwt %d | dcwt %d | isent %d | wp %d\n", doCloseFormat, doCloseOwnFormat, createWord, isWikiTag, doCloseWikiTag, isEntity, writerPos);

      if (writerPos != 0)  {
        if (isEntity) {
          #if DEBUG
          printf("[DEBUG] [WIKITAG] LINE: %d | POS: %5d | READER: %6d | FOUND [ ENTITY  ] => \"%s\"", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, readerPosInherited + readerPos, entityBuffer);

          if (ownFormatTypeInternal != -1) printf(" => %s", formatNames[ownFormatTypeInternal]);
          if (dataFormatTypeInternal != -1) printf(" => %s", formatNames[dataFormatTypeInternal]);
          if (isFormatStartInternal) printf(" < isStart");
          if (isFormatEndInternal) printf(" < isEnd");
          #endif

          if (addEntity(1, tag, dataFormatTypeInternal, ownFormatTypeInternal, isFormatStartInternal, isFormatEndInternal, preSpacesCountInternal, spacesCountInternal, hasPipe, entityBuffer, parserRunTimeData)) {
            isAdded = true;
            ++cData->entityCount;
            cData->byteEntites += writerPos;
          }

          hasPipe = false;
          isEntity = false;
          preSpacesCountInternal = 0;
          spacesCountInternal = 0;
        } else if (isWikiTag) {
          #if DEBUG
          printf("[DEBUG] [WIKITAG] LINE: %d | POS: %5d | READER: %6d | FOUND [ WIKITAG ] => \"%s\" (%s)", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, readerPosInherited + readerPos, parserData, wikiTagNames[tagType]);

          if (ownFormatTypeInternal != -1) printf(" => %s", formatNames[ownFormatTypeInternal]);
          if (dataFormatTypeInternal != -1) printf(" => %s", formatNames[dataFormatTypeInternal]);
          if (isFormatStartInternal) printf(" < isStart");
          if (isFormatEndInternal) printf(" < isEnd");
          #endif

          if (addWikiTag(1, tag, dataFormatTypeInternal, ownFormatTypeInternal, isFormatStartInternal, isFormatEndInternal, tagType, preSpacesCountInternal, spacesCountInternal, hasPipe, parserData, parserRunTimeData)) {
            isAdded = true;
            ++cData->wikiTagCount;
          }

          hasPipe = false;
          preSpacesCountInternal = 0;
          spacesCountInternal = 0;
        } else {
          #if DEBUG
          printf("[DEBUG] [WIKITAG] LINE: %d | POS: %5d | READER: %6d | FOUND [ WORD    ] => \"%s\"", parserRunTimeData->currentLine, parserRunTimeData->currentPosition, readerPosInherited + readerPos, parserData);

          if (ownFormatTypeInternal != -1) printf(" => %s", formatNames[ownFormatTypeInternal]);
          if (dataFormatTypeInternal != -1) printf(" => %s", formatNames[dataFormatTypeInternal]);
          if (isFormatStartInternal) printf(" < isStart");
          if (isFormatEndInternal) printf(" < isEnd");
          #endif

          if (addWord(1, tag, dataFormatTypeInternal, ownFormatTypeInternal, isFormatStartInternal, isFormatEndInternal, preSpacesCountInternal, spacesCountInternal, hasPipe, parserData, parserRunTimeData)) {
            isAdded = true;
            ++cData->wordCount;
            cData->byteWords += writerPos;
          }

          hasPipe = false;
          preSpacesCountInternal = 0;
          spacesCountInternal = 0;
        }

        if (!isAdded) {
          ++cData->failedElements;

        #if DEBUG
          printf(" [FAILED]\n");
        } else {
          printf("\n");
        #endif
        }

        if (doCloseWikiTag) {
          tagType = -1;
          doCloseWikiTag = false;
          isWikiTag = false;
        }

        if (doCloseFormat) {
          dataFormatTypeInternal = -1;
          doCloseFormat = false;
          isFormatEndInternal = false;
        }

        if (doCloseOwnFormat) {
          ownFormatTypeInternal = -1;
          doCloseOwnFormat = false;
        }

        isFormatStartInternal = false;
        isAdded = false;
      }

      writerPos = 0;
      createWord = false;
    }

    ++readerPos;
  }

  #if DEBUG
  printf("\n");
  #endif


  if (tag->target == NULL && !hasTargetData) {
    targetData[targetWritePos] = '\0';
    tag->target = malloc(sizeof(char) * (targetWritePos + 1));
    tag->hasPipe = hasPipe;
    strcpy(tag->target, targetData);
    cData->byteWikiTags += targetWritePos;
  }

  // 0 WORD, 1 WIKITAG, 2 ENTITY

  return 1;
}

//------------------------------------------------------------------------------


bool addEntity(const short elementType, void *element, const short dataFormatType, const short ownFormatType, const bool isFormatStart, const bool isFormatEnd, const unsigned char preSpacesCount, const unsigned char spacesCount, const bool hasPipe, const char* entityBuffer, struct parserBaseStore* parserRunTimeData) {
  entity *tagEntity = NULL;
  xmlNode *xmlTag = NULL;
  wikiTag *tag = NULL;

  if (elementType == 0) {
    xmlTag = element;
    xmlTag->entities = (entity*) realloc(xmlTag->entities, sizeof(entity) * (xmlTag->entityCount + 1));
    tagEntity = &xmlTag->entities[xmlTag->entityCount];
  } else {
    tag = element;
    tag->pipedEntities = (entity*) realloc(tag->pipedEntities, sizeof(entity) * (tag->entityCount + 1));
    tagEntity = &tag->pipedEntities[tag->entityCount];
  }

  tagEntity->lineNum = parserRunTimeData->currentLine;
  tagEntity->dataFormatType = dataFormatType;
  tagEntity->ownFormatType = ownFormatType;
  tagEntity->formatStart = isFormatStart;
  tagEntity->formatEnd = isFormatEnd;
  tagEntity->preSpacesCount = preSpacesCount;
  tagEntity->spacesCount = spacesCount;
  tagEntity->position = ++parserRunTimeData->currentPosition;
  tagEntity->hasPipe = hasPipe;
  strcpy(tagEntity->data, entityBuffer);

  if (elementType == 0) {
    ++xmlTag->entityCount;
    if (xmlTag->firstAddedType == -1) xmlTag->firstAddedType = 2;
    xmlTag->lastAddedType = 2;
  } else ++tag->entityCount;

  return 1;
}

//------------------------------------------------------------------------------


bool addWord(const short elementType, void *element, const short dataFormatType, const short ownFormatType, const bool isFormatStart, const bool isFormatEnd, const unsigned char preSpacesCount, const unsigned char spacesCount, const bool hasPipe, const char *readData, struct parserBaseStore* parserRunTimeData) {
  word *tagWord = NULL;
  xmlNode *xmlTag = NULL;
  wikiTag *tag = NULL;

  if (elementType == 0) {
    xmlTag = element;
    xmlTag->words = (word*) realloc(xmlTag->words, sizeof(word) * (xmlTag->wordCount + 1));
    tagWord = &xmlTag->words[xmlTag->wordCount];
  } else {
    tag = element;
    tag->pipedWords = (word*) realloc(tag->pipedWords, sizeof(word) * (tag->wordCount + 1));
    tagWord = &tag->pipedWords[tag->wordCount];
  }

  tagWord->lineNum = parserRunTimeData->currentLine;
  tagWord->dataFormatType = dataFormatType;
  tagWord->ownFormatType = ownFormatType;
  tagWord->preSpacesCount = preSpacesCount;
  tagWord->spacesCount = spacesCount;
  tagWord->position = ++parserRunTimeData->currentPosition;
  tagWord->formatStart = isFormatStart;
  tagWord->formatEnd = isFormatEnd;
  tagWord->hasPipe = hasPipe;
  tagWord->data = malloc(sizeof(char) * (strlen(readData) + 1));
  strcpy(tagWord->data, readData);

  if (elementType == 0) {
    ++xmlTag->wordCount;
    if (xmlTag->firstAddedType == -1) xmlTag->firstAddedType = 0;
    xmlTag->lastAddedType = 0;
  } else ++tag->wordCount;

  return 1;
}

//------------------------------------------------------------------------------
bool writeOutDataFiles(const struct parserBaseStore* parserRunTimeData, struct xmlDataCollection* xmlCollection) {
  struct xmlNode *xmlTag = NULL;
  struct wikiTag *wTag = NULL;
  struct word* wordElement = NULL;
  struct entity* entityElement = NULL;

  #if STRAIGHTWRITEOUT
    for (unsigned int i = 0; i < xmlCollection->count; ++i) {
      xmlTag = &xmlCollection->nodes[i];
      fprintf(parserRunTimeData->xmltagFile, "%d\t%d\t%d\t%d\t%s\n", xmlTag->start , xmlTag->end, xmlTag->isClosed, xmlTag->isDataNode, xmlTag->name);

      for (unsigned int j = 0; j < xmlTag->keyValuePairs; ++j) {
        fprintf(parserRunTimeData->xmldataFile, "%d\t%d\t%s\t%s\n", xmlTag->start, xmlTag->end, xmlTag->keyValues[j].key, xmlTag->keyValues[j].value);
      }

      for (unsigned int j = 0; j < xmlTag->wordCount; ++j) {
        wordElement = &xmlTag->words[j];
        fprintf(parserRunTimeData->dictFile, "%u\t%u\t%d\t%d\t%d\t%ld\t%d\t%d\t%d\t%d\t%d\t%s\n", wordElement->position, wordElement->lineNum, -1, wordElement->preSpacesCount, wordElement->spacesCount, strlen(wordElement->data), wordElement->dataFormatType, wordElement->ownFormatType, wordElement->formatStart, wordElement->formatEnd, wordElement->hasPipe, wordElement->data);
      }

      for (unsigned int j = 0; j < xmlTag->entityCount; ++j) {
        entityElement = &xmlTag->entities[j];
        fprintf(parserRunTimeData->entitiesFile, "%u\t%u\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",  entityElement->position, entityElement->lineNum, -1, entityElement->preSpacesCount, entityElement->spacesCount, entityElement->dataFormatType, entityElement->ownFormatType, entityElement->formatStart, entityElement->formatEnd, entityElement->hasPipe, entityElement->data);
      }

      for (unsigned int j = 0; j < xmlTag->wTagCount; ++j) {
        wTag = &xmlTag->wikiTags[j];
        writeOutTagData(parserRunTimeData, wTag);
      }
    }
  #else
    // SORTED WRITE OUT
    for (unsigned int i = 0; i < xmlCollection->count; ++i) {
      xmlTag = &xmlCollection->nodes[i];
      fprintf(parserRunTimeData->xmltagFile, "%x\t%x\t%d\t%d\t%s\n", xmlTag->start , xmlTag->end, xmlTag->isClosed, xmlTag->isDataNode, xmlTag->name);

      for (unsigned int j = 0; j < xmlTag->keyValuePairs; ++j) {
        fprintf(parserRunTimeData->xmldataFile, "%x\t%x\t%s\t%s\n", xmlTag->start, xmlTag->end, xmlTag->keyValues[j].key, xmlTag->keyValues[j].value);
      }
    }

    unsigned int lineNum = 1;
    unsigned int lastChecked = 0;
    while (lineNum <= parserRunTimeData->currentLine) {
      for (unsigned int i = lastChecked; i < xmlCollection->count; ++i) {
        xmlTag = &xmlCollection->nodes[i];
        if (xmlTag->end < lineNum) continue;
        else if (xmlTag->start > lineNum) break;

        lastChecked = i;

        for (unsigned int j = 0; j < xmlTag->wordCount; ++j) {
          wordElement = &xmlTag->words[j];
          if (wordElement->lineNum == lineNum) {
            fprintf(parserRunTimeData->dictFile, "%x\t%x\t%d\t%x\t%x\t%ld\t%d\t%d\t%d\t%d\t%d\t%s\n", wordElement->position, wordElement->lineNum, -1, wordElement->preSpacesCount, wordElement->spacesCount, strlen(wordElement->data), wordElement->dataFormatType, wordElement->ownFormatType, wordElement->formatStart, wordElement->formatEnd, wordElement->hasPipe, wordElement->data);
          }
        }

        for (unsigned int j = 0; j < xmlTag->entityCount; ++j) {
          entityElement = &xmlTag->entities[j];
          if (entityElement->lineNum == lineNum) {
            fprintf(parserRunTimeData->entitiesFile, "%x\t%x\t%d\t%x\t%x\t%d\t%d\t%d\t%d\t%d\t%s\n",  entityElement->position, entityElement->lineNum, -1, entityElement->preSpacesCount, entityElement->spacesCount, entityElement->dataFormatType, entityElement->ownFormatType, entityElement->formatStart, entityElement->formatEnd, entityElement->hasPipe, entityElement->data);
          }
        }

        for (unsigned int j = 0; j < xmlTag->wTagCount; ++j) {
          wTag = &xmlTag->wikiTags[j];
          if (wTag->lineNum == lineNum) {
            writeOutTagDataByLine(parserRunTimeData, wTag, lineNum);
          }
        }
      }

      ++lineNum;
    }

  #endif
  return true;
}

// ----------------------------------------------------------

bool writeOutTagDataByLine(const struct parserBaseStore* parserRunTimeData, wikiTag *wTag, const unsigned int lineNum) {
  struct word* wordElement = NULL;
  struct entity* entityElement = NULL;
  struct wikiTag* wikiTagElement = NULL;

  if (wTag->lineNum == lineNum) {
    fprintf(parserRunTimeData->wtagFile, "%x\t%x\t%x\t%x\t%d\t%d\t%d\t%d\t%d\t%d\t%lu\t%d\t%s\n",  wTag->position, wTag->lineNum, wTag->preSpacesCount, wTag->spacesCount, wTag->tagType, wTag->dataFormatType, wTag->ownFormatType, wTag->formatStart, wTag->formatEnd, wTag->tagLength, strlen(wTag->target), wTag->hasPipe, wTag->target);
  }

  for (unsigned int k = 0; k < wTag->wordCount; ++k) {
    wordElement = &wTag->pipedWords[k];
    if (wordElement->lineNum == lineNum) {
      fprintf(parserRunTimeData->dictFile, "%x\t%x\t%d\t%x\t%x\t%ld\t%d\t%d\t%d\t%d\t%d\t%s\n", wordElement->position, wordElement->lineNum, wTag->position, wordElement->preSpacesCount, wordElement->spacesCount, strlen(wordElement->data), wordElement->dataFormatType, wordElement->ownFormatType, wordElement->formatStart, wordElement->formatEnd, wordElement->hasPipe, wordElement->data);
    }
  }

  for (unsigned int k = 0; k < wTag->wTagCount; ++k) {
    wikiTagElement = &wTag->pipedTags[k];
    if (wikiTagElement->lineNum == lineNum) {
      writeOutTagDataByLine(parserRunTimeData, wikiTagElement, lineNum);
    }
  }

  for (unsigned int k = 0; k < wTag->entityCount; ++k) {
    entityElement = &wTag->pipedEntities[k];
    if (entityElement->lineNum == lineNum) {
      fprintf(parserRunTimeData->entitiesFile, "%x\t%x\t%d\t%x\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",  entityElement->position, entityElement->lineNum, wTag->position, entityElement->preSpacesCount, entityElement->spacesCount, entityElement->dataFormatType, entityElement->ownFormatType, entityElement->formatStart, entityElement->formatEnd, entityElement->hasPipe, entityElement->data);
    }
  }

  return true;
}

bool writeOutTagData(const struct parserBaseStore* parserRunTimeData, wikiTag *wTag) {
  struct word* wordElement = NULL;
  struct entity* entityElement = NULL;
  struct wikiTag* wikiTagElement = NULL;

  fprintf(parserRunTimeData->wtagFile, "%x\t%x\t%x\t%x\t%d\t%d\t%d\t%d\t%d\t%d\t%lu\t%d\t%s\n",  wTag->position, wTag->lineNum, wTag->preSpacesCount, wTag->spacesCount, wTag->tagType, wTag->dataFormatType, wTag->ownFormatType, wTag->formatStart, wTag->formatEnd, wTag->tagLength, strlen(wTag->target), wTag->hasPipe, wTag->target);


  for (unsigned int k = 0; k < wTag->wordCount; ++k) {
    wordElement = &wTag->pipedWords[k];
    fprintf(parserRunTimeData->dictFile, "%x\t%x\t%x\t%x\t%x\t%ld\t%d\t%d\t%d\t%d\t%d\t%s\n", wordElement->position, wordElement->lineNum, wTag->position, wordElement->preSpacesCount, wordElement->spacesCount, strlen(wordElement->data), wordElement->dataFormatType, wordElement->ownFormatType, wordElement->formatStart, wordElement->formatEnd, wordElement->hasPipe, wordElement->data);
  }

  for (unsigned int k = 0; k < wTag->wTagCount; ++k) {
    wikiTagElement = &wTag->pipedTags[k];
    writeOutTagData(parserRunTimeData, wikiTagElement);
  }

  for (unsigned int k = 0; k < wTag->entityCount; ++k) {
    entityElement = &wTag->pipedEntities[k];
    fprintf(parserRunTimeData->entitiesFile, "%x\t%x\t%x\t%x\t%x\t%d\t%d\t%d\t%d\t%d\t%s\n",  entityElement->position, entityElement->lineNum, wTag->position, entityElement->preSpacesCount, entityElement->spacesCount, entityElement->dataFormatType, entityElement->ownFormatType, entityElement->formatStart, entityElement->formatEnd, entityElement->hasPipe, entityElement->data);
  }

  return true;
}


//------------------------------------------------------------------------------

void freeXMLCollection(xmlDataCollection *xmlCollection) {
  xmlNode *xmlTag = NULL;

  for (unsigned int i = 0; i < xmlCollection->count; ++i) {
    xmlTag = &xmlCollection->nodes[i];

    for (unsigned int j = 0; j < xmlTag->keyValuePairs; ++j) {
      free(xmlTag->keyValues[j].key);
      free(xmlTag->keyValues[j].value);
    }

    for (unsigned int j = 0; j < xmlTag->wordCount; ++j) free(xmlTag->words[j].data);
    for (unsigned int j = 0; j < xmlTag->wTagCount; ++j) freeXMLCollectionTag(&xmlTag->wikiTags[j]);

    free(xmlTag->name);
    free(xmlTag->keyValues);
    free(xmlTag->words);
    free(xmlTag->entities);
    free(xmlTag->wikiTags);
  }

  free(xmlCollection->nodes);
  free(xmlCollection->openNodes);

  #if DEBUG
  printf("[DEBUG] Successfully cleaned up xmlCollection...\n");
  #endif

  return;
}

void freeXMLCollectionTag(wikiTag *wTag) {
  for (unsigned int i = 0; i < wTag->wordCount; ++i) {
    free(wTag->pipedWords[i].data);
  }

  for (unsigned int i = 0; i < wTag->wTagCount; ++i) {
    freeXMLCollectionTag(&wTag->pipedTags[i]);
  }

  free(wTag->target);
  free(wTag->pipedWords);
  free(wTag->pipedTags);
  free(wTag->pipedEntities);

  return;
}
